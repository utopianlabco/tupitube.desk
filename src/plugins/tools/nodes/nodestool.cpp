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

#include "nodestool.h"
#include "tupprojectrequest.h"
#include "tupprojectresponse.h"
#include "tuprequestbuilder.h"
#include "tupproxyitem.h"

#include <QMessageBox>

NodesTool::NodesTool()
{
    setupActions();
}

NodesTool::~NodesTool()
{
}

void NodesTool::init(TupGraphicsScene *gScene)
{
    activeSelection = false;
    shiftEnabled = false;
    ctrlEnabled = false;
    scene = gScene;

    clearSelection();

    nodeZValue = ((BG_LAYERS + 1) * ZLAYER_LIMIT) + (scene->currentScene()->layersCount() * ZLAYER_LIMIT);
    if (scene->getSpaceContext() == TupProject::VECTOR_FG_MODE)
        nodeZValue += ZLAYER_LIMIT;
}

QList<TAction::ActionId> NodesTool::keys() const
{
    return QList<TAction::ActionId>() << TAction::NodesEditor;
}

void NodesTool::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    Q_UNUSED(input)
    Q_UNUSED(brushManager)
    Q_UNUSED(gScene)
}

void NodesTool::move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    Q_UNUSED(input)
    Q_UNUSED(brushManager)
    Q_UNUSED(gScene)
}

void NodesTool::release(const TupInputDeviceInformation *input, TupBrushManager *brushManager,
                        TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::release()]";
    #endif

    QList<QGraphicsItem *> currentSelection = gScene->selectedItems();
    if (!currentSelection.isEmpty()) { // At least one item was selected
        if (!setPathNodes(input->pos(), currentSelection, brushManager->penWidth(), gScene)) {
            #ifdef TUP_DEBUG
                qDebug() << "[NodesTool::release()] - Warning: Nodes weren't added.";
            #endif

            return;
        }
    } else {
        if (activeSelection)
            addNode(input->pos(), brushManager->penWidth(), gScene);
    }
}

