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

#include "geometrictool.h"
#include "tconfig.h"
#include "tuprectitem.h"
#include "tupellipseitem.h"
#include "tuplineitem.h"
#include "tuplibraryobject.h"
#include "tupinputdeviceinformation.h"
#include "tupgraphicsscene.h"
#include "tupscene.h"
#include "tupframe.h"
#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tupbrushmanager.h"

GeometricTool::GeometricTool()
{
    scene = nullptr;
    linePath = nullptr;
    setupActions();
}

GeometricTool::~GeometricTool()
{
}

QList<TAction::ActionId> GeometricTool::keys() const
{    
    return QList<TAction::ActionId>() << TAction::Rectangle
                                      << TAction::Ellipse
                                      << TAction::Line
                                      << TAction::Triangle
                                      << TAction::Hexagon;
}

void GeometricTool::init(TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::init()]";
    #endif

    scene = gScene;
    delete linePath;
    linePath = nullptr;
    proportion = false;
    side = false;

    triangleType = GeometricSettings::Top;
    hexagonType = GeometricSettings::Horizontal;

    if (settings && toolId() == TAction::Line) {
        setZValueReferences();
        straightMode = false;
        TCONFIG->beginGroup("GeometricTool");
        int type = TCONFIG->value("LineType", 0).toInt();
        if (type)
            straightMode = true;

        TCONFIG->beginGroup("BrushParameters");
        eraserSize = TCONFIG->value("EraserSize", 10).toInt();

        circleZValue = ZLAYER_BASE + (scene->layersCount() * ZLAYER_LIMIT);

        currentToolMode = LineMode;
        settings->enableLineMode();
        settings->updateLineType(type);

        brushManager = scene->getBrushManager();

        qreal radius = eraserSize/2;
        eraserDistance = QPointF(radius + 2, radius + 2);
        eraserPen = QPen(Qt::red, 3, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin);
        eraserCircle = new QGraphicsEllipseItem(0, 0, eraserSize, eraserSize);
        eraserCircle->setPen(eraserPen);
    }

    foreach (QGraphicsView *view, scene->views())
        view->setDragMode(QGraphicsView::NoDrag);
}

void GeometricTool::setupActions()
{
    eraserCursor = QCursor(QPixmap(kAppProp->themeDir() + "cursors/eraser.png"), 4, 4);

    TAction *action1 = new TAction(QIcon(ICONS_DIR + "square.png"), tr("Rectangle"), this);
    action1->setShortcut(QKeySequence(tr("R")));
    action1->setToolTip(tr("Rectangle") + " - " + tr("R"));
    squareCursor = QCursor(kAppProp->themeDir() + "cursors/square.png", 0, 0);
    action1->setCursor(squareCursor);
    action1->setActionId(TAction::Rectangle);
    
    geoActions.insert(TAction::Rectangle, action1);
    
    TAction *action2 = new TAction(QIcon(ICONS_DIR + "ellipse.png"), tr("Ellipse"), this);
    action2->setShortcut(QKeySequence(tr("C")));
    action2->setToolTip(tr("Ellipse") + " - " + tr("C"));
    circleCursor = QCursor(kAppProp->themeDir() + "cursors/circle.png", 2, 2);
    action2->setCursor(circleCursor);
    action2->setActionId(TAction::Ellipse);

    geoActions.insert(TAction::Ellipse, action2);
    
    TAction *action3 = new TAction(QIcon(ICONS_DIR + "line.png"), tr("Line"), this);
    action3->setShortcut(QKeySequence(tr("L")));
    action3->setToolTip(tr("Line") + " - " + tr("L"));
    lineCursor = QCursor(kAppProp->themeDir() + "cursors/line.png", 0, 15);
    action3->setCursor(lineCursor);
    action3->setActionId(TAction::Line);

    geoActions.insert(TAction::Line, action3);

    TAction *action4 = new TAction(QIcon(ICONS_DIR + "triangle.png"), tr("Triangle"), this);
    action4->setShortcut(QKeySequence(tr("Ctrl+T")));
    action4->setToolTip(tr("Triangle") + " - " + tr("Ctrl+T"));
    triangleCursor = QCursor(kAppProp->themeDir() + "cursors/triangle.png", 5, 3);
    action4->setCursor(triangleCursor);
    action4->setActionId(TAction::Triangle);

    geoActions.insert(TAction::Triangle, action4);

    TAction *action5 = new TAction(QIcon(ICONS_DIR + "hexagon.png"), tr("Hexagon"), this);
    action5->setShortcut(QKeySequence(tr("H")));
    action5->setToolTip(tr("Hexagon") + " - " + tr("H"));
    hexagonCursor = QCursor(kAppProp->themeDir() + "cursors/hexagon.png", 5, 4);
    action5->setCursor(hexagonCursor);
    action5->setActionId(TAction::Hexagon);

    geoActions.insert(TAction::Hexagon, action5);
}

