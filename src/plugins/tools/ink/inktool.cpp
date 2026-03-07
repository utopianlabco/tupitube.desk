/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              * 
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
 *   2010:                                                                 *
 *    Gustav Gonzalez                                                      *
 *   ---                                                                   *
 *   KTooN's versions:                                                     *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "inktool.h"
#include "tupinputdeviceinformation.h"
#include "tupbrushmanager.h"
#include "tupgraphicalgorithm.h"
#include "tupgraphicsscene.h"
#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tupellipseitem.h"
#include "tuplineitem.h"
#include "tuptextitem.h"
#include "taction.h"
#include "talgorithm.h"
#include "tconfig.h"

InkTool::InkTool()
{
    settings = nullptr;
    guidePath = nullptr;
    inkCursor = QCursor(kAppProp->themeDir() + "cursors/target.png", 4, 4);

    setupActions();
}

InkTool::~InkTool()
{
}

void InkTool::init(TupGraphicsScene *gScene)
{
    Q_UNUSED(gScene)

    penPress = 1;
    TCONFIG->beginGroup("InkTool");
    sensibility = TCONFIG->value("Sensibility", 1).toInt();
    smoothness = TCONFIG->value("Smoothness", 2).toDouble();
    showBorder = TCONFIG->value("BorderEnabled", true).toBool();
    showFill = TCONFIG->value("FillEnabled", true).toBool();
    borderSize = TCONFIG->value("BorderSize", 1).toInt();

    foreach (QGraphicsView *view, gScene->views())
        view->setDragMode(QGraphicsView::NoDrag);
}

QList<TAction::ActionId> InkTool::keys() const
{
    return QList<TAction::ActionId>() << TAction::Ink;
}