bool NodesTool::setPathNodes(QPointF coord, QList<QGraphicsItem *> currentSelection, int penWidth,
                             TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::setPathNodes()]";
    #endif

    QString pathStr = "";
    TupPathItem *pathItem = nullptr;
    TupFrame *frame = getCurrentFrame();
    int itemIndex;

    foreach(QGraphicsItem *item, currentSelection) {
        itemIndex = frame->indexOf(item);

        if (itemIndex < 0) {
            #ifdef TUP_DEBUG
                qDebug() << "[NodesTool::setPathNodes()] - Fatal Error: Invalid item index! ->" << itemIndex;
            #endif
            return false;
        }

        // SVG items are not allowed
        if (qgraphicsitem_cast<TupSvgItem *>(item)) {
            TOsd::self()->display(TOsd::Error, tr("SVG objects cannot be edited!"));

            return false;
        }

        // Raster images are not allowed
        if (TupGraphicLibraryItem *libraryItem = qgraphicsitem_cast<TupGraphicLibraryItem *>(item)) {
            if (libraryItem->getItemType() == TupLibraryObject::Image) {
                TOsd::self()->display(TOsd::Error, tr("Images have no nodes!"));

                return false;
            }
        }

        // Item is a group, so it must be split
        if (qgraphicsitem_cast<TupItemGroup *>(item)) {
            if (activeSelection)
                nodeGroup->clear();

            TupProjectRequest event = TupRequestBuilder::createItemRequest(
                gScene->currentSceneIndex(),
                currentLayer, currentFrame,
                itemIndex, coord,
                gScene->getSpaceContext(), TupLibraryObject::Item,
                TupProjectRequest::Ungroup);
            emit requested(&event);
            TOsd::self()->display(TOsd::Info, tr("Splitting item group!"));

            return false;
        }

        // Check if the selected item is either a node or a path
        if (!qgraphicsitem_cast<TControlNode*>(item)) {
            if (!qgraphicsitem_cast<TupPathItem *>(item)) {
                TOsd::self()->display(TOsd::Error, tr("Only pencil/ink lines can be edited!"));

                return false;
            }
        }

        pathItem = qgraphicsitem_cast<TupPathItem *>(item);
        if (pathItem)
            break;
    }

    if (pathItem) {
        QPointF itemPos = pathItem->boundingRect().topLeft();
        QPointF shift = itemPos - pathItem->mapToScene(itemPos);
        coord += shift;

        if ((shiftEnabled || ctrlEnabled) && activeSelection) { // Adding a new node
            if (pathItem->pointMatchesPath(coord, penWidth, PencilMode)) { // Point is part of the path
                #ifdef TUP_DEBUG
                    qDebug() << "[NodesTool::setPathNodes()] - Inserting a node in the path...";
                #endif

                NodeType node = CurveNode;
                if (ctrlEnabled)
                    node = LineNode;

                pathStr = pathItem->addInnerNode(penWidth, node);

                nodeGroup->clear();
                nodeGroup->createNodes(pathItem);
                nodeGroup->resizeNodes(realFactor);

                if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem()))
                    configPanel->setNodesTotal(pathItem->nodesCount());

                if (itemIndex >= 0) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, pathStr, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                    emit requested(&event);
                }
            } else { // Point is out the path but inside the item rect
                #ifdef TUP_DEBUG
                    qDebug() << "[NodesTool::setPathNodes()] - Extending the path...";
                #endif

                QPainterPath qPath = pathItem->path();
                if (ctrlEnabled) {
                    qPath.lineTo(coord);
                } else if (shiftEnabled) {
                    QPair<QPointF,QPointF> points = pathItem->calculateEndPathCPoints(coord);
                    qPath.cubicTo(points.first, points.second, coord);
                }

                pathItem->setPath(qPath);
                pathStr = pathItem->pathToString();

                nodeGroup->clear();
                nodeGroup->createNodes(pathItem);
                nodeGroup->resizeNodes(realFactor);

                if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem()))
                    configPanel->setNodesTotal(pathItem->nodesCount());

                if (itemIndex >= 0) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, pathStr, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                    emit requested(&event);
                }
            }
        } else { // Processing path
            if (itemIndex == -1) { // Node action
                // Node was selected
                if (qgraphicsitem_cast<TControlNode*>(pathItem)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[NodesTool::setPathNodes()] - Handling node...";
                    #endif

                    QGraphicsItem *item = nodeGroup->parentItem();
                    int nodeIndex = frame->indexOf(item);
                    if (nodeIndex >= 0) {
                        QString path = qgraphicsitem_cast<TupPathItem *>(item)->pathToString();
                        TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                                       currentLayer, currentFrame, nodeIndex,
                                                                                       QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                                       TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(item));
                        emit requested(&event);
                        nodeGroup->clearChangedNodes();
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[NodesTool::setPathNodes()] - Fatal Error: Invalid position ->" << nodeIndex;
                        #endif

                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[NodesTool::setPathNodes()] - Fatal Error: Invalid selected item index ->" << itemIndex;
                    #endif

                    return false;
                }
            }

            // Avoiding to select the same item twice
            if (activeSelection) {
                TupFrame *frame = getCurrentFrame();
                int oldIndex = frame->indexOf(nodeGroup->parentItem());
                if (oldIndex != itemIndex) { // New selection
                    #ifdef TUP_DEBUG
                        qDebug() << "[NodesTool::setPathNodes()] - A new object has been selected! (active selection was on)";
                    #endif
                    nodeGroup->clear();

                    nodeGroup = new TNodeGroup(pathItem, gScene, TNodeGroup::PathSelection, nodeZValue);
                    connect(nodeGroup, SIGNAL(nodeRemoved(int)), this, SLOT(removeNodeFromPath(int)));
                    connect(nodeGroup, SIGNAL(nodeTypeChanged(int)), this, SLOT(modifyNodeFromPath(int)));
                    nodeGroup->show();
                    nodeGroup->resizeNodes(realFactor);

                    pathItem->resetPathHistory();
                    if (pathItem->isNotEdited())
                        pathItem->saveOriginalPath();

                    configPanel->resetHistory();
                    restorablePaths.clear();
                    restorablePaths.insert(pathItem->nodesCount(), pathItem->pathToString());
                    configPanel->setNodesTotal(pathItem->nodesCount());

                    #ifdef TUP_DEBUG
                        qDebug() << "---";
                    #endif
                } else {
                    if (nodeGroup->hasChangedNodes()) { // If path was edited
                        #ifdef TUP_DEBUG
                            qDebug() << "[NodesTool::setPathNodes()] - Path was edited!";
                        #endif

                        QGraphicsItem *item = nodeGroup->parentItem();
                        int index = frame->indexOf(item);
                        if (index >= 0) {
                            QString path = pathItem->pathToString();
                            TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                                           currentLayer, currentFrame, index,
                                                                                           QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                                           TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(item));
                            emit requested(&event);
                            nodeGroup->clearChangedNodes();
                        } else {
                            #ifdef TUP_DEBUG
                                qDebug() << "[NodesTool::setPathNodes()] - Fatal Error: Invalid position ->" << index;
                            #endif

                            return false;
                        }
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[NodesTool::setPathNodes()] - Same item selected. Node group has NO changes!";
                        #endif

                        return false;
                    }
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[NodesTool::setPathNodes()] - Adding nodes to the selected path for the first time...";
                #endif

                int nodesTotal = pathItem->nodesCount();
                if (nodesTotal > 100) {
                    QPair<int, int> dimension = TAlgorithm::screenDimension();
                    int screenWidth = dimension.first;
                    int screenHeight = dimension.second;

                    QMessageBox msgBox;
                    msgBox.setWindowTitle(tr("Too many nodes!"));
                    msgBox.setIcon(QMessageBox::Information);
                    msgBox.setText(tr("The selected path contains too many nodes."));
                    msgBox.setInformativeText(tr("It will be simplified, so you can edit it."));
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.show();

                    msgBox.move(static_cast<int> ((screenWidth - msgBox.width()) / 2),
                                static_cast<int> ((screenHeight - msgBox.height()) / 2));

                    if (msgBox.exec() == QMessageBox::Ok)
                        msgBox.close();

                    QPainterPath route = pathItem->clearPath(penWidth);
                    pathItem->setPath(route);
                }

                nodeGroup = new TNodeGroup(pathItem, gScene, TNodeGroup::PathSelection, nodeZValue);
                connect(nodeGroup, SIGNAL(nodeRemoved(int)), this, SLOT(removeNodeFromPath(int)));
                connect(nodeGroup, SIGNAL(nodeTypeChanged(int)), this, SLOT(modifyNodeFromPath(int)));
                nodeGroup->show();
                activeSelection = true;

                nodeGroup->resizeNodes(realFactor);

                pathItem->resetPathHistory();
                if (pathItem->isNotEdited())
                    pathItem->saveOriginalPath();

                configPanel->resetHistory();
                restorablePaths.clear();
                restorablePaths.insert(pathItem->nodesCount(), pathItem->pathToString());
                configPanel->setNodesTotal(pathItem->nodesCount());
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[NodesTool::setPathNodes()] - Fatal Error: Path item is NULL!!!";
        #endif

        return false;
    }

    return true;
}