void GeometricTool::setZValueReferences()
{
    baseZValue = scene->getFrameZLevel(scene->currentLayerIndex(), scene->currentFrameIndex());
    topZValue = baseZValue + ITEMS_PER_FRAME;
}

QBrush GeometricTool::setLiteBrush(QColor color, Qt::BrushStyle style)
{
    QBrush brush;
    QColor pathColor = color;
    pathColor.setAlpha(50);
    brush.setColor(pathColor);
    brush.setStyle(style);

    return brush;
}

void GeometricTool::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager,
                          TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::press()]";
    #endif

    Q_UNUSED(input)
    Q_UNUSED(brushManager)

    if (input->buttons() == Qt::LeftButton) {
        fillBrush = brushManager->brush();
        if (toolId() == TAction::Rectangle) {
            added = false;
            rect = new TupRectItem(QRectF(input->pos(), QSize(0,0)));
            rect->setPen(brushManager->pen());
            if (brushManager->brush().color().alpha() > 0)
                rect->setBrush(setLiteBrush(brushManager->brush().color(), brushManager->brush().style()));
            else
                rect->setBrush(brushManager->brush());

            currentPoint = input->pos();
        } else if (toolId() == TAction::Ellipse) {
            added = false;
            ellipse = new TupEllipseItem(QRectF(input->pos(), QSize(0,0)));
            ellipse->setPen(brushManager->pen());
            if (brushManager->brush().color().alpha() > 0)
                ellipse->setBrush(setLiteBrush(brushManager->brush().color(), brushManager->brush().style()));
            else
                ellipse->setBrush(brushManager->brush());

            currentPoint = input->pos();
        } else if (toolId() == TAction::Line) {
            currentPoint = input->pos();

            if (currentToolMode == LineMode) {
                if (linePath) {
                    QPainterPath painterPath = linePath->path();
                    if (straightMode)
                        painterPath.lineTo(lastPoint);
                    else
                        painterPath.cubicTo(lastPoint, lastPoint, lastPoint);

                    linePath->setPath(painterPath);
                } else {
                    linePath = new TupPathItem;
                    linePath->setPen(brushManager->pen());
                    if (brushManager->brush().color().alpha() > 0)
                        linePath->setBrush(setLiteBrush(brushManager->brush().color(),
                                                        brushManager->brush().style()));
                    else
                        linePath->setBrush(brushManager->brush());

                    QPainterPath painterPath;
                    painterPath.moveTo(currentPoint);
                    linePath->setPath(painterPath);
                    gScene->includeObject(linePath);

                    guideLine = new TupLineItem();
                    if (brushManager->pen().color().alpha() == 0) { // Show border guide line
                        QPen pen;
                        pen.setWidth(1);
                        pen.setBrush(QBrush(Qt::black));
                        guideLine->setPen(pen);
                    } else {
                        guideLine->setPen(brushManager->pen());
                    }

                    guideLine->setLine(QLineF(input->pos().x(), input->pos().y(), input->pos().x(),
                                              input->pos().y()));
                    gScene->includeObject(guideLine);
                }
            } else { // Eraser mode
                eraserCircle->setPos(currentPoint - eraserDistance);
                gScene->includeObject(eraserCircle);
                if (!lineItems.isEmpty())
                    runEraser(currentPoint);
            }
        } else if (toolId() == TAction::Triangle) {
            added = false;
            triangle = new TupPathItem();
            triangle->setPen(brushManager->pen());
            if (brushManager->brush().color().alpha() > 0)
                triangle->setBrush(setLiteBrush(brushManager->brush().color(), brushManager->brush().style()));
            else
                triangle->setBrush(brushManager->brush());

            currentPoint = input->pos();
        } else if (toolId() == TAction::Hexagon) {
            added = false;
            hexagon = new TupPathItem();
            hexagon->setPen(brushManager->pen());
            if (brushManager->brush().color().alpha() > 0)
                hexagon->setBrush(setLiteBrush(brushManager->brush().color(), brushManager->brush().style()));
            else
                hexagon->setBrush(brushManager->brush());

            currentPoint = input->pos();
        }
    }
}