void InkTool::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager,
                    TupGraphicsScene *gScene)
{
    guidePoints.clear();
    pointPress.clear();

    firstHalfOnTop = true;
    previousDirection = None;
    // oldSlope = 0;
    initPenWidth = brushManager->pen().widthF() / 6;
    penWidth = initPenWidth;

    firstPoint = input->pos();

    guidePainterPath = QPainterPath();
    guidePainterPath.moveTo(firstPoint);

    inkPath = QPainterPath();
    inkPath.setFillRule(Qt::WindingFill);
    inkPath.moveTo(firstPoint);

    shapePoints.clear();
    shapePoints << firstPoint;

    oldPos = input->pos();
    firstHalfPrevious = input->pos();
    secondHalfPrevious = input->pos();
    oldPos = input->pos();

    // Guide line
    guidePath = new TupPathItem();
    QColor color(55, 155, 55, 200);
    QPen pen(QBrush(color), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    guidePath->setPen(pen);

    gScene->includeObject(guidePath);

    firstArrow = rand() % 10 + 1;
    arrowSize = -1;
}

void InkTool::move(const TupInputDeviceInformation *input, TupBrushManager *brushManager,
                   TupGraphicsScene *gScene)
{
    Q_UNUSED(brushManager)

    gScene->views().at(0)->setDragMode(QGraphicsView::NoDrag);
    QPointF currentPoint = input->pos();

    // guidePath is the guideline to calculate the real QGraphicsPathItem
    guidePainterPath.lineTo(currentPoint);
    guidePath->setPath(guidePainterPath);

    if (currentPoint != previousPoint) {
        pointPress << penWidth;
        guidePoints << currentPoint;
    }

    previousPoint = currentPoint;
}

void InkTool::release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    gScene->removeItem(guidePath);
    QPointF currentPoint = input->pos();
    qreal length = sqrt(pow(std::abs(firstPoint.x() - currentPoint.x()), 2)
                        + pow(std::abs(firstPoint.y() - currentPoint.y()), 2));

    if (length > 10) {
        // Drawing a stroke
        QList<qreal> strokeWidths;
        
        if (device == InkSettings::Mouse) {
            // Calculate velocity-based widths for more natural mouse strokes
            // Using sensibility to control width variation intensity
            qreal baseWidth = initPenWidth * (4 + sensibility);  // sensibility: 1-5
            strokeWidths = calculateVelocityWidths(guidePoints, baseWidth, sensibility);
        } else {
            // Use recorded pressure values for pen mode
            strokeWidths = pointPress;
        }

        // Smooth width transitions to avoid abrupt changes
        // Map smoothness (0-10) to smoothing factor (0.1-0.5)
        // Lower smoothness value = more smoothing (lower factor)
        double smoothFactor = 0.1 + (smoothness / 10.0) * 0.4;
        smoothWidths(strokeWidths, smoothFactor);

        // Apply natural tapering at stroke start and end
        // Taper length scales with sensibility (more pressure sensitivity = longer taper)
        int taperLen = qMin(static_cast<int>(guidePoints.size() / 4), 4 + sensibility);
        applyTapering(strokeWidths, taperLen);

        // Process points with improved widths
        for (int i = 0; i < guidePoints.size(); i++) {
            qreal width = (i < strokeWidths.size()) ? strokeWidths.at(i) : initPenWidth;
            processPoint(guidePoints.at(i), width);
        }
    } else {
        // Drawing a point
        qreal pressCo;
        qreal radius;

        if (device == InkSettings::Pen) {
            qreal pressCo = penPress * 10;
            switch(sensibility) {
                case 2:
                    pressCo += 0.2;
                break;
                case 3:
                    pressCo += 0.4;
                break;
                case 4:
                    pressCo += 1.6;
                break;
                case 5:
                    pressCo += 3.2;
                break;
            }

            if (penPress > 0.4)
                pressCo *= 0.4;

            radius = brushManager->pen().width() * pressCo;
        } else {
            pressCo = (rand() % 9) + 1;
            radius = brushManager->pen().width() + pressCo; 
        }

        qreal half = (radius + 2) / 2;
        QPointF distance(half, half);
        QPointF center = currentPoint - distance;
        QPen inkPen(brushManager->penColor(), borderSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        TupEllipseItem *blackEllipse = new TupEllipseItem(QRectF(center, QSize(static_cast<int>(radius), static_cast<int>(radius))));

        if (showBorder)
            blackEllipse->setPen(inkPen);
        if (showFill) {
            Qt::BrushStyle style = brushManager->penBrush().style();
            QBrush brush = brushManager->brush();
            brush.setStyle(style);
            if (!showBorder)
                blackEllipse->setPen(Qt::NoPen);
            blackEllipse->setBrush(brush);
        }
        gScene->includeObject(blackEllipse);

        QDomDocument doc;
        doc.appendChild(blackEllipse->toXml(doc));
        TupProjectRequest request = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(),
                                                                         gScene->currentFrameIndex(), 0, QPointF(0, 0),
                                                                         gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                         TupProjectRequest::Add, doc.toString());
        emit requested(&request);
        return;
    }

    inkPath.lineTo(currentPoint);

    // Creating the whole shape of the line (QPainterPath)
    for (int i = shapePoints.size()-1; i > 0; i--)
        inkPath.lineTo(shapePoints.at(i-1));

    if (settings->smooothnessIsEnabled() && smoothness > 0)
        smoothPath(inkPath, smoothness, 0, -1, true);

    TupPathItem *stroke = new TupPathItem();
    stroke->setPath(inkPath);

    /* SQA: Debugging code
    qDebug() << "";
    qDebug() << "showBorder: " << showBorder;
    qDebug() << "showFill: " << showFill;
    qDebug() << "smoothness: " << smoothness;
    qDebug() << "borderSize: " << borderSize;
    qDebug() << "pressure sensibility: " << sensibility;
    */

    if (showBorder) // Set border color for shape
        stroke->setPen(QPen(brushManager->penColor(), borderSize));

    if (showFill) { // Set fill color for shape
        Qt::BrushStyle style = brushManager->penBrush().style();
        QBrush brush = brushManager->brush();
        brush.setStyle(style);
        if (!showBorder)
            stroke->setPen(Qt::NoPen);
        stroke->setBrush(brush);
    }

    gScene->includeObject(stroke);

    QDomDocument doc;
    doc.appendChild(stroke->toXml(doc));
    TupProjectRequest request = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(), gScene->currentFrameIndex(),
                                                                     0, QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                                     doc.toString());
    emit requested(&request);

    // SQA: Code for debugging
    /*
    smoothPath(guidePainterPath, 2);

    qDebug() << "";
    qDebug() << "ELEMENT COUNT: " << guidePainterPath.elementCount();
    qDebug() << "";

    TupPathItem *greenLine = new TupPathItem();
    QColor color(55, 155, 55, 200);
    QPen pen(QBrush(color), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    greenLine->setPen(pen);
    greenLine->setPath(guidePainterPath);
    gScene->includeObject(greenLine);
    QDomDocument data;
    data.appendChild(greenLine->toXml(data));
    request = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(), gScene->currentFrameIndex(),
                                                   0, QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                   data.toString());
    emit requested(&request);
    */
}