void NodesTool::addNode(QPointF coord, int penWidth, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::addNode()]";
    #endif

    if (shiftEnabled || ctrlEnabled) {
        QGraphicsItem *item = nodeGroup->parentItem();
        TupFrame *frame = getCurrentFrame();
        int itemIndex = frame->indexOf(item);
        TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(item);
        QPointF itemPos = pathItem->boundingRect().topLeft();
        QPointF shift = itemPos - pathItem->mapToScene(itemPos);
        coord += shift;

        if (pathItem->pointMatchesPath(coord, penWidth, PencilMode)) { // Point is part of the path
            #ifdef TUP_DEBUG
                qDebug() << "[NodesTool::addNode()] - Inserting a node in the path...";
            #endif

            NodeType node = CurveNode;
            if (ctrlEnabled)
                node = LineNode;

            QString pathStr = pathItem->addInnerNode(penWidth, node);

            nodeGroup->clear();
            nodeGroup->createNodes(pathItem);
            nodeGroup->resizeNodes(realFactor);

            configPanel->setNodesTotal(pathItem->nodesCount());

            if (itemIndex >= 0) {
                TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                               currentLayer, currentFrame, itemIndex,
                                                                               QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                               TupProjectRequest::EditNodes, pathStr, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                emit requested(&event);
            }
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[NodesTool::addNode()] - Extending path...";
            #endif
            if (pathItem) {
                QPainterPath qPath = pathItem->path();

                if (ctrlEnabled) {
                    qPath.lineTo(coord);
                } else if (shiftEnabled) {
                    QPair<QPointF,QPointF> points = pathItem->calculateEndPathCPoints(coord);
                    qPath.cubicTo(points.first, points.second, coord);
                }

                pathItem->setPath(qPath);
                QString pathStr = pathItem->pathToString();

                nodeGroup->clear();
                nodeGroup->createNodes(pathItem);
                nodeGroup->resizeNodes(realFactor);

                configPanel->setNodesTotal(pathItem->nodesCount());

                if (itemIndex >= 0) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(gScene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, itemIndex,
                                                                                   QPointF(), gScene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, pathStr, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                    emit requested(&event);
                }
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[NodesTool::addNode()] - Empty selection! Removing nodes...";
        #endif
        if (nodeGroup) {
            nodeGroup->clear();
            disconnect(nodeGroup, SIGNAL(nodeRemoved(int)), this, SLOT(removeNodeFromPath(int)));
            disconnect(nodeGroup, SIGNAL(nodeTypeChanged(int)), this, SLOT(modifyNodeFromPath(int)));
            nodeGroup = nullptr;
        }
        activeSelection = false;
        configPanel->showClearPanel(false);
    }
}

TupFrame* NodesTool::getCurrentFrame()
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

void NodesTool::layerResponse(const TupLayerResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::layerResponse()] - action ->" << response->getAction();
    #endif

    switch (response->getAction()) {
        case TupProjectRequest::Move:
        {
            init(scene);
        }
    }
}