void GeometricTool::move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::move()]";
    #endif
    */

    Q_UNUSED(brushManager)
    Q_UNUSED(gScene)

    if (toolId() == TAction::Line) {
        if (currentToolMode == EraserMode) {
            QPointF currentPoint = input->pos();
            eraserCircle->setPos(currentPoint - eraserDistance);
            if (!lineItems.isEmpty())
                runEraser(currentPoint);
        }
    } else if (toolId() == TAction::Rectangle || toolId() == TAction::Ellipse ||
        toolId() == TAction::Triangle || toolId() == TAction::Hexagon) {
        if (!added) {
            if (toolId() == TAction::Rectangle)
                gScene->includeObject(rect);
            else if (toolId() == TAction::Ellipse)
                gScene->includeObject(ellipse);
            else if (toolId() == TAction::Triangle)
                gScene->includeObject(triangle);
            else if (toolId() == TAction::Hexagon)
                gScene->includeObject(hexagon);
            added = true;
        }

        int xMouse = static_cast<int> (input->pos().x());
        int yMouse = static_cast<int> (input->pos().y());
        int xInit = static_cast<int> (currentPoint.x());
        int yInit = static_cast<int> (currentPoint.y());

        QRectF rectVar;
        if (toolId() == TAction::Rectangle) {
            rectVar = rect->rect();
        } else if (toolId() == TAction::Ellipse) {
            rectVar = ellipse->rect();
        } else if (toolId() == TAction::Triangle) {
            rectVar = QRectF(currentPoint, QSize(0, 0));
        } else if (toolId() == TAction::Hexagon) {
            rectVar = QRectF(currentPoint, QSize(0, 0));
        }

        int width = abs(xMouse - xInit);
        int height = abs(yMouse - yInit);
        bool xWins = false;
        if (width <= height)
            xWins = true;

        if (proportion) {
            QPointF target;
            if (xMouse >= xInit) {
                if (yMouse >= yInit) {
                    if (xWins)
                        target = QPointF(xInit + width, yInit + width);      
                    else
                        target = QPointF(xInit + height, yInit + height);

                    rectVar.setBottomRight(target);
                } else {
                    if (xWins)
                        target = QPointF(xInit + width, yInit - width);
                    else
                        target = QPointF(xInit + height, yInit - height);

                    rectVar.setTopRight(target);
                }
            } else {
                if (yMouse >= yInit) {
                    if (xWins)
                        target = QPointF(xInit - width, yInit + width);
                    else
                        target = QPointF(xInit - height, yInit + height);

                    rectVar.setBottomLeft(target);
                } else {
                    if (xWins)
                        target = QPointF(xInit - width, yInit - width);
                    else
                        target = QPointF(xInit - height, yInit - height);

                    rectVar.setTopLeft(target);
                }
            }            
        } else { // No proportion
            if (side) {
                QPointF target;
                if (xMouse >= xInit) {
                    if (yMouse >= yInit) { // Right-Down
                        if (xWins) // Y > X
                            target = QPointF(xInit + xSideLength, yInit + height);
                        else // X > Y
                            target = QPointF(xInit + width, yInit + ySideLength);
                        rectVar.setBottomRight(target);
                    } else { // Right-Up
                        if (xWins) // Y > X
                            target = QPointF(xInit + xSideLength, yInit - height);
                        else // X > Y
                            target = QPointF(xInit + width, yInit - ySideLength);
                        rectVar.setTopRight(target);
                    }
                } else {
                    if (yMouse >= yInit) { // Left-Down
                        if (xWins) // Y > X
                            target = QPointF(xInit - xSideLength, yInit + height);
                        else // X > Y
                            target = QPointF(xInit - width, yInit + ySideLength);
                        rectVar.setBottomLeft(target);
                    } else { // Left-Up
                        if (xWins) // Y > X
                            target = QPointF(xInit - xSideLength, yInit - height);
                        else // X > Y
                            target = QPointF(xInit - width, yInit - ySideLength);
                        rectVar.setTopLeft(target);
                    }
                }
            } else {
                xSideLength = static_cast<int> (abs(xInit - input->pos().x()));
                ySideLength = static_cast<int> (abs(yInit - input->pos().y()));

                if (xMouse >= xInit) {
                    if (yMouse >= yInit)
                        rectVar.setBottomRight(input->pos());
                    else
                        rectVar.setTopRight(input->pos());
                } else {
                    if (yMouse >= yInit)
                        rectVar.setBottomLeft(input->pos());
                    else
                        rectVar.setTopLeft(input->pos());
                }
            }
        }

        if (toolId() == TAction::Rectangle) {
            rect->setRect(rectVar);
        } else if (toolId() == TAction::Ellipse) {
            ellipse->setRect(rectVar);
        } else if (toolId() == TAction::Triangle) {
            QPointF leftTopCorner = rectVar.topLeft();
            QPointF rightTopCorner = rectVar.topRight();
            QPointF leftBottomCorner = rectVar.bottomLeft();
            QPointF rightBottomCorner = rectVar.bottomRight();

            QPointF point1;
            QPointF point2;
            QPointF point3;

            trianglePath = QPainterPath();

            switch(triangleType) {
                case GeometricSettings::Top:
                {
                    double topCornerX = leftTopCorner.x() + ((rightBottomCorner.x() - leftTopCorner.x())/2);
                    double topCornerY = leftTopCorner.y();

                    point1 = QPointF(topCornerX, topCornerY);
                    point2 = leftBottomCorner;
                    point3 = rightBottomCorner;

                    break;
                }
                case GeometricSettings::Bottom:
                {
                    point1 = leftTopCorner;
                    point2 = rightTopCorner;

                    double bottomCornerX = leftBottomCorner.x() + ((rightBottomCorner.x() - leftBottomCorner.x())/2);
                    double bottomCornerY = leftBottomCorner.y();
                    point3 = QPointF(bottomCornerX, bottomCornerY);

                    break;
                }
                case GeometricSettings::Left:
                {
                    double leftCornerX = leftTopCorner.x();
                    double leftCornerY = leftTopCorner.y() + ((leftBottomCorner.y() - leftTopCorner.y())/2);

                    point1 = QPointF(leftCornerX, leftCornerY);
                    point2 = rightTopCorner;
                    point3 = rightBottomCorner;

                    break;
                }
                case GeometricSettings::Right:
                {
                    double rightCornerX = rightTopCorner.x();
                    double rightCornerY = rightTopCorner.y() + ((rightBottomCorner.y() - rightTopCorner.y())/2);

                    point1 = QPointF(rightCornerX, rightCornerY);
                    point2 = leftTopCorner;
                    point3 = leftBottomCorner;

                    break;
                }
                case GeometricSettings::TopLeft:
                {
                    point1 = leftTopCorner;
                    point2 = rightTopCorner;
                    point3 = leftBottomCorner;

                    break;
                }
                case GeometricSettings::TopRight:
                {
                    point1 = rightTopCorner;
                    point2 = leftTopCorner;
                    point3 = rightBottomCorner;

                    break;
                }
                case GeometricSettings::BottomLeft:
                {
                    point1 = leftBottomCorner;
                    point2 = leftTopCorner;
                    point3 = rightBottomCorner;

                    break;
                }
                case GeometricSettings::BottomRight:
                {
                    point1 = rightBottomCorner;
                    point2 = rightTopCorner;
                    point3 = leftBottomCorner;

                    break;
                }
            }
            trianglePath.moveTo(point1);

            if (straightMode) { // Straight Line
                trianglePath.lineTo(point2);
                trianglePath.lineTo(point3);
                trianglePath.lineTo(point1);
            } else { // Curve
                trianglePath.cubicTo(point2, point2, point2);
                trianglePath.cubicTo(point3, point3, point3);
                trianglePath.cubicTo(point1, point1, point1);
            }

            triangle->setPath(trianglePath);        
        } else if (toolId() == TAction::Hexagon) {
            QPointF leftTopCorner = rectVar.topLeft();
            QPointF rightTopCorner = rectVar.topRight();
            QPointF rightBottomCorner = rectVar.bottomRight();
            QPointF leftBottomCorner = rectVar.bottomLeft();

            hexagonPath = QPainterPath();

            QList<QPointF> points;

            if (hexagonType == GeometricSettings::Horizontal) {
                int xSide = (rightTopCorner.x() - leftTopCorner.x())/2;
                int shortXSide = xSide/2;
                int shortYSide = rectVar.height() / 11;

                points << QPointF(leftTopCorner.x() + shortXSide, leftTopCorner.y() + shortYSide);
                points << QPointF(leftTopCorner.x() + shortXSide + xSide, leftTopCorner.y() + shortYSide);
                points << QPointF(rightTopCorner.x(), rightTopCorner.y() + ((rightBottomCorner.y() - rightTopCorner.y())/2));
                points << QPointF(leftBottomCorner.x() + xSide + shortXSide, leftBottomCorner.y() - shortYSide);
                points << QPointF(leftBottomCorner.x() + shortXSide, leftBottomCorner.y() - shortYSide);
                points << QPointF(leftTopCorner.x(), rightTopCorner.y() + ((rightBottomCorner.y() - rightTopCorner.y())/2));
            } else { // Vertical
                int ySide = (rightBottomCorner.y() - rightTopCorner.y())/2;
                int shortYSide = ySide/2;
                int shortXSide = rectVar.width() / 11;

                points << QPointF(leftTopCorner.x() + shortXSide, leftTopCorner.y() + shortYSide);
                points << QPointF(leftTopCorner.x() + ((rightTopCorner.x() - leftTopCorner.x())/2), leftTopCorner.y());
                points << QPointF(rightTopCorner.x() - shortXSide, leftTopCorner.y() + shortYSide);
                points << QPointF(rightTopCorner.x() - shortXSide, leftTopCorner.y() + shortYSide + ySide);
                points << QPointF(leftTopCorner.x() + ((rightTopCorner.x() - leftTopCorner.x())/2), leftBottomCorner.y());
                points << QPointF(leftTopCorner.x() + shortXSide, leftTopCorner.y() + shortYSide + ySide);
            }

            hexagonPath.moveTo(points[0]);

            if (straightMode) { // Straight Line
                hexagonPath.lineTo(points[1]);
                hexagonPath.lineTo(points[2]);
                hexagonPath.lineTo(points[3]);
                hexagonPath.lineTo(points[4]);
                hexagonPath.lineTo(points[5]);
                hexagonPath.lineTo(points[0]);
            } else { // Curve
                hexagonPath.cubicTo(points[1], points[1], points[1]);
                hexagonPath.cubicTo(points[2], points[2], points[2]);
                hexagonPath.cubicTo(points[3], points[3], points[3]);
                hexagonPath.cubicTo(points[4], points[4], points[4]);
                hexagonPath.cubicTo(points[5], points[5], points[5]);
                hexagonPath.cubicTo(points[0], points[0], points[0]);
            }

            hexagon->setPath(hexagonPath);
        }
    }
}