void InkTool::removeExtraPoints()
{
    if (shapePoints.size() > 3) {
        for (int i=0; i<4; i++)
            shapePoints.removeLast();
    }
}

void InkTool::setupActions()
{
    TAction *inkPen = new TAction(QPixmap(ICONS_DIR + "ink.png"), tr("Ink"), this);
    inkPen->setShortcut(QKeySequence(tr("K")));
    inkPen->setToolTip(tr("Ink") + " - " + tr("K"));
    inkPen->setCursor(inkCursor);
    inkPen->setActionId(TAction::Ink);

    inkActions.insert(TAction::Ink, inkPen);
}

QMap<TAction::ActionId, TAction *> InkTool::actions() const
{
    return inkActions;
}

TAction * InkTool::getAction(TAction::ActionId toolId)
{
    return inkActions[toolId];
}

int InkTool::toolType() const
{
    return TupToolInterface::Brush;
}

QWidget *InkTool::configurator()
{
    if (!settings) {
        settings = new InkSettings;
        connect(settings, SIGNAL(deviceUpdated(InkSettings::Device)),
                this, SLOT(setDevice(InkSettings::Device)));
        connect(settings, SIGNAL(borderUpdated(bool)), this, SLOT(updateBorderFeature(bool)));
        connect(settings, SIGNAL(fillUpdated(bool)), this, SLOT(updateFillFlag(bool)));
        connect(settings, SIGNAL(borderSizeUpdated(int)), this, SLOT(updateBorderSize(int)));
        connect(settings, SIGNAL(pressureUpdated(int)), this, SLOT(updatePressure(int)));
        connect(settings, SIGNAL(smoothnessUpdated(double)), this, SLOT(updateSmoothness(double)));

        TCONFIG->beginGroup("InkTool");
        smoothness = TCONFIG->value("Smoothness", 4.0).toDouble();
        if (smoothness == 0.0)
            smoothness = 4.0;
        settings->updateSmoothness(smoothness);
        device = settings->currentDevice();
    }

    return settings;
}

void InkTool::setDevice(InkSettings::Device dev)
{
    device = dev;
}

void InkTool::aboutToChangeTool() 
{
}

void InkTool::saveConfig()
{
    if (settings) {
        TCONFIG->beginGroup("InkTool");
        TCONFIG->setValue("BorderEnabled", showBorder);
        TCONFIG->setValue("BorderSize", borderSize);
        TCONFIG->setValue("FillEnabled", showFill);
        TCONFIG->setValue("Sensibility", sensibility);

        if (smoothness == 0.0)
            smoothness = 4.0;
        TCONFIG->setValue("Smoothness", smoothness);
    }
}

void InkTool::updateBorderFeature(bool border)
{
    showBorder = border;
}