void NodesTool::itemResponse(const TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::itemResponse()]";
    #endif

    QGraphicsItem *item = nullptr;

    if (response->getAction() != TupProjectRequest::Remove) {
        TupFrame *frame = getCurrentFrame();
        if (response->getAction() == TupProjectRequest::Ungroup) {
            QPointF point = response->position();
            item = scene->itemAt(point, QTransform());
        } else {
            item = frame->item(response->getItemIndex());
        }
    }

    switch (response->getAction()) {
        case TupProjectRequest::Convert:
        {
             #ifdef TUP_DEBUG
                 qDebug() << "[NodesTool::itemResponse()] - Convert case: invalidating node selection";
             #endif

             // Convert replaces the drawable representation while preserving the
             // logical TupGraphicObject. Any node editor state necessarily refers
             // to the old representation and must not survive the replacement.
             clearSelection();
             if (configPanel)
                 configPanel->showClearPanel(false);
        }
        break;
        case TupProjectRequest::EditNodes:
        {  
             #ifdef TUP_DEBUG
                 qDebug() << "[NodesTool::itemResponse()] - EditNodes case";
             #endif

             if (item) {
                 if (activeSelection) {
                     QGraphicsPathItem *path = qgraphicsitem_cast<QGraphicsPathItem *>(nodeGroup->parentItem());
                     if (path == item) {
                         #ifdef TUP_DEBUG
                             qDebug() << "[NodesTool::itemResponse()] - Showing nodes from selected item! (Existing nodeGroup)";
                         #endif
                         nodeGroup->show();
                         nodeGroup->syncNodesFromParent();
                         nodeGroup->saveParentProperties();

                         if (!path->isSelected())
                             path->setSelected(true);

                         if (expandNode) {
                             nodeGroup->expandNode(nodeIndex);
                             expandNode = false;
                         }
                     }
                 } else {
                     #ifdef TUP_DEBUG
                         qDebug() << "[NodesTool::itemResponse()] - Showing nodes from selected item! (Creating new nodeGroup)";
                     #endif
                     nodeGroup = new TNodeGroup(item, scene, TNodeGroup::PathSelection, nodeZValue);
                     connect(nodeGroup, SIGNAL(nodeRemoved(int)), this, SLOT(removeNodeFromPath(int)));
                     connect(nodeGroup, SIGNAL(nodeTypeChanged(int)), this, SLOT(modifyNodeFromPath(int)));
                     nodeGroup->show();
                     activeSelection = true;
                     nodeGroup->resizeNodes(realFactor);

                     if (!item->isSelected())
                         item->setSelected(true);

                     if (expandNode) {
                         nodeGroup->expandNode(nodeIndex);
                         expandNode = false;
                     }
                 }

                 if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(item))
                     configPanel->setNodesTotal(pathItem->nodesCount());
            }
        }
        break;
        case TupProjectRequest::Remove:
        {
             #ifdef TUP_DEBUG
                 qDebug() << "[NodesTool::itemResponse()] - Remove case";
             #endif
             configPanel->showClearPanel(false);

             return;
        }
        case TupProjectRequest::Ungroup:
        {
             #ifdef TUP_DEBUG
                 qDebug() << "[NodesTool::itemResponse()] - Ungroup case";
             #endif

             if (item) {
                 nodeGroup = new TNodeGroup(item, scene, TNodeGroup::PathSelection, nodeZValue);
                 connect(nodeGroup, SIGNAL(nodeRemoved(int)), this, SLOT(removeNodeFromPath(int)));
                 connect(nodeGroup, SIGNAL(nodeTypeChanged(int)), this, SLOT(modifyNodeFromPath(int)));
                 nodeGroup->show();
                 activeSelection = true;
                 nodeGroup->resizeNodes(realFactor);
             } else {
                 #ifdef TUP_DEBUG
                     qDebug() << "[NodesTool::itemResponse()] - Fatal Error: No item was found";
                 #endif
             }

             return;
        }
        default:
        {
             #ifdef TUP_DEBUG
                 qDebug() << "[NodesTool::itemResponse()] - default action";
             #endif

             if (activeSelection) {
                 nodeGroup->show();
                 if (nodeGroup->parentItem()) {
                     nodeGroup->parentItem()->setSelected(true);
                     nodeGroup->syncNodesFromParent();
                 }
             }
        }
    }
}