void GeometricTool::release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::release()]";
    #endif

    Q_UNUSED(input)
    Q_UNUSED(brushManager)

    QDomDocument doc;
    QPointF point;

    auto discardDegenerateItem = [](QGraphicsItem *item) {
        if (!item)
            return;

        QGraphicsScene *itemScene = item->scene();
        if (itemScene)
            itemScene->removeItem(item);
        delete item;
    };

    if (toolId() == TAction::Rectangle) {
        const QRectF geometry = rect->rect().normalized();
        if (qFuzzyIsNull(geometry.width()) || qFuzzyIsNull(geometry.height())) {
            discardDegenerateItem(rect);
            rect = nullptr;
            added = false;
            return;
        }

        rect->setBrush(fillBrush);
        doc.appendChild(dynamic_cast<TupAbstractSerializable *>(rect)->toXml(doc));
        point = rect->pos();
    } else if (toolId() == TAction::Ellipse) {
        const QRectF geometry = ellipse->rect().normalized();
        if (qFuzzyIsNull(geometry.width()) || qFuzzyIsNull(geometry.height())) {
            discardDegenerateItem(ellipse);
            ellipse = nullptr;
            added = false;
            return;
        }

        ellipse->setBrush(fillBrush);
        doc.appendChild(dynamic_cast<TupAbstractSerializable *>(ellipse)->toXml(doc));
        point = QPoint(0, 0);
    } else if (toolId() == TAction::Triangle) {
        const QRectF geometry = triangle->path().boundingRect().normalized();
        if (triangle->path().isEmpty()
                || qFuzzyIsNull(geometry.width())
                || qFuzzyIsNull(geometry.height())) {
            discardDegenerateItem(triangle);
            triangle = nullptr;
            added = false;
            return;
        }

        triangle->setBrush(fillBrush);
        doc.appendChild(dynamic_cast<TupAbstractSerializable *>(triangle)->toXml(doc));
        point = triangle->pos();        
    } else if (toolId() == TAction::Hexagon) {
        const QRectF geometry = hexagon->path().boundingRect().normalized();
        if (hexagon->path().isEmpty()
                || qFuzzyIsNull(geometry.width())
                || qFuzzyIsNull(geometry.height())) {
            discardDegenerateItem(hexagon);
            hexagon = nullptr;
            added = false;
            return;
        }

        hexagon->setBrush(fillBrush);
        doc.appendChild(dynamic_cast<TupAbstractSerializable *>(hexagon)->toXml(doc));
        point = hexagon->pos();
    } else if (toolId() == TAction::Line) {
        if (currentToolMode == EraserMode) {
            gScene->removeItem(eraserCircle);
            if (!lineItems.isEmpty())
                runEraser(input->pos());
        }
        return;
    }

    if (!doc.toString().isEmpty()) {
        TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(),
                                  gScene->currentFrameIndex(), 0, point, gScene->getSpaceContext(), TupLibraryObject::Item,
                                  TupProjectRequest::Add, doc.toString());
        emit requested(&event);
    }
}

