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

#include "penciltool.h"
#include "tuppaintareaevent.h"
#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tupgraphicalgorithm.h"

#include <QGraphicsView>
#include <QPolygonF>
#include <cmath>

PencilTool::PencilTool(): TupToolPlugin()
{
    settings = nullptr;
    item = nullptr;

    setupActions();
}

PencilTool::~PencilTool()
{
}

void PencilTool::setupActions()
{
    penCursor = QCursor(QPixmap(kAppProp->themeDir() + "cursors/target.png"), 4, 4);
    eraserCursor = QCursor(QPixmap(kAppProp->themeDir() + "cursors/eraser.png"), 4, 4);

    TAction *pencil = new TAction(QPixmap(ICONS_DIR + "pencil.png"), tr("Pencil"), this);
    pencil->setShortcut(QKeySequence(tr("P")));
    pencil->setToolTip(tr("Pencil") + " - " + tr("P"));
    pencil->setCursor(penCursor);
    pencil->setActionId(TAction::Pencil);

    penActions.insert(TAction::Pencil, pencil);
}

void PencilTool::init(TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::init()]";
    #endif

    currentToolMode = PencilMode;
    settings->enablePencilMode();

    scene = gScene;
    brushManager = scene->getBrushManager();
    input = scene->inputDeviceInformation();
    resizeMode = false;

    setZValueReferences();
    circleZValue = ZLAYER_BASE + (scene->layersCount() * ZLAYER_LIMIT);

    TCONFIG->beginGroup("BrushParameters");
    penWidth = TCONFIG->value("Thickness", 3).toInt();
    eraserSize = TCONFIG->value("EraserSize", 10).toInt();

    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::init()] - eraserSize ->" << eraserSize;
    #endif

    qreal radius = eraserSize/2;
    eraserDistance = QPointF(radius + 2, radius + 2);
    eraserPen = QPen(Qt::red, 3, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin);
    eraserCircle = new QGraphicsEllipseItem(0, 0, eraserSize, eraserSize);
    eraserCircle->setPen(eraserPen);

    foreach (QGraphicsView *view, scene->views())
        view->setDragMode(QGraphicsView::NoDrag);

    lineAdded = false;
}

void PencilTool::setZValueReferences()
{
    baseZValue = scene->getFrameZLevel(scene->currentLayerIndex(), scene->currentFrameIndex());
    topZValue = baseZValue + ITEMS_PER_FRAME;
}

QList<TAction::ActionId> PencilTool::keys() const
{    
    return QList<TAction::ActionId>() << TAction::Pencil;
}

void PencilTool::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::press()] - currentTool ->" << currentToolMode;
    #endif

    firstPoint = input->pos();

    if (currentToolMode == PencilMode) {
        if (!resizeMode) {
            path = QPainterPath();
            path.moveTo(firstPoint);

            previousPos = input->pos();

            item = new TupPathItem();
            if (brushManager->pen().color().alpha() == 0) {
                QPen pen;
                pen.setWidth(1);
                pen.setBrush(QBrush(Qt::black));
                item->setPen(pen);
            } else {
                item->setPen(brushManager->pen());
            }
            gScene->includeObject(item);
        }
    } else { // EraserMode
        eraserCircle->setPos(firstPoint - eraserDistance);
        gScene->includeObject(eraserCircle);
        if (!lineItems.isEmpty())
            runEraser(firstPoint);
    }
}

void PencilTool::move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::move()]";
    #endif

    Q_UNUSED(brushManager)
    Q_UNUSED(gScene)

    QPointF currentPoint = input->pos();
    if (currentToolMode == PencilMode) {
        if (resizeMode) {
            QPointF result = penCirclePos - currentPoint;
            penWidth = static_cast<int>(sqrt(pow(result.x(), 2) + pow(result.y(), 2)));

            QPointF topLeft(penCirclePos.x() - (penWidth/2), penCirclePos.y() - (penWidth/2));
            QSize size(penWidth, penWidth);
            QRectF rect(topLeft, size);
            penCircle->setRect(rect);
        } else {
            if (!item)
                return;

            QPointF lastPoint = input->pos();

            path.moveTo(previousPos);
            path.lineTo(lastPoint);

            item->setPath(path);
            previousPos = lastPoint;
        }
    } else {
        eraserCircle->setPos(currentPoint - eraserDistance);
        if (!lineItems.isEmpty())
            runEraser(currentPoint);
    }
}