void NodesTool::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    if (event->modifiers() == Qt::ShiftModifier) {
        shiftEnabled = true;
        QApplication::setOverrideCursor(targetCursor);
    }

    if (event->modifiers() == Qt::ControlModifier) {
        ctrlEnabled = true;
        QApplication::setOverrideCursor(targetCursor);
    }

    if (key == Qt::Key_F11 || key == Qt::Key_Escape) {
        emit closeHugeCanvas();
    } else {
        if (!activeSelection) {
            QPair<int, int> flags = TAction::setKeyAction(key, event->modifiers());
            if (flags.first != -1 && flags.second != -1)
                emit callForPlugin(flags.first, flags.second);
        } else if (key == Qt::Key_X) {
            if (nodeGroup) {
                nodeGroup->syncNodesFromParent();
                nodeGroup->removeSelectedNode();

                if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem()))
                    configPanel->setNodesTotal(pathItem->nodesCount());
            }
        } else if (key == Qt::Key_M) {
            if (nodeGroup)
                nodeGroup->changeSelectedNode();
        } else if ((key == Qt::Key_Left) || (key == Qt::Key_Up)
                   || (key == Qt::Key_Right) || (key == Qt::Key_Down)) {
            int delta = 5;

            if (event->modifiers() == Qt::ShiftModifier)
                delta = 1;

            if (event->modifiers() == Qt::ControlModifier)
                delta = 10;

            TupFrame *frame = getCurrentFrame();
            QGraphicsItem *item = nodeGroup->parentItem();

            if (key == Qt::Key_Left)
                item->moveBy(-delta, 0);

            if (key == Qt::Key_Up)
                item->moveBy(0, -delta);

            if (key == Qt::Key_Right)
                item->moveBy(delta, 0);

            if (key == Qt::Key_Down)
                item->moveBy(0, delta);

            QTimer::singleShot(0, this, SLOT(syncNodes()));
            requestTransformation(item, frame);
        } else {
            QPair<int, int> flags = TAction::setKeyAction(key, event->modifiers());
            if (flags.first != -1 && flags.second != -1)
                emit callForPlugin(flags.first, flags.second);
        }
    }
}