QMap<TAction::ActionId, TAction *> GeometricTool::actions() const
{
    return geoActions;
}

TAction * GeometricTool::getAction(TAction::ActionId toolId)
{
    return geoActions[toolId];
}

int GeometricTool::toolType() const
{
    return Shape;
}
        
QWidget *GeometricTool::configurator()
{
    GeometricSettings::ToolType toolType = GeometricSettings::Line;

    if (toolId() == TAction::Rectangle)
        toolType = GeometricSettings::Rectangle;
    else if (toolId() == TAction::Ellipse)
        toolType = GeometricSettings::Ellipse;
    else if (toolId() == TAction::Triangle)
        toolType = GeometricSettings::Triangle;
    else if (toolId() == TAction::Hexagon)
        toolType = GeometricSettings::Hexagon;

    settings = new GeometricSettings(toolType);
    connect(settings, SIGNAL(lineTypeChanged(GeometricSettings::LineType)),
            this, SLOT(updateLineMode(GeometricSettings::LineType)));
    connect(settings, SIGNAL(triangleTypeChanged(GeometricSettings::TriangleType)),
            this, SLOT(updateTriangleType(GeometricSettings::TriangleType)));
    connect(settings, SIGNAL(hexagonTypeChanged(GeometricSettings::HexagonType)),
            this, SLOT(updateHexagonType(GeometricSettings::HexagonType)));

    connect(settings, SIGNAL(toolEnabled(ToolMode)),
            this, SLOT(updateToolMode(ToolMode)));
    connect(settings, SIGNAL(eraserSizeChanged(int)),
            this, SLOT(updateEraserSize(int)));

    return settings;
}