void PencilTool::release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::release()] - currentTool ->" << currentToolMode;
    #endif

    Q_UNUSED(brushManager)

    QPointF currentPoint = input->pos();
    if (currentToolMode == PencilMode) {
        if (!resizeMode) {
            if (!item)
                return;

            // Drawing a point
            if (firstPoint == input->pos() && path.elementCount() == 1) {
                gScene->removeItem(item);

                qreal radius = brushManager->pen().width();
                QPointF distance((radius + 2)/2, (radius + 2)/2);
                QPen inkPen(brushManager->penColor(), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                TupEllipseItem *blackEllipse = new TupEllipseItem(QRectF(currentPoint - distance,
                                                                  QSize(static_cast<int>(radius + 2), static_cast<int>(radius + 2))));
                blackEllipse->setPen(inkPen);
                blackEllipse->setBrush(inkPen.brush());
                gScene->includeObject(blackEllipse);

                QDomDocument doc;
                doc.appendChild(blackEllipse->toXml(doc));
                TupProjectRequest request = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(), gScene->currentFrameIndex(),
                                                                                 0, QPointF(0, 0), gScene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                                                 doc.toString());
                emit requested(&request);

                return;
            }

            TupGraphicalAlgorithm::smoothPath(path, smoothness);

            item->setPen(brushManager->pen());
            item->setBrush(brushManager->brush());
            item->setPath(path);

            QDomDocument doc;
            doc.appendChild(item->toXml(doc));

            TupProjectRequest request = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(), gScene->currentLayerIndex(), gScene->currentFrameIndex(),
                                                                             0, QPoint(), gScene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                                             doc.toString());
            emit requested(&request);

            // Temporary code for debugging purposes
            // addKeyPoints(item);
            // addCurvePoints(item);
        }
    } else { // Eraser Mode
        gScene->removeItem(eraserCircle);
        if (!lineItems.isEmpty())
            runEraser(currentPoint);
    }
}

QMap<TAction::ActionId, TAction *> PencilTool::actions() const
{
    return penActions;
}

TAction * PencilTool::getAction(TAction::ActionId toolId)
{
    return penActions[toolId];
}

int PencilTool::toolType() const
{
    return TupToolInterface::Brush;
}

QWidget *PencilTool::configurator()
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::configurator()]";
    #endif

    if (!settings) {
        settings = new PencilSettings;
        connect(settings, SIGNAL(toolEnabled(ToolMode)), this, SLOT(updateToolMode(ToolMode)));
        connect(settings, SIGNAL(smoothnessUpdated(double)), this, SLOT(updateSmoothness(double)));
        connect(settings, SIGNAL(eraserSizeChanged(int)), this, SLOT(updateEraserSize(int)));

        TCONFIG->beginGroup("PencilTool");
        smoothness = TCONFIG->value("Smoothness", 4.0).toDouble();
        if (smoothness == 0.0)
            smoothness = 4.0;
        settings->updateSmoothness(smoothness);
    }

    return settings;
}

void PencilTool::updateToolMode(ToolMode tool)
{
    #ifdef TUP_DEBUG
        QString toolName = "Eraser";
        if (tool == 0)
            toolName = "Pencil";

        qDebug() << "---";
        qDebug() << "[PencilTool::updateToolMode()] - Switching to tool ->" << toolName;
    #endif

    emit toolModeUpdated(tool);

    currentToolMode = tool;
    if (tool == EraserMode) {
        storePathItems();
        /*
        // Temporary code for debugging purposes
        for (int i=0; i<lineItems.size(); i++) {
            TupPathItem *item = lineItems.at(i);
            addCurvePoints(item);
            addKeyPoints(item);
        }
        */
    }
}