void NodesTool::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event)

    shiftEnabled = false;
    ctrlEnabled = false;

    QApplication::restoreOverrideCursor();
}

void NodesTool::setupActions()
{
    configPanel = nullptr;
    nodeGroup = nullptr;
    activeSelection = false;

    TAction *nodes = new TAction(QPixmap(ICONS_DIR + "nodes.png"), tr("Nodes Selection"), this);
    nodes->setShortcut(QKeySequence(tr("N")));
    nodes->setToolTip(tr("Nodes Selection") + " - " + tr("N"));
    nodes->setActionId(TAction::NodesEditor);

    nodesActions.insert(TAction::NodesEditor, nodes);

    targetCursor = QCursor(kAppProp->themeDir() + "cursors/target.png", 4, 4);
}

QMap<TAction::ActionId, TAction *> NodesTool::actions() const
{
    return nodesActions;
}

TAction * NodesTool::getAction(TAction::ActionId toolId)
{
    return nodesActions[toolId];
}

int NodesTool::toolType() const
{
    return TupToolInterface::Selection;
}

QWidget *NodesTool::configurator()
{
    if (!configPanel) {
        configPanel = new NodeSettings;
        connect(configPanel, SIGNAL(nodesChanged(int)), this, SLOT(updateCurrentPath(int)));
        connect(configPanel, SIGNAL(policyChanged()), this, SLOT(resetPathHistory()));
    }

    return configPanel;
}

void NodesTool::aboutToChangeScene(TupGraphicsScene *scene)
{
    Q_UNUSED(scene)
}

void NodesTool::aboutToChangeTool()
{
}

void NodesTool::saveConfig()
{
}

QCursor NodesTool::toolCursor()
{
    return QCursor(Qt::ArrowCursor);
}

void NodesTool::resizeNode(qreal scaleFactor)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::resizeNodes()]";
    #endif

    realFactor = scaleFactor;
    if (activeSelection)
        nodeGroup->resizeNodes(scaleFactor);
}

void NodesTool::updateZoomFactor(qreal scaleFactor)
{
    realFactor = scaleFactor;
    // resizeNode(realFactor);
}

void NodesTool::clearSelection()
{
    if (scene && !scene->selectedItems().isEmpty())
        scene->clearSelection();

    activeSelection = false;

    if (nodeGroup) {
        nodeGroup->clear();
        nodeGroup = nullptr;
    }
}

void NodesTool::requestTransformation(QGraphicsItem *item, TupFrame *frame)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::requestTransformation(QGraphicsItem *, TupFrame *)]";
    #endif

    QDomDocument doc;
    doc.appendChild(TupSerializer::properties(item, doc));

    TupSvgItem *svg = qgraphicsitem_cast<TupSvgItem *>(item);
    int position = -1;
    TupLibraryObject::ObjectType type;

    if (svg) {
        type = TupLibraryObject::Svg;
        position = frame->indexOf(svg);
    } else {
        if (TupGraphicLibraryItem *libraryItem = qgraphicsitem_cast<TupGraphicLibraryItem *>(item)) {
            if (libraryItem->getItemType() == TupLibraryObject::Image)
                type = TupLibraryObject::Image;
            else
                type = TupLibraryObject::Item;
        } else {
            type = TupLibraryObject::Item;
        }
        position = frame->indexOf(item);
    }

    if (position >= 0) {
        TupProjectRequest event = TupRequestBuilder::createItemRequest(
                          scene->currentSceneIndex(), currentLayer, currentFrame,
                          position, QPointF(), scene->getSpaceContext(), type,
                          TupProjectRequest::Transform, doc.toString(),
                          QByteArray(), QString(), QString(),
                          svg ? QString() : scene->objectId(item));

        emit requested(&event);
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "NodesTool::requestTransformation() - Fatal Error: Invalid item position !!! [ "
                     << position << " ]";
        #endif
    }
}