void GeometricTool::aboutToChangeScene(TupGraphicsScene *scene)
{
    Q_UNUSED(scene)

    endItem();
}

void GeometricTool::aboutToChangeTool() 
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::aboutToChangeTool()]";
    #endif

    endItem();
    saveLineSettings();
}

void GeometricTool::saveConfig()
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::saveConfig()]";
    #endif

    saveLineSettings();
}

void GeometricTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11 || event->key() == Qt::Key_Escape) {
        emit closeHugeCanvas();

        return;
    } else if (event->key() == Qt::Key_Control) {
        proportion = true;
    } else if (event->key() == Qt::Key_Shift) {
        side = true;
    } else if (event->key() == Qt::Key_X) {
        if (toolId() == TAction::Line)
            endItem();
    } else {
        QPair<int, int> flags = TAction::setKeyAction(event->key(), event->modifiers());
        if (flags.first != -1 && flags.second != -1)
            emit callForPlugin(flags.first, flags.second);
    }
}

void GeometricTool::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
        side = false;
    else if (event->key() == Qt::Key_Control)
        proportion = false;
}

QCursor GeometricTool::toolCursor()
{
    if (toolId() == TAction::Rectangle) {
        return squareCursor;
    } else if (this->toolId() == TAction::Ellipse) {
        return circleCursor;
    } else if (this->toolId() == TAction::Line) {
        if (currentToolMode == EraserMode)
            return eraserCursor;
        return lineCursor;
    } else if (this->toolId() == TAction::Hexagon) {
        return hexagonCursor;
    }    

    return QCursor(Qt::ArrowCursor);
}