void InkTool::updateFillFlag(bool fill)
{
    showFill = fill;
}

void InkTool::updateBorderSize(int size)
{
    borderSize = size;
}

void InkTool::updatePressure(int value)
{
    sensibility = value;
}

void InkTool::updateSmoothness(double value)
{
    smoothness = value;
}

void InkTool::smoothPath(QPainterPath &path, double smoothness, int from, int to, bool closePath)
{
    QPolygonF polygonPoints;
    QList<QPolygonF> polygons = path.toSubpathPolygons();
    QList<QPolygonF>::iterator it = polygons.begin();
    QPolygonF::iterator pointIt;

    while (it != polygons.end()) {
        pointIt = (*it).begin();
        while (pointIt <= (*it).end()-2) {
            polygonPoints << (*pointIt);
            pointIt += 2;
        }
        ++it;
    }

    if (smoothness > 0) {
        path = TupGraphicalAlgorithm::bezierFit(polygonPoints, static_cast<float>(smoothness), from, to,
                                                closePath);
    } else {
        path = QPainterPath();
        path.addPolygon(polygonPoints);
    }
}

void InkTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11 || event->key() == Qt::Key_Escape) {
        emit closeHugeCanvas();
    } else {
        QPair<int, int> flags = TAction::setKeyAction(event->key(), event->modifiers());
        if (flags.first != -1 && flags.second != -1)
            emit callForPlugin(flags.first, flags.second);
    }
}

QCursor InkTool::toolCursor() // const
{
    return inkCursor;
}

void InkTool::updatePressure(qreal pressure)
{    
    #ifdef TUP_DEBUG
        qDebug() << "InkTool::updatePressure() - pressure: " << pressure;
    #endif

    if (device == InkSettings::Mouse) {
        penWidth = 5 + (rand() % 5);
        sensibility = 1;
    } else {
        penPress = pressure;
        double factor = sensibility;
        if (sensibility > 1)
            factor = pow(sensibility, 2);

        if (pressure <= 0.2) {
            penWidth = initPenWidth / (3 * factor);
        } else if (pressure > 0.2 && pressure < 0.6) {
            penWidth = initPenWidth + (initPenWidth * pressure * (4 + factor));
        } else if (pressure >= 0.6) {
            penWidth = initPenWidth + (initPenWidth * pressure * (6 + factor));
        }
    }
}