void NodesTool::updateCurrentPath(int newTotal)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::updateCurrentPath()] - Update nodes total to newTotal ->" << newTotal;
    #endif

    if (activeSelection) {
        if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem())) {
            int nodesTotal = pathItem->nodesCount();
            TupFrame *frame = getCurrentFrame();
            int position = frame->indexOf(nodeGroup->parentItem());

            if (position < 0)
                return;

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            if (nodesTotal > newTotal) { // Removing nodes
                if (nodesTotal > 2) {
                    int iterations = nodesTotal - newTotal;
                    int nodesCounter = nodesTotal;
                    for (int i = 0; i < iterations; i++) {
                        // Keep the exact geometry for every node count so the slider can
                        // restore it even after authoritative EditNodes responses.
                        restorablePaths.insert(nodesCounter, pathItem->pathToString());

                        QString path = pathItem->refactoringPath(configPanel->policyParam(), nodesCounter);
                        nodesCounter--;

                        TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                       currentLayer, currentFrame, position,
                                                                                       QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                                       TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                        emit requested(&event);
                    }
                }
            } else if (newTotal > nodesTotal) { // Restoring nodes
                QString path = restorablePaths.value(newTotal);
                if (path.isEmpty())
                    path = pathItem->pathRestored(newTotal);

                if (!path.isEmpty()) {
                    TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                                   currentLayer, currentFrame, position,
                                                                                   QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                                   TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(pathItem));
                    emit requested(&event);
                }
            }

            QApplication::restoreOverrideCursor();
        }
    }
}

void NodesTool::resetPathHistory()
{
    if (activeSelection) {
        if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem())) {
            #ifdef TUP_DEBUG
                qDebug() << "[NodesTool::resetPathHistory()] - Resetting path history...";
            #endif
            pathItem->resetPathHistory();
            restorablePaths.clear();
            restorablePaths.insert(pathItem->nodesCount(), pathItem->pathToString());
        }
    }
}

void NodesTool::removeNodeFromPath(int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::removeNodeFromPath()] - index ->" << index;
    #endif

    if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem())) {
        TupFrame *frame = getCurrentFrame();
        int position = frame->indexOf(nodeGroup->parentItem());

        int nodesTotal = nodeGroup->mainNodesCount();
        #ifdef TUP_DEBUG
            qDebug() << "[NodesTool::removeNodeFromPath()] - nodesTotal ->" << nodesTotal;
        #endif
        if (nodesTotal == 2) {
            TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                      currentLayer, currentFrame, position, QPointF(), scene->getSpaceContext(),
                                      TupLibraryObject::Item, TupProjectRequest::Remove, QString(),
                                      QByteArray(), QString(), QString(),
                                      scene->objectId(nodeGroup->parentItem()));
            emit requested(&event);
        } else {
            QString path = pathItem->removeNodeFromPath(index);
            TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                           currentLayer, currentFrame, position,
                                                                           QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                           TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(nodeGroup->parentItem()));
            emit requested(&event);

            configPanel->setNodesTotal(pathItem->nodesCount());
        }
    }
}

void NodesTool::modifyNodeFromPath(int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[NodesTool::modifyNodeFromPath()] - index ->" << index;
    #endif

    if (TupPathItem *pathItem = qgraphicsitem_cast<TupPathItem *>(nodeGroup->parentItem())) {
        TupFrame *frame = getCurrentFrame();
        int position = frame->indexOf(nodeGroup->parentItem());

        QString path = pathItem->changeNodeTypeFromPath(index);
        expandNode = true;
        nodeIndex = index;

        TupProjectRequest event = TupRequestBuilder::createItemRequest(scene->currentSceneIndex(),
                                                                       currentLayer, currentFrame, position,
                                                                       QPointF(), scene->getSpaceContext(), TupLibraryObject::Item,
                                                                       TupProjectRequest::EditNodes, path, QByteArray(), QString(), QString(), scene->objectId(nodeGroup->parentItem()));
        emit requested(&event);
    }
}