void GeometricTool::endItem()
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::endItem()]";
    #endif

    if (linePath) {
        const QPainterPath path = linePath->path();
        if (path.elementCount() <= 1) {
            QGraphicsScene *itemScene = linePath->scene();
            if (itemScene)
                itemScene->removeItem(linePath);
            delete linePath;
            linePath = nullptr;

            if (guideLine) {
                QGraphicsScene *guideScene = guideLine->scene();
                if (guideScene)
                    guideScene->removeItem(guideLine);
                delete guideLine;
                guideLine = nullptr;
            }
            return;
        }

        linePath->setBrush(fillBrush);
        QDomDocument doc;
        doc.appendChild(dynamic_cast<TupAbstractSerializable *>(linePath)->toXml(doc));
        QPointF point = QPointF(0, 0);

        TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(), scene->currentLayerIndex(),
                                  scene->currentFrameIndex(), 0, point, scene->getSpaceContext(), TupLibraryObject::Item,
                                  TupProjectRequest::Add, doc.toString());

        emit requested(&event);
        linePath = nullptr;
    }
}

void GeometricTool::updatePos(QPointF pos)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::updatePos()] - pos -> " << pos;
    #endif
    */

    if (linePath) {
        QLineF lineVar;
        if (proportion) {
            qreal dx = pos.x() - currentPoint.x();
            qreal dy = pos.y() - currentPoint.y();
            qreal m = fabs(dx/dy);

            if (m > 1) {
                lineVar = QLineF(currentPoint.x(), currentPoint.y(), pos.x(), currentPoint.y());
                lastPoint = QPointF(pos.x(), currentPoint.y());
            } else {
                lineVar = QLineF(currentPoint.x(), currentPoint.y(), currentPoint.x(), pos.y());
                lastPoint = QPointF(currentPoint.x(), pos.y());
            }

        } else {
            lineVar = QLineF(currentPoint, pos);
            lastPoint = pos;
        }
        if (guideLine)
            guideLine->setLine(lineVar);
    }
}

/*
void GeometricTool::doubleClick(const TupInputDeviceInformation *input, TupGraphicsScene *gScene)
{
    Q_UNUSED(input);
    Q_UNUSED(gScene);

    endItem();
}
*/

void GeometricTool::sceneResponse(const TupSceneResponse *event)
{
    Q_UNUSED(event)

    if (toolId() == TAction::Line)
        init(scene);
}

void GeometricTool::layerResponse(const TupLayerResponse *event)
{
    Q_UNUSED(event)

    if (toolId() == TAction::Line)
        init(scene);
}

void GeometricTool::frameResponse(const TupFrameResponse *event)
{
    Q_UNUSED(event)

    if (toolId() == TAction::Line) {
        init(scene);
        setZValueReferences();
    }
}

void GeometricTool::itemResponse(const TupItemResponse *event)
{
    if (currentToolMode == EraserMode) {
        scene->drawCurrentPhotogram();
        if (event->getAction() == TupProjectRequest::Add)
            storePathItems();
        for (int i=0; i<lineItems.size(); i++) {
            TupPathItem *item = lineItems.at(i);
            if (item->isBlocked())
                item->updateBlockingFlag(false);
        }
    }
}

void GeometricTool::updateLineMode(GeometricSettings::LineType type)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::updateLineMode()] - type ->" << type;
    #endif

    if (type == GeometricSettings::Bendable)
        straightMode = false;
    else
        straightMode = true;
}

void GeometricTool::saveLineSettings()
{
    TCONFIG->beginGroup("GeometricTool");
    if (straightMode)
        TCONFIG->setValue("LineType", 1);
    else
        TCONFIG->setValue("LineType", 0);
}

void GeometricTool::updateTriangleType(GeometricSettings::TriangleType type)
{
    triangleType = type;
}

void GeometricTool::updateHexagonType(GeometricSettings::HexagonType type)
{
    hexagonType = type;
}