void InkTool::processPoint(QPointF currentPoint, qreal strokeWidth)
{
    qreal my = currentPoint.y() - oldPos.y();
    qreal mx = currentPoint.x() - oldPos.x();
    qreal m;

    if (static_cast<int>(mx) != 0) // Calculating slope
        m = my / mx;
    else
        m = 100; // mx = 0 -> path is vertical | 100 == infinite

    // Calculating distance between current point and previous
    qreal distance = sqrt(pow(std::abs(currentPoint.x() - oldPos.x()), 2)
                          + pow(std::abs(currentPoint.y() - oldPos.y()), 2));

    // Time to calculate a new point of the QGraphicsPathItem (stroke)
    if (distance > 5) {
        /* SQA: Debugging code
        qreal pm; // Perpendicular slope
        if (static_cast<int>(m) == 0) // path is horizontal
            pm = 100;
        else
            pm = (-1) * (1/m);

        #ifdef TUP_DEBUG
            qDebug() << "";
            qDebug() << "InkTool::move() - old slope: " << oldSlope;
            qDebug() << "InkTool::move() - slope: " << m;

            bool isNAN = false;
            if (static_cast<int>(m) == 0) // path is horizontal
                isNAN = true;

            if (static_cast<int>(m) == 100) // path is vertical | 100 == infinite
                qDebug() << "InkTool::move() - M: NAN";
            else
                qDebug() << "InkTool::move() - M: " + QString::number(m);

            if (isNAN)
                qDebug() << "InkTool::move() - M(inv): NAN";
            else
                qDebug() << "InkTool::move() - M(inv): " + QString::number(pm);
        #endif
        */

        QPointF firstHalfPoint;
        QPointF secondHalfPoint;

        double lineAngle = atan(m);
        double angle = 1.5708 - lineAngle;
        double deltaX = fabs(cos(angle) * strokeWidth);
        double deltaY = fabs(sin(angle) * strokeWidth);

        /* SQA: Debugging code
          qDebug() << "InkTool::move() - inv tan: " << lineAngle;
          qDebug() << "InkTool::move() - degrees: " << degrees;
          qDebug() << "InkTool::move() - angle: " << angle;
          qDebug() << "InkTool::move() - deltaX: " << deltaX;
          qDebug() << "InkTool::move() - deltaY: " << deltaY;
        */

        qreal x0 = 0;
        qreal y0 = 0;
        qreal x1 = 0;
        qreal y1 = 0;
        if (oldPos.x() < currentPoint.x()) {
            if (oldPos.y() < currentPoint.y()) {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going right-down";
                #endif

                x0 = currentPoint.x() + deltaX;
                x1 = currentPoint.x() - deltaX;
                y0 = currentPoint.y() - deltaY;
                y1 = currentPoint.y() + deltaY;

                if (previousDirection == None)
                    previousDirection = RightDown;

                switch(previousDirection) {
                    case Up: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            removeExtraPoints();
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Down: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Right: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightUp: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightDown: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Left: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);

                            // SQA: Fixing acute angule cases
                            double xFix = currentPoint.x() - strokeWidth;
                            double yFix = oldPos.y();
                            QPointF fixPoint = QPointF(xFix, yFix);
                            inkPath.lineTo(fixPoint);
                            removeExtraPoints();

                            firstHalfOnTop = false;
                        } else {
                            shapePoints.removeAt(shapePoints.size() - 1);
                            shapePoints.removeAt(shapePoints.size() - 1);

                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftUp: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftDown: // to right-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);

                            // SQA: Fixing acute angule cases
                            double xFix = currentPoint.x() - strokeWidth;
                            double yFix = firstHalfPoint.y() - fabs(firstHalfPoint.y() - firstHalfPrevious.y()) / 2;
                            QPointF fixPoint = QPointF(xFix, yFix);
                            inkPath.lineTo(fixPoint);
                            removeExtraPoints();

                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);

                            firstHalfOnTop = true;
                        }
                    break;
                    case None:
                    break;
                }
                // qDebug() << "InkTool::move() - firstHalfOnTop: " << firstHalfOnTop;
                previousDirection = RightDown;
            } else if (oldPos.y() > currentPoint.y()) {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going right-up";
                #endif

                x0 = currentPoint.x() - deltaX;
                x1 = currentPoint.x() + deltaX;
                y0 = currentPoint.y() - deltaY;
                y1 = currentPoint.y() + deltaY;

                if (previousDirection == None)
                    previousDirection = RightUp;

                switch(previousDirection) {
                    case Up: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Down: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Right: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightUp: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightDown: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);

                            // SQA: Fixing acute angule cases
                            double yFix = oldPos.y() - strokeWidth;
                            double xFix = oldPos.x();
                            QPointF fixPoint = QPointF(xFix, yFix);
                            inkPath.lineTo(fixPoint);
                            removeExtraPoints();
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Left: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftUp: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftDown: // to right-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case None:
                    break;
                }
                previousDirection = RightUp;
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going right";
                #endif

                x0 = currentPoint.x();
                y0 = currentPoint.y() - strokeWidth;
                x1 = currentPoint.x();
                y1 = currentPoint.y() + strokeWidth;

                if (previousDirection == None)
                    previousDirection = Right;

                switch(previousDirection) {
                    case Up: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Down: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Right: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightUp: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightDown: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Left: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftUp: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftDown: // to right
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;

                            // SQA: Fixing acute angule cases
                            double xFix = currentPoint.x() - strokeWidth;
                            double yFix = firstHalfPoint.y() - fabs(firstHalfPoint.y() - firstHalfPrevious.y()) / 2;
                            QPointF fixPoint = QPointF(xFix, yFix);
                            inkPath.lineTo(fixPoint);
                            removeExtraPoints();
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = Right;
            }
        } else if (oldPos.x() > currentPoint.x()) {
            if (oldPos.y() < currentPoint.y()) {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going left-down";
                #endif

                x0 = currentPoint.x() - deltaX;
                x1 = currentPoint.x() + deltaX;
                y0 = currentPoint.y() - deltaY;
                y1 = currentPoint.y() + deltaY;

                if (previousDirection == None)
                    previousDirection = LeftDown;

                switch(previousDirection) {
                    case Up: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Down: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Right: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;

                            // SQA: Fixing acute angule cases
                            double xFix = currentPoint.x() + strokeWidth;
                            double yFix = currentPoint.y();
                            QPointF fixPoint = QPointF(xFix, yFix);
                            inkPath.lineTo(fixPoint);
                            removeExtraPoints();
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightUp: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightDown: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Left: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftUp: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftDown: // to left-down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = LeftDown;
            } else if (oldPos.y() > currentPoint.y()) {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going left-up";
                #endif

                x0 = currentPoint.x() + deltaX;
                x1 = currentPoint.x() - deltaX;
                y0 = currentPoint.y() - deltaY;
                y1 = currentPoint.y() + deltaY;

                if (previousDirection == None)
                    previousDirection = LeftUp;

                switch(previousDirection) {
                    case Up: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Down: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Right: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightUp: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightDown: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Left: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftUp: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftDown: // to left-up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = LeftUp;
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going left";
                #endif
                x0 = currentPoint.x();
                y0 = currentPoint.y() - strokeWidth;
                x1 = currentPoint.x();
                y1 = currentPoint.y() + strokeWidth;

                if (previousDirection == None)
                    previousDirection = Left;

                switch(previousDirection) {
                    case Up: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Down: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Right: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightUp: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightDown: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Left: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftUp: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftDown: // to left
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = Left;
            }
        } else if (static_cast<int>(oldPos.x()) == static_cast<int>(currentPoint.x())) {
            if (oldPos.y() > currentPoint.y()) {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going up";
                #endif

                x0 = currentPoint.x() - strokeWidth;
                y0 = currentPoint.y();
                x1 = currentPoint.x() + strokeWidth;
                y1 = currentPoint.y();

                if (previousDirection == None)
                    previousDirection = Up;

                switch(previousDirection) {
                    case Up: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Down: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Right: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightUp: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case RightDown: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Left: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftUp: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case LeftDown: // to up
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = Up;
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "    -> InkTool::move() - Going down";
                #endif

                x0 = currentPoint.x() - strokeWidth;
                y0 = currentPoint.y();
                x1 = currentPoint.x() + strokeWidth;
                y1 = currentPoint.y();

                if (previousDirection == None)
                    previousDirection = Down;

                switch(previousDirection) {
                    case Up: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Down: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case Right: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightUp: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case RightDown: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                            firstHalfOnTop = false;
                        } else {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                            firstHalfOnTop = true;
                        }
                    break;
                    case Left: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftUp: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case LeftDown: // to down
                        if (firstHalfOnTop) {
                            firstHalfPoint = QPointF(x0, y0);
                            secondHalfPoint = QPointF(x1, y1);
                        } else {
                            firstHalfPoint = QPointF(x1, y1);
                            secondHalfPoint = QPointF(x0, y0);
                        }
                    break;
                    case None:
                    break;
                }

                previousDirection = Down;
            }
        }

        inkPath.lineTo(firstHalfPoint);
        firstHalfPrevious = firstHalfPoint;

        secondHalfPrevious = secondHalfPoint;
        shapePoints << secondHalfPoint;

        oldPos = currentPoint;
        // oldSlope = m;
    }
}

// Apply natural tapering at stroke start and end
void InkTool::applyTapering(QList<qreal> &widths, int taperLength)
{
    int size = widths.size();
    if (size < 2 || taperLength <= 0)
        return;

    // Ensure taper doesn't exceed half the stroke
    taperLength = qMin(taperLength, size / 2);

    // Taper at start (ease-in: thin to thick)
    for (int i = 0; i < taperLength; i++) {
        qreal t = static_cast<qreal>(i) / taperLength;
        // Ease-in quadratic curve for natural feel
        qreal factor = t * t;
        widths[i] *= factor;
    }

    // Taper at end (ease-out: thick to thin)
    for (int i = 0; i < taperLength; i++) {
        int idx = size - 1 - i;
        qreal t = static_cast<qreal>(i) / taperLength;
        // Ease-out quadratic
        qreal factor = t * t;
        widths[idx] *= factor;
    }
}

// Smooth width transitions using exponential moving average
void InkTool::smoothWidths(QList<qreal> &widths, double factor)
{
    if (widths.size() < 2 || factor <= 0)
        return;

    // Forward pass
    QList<qreal> smoothed;
    smoothed.reserve(widths.size());
    smoothed << widths[0];

    for (int i = 1; i < widths.size(); i++) {
        qreal smoothedValue = factor * widths[i] + (1.0 - factor) * smoothed[i - 1];
        smoothed << smoothedValue;
    }

    // Backward pass for symmetric smoothing
    for (int i = widths.size() - 2; i >= 0; i--) {
        smoothed[i] = factor * smoothed[i] + (1.0 - factor) * smoothed[i + 1];
    }

    widths = smoothed;
}

// Calculate velocity-based widths for mouse mode (slower = thicker, faster = thinner)
// sensitivity (1-5) controls how much velocity affects width variation
QList<qreal> InkTool::calculateVelocityWidths(const QList<QPointF> &points, qreal baseWidth, int sensitivity)
{
    QList<qreal> widths;
    if (points.isEmpty())
        return widths;

    // Calculate distances between consecutive points (as velocity proxy)
    QList<qreal> distances;
    for (int i = 1; i < points.size(); i++) {
        qreal dx = points[i].x() - points[i - 1].x();
        qreal dy = points[i].y() - points[i - 1].y();
        qreal dist = sqrt(dx * dx + dy * dy);
        distances << dist;
    }

    if (distances.isEmpty()) {
        widths << baseWidth;
        return widths;
    }

    // Find typical distance for normalization
    qreal avgDist = 0;
    for (qreal d : distances)
        avgDist += d;
    avgDist /= distances.size();

    // Clamp to reasonable range
    avgDist = qMax(avgDist, 3.0);

    // First point uses base width
    widths << baseWidth;

    // Sensitivity affects range of velocity factor (1=subtle, 5=dramatic)
    qreal minFactor = 1.0 - (0.1 * sensitivity);  // 0.9 to 0.5
    qreal maxFactor = 1.0 + (0.2 * sensitivity);  // 1.2 to 2.0

    // Generate widths based on velocity (inverse relationship)
    for (int i = 0; i < distances.size(); i++) {
        qreal dist = distances[i];
        // Normalize: fast movement (high dist) = thin, slow movement (low dist) = thick
        qreal velocityFactor = avgDist / qMax(dist, 1.0);
        // Clamp factor based on sensitivity setting
        velocityFactor = qBound(minFactor, velocityFactor, maxFactor);
        // Apply variation with some randomness for organic feel (more random at higher sensitivity)
        qreal randomRange = 0.05 + (0.03 * sensitivity);  // 0.08 to 0.20
        qreal variation = (1.0 - randomRange) + (rand() % 100) / 100.0 * (2 * randomRange);
        qreal width = baseWidth * velocityFactor * variation;
        widths << width;
    }

    return widths;
}