void PencilTool::storePathItems()
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::storePathItems()]";
    #endif

    // Store all the path items of the current frame in a list
    lineItems.clear();
    foreach (QGraphicsItem *item, scene->items()) {
        if (TupPathItem *line = qgraphicsitem_cast<TupPathItem *> (item)) {
            qDebug() << "---";
            qDebug() << "   BASE Z Value -> " << baseZValue;
            qDebug() << "   -- line Z Value -> " << line->zValue();
            qDebug() << "   TOP Z Value -> " << topZValue;
            qDebug() << "---";

            int zVal = line->zValue();
            if (baseZValue <= zVal && zVal < topZValue) {
                qDebug() << "Storing line!";
                lineItems << line;
            } else {
                qDebug() << "Path is NOT part of the same frame!";
            }
        }
    }
}

void PencilTool::updateSmoothness(double value)
{
    smoothness = value;

    TCONFIG->beginGroup("PencilTool");
    TCONFIG->setValue("Smoothness", QString::number(smoothness, 'f', 2));
}

void PencilTool::aboutToChangeTool() 
{
}

void PencilTool::saveConfig()
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::saveConfig()]";
    #endif

    TCONFIG->beginGroup("PencilTool");
    TCONFIG->setValue("Smoothness", QString::number(smoothness, 'f', 2));

    TCONFIG->beginGroup("BrushParameters");
    TCONFIG->setValue("EraserSize", eraserSize);
}

void PencilTool::keyPressEvent(QKeyEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::keyPressEvent()]";
    #endif

    if (event->modifiers() == Qt::ShiftModifier && currentToolMode == PencilMode) {
        resizeMode = true;
        input = scene->inputDeviceInformation();
        int diameter = brushManager->penWidth();
        int radius = diameter/2;
        penCirclePos = input->pos();

        penCircle = new QGraphicsEllipseItem(penCirclePos.x() - radius, penCirclePos.y() - radius, diameter, diameter);
        penCircle->setZValue(circleZValue);
        scene->addItem(penCircle);

        return;
    }

    if (event->key() == Qt::Key_F11 || event->key() == Qt::Key_Escape) {
        emit closeHugeCanvas();

        return;
    }

    QPair<int, int> flags = TAction::setKeyAction(event->key(), event->modifiers());
    if (flags.first != -1 && flags.second != -1)
        emit callForPlugin(flags.first, flags.second);
}

void PencilTool::keyReleaseEvent(QKeyEvent *event) 
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::keyReleaseEvent()]";
    #endif

    Q_UNUSED(event)

    if (resizeMode) {
        resizeMode = false;
        scene->removeItem(penCircle);

        TCONFIG->beginGroup("BrushParameters");
        TCONFIG->setValue("Thickness", penWidth);

        emit penWidthChanged(penWidth);
    }
}

QCursor PencilTool::toolCursor() // const
{    
    if (currentToolMode == PencilMode) {
        return penCursor;
    } else if (currentToolMode == EraserMode) {
        return eraserCursor;
    }

    return QCursor(Qt::ArrowCursor);
}

void PencilTool::sceneResponse(const TupSceneResponse *event)
{
    Q_UNUSED(event)
}

void PencilTool::frameResponse(const TupFrameResponse *event)
{
    Q_UNUSED(event)

    setZValueReferences();
}

void PencilTool::itemResponse(const TupItemResponse *event)
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

void PencilTool::updateEraserSize(int size)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::updateEraserSize()] - size ->" << size;
    #endif

    eraserSize = size;
    qreal radius = eraserSize/2;
    eraserDistance = QPointF(radius, radius);
    eraserCircle->setRect(radius/2, radius/2, radius, radius);
}

TupFrame* PencilTool::getCurrentFrame()
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