void GeometricTool::updateToolMode(ToolMode tool)
{
    #ifdef TUP_DEBUG
        QString toolName = "Eraser";
        if (tool == 0)
            toolName = "Line";

        qDebug() << "---";
        qDebug() << "[GeometricTool::updateToolMode()] - Switching to tool ->" << toolName;
    #endif

    emit toolModeUpdated(tool);

    currentToolMode = tool;
    if (tool == EraserMode) {
        endItem();
        storePathItems();
    }
}

void GeometricTool::storePathItems()
{
    // Store all the path items of the current frame in a list
    lineItems.clear();
    foreach (QGraphicsItem *item, scene->items()) {
        if (TupPathItem *line = qgraphicsitem_cast<TupPathItem *> (item)) {
            int zVal = line->zValue();
            if (baseZValue <= zVal && zVal < topZValue)
                lineItems << line;
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::storePathItems()] - lineItems.size() ->" << lineItems.size();
    #endif
}

TupFrame* GeometricTool::getCurrentFrame()
{
    TupFrame *frame = nullptr;
    if (scene->getSpaceContext() == TupProject::FRAMES_MODE) {
        frame = scene->currentFrame();
        currentLayer = scene->currentLayerIndex();
        currentFrame = scene->currentFrameIndex();
    } else {
        currentLayer = -1;
        currentFrame = -1;

        TupScene *tupScene = scene->currentScene();
        TupBackground *bg = tupScene->sceneBackground();
        if (tupScene && bg) {
            if (scene->getSpaceContext() == TupProject::VECTOR_STATIC_BG_MODE) {
                frame = bg->vectorStaticFrame();
            } else if (scene->getSpaceContext() == TupProject::VECTOR_FG_MODE) {
                frame = bg->vectorForegroundFrame();
            } else if (scene->getSpaceContext() == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                frame = bg->vectorDynamicFrame();
            }
        }
    }

    return frame;
}

void GeometricTool::runEraser(const QPointF &point)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::runEraser()] - Evaluating point ->" << point;
    #endif

    // Checking if the point matches any of the frame paths
    for (int i=0; i<lineItems.size(); i++) {
        TupPathItem *item = lineItems.at(i);
        if (item->isBlocked())
            continue;

        if (item->pointMatchesPath(point, eraserSize/2, EraserMode)) {
            #ifdef TUP_DEBUG
                qDebug() << "---";
                qDebug() << "[GeometricTool::runEraser()] - MATCH!!!";
            #endif

            // Path matches the point
            QPair<QString, QString> segments = item->recalculatePath(point, eraserSize/2);
            QString segment1 = segments.first;
            QString segment2 = segments.second;

            if (segment1.compare("-1") != 0) {
                TupFrame *frame = getCurrentFrame();
                int itemIndex = frame->indexOf(item);

                if (itemIndex == -1) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[GeometricTool::runEraser()] - Fatal Error: Invalid item index -> -1";
                    #endif
                    return;
                }

                if (!segment1.isEmpty() && !segment2.isEmpty()) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, segment1);
                    emit requested(&event);

                    TupPathItem *lineItem = new TupPathItem;
                    lineItem->setPen(brushManager->pen());
                    lineItem->setBrush(brushManager->brush());
                    lineItem->setPathFromString(segment2);

                    QDomDocument doc;
                    doc.appendChild(lineItem->toXml(doc));
                    event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(), scene->currentLayerIndex(), scene->currentFrameIndex(),
                                                                 0, QPoint(), scene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                                 doc.toString());
                    emit requested(&event);
                } else if (segment2.isEmpty() && !segment1.isEmpty()) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, segment1);
                    emit requested(&event);
                } else {
                    scene->removeItem(item);
                    lineItems.removeAt(i);

                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex, QPointF(), scene->getSpaceContext(),
                                                                                   TupLibraryObject::Item, TupProjectRequest::Remove);
                    emit requested(&event);
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[GeometricTool::runEraser()] - Warning: Eraser action FAILED!";
                #endif
            }
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "---";
                qDebug() << "[GeometricTool::runEraser()] - NO match!!!";
            #endif
        }
    }
}

void GeometricTool::updateEraserSize(int size)
{
    #ifdef TUP_DEBUG
        qDebug() << "[GeometricTool::updateEraserSize()] - size ->" << size;
    #endif

    eraserSize = size;
    qreal radius = eraserSize/2;
    eraserDistance = QPointF(radius, radius);
    eraserCircle->setRect(radius/2, radius/2, radius, radius);
}