void PencilTool::runEraser(const QPointF &point)
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[PencilTool::runEraser()] - Evaluating point ->" << point;
    #endif

    qDebug() << "PencilTool::runEraser() - lineItems.size() ->" << lineItems.size();
    // Checking if the point matches any of the frame paths
    for (int i=0; i<lineItems.size(); i++) {
        TupPathItem *item = lineItems.at(i);
        if (item->isBlocked()) {
            qDebug() << "PencilTool::runEraser() - Path is blocked... ignoring...";
            continue;
        }

        if (item->pointMatchesPath(point, eraserSize/2, EraserMode)) {
            // Path matches the point
            qDebug() << "-------------------------------------------------------";
            qDebug() << "PencilTool::runEraser() - MATCH!!! MATCH!!! MATCH!!!";
            // qDebug() << "PencilTool::runEraser() - eraser size ->" << eraserSize;
            QPair<QString, QString> segments = item->recalculatePath(point, eraserSize/2);
            QString segment1 = segments.first;
            QString segment2 = segments.second;

            qDebug() << "  *** segment1 ->" << segment1;
            qDebug() << "  *** segment2 ->" << segment2;

            if (segment1.compare("-1") != 0) {
                TupFrame *frame = getCurrentFrame();
                int itemIndex = frame->indexOf(item);

                qDebug() << "  *** itemIndex ->" << itemIndex;
                qDebug() << "  *** item zValue ->" << item->zValue();
                qDebug() << "  *** frame total count ->" << frame->itemsTotalCount();

                if (itemIndex == -1) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[PencilTool::runEraser()] - Fatal Error: Invalid item index -> -1";
                    #endif

                    return;
                }

                if (!segment1.isEmpty() && !segment2.isEmpty()) {
                    qDebug() << "PencilTool::runEraser() - Adding TWO segments!";
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

                    qDebug() << "PencilTool::runEraser() - Creating additional path segment!";
                    qDebug() << "PencilTool::runEraser() - segment2 ->" << segment2;
                    qDebug() << "PencilTool::runEraser() - path definition ->" << doc.toString();

                    event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(), scene->currentLayerIndex(), scene->currentFrameIndex(),
                                                                 0, QPoint(), scene->getSpaceContext(), TupLibraryObject::Item, TupProjectRequest::Add,
                                                                 doc.toString());
                    emit requested(&event);
                } else if (segment2.isEmpty() && !segment1.isEmpty()) {
                    qDebug() << "PencilTool::runEraser() - Adding segment1";
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, segment1);
                    emit requested(&event);

                    /*
                    // Temporary code for debugging purposes
                    TupPathItem *debugPath = new TupPathItem;
                    debugPath->setPathFromString(segment1);
                    addCurvePoints(debugPath);
                    addKeyPoints(debugPath);
                    */
                } else {
                    qDebug() << "PencilTool::runEraser() - Removing item...";
                    qDebug() << "currentLayer ->" << currentLayer;
                    qDebug() << "currentFrame ->" << currentFrame;
                    qDebug() << "position ->" << itemIndex;

                    scene->removeItem(item);
                    lineItems.removeAt(i);

                    // Temporary code for debugging purposes
                    // removeKeyPoints();

                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex, QPointF(), scene->getSpaceContext(),
                                                                                   TupLibraryObject::Item, TupProjectRequest::Remove);
                    emit requested(&event);
                }
            } else {
                qDebug() << "PencilTool::runEraser() - Warning: Eraser action FAILED!";
            }

            qDebug() << "---";
            qDebug() << "";
        } else {
            qDebug() << "---";
            qDebug() << "PencilTool::runEraser() - NO match!!!";
        }
    }
}

/*
// Temporary code for debugging purposes
void PencilTool::addKeyPoints(TupPathItem *item)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::addKeyPoints()]";
    #endif

    foreach (TupEllipseItem *item, pathEllipsesList)
        scene->removeItem(item);
    pathEllipsesList.clear();

    if (!item) {
        #ifdef TUP_DEBUG
            qDebug() << "[PencilTool::addKeyPoints()] - Fatal Error: Path is NULL!";
        #endif

        return;
    }

    qDebug() << "[PencilTool::addKeyPoints()] - path ->" << item->pathToString();

    QList<QPointF> points = item->keyNodes();
    if (points.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[PencilTool::addKeyPoints()] - Fatal Error: Path item has no points!";
        #endif

        return;
    }

    QList<QColor> colors = item->nodeColors();
    QList<QString> tips = item->nodeTips();
    int pointsCounter = 0;

    if (lineAdded)
        scene->removeItem(lineItem);

    QPainterPath path;
    path.moveTo(points.at(0));

    foreach (QPointF point, points) {
        qreal radius = penWidth;
        QPointF distance((radius + 2)/2, (radius + 2)/2);
        QPen pointPen(colors.at(pointsCounter), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        TupEllipseItem *ellipse = new TupEllipseItem(QRectF(point - distance,
                                                     QSize(static_cast<int>(radius + 2), static_cast<int>(radius + 2))));
        ellipse->setPen(pointPen);
        ellipse->setBrush(pointPen.brush());
        ellipse->setToolTip(tips.at(pointsCounter));
        scene->includeObject(ellipse);
        pathEllipsesList << ellipse;

        if (pointsCounter > 0) {
           path.lineTo(point);
        }

        pointsCounter++;
    }

    qDebug() << "[PencilTool::addKeyPoints()] - Adding axis line!";
    qDebug() << "[PencilTool::addKeyPoints()] - pointsCounter ->" << pointsCounter;
    qDebug() << "[PencilTool::addKeyPoints()] - points.size() ->" << points.size();

    QPen linePen(Qt::red, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    lineItem = new TupPathItem;
    lineItem->setPen(linePen);
    lineItem->setPath(path);
    scene->includeObject(lineItem);
    lineAdded = true;
}
*/

/*
// Temporary code for debugging purposes
void PencilTool::addCurvePoints(TupPathItem *item)
{
    #ifdef TUP_DEBUG
        qDebug() << "[PencilTool::addCurvePoints()]";
    #endif

    foreach (TupEllipseItem *item, curveEllipsesList)
        scene->removeItem(item);
    curveEllipsesList.clear();

    if (!item) {
        #ifdef TUP_DEBUG
            qDebug() << "[PencilTool::addCurvePoints()] - Fatal Error: Path is NULL!";
        #endif

        return;
    }

    qDebug() << "[PencilTool::addCurvePoints()] - path ->" << item->pathToString();

    QList<QPointF> points = item->debuggingCurvePoints(eraserSize/2);
    if (points.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[PencilTool::addCurvePoints()] - Fatal Error: Path item has no points!";
        #endif

        return;
    }

    // QList<QColor> colors = item->nodeColors();
    // QList<QString> tips = item->nodeTips();
    int pointsCounter = 0;

    if (lineAdded)
        scene->removeItem(lineItem);

    QPainterPath path;
    path.moveTo(points.at(0));

    foreach (QPointF point, points) {
        qreal radius = penWidth;
        QPointF distance((radius + 2)/2, (radius + 2)/2);
        QPen pointPen(Qt::blue, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        TupEllipseItem *ellipse = new TupEllipseItem(QRectF(point - distance,
                                                            QSize(static_cast<int>(radius + 2), static_cast<int>(radius + 2))));
        ellipse->setPen(pointPen);
        ellipse->setBrush(pointPen.brush());
        // ellipse->setToolTip(tips.at(pointsCounter));
        scene->includeObject(ellipse);
        curveEllipsesList << ellipse;

        if (pointsCounter > 0) {
           path.lineTo(point);
        }

        pointsCounter++;
    }

    qDebug() << "[PencilTool::addCurvePoints()] - Adding axis line!";
    qDebug() << "[PencilTool::addCurvePoints()] - pointsCounter ->" << pointsCounter;
    qDebug() << "[PencilTool::addCurvePoints()] - points.size() ->" << points.size();

    QPen linePen(Qt::red, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    lineItem = new TupPathItem;
    lineItem->setPen(linePen);
    lineItem->setPath(path);
    scene->includeObject(lineItem);
    lineAdded = true;
}
*/

/*
// Temporary code for debugging purposes
void PencilTool::removeKeyPoints()
{
    if (lineAdded) {
        scene->removeItem(lineItem);
        lineAdded = false;
    }

    foreach (TupEllipseItem *ellipse, pathEllipsesList)
        scene->removeItem(ellipse);

    pathEllipsesList.clear();
}
*/

/*
// Temporary code for debugging purposes
void PencilTool::removeCurveKeyPoints()
{
    if (lineAdded) {
        scene->removeItem(lineItem);
        lineAdded = false;
    }

    foreach (TupEllipseItem *ellipse, curveEllipsesList)
        scene->removeItem(ellipse);

    curveEllipsesList.clear();
}
*/
