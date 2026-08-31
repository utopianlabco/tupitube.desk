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

#include "tupcommandexecutor.h"
#include "tupscene.h"

#include "tuppathitem.h"
#include "tuprectitem.h"
#include "tuplineitem.h"
#include "tupellipseitem.h"
#include "tupitemconverter.h"
#include "tupsvg2qt.h"

#include "tupprojectrequest.h"
#include "tuprequestbuilder.h"
#include "tupitemfactory.h"
#include "tupprojectresponse.h"
#include "tupproxyitem.h"
#include "tuptweenerstep.h"
#include "tupitemtweener.h"
#include "tupgraphicobject.h"
#include "tuplayer.h"
#include "tupbackground.h"

#include <QGraphicsItem>
#include <QDomDocument>
#include <algorithm>

namespace
{
    bool isDegeneratePrimitiveCreation(const QString &xml)
    {
        QDomDocument document;
        if (!document.setContent(xml))
            return false;

        const QDomElement root = document.documentElement();
        if (root.tagName() == QStringLiteral("rect")) {
            bool widthOk = false;
            bool heightOk = false;
            const qreal width = root.attribute(QStringLiteral("width")).toDouble(&widthOk);
            const qreal height = root.attribute(QStringLiteral("height")).toDouble(&heightOk);
            return widthOk && heightOk
                && (qFuzzyIsNull(width) || qFuzzyIsNull(height));
        }

        if (root.tagName() == QStringLiteral("ellipse")) {
            bool rxOk = false;
            bool ryOk = false;
            const qreal rx = root.attribute(QStringLiteral("rx")).toDouble(&rxOk);
            const qreal ry = root.attribute(QStringLiteral("ry")).toDouble(&ryOk);
            return rxOk && ryOk
                && (qFuzzyIsNull(rx) || qFuzzyIsNull(ry));
        }

        if (root.tagName() == QStringLiteral("path")) {
            const QString route = root.attribute(QStringLiteral("coords")).trimmed();
            if (route.isEmpty())
                return true;

            QPainterPath path;
            TupSvg2Qt::svgpath2qtpath(route, path);
            return path.elementCount() <= 1;
        }

        return false;
    }
}

bool TupCommandExecutor::createItem(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::createItem()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    TupLibraryObject::ObjectType type = response->getItemType();
    QPointF point = response->position();

    TupProject::Mode mode = response->spaceMode();
    QString xml = response->getArg().toString();

    if (response->getMode() == TupProjectResponse::Do
            && type != TupLibraryObject::Svg
            && isDegeneratePrimitiveCreation(xml)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::createItem()] - Rejecting degenerate primitive creation";
        #endif
        return false;
    }

    // Validate indices for FRAMES_MODE
    if (mode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return true; // Silent skip for invalid indices

    // For background modes, only validate scene
    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return true;

    /*
    if (xml.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupCommandExecutor::createItem()] - Fatal Error: xml content is empty!";
        #endif

        return false;
    }
    */

    TupScene *scene = project->sceneAt(sceneIndex);
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    if (type == TupLibraryObject::Svg) {
                        if (response->getMode() == TupProjectResponse::Do) {
                            TupSvgItem *svg = frame->createSvgItem(point, xml);
                            if (svg) {
                                response->setItemIndex(frame->svgItemsCount()-1);
                            } else {                            
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - Error: Svg object is invalid!";
                                #endif                            
                                return false;
                            }
                        } else if (response->getMode() == TupProjectResponse::Undo) {
                            // Undo an Add: remove the item that was added
                            frame->removeSvg(response->getItemIndex());
                        } else {
                            // Redo: restore the item
                            frame->restoreSvg();
                        }
                    } else {
                        if (response->getMode() == TupProjectResponse::Do) {
                            QGraphicsItem *item = frame->createItem(point, xml);
                            if (item) {
                                const int createdIndex = frame->graphicsCount() - 1;
                                response->setItemIndex(createdIndex);

                                TupGraphicObject *createdObject = frame->graphicAt(createdIndex);
                                if (createdObject) {
                                    const QString requestedObjectId = response->getObjectId().trimmed();
                                    if (!requestedObjectId.isEmpty())
                                        createdObject->setObjectId(requestedObjectId);

                                    response->setObjectId(createdObject->objectId());
                                }
                            } else {
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - Error: QGraphicsItem object is invalid!";
                                #endif    
                                return false;
                            }
                        } else if (response->getMode() == TupProjectResponse::Undo) {
                            // Undo an Add: remove the item that was added
                            int itemIdx = response->getItemIndex();
                            int count = frame->graphicsCount();
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupCommandExecutor::createItem()] - UNDO Add: removing item at index ->" 
                                         << itemIdx << "/ graphics count:" << count;
                            #endif
                            if (itemIdx >= 0 && itemIdx < count) {
                                bool removed = frame->removeGraphic(itemIdx);
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - UNDO Add: removeGraphic returned ->" << removed
                                             << "/ new count:" << frame->graphicsCount();
                                #endif
                            } else {
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - UNDO Add: Invalid index! itemIdx=" 
                                             << itemIdx << " count=" << count;
                                #endif
                            }
                        } else {
                            // Redo: restore the item
                            frame->restoreGraphic();
                        }
                    }

                    response->setFrameState(frame->isEmpty());
                    emit responsed(response);
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::createItem()] - Error: Frame index doesn't exist! -> "
                                 << frameIndex;
                    #endif    
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::createItem()] - Error: Layer index doesn't exist! -> "
                             << layerIndex;
                #endif
                return false;
            }
        } else { 
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::createItem()] - Error: Invalid mode!";
                    #endif    
                    return false;
                }

                if (frame) {
                    if (type == TupLibraryObject::Svg) {
                        if (response->getMode() == TupProjectResponse::Do) {
                            TupSvgItem *svg = frame->createSvgItem(point, xml);
                            if (svg) {
                                response->setItemIndex(frame->indexOf(svg));
                            } else {
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - Error: Svg object is invalid!";
                                #endif    
                                return false;
                            }
                        } else if (response->getMode() == TupProjectResponse::Undo) {
                            // Undo an Add: remove the item that was added
                            frame->removeSvg(response->getItemIndex());
                        } else {
                            // Redo: restore the item
                            frame->restoreSvg();
                        }
                    } else { 
                        if (response->getMode() == TupProjectResponse::Do) {
                            QGraphicsItem *item = frame->createItem(point, xml);
                            if (item) {
                                const int createdIndex = frame->indexOf(item);
                                response->setItemIndex(createdIndex);

                                TupGraphicObject *createdObject = frame->graphicAt(createdIndex);
                                if (createdObject) {
                                    const QString requestedObjectId = response->getObjectId().trimmed();
                                    if (!requestedObjectId.isEmpty())
                                        createdObject->setObjectId(requestedObjectId);

                                    response->setObjectId(createdObject->objectId());
                                }
                            } else {
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::createItem()] - Error: QGraphicsItem object is invalid!";
                                #endif    
                                return false;
                            }
                        } else if (response->getMode() == TupProjectResponse::Undo) {
                            // Undo an Add: remove the item that was added
                            frame->removeGraphic(response->getItemIndex());
                        } else {
                            // Redo: restore the item
                            frame->restoreGraphic();
                        }
                    }

                    emit responsed(response);
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::createItem()] - Error: Invalid background frame!";
                    #endif    
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::createItem()] - Error: Invalid background data structure!";
                #endif                
                return false;
            }
        }

    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupCommandExecutor::createItem()] - Error: Invalid scene index!";
        #endif    
        return false;
    }
    
    return true;
}

bool TupCommandExecutor::removeItem(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::removeItem()] - mode:" << response->getMode();
    #endif    

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    TupLibraryObject::ObjectType type = response->getItemType();
    TupProject::Mode spaceMode = response->spaceMode();
    TupProjectResponse::Mode actionMode = response->getMode();

    // Validate indices - if target doesn't exist, consider it already removed
    if (spaceMode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return true; // Already removed - not an error

    if (spaceMode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return true;

    TupScene *scene = project->sceneAt(sceneIndex);

    if (scene) {
        if (spaceMode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);

            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);

                if (frame) {
                    // Handle UNDO mode - restore the item instead of removing
                    if (actionMode == TupProjectResponse::Undo) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::removeItem()] - UNDO: restoring item";
                        #endif
                        if (type == TupLibraryObject::Svg) {
                            frame->restoreSvg();
                        } else {
                            frame->restoreGraphic();
                        }
                        response->setFrameState(frame->isEmpty());
                        emit responsed(response);
                        return true;
                    }

                    // Handle DO/REDO mode - remove the item
                    if (type == TupLibraryObject::Svg) {
                        frame->removeSvg(response->getItemIndex());

                        response->setFrameState(frame->isEmpty());
                        emit responsed(response);
                        return true;
                    } else {
                        const int itemIndex = resolveItemIndex(frame, response);
                        if (itemIndex < 0) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupCommandExecutor::removeItem()] - Error: Object id was not found ->"
                                         << response->getObjectId();
                            #endif
                            return false;
                        }

                        TupGraphicObject *object = frame->graphicAt(itemIndex);
                        if (object) {
                            frame->removeGraphic(itemIndex);

                            // if (object->hasTween()) 
                            //     scene->removeTweenObject(layerIndex, object);

                            response->setFrameState(frame->isEmpty());
                            emit responsed(response);
                            return true;
                        } else {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid object index (value: "
                                         << response->getItemIndex() << ")";
                            #endif                                
                            return false;
                        }
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid frame index (value: "
                                 << frameIndex << ")";
                    #endif
                    return false;
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (spaceMode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (spaceMode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (spaceMode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    // Handle UNDO mode - restore the item
                    if (actionMode == TupProjectResponse::Undo) {
                        if (type == TupLibraryObject::Svg)
                            frame->restoreSvg();
                        else
                            frame->restoreGraphic();
                    } else {
                        // Handle DO/REDO mode - remove the item
                        if (type == TupLibraryObject::Svg) {
                            frame->removeSvg(response->getItemIndex());
                        } else {
                            const int itemIndex = resolveItemIndex(frame, response);
                            if (itemIndex < 0) {
                                #ifdef TUP_DEBUG
                                    qDebug() << "[TupCommandExecutor::removeItem()] - Error: Object id was not found ->"
                                             << response->getObjectId();
                                #endif
                                return false;
                            }
                            frame->removeGraphic(itemIndex);
                        }
                    }

                    emit responsed(response);
                    return true;
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }

    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupCommandExecutor::removeItem()] - Error: Invalid scene index!";
        #endif
        return false;
    }
    
    return false;
}

bool TupCommandExecutor::moveItem(TupItemResponse *response)
{    
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::moveItem()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int objectIndex = response->getItemIndex();
    const QString actionArgument = response->getArg().toString().trimmed();
    const QString restorePrefix = QStringLiteral("restore_z:");
    const bool authoritativeZRestore = actionArgument.startsWith(restorePrefix);

    const int action = authoritativeZRestore ? 0 : actionArgument.toInt();

    bool restoreZLevelOk = false;
    const int authoritativeTargetZLevel = authoritativeZRestore
        ? actionArgument.mid(restorePrefix.size()).toInt(&restoreZLevelOk)
        : 0;
    if (authoritativeZRestore && !restoreZLevelOk)
        return false;

    TupLibraryObject::ObjectType type = response->getItemType();
    TupProject::Mode mode = response->spaceMode();

    // Validate indices
    if (mode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return true; // Silent skip for invalid indices

    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return true;
    
    TupScene *scene = project->sceneAt(sceneIndex);
    
    if (scene) {
        TupFrame *frame = nullptr;
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer)
                frame = layer->frameAt(frameIndex);
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::moveItem()] - Error: Invalid mode!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::moveItem()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }

        if (!frame) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupCommandExecutor::moveItem()] - Error: Invalid target frame!";
            #endif
            return false;
        }

        if (type != TupLibraryObject::Svg) {
            objectIndex = resolveItemIndex(frame, response);
            if (objectIndex < 0)
                return false;
        }

        if (authoritativeZRestore) {
            if (response->getMode() != TupProjectResponse::Do
                    || type == TupLibraryObject::Svg
                    || response->getObjectId().trimmed().isEmpty()) {
                return false;
            }

            #ifdef TUP_DEBUG
                qDebug() << "[TupCommandExecutor::moveItem()] - Authoritative z-level restore ->"
                         << authoritativeTargetZLevel;
            #endif

            // The action parameter is only needed by restoreItemZLevel() for
            // legacy SVG index recovery. Authoritative restore requests are
            // native-object operations and resolve the target by object_id.
            if (!frame->restoreItemZLevel(
                    type, objectIndex, TupFrame::MoveOneLevelBack,
                    authoritativeTargetZLevel)) {
                return false;
            }
        } else if (response->getMode() == TupProjectResponse::Undo) {
            bool ok = false;
            const int targetZLevel = QString::fromUtf8(response->getData()).toInt(&ok);
            if (!ok) {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupCommandExecutor::moveItem()] - Error: Missing original z-level snapshot";
                #endif
                return false;
            }

            if (!frame->restoreItemZLevel(type, objectIndex, action, targetZLevel))
                return false;
        } else {
            if (response->getMode() == TupProjectResponse::Do) {
                int originalZLevel = -1;
                if (type == TupLibraryObject::Svg) {
                    TupSvgItem *item = frame->svgAt(objectIndex);
                    if (!item)
                        return false;
                    originalZLevel = static_cast<int>(item->zValue());
                } else {
                    TupGraphicObject *object = frame->graphicAt(objectIndex);
                    if (!object)
                        return false;
                    originalZLevel = object->itemZValue();
                }
                response->setData(QByteArray::number(originalZLevel));
            }

            if (!frame->moveItem(type, objectIndex, action))
                return false;
        }

        emit responsed(response);
        return true;
    }

    return false;
}

static TupFrame *groupingFrame(TupScene *scene, TupProject::Mode mode,
                               int layerIndex, int frameIndex)
{
    if (!scene)
        return nullptr;

    if (mode == TupProject::FRAMES_MODE) {
        TupLayer *layer = scene->layerAt(layerIndex);
        return layer ? layer->frameAt(frameIndex) : nullptr;
    }

    TupBackground *bg = scene->sceneBackground();
    if (!bg)
        return nullptr;

    if (mode == TupProject::VECTOR_STATIC_BG_MODE)
        return bg->vectorStaticFrame();
    if (mode == TupProject::VECTOR_FG_MODE)
        return bg->vectorForegroundFrame();
    if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE)
        return bg->vectorDynamicFrame();

    return nullptr;
}

static QStringList groupedObjectIds(const QByteArray &data)
{
    QStringList objectIds;
    const QStringList values = QString::fromUtf8(data).split(
        QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &value : values) {
        const QString objectId = value.trimmed();
        if (!objectId.isEmpty() && !objectIds.contains(objectId))
            objectIds << objectId;
    }
    return objectIds;
}

static QList<int> groupedObjectPositions(TupFrame *frame, const QStringList &objectIds)
{
    QList<int> positions;
    if (!frame || objectIds.isEmpty())
        return positions;

    for (const QString &objectId : objectIds) {
        const int index = frame->graphicIndexById(objectId);
        if (index < 0)
            return QList<int>();
        positions << index;
    }

    std::sort(positions.begin(), positions.end());
    positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
    return positions;
}

static QString groupedPositionsArgument(TupFrame *frame,
                                        const QList<TupGraphicObject *> &objects)
{
    QStringList indexes;
    for (TupGraphicObject *object : objects) {
        if (!object)
            continue;
        const int index = frame->indexOf(object);
        if (index >= 0)
            indexes << QString::number(index);
    }
    return QStringLiteral("(") + indexes.join(QStringLiteral(" , ")) + QStringLiteral(")");
}

static QStringList groupedIds(const QList<TupGraphicObject *> &objects)
{
    QStringList objectIds;
    for (TupGraphicObject *object : objects) {
        if (!object)
            continue;
        const QString objectId = object->objectId().trimmed();
        if (!objectId.isEmpty())
            objectIds << objectId;
    }
    return objectIds;
}

bool TupCommandExecutor::groupItems(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::groupItems()]";
    #endif

    if (!response)
        return false;

    const int sceneIndex = response->getSceneIndex();
    const int layerIndex = response->getLayerIndex();
    const int frameIndex = response->getFrameIndex();
    const TupProject::Mode mode = response->spaceMode();

    if (mode == TupProject::FRAMES_MODE
            && !validateIndices(sceneIndex, layerIndex, frameIndex)) {
        return false;
    }
    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return false;

    TupScene *scene = project->sceneAt(sceneIndex);
    TupFrame *frame = groupingFrame(scene, mode, layerIndex, frameIndex);
    if (!frame)
        return false;

    if (response->getMode() == TupProjectResponse::Undo) {
        int groupIndex = response->getItemIndex();
        const QString groupObjectId = response->getObjectId().trimmed();
        if (!groupObjectId.isEmpty())
            groupIndex = frame->graphicIndexById(groupObjectId);
        if (groupIndex < 0)
            return false;

        const QList<TupGraphicObject *> objects = frame->splitGroup(groupIndex);
        if (objects.isEmpty())
            return false;

        response->setItemIndex(groupIndex);
        response->setArg(groupedPositionsArgument(frame, objects));
        response->setData(groupedIds(objects).join(QStringLiteral("\n")).toUtf8());
        emit responsed(response);
        return true;
    }

    QStringList objectIds = groupedObjectIds(response->getData());
    QList<int> positions;

    if (!objectIds.isEmpty()) {
        positions = groupedObjectPositions(frame, objectIds);
        if (positions.size() != objectIds.size())
            return false;
    } else {
        QString strList = response->getArg().toString();
        if (strList.trimmed().isEmpty())
            return false;

        QString::const_iterator itr = strList.constBegin();
        positions = TupSvg2Qt::parseIntList(++itr);
        std::sort(positions.begin(), positions.end());
        positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
        if (positions.size() < 2)
            return false;

        for (int index : positions) {
            TupGraphicObject *object = frame->graphicAt(index);
            if (!object || object->objectId().trimmed().isEmpty())
                return false;
            objectIds << object->objectId().trimmed();
        }
    }

    const int position = positions.first();
    const int resultPosition = frame->createItemGroup(
        position, positions, response->getObjectId());
    if (resultPosition < 0)
        return false;

    TupGraphicObject *groupObject = frame->graphicAt(resultPosition);
    if (!groupObject)
        return false;

    response->setItemIndex(resultPosition);
    response->setObjectId(groupObject->objectId());
    response->setData(objectIds.join(QStringLiteral("\n")).toUtf8());
    emit responsed(response);
    return true;
}

bool TupCommandExecutor::ungroupItems(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::ungroupItems()]";
    #endif

    if (!response)
        return false;

    const int sceneIndex = response->getSceneIndex();
    const int layerIndex = response->getLayerIndex();
    const int frameIndex = response->getFrameIndex();
    const TupProject::Mode mode = response->spaceMode();

    if (mode == TupProject::FRAMES_MODE
            && !validateIndices(sceneIndex, layerIndex, frameIndex)) {
        return false;
    }
    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return false;

    TupScene *scene = project->sceneAt(sceneIndex);
    TupFrame *frame = groupingFrame(scene, mode, layerIndex, frameIndex);
    if (!frame)
        return false;

    if (response->getMode() == TupProjectResponse::Undo) {
        const QStringList objectIds = groupedObjectIds(response->getData());
        const QList<int> positions = groupedObjectPositions(frame, objectIds);
        if (positions.size() < 2 || positions.size() != objectIds.size())
            return false;

        const int resultPosition = frame->createItemGroup(
            positions.first(), positions, response->getObjectId());
        if (resultPosition < 0)
            return false;

        TupGraphicObject *groupObject = frame->graphicAt(resultPosition);
        if (!groupObject)
            return false;

        response->setItemIndex(resultPosition);
        response->setObjectId(groupObject->objectId());
        emit responsed(response);
        return true;
    }

    int groupIndex = response->getItemIndex();
    QString groupObjectId = response->getObjectId().trimmed();
    if (!groupObjectId.isEmpty())
        groupIndex = frame->graphicIndexById(groupObjectId);
    if (groupIndex < 0)
        return false;

    TupGraphicObject *groupObject = frame->graphicAt(groupIndex);
    if (!groupObject)
        return false;
    if (groupObjectId.isEmpty()) {
        groupObjectId = groupObject->objectId().trimmed();
        response->setObjectId(groupObjectId);
    }

    const QList<TupGraphicObject *> objects = frame->splitGroup(groupIndex);
    if (objects.isEmpty())
        return false;

    response->setItemIndex(groupIndex);
    response->setArg(groupedPositionsArgument(frame, objects));
    response->setData(groupedIds(objects).join(QStringLiteral("\n")).toUtf8());
    emit responsed(response);
    return true;
}

static TupFrame *conversionFrame(TupScene *scene, TupProject::Mode mode,
                                 int layerIndex, int frameIndex)
{
    if (!scene)
        return nullptr;

    if (mode == TupProject::FRAMES_MODE) {
        TupLayer *layer = scene->layerAt(layerIndex);
        return layer ? layer->frameAt(frameIndex) : nullptr;
    }

    TupBackground *bg = scene->sceneBackground();
    if (!bg)
        return nullptr;

    if (mode == TupProject::VECTOR_STATIC_BG_MODE)
        return bg->vectorStaticFrame();
    if (mode == TupProject::VECTOR_FG_MODE)
        return bg->vectorForegroundFrame();
    if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE)
        return bg->vectorDynamicFrame();

    return nullptr;
}

bool TupCommandExecutor::convertItem(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::convertItem()]";
    #endif

    if (!response)
        return false;

    const QString objectId = response->getObjectId().trimmed();
    if (objectId.isEmpty()) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Error: object_id is required";
        #endif
        return false;
    }

    if (response->getItemType() == TupLibraryObject::Svg) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Error: SVG conversion is not supported";
        #endif
        return false;
    }

    const QString target = response->getArg().toString().trimmed().toLower();
    if (target != QStringLiteral("path")) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Error: Unsupported conversion target ->" << target;
        #endif
        return false;
    }

    const int sceneIndex = response->getSceneIndex();
    const int layerIndex = response->getLayerIndex();
    const int frameIndex = response->getFrameIndex();
    const TupProject::Mode mode = response->spaceMode();

    if (mode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return false;
    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return false;

    TupScene *scene = project->sceneAt(sceneIndex);
    TupFrame *frame = conversionFrame(scene, mode, layerIndex, frameIndex);
    if (!frame) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Error: Invalid target frame";
        #endif
        return false;
    }

    const int itemIndex = frame->graphicIndexById(objectId);
    if (itemIndex < 0) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Error: object_id was not found ->" << objectId;
        #endif
        return false;
    }

    response->setItemIndex(itemIndex);
    TupGraphicObject *object = frame->graphicAt(itemIndex);
    if (!object)
        return false;

    QString errorCode;
    bool success = false;

    if (response->getMode() == TupProjectResponse::Undo) {
        if (!response->hasConversionSnapshots())
            return false;
        success = TupItemConverter::applyRepresentation(
            object, response->conversionSourceSnapshot(), &errorCode);
    } else if (response->getMode() == TupProjectResponse::Redo) {
        if (!response->hasConversionSnapshots())
            return false;
        success = TupItemConverter::applyRepresentation(
            object, response->conversionTargetSnapshot(), &errorCode);
    } else {
        const QByteArray authoritativeData = response->getData();
        if (response->external() && !authoritativeData.isEmpty()) {
            const QString sourceSnapshot =
                TupItemConverter::representationSnapshot(object, &errorCode);
            const QString targetSnapshot = QString::fromUtf8(authoritativeData);

            if (!sourceSnapshot.isEmpty()) {
                success = TupItemConverter::applyRepresentation(
                    object, targetSnapshot, &errorCode);
                if (success)
                    response->setConversionSnapshots(sourceSnapshot, targetSnapshot);
            }
        } else {
            TupConversionResult result;
            success = TupItemConverter::convertToPath(object, &result, &errorCode);
            if (success)
                response->setConversionSnapshots(result.sourceRepresentation, result.targetRepresentation);
        }
    }

    if (!success) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::convertItem()] - Conversion failed ->" << errorCode;
        #endif
        return false;
    }

    emit responsed(response);
    return true;
}

bool TupCommandExecutor::transformItem(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::transformItem()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();
    TupLibraryObject::ObjectType type = response->getItemType();
    QString xml = response->getArg().toString();

    // Validate indices
    if (mode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return true; // Silent skip for invalid indices

    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return true;

    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::transformItem()] - xml -> " << xml;
    #endif

    TupScene *scene = project->sceneAt(sceneIndex);
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    if (type != TupLibraryObject::Svg) {
                        itemIndex = resolveItemIndex(frame, response);
                        if (itemIndex < 0)
                            return false;
                    }

                    QGraphicsItem *item;
                    if (type == TupLibraryObject::Svg)
                        item = frame->svgAt(itemIndex);
                    else
                        item = frame->item(itemIndex);

                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->storeItemTransformation(type, itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoTransformation(type, itemIndex);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoTransformation(type, itemIndex);

                        response->setArg(xml);
                        emit responsed(response);
                    
                        return true;
                    } 
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::transformItem()] - Error: Invalid spaceMode!";
                    #endif
                    return false;
                }

                if (frame) {
                    if (type != TupLibraryObject::Svg) {
                        itemIndex = resolveItemIndex(frame, response);
                        if (itemIndex < 0)
                            return false;
                    }

                    QGraphicsItem *item;
                    if (type == TupLibraryObject::Svg)
                        item = frame->svgAt(itemIndex);
                    else
                        item = frame->item(itemIndex);

                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->storeItemTransformation(type, itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoTransformation(type, itemIndex);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoTransformation(type, itemIndex);

                        response->setArg(xml);
                        emit responsed(response);

                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::transformItem()] - Error: Invalid item index!";
                        #endif
                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::transformItem()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::transformItem()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }
    
    return false;
}

// Nodes Edition

bool TupCommandExecutor::setPathItem(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::setPathItem()]";
        qDebug() << response->getArg().toString();
    #endif
    
    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();
    QString route = response->getArg().toString();
    TupScene *scene = project->sceneAt(sceneIndex);
    
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    TupPathItem *item = qgraphicsitem_cast<TupPathItem *>(frame->item(itemIndex));
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do) {
                            const QString currentRoute = item->pathToString();
                            QString sourceRoute = currentRoute;
                            const QString suppliedSource = QString::fromUtf8(response->getData());

                            // NodesTool edits the live path before submitting the command.
                            // If the model already equals the requested target, use the
                            // producer's pre-edit snapshot. Otherwise the current model is
                            // authoritative and is the correct source snapshot.
                            if (currentRoute == route && !suppliedSource.isEmpty())
                                sourceRoute = suppliedSource;

                            response->setState(sourceRoute);
                            item->setPathFromString(route);
                        }

                        if (response->getMode() == TupProjectResponse::Redo) {
                            QPainterPath path;
                            TupSvg2Qt::svgpath2qtpath(route, path);
                            item->setPath(path);
                        }

                        if (response->getMode() == TupProjectResponse::Undo) {
                            const QString sourceRoute = response->getState();
                            if (sourceRoute.isEmpty())
                                return false;

                            QPainterPath path;
                            TupSvg2Qt::svgpath2qtpath(sourceRoute, path);
                            item->setPath(path);
                        }

                        emit responsed(response);
                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setPathItem()] - Invalid path item at index -> "
                                     << itemIndex;
                        #endif
                        return false;
                    }
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setPathItem()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    TupPathItem *item = qgraphicsitem_cast<TupPathItem *>(frame->item(itemIndex));
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do) {
                            const QString currentRoute = item->pathToString();
                            QString sourceRoute = currentRoute;
                            const QString suppliedSource = QString::fromUtf8(response->getData());

                            // NodesTool edits the live path before submitting the command.
                            // If the model already equals the requested target, use the
                            // producer's pre-edit snapshot. Otherwise the current model is
                            // authoritative and is the correct source snapshot.
                            if (currentRoute == route && !suppliedSource.isEmpty())
                                sourceRoute = suppliedSource;

                            response->setState(sourceRoute);
                            item->setPathFromString(route);
                        }

                        if (response->getMode() == TupProjectResponse::Redo) {
                            QPainterPath path;
                            TupSvg2Qt::svgpath2qtpath(route, path);
                            item->setPath(path);
                        }

                        if (response->getMode() == TupProjectResponse::Undo) {
                            const QString sourceRoute = response->getState();
                            if (sourceRoute.isEmpty())
                                return false;

                            QPainterPath path;
                            TupSvg2Qt::svgpath2qtpath(sourceRoute, path);
                            item->setPath(path);
                        }

                        emit responsed(response);
                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setPathItem()] - Invalid path item at index -> "
                                     << itemIndex;
                        #endif
                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setPathItem()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }

            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::setPathItem()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }

    return false;
}

static int tweenIndexByNameAndType(const QList<TupItemTweener *> &tweens,
                                   const QString &name,
                                   TupItemTweener::Type type)
{
    for (int i = 0; i < tweens.count(); ++i) {
        TupItemTweener *tween = tweens.at(i);
        if (tween && tween->getTweenName() == name && tween->getType() == type)
            return i;
    }

    return -1;
}

static QString tweenSnapshot(TupItemTweener *tween)
{
    if (!tween)
        return QString();

    QDomDocument document;
    document.appendChild(tween->toXml(document));
    return document.toString(0);
}

bool TupCommandExecutor::setTween(TupItemResponse *response)
{    
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::setTween()]";
        SHOW_VAR(response)
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    TupLibraryObject::ObjectType itemType = response->getItemType();
    int itemIndex = response->getItemIndex();
    QString xml = response->getArg().toString();
    TupScene *scene = project->sceneAt(sceneIndex);
    
    if (scene) {
        TupLayer *layer = scene->layerAt(layerIndex);
        if (layer) {
            TupFrame *frame = layer->frameAt(frameIndex);
            if (frame) {
                TupItemTweener *tween = new TupItemTweener();
                tween->fromXml(xml);

                if (itemType == TupLibraryObject::Item) {
                    const QString objectId = response->getObjectId().trimmed();
                    if (!objectId.isEmpty()) {
                        itemIndex = resolveItemIndex(frame, response);
                        if (itemIndex < 0) {
                            delete tween;
                            return false;
                        }
                    } else {
                        // Legacy compatibility is required until tween-start
                        // relocation becomes one atomic object-id-preserving
                        // domain operation. The existing relocation path still
                        // emits Add -> Remove -> SetTween and cannot safely
                        // reuse an object_id across those committed revisions.
                        #ifdef TUP_DEBUG
                            qWarning() << "[TupCommandExecutor::setTween()] - "
                                          "Native tween request has no object_id; "
                                          "using legacy positional lookup ->"
                                       << itemIndex;
                        #endif
                    }

                    tween->setZLevel(itemIndex);
                    TupGraphicObject *object = frame->graphicAt(itemIndex);
                    if (object) {
                        object->addTween(tween);
                        scene->addTweenObject(layerIndex, object);
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setTween()] - Error: Invalid graphic index -> "
                                     << itemIndex;
                        #endif
                        delete tween;
                        return false;
                    }
                } else {
                    tween->setZLevel(itemIndex);
                    TupSvgItem *svg = frame->svgAt(itemIndex); 
                    if (svg) {
                        svg->addTween(tween);
                        scene->addTweenObject(layerIndex, svg);
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setTween()] - Error: Invalid svg index -> "
                                     << itemIndex;
                        #endif
                        return false;
                    }
                }

                emit responsed(response);
                return true;
            }
        }
    }
    
    return false;
}

bool TupCommandExecutor::removeTween(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::removeTween()]";
        SHOW_VAR(response)
    #endif

    if (!response)
        return false;

    const int sceneIndex = response->getSceneIndex();
    const int layerIndex = response->getLayerIndex();
    const int frameIndex = response->getFrameIndex();
    const TupLibraryObject::ObjectType itemType = response->getItemType();
    int itemIndex = response->getItemIndex();
    const QString tweenName = response->getArg().toString();

    bool typeOk = false;
    const int tweenTypeValue = QString::fromUtf8(response->getData()).toInt(&typeOk);
    if (tweenName.isEmpty() || !typeOk) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::removeTween()] - Invalid tween identity ->"
                       << tweenName << response->getData();
        #endif
        return false;
    }
    const TupItemTweener::Type tweenType = static_cast<TupItemTweener::Type>(tweenTypeValue);

    if (!validateIndices(sceneIndex, layerIndex, frameIndex))
        return false;

    TupScene *scene = project->sceneAt(sceneIndex);
    TupLayer *layer = scene ? scene->layerAt(layerIndex) : nullptr;
    TupFrame *frame = layer ? layer->frameAt(frameIndex) : nullptr;
    if (!scene || !layer || !frame)
        return false;

    if (itemType == TupLibraryObject::Item) {
        const QString objectId = response->getObjectId().trimmed();
        if (objectId.isEmpty()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupCommandExecutor::removeTween()] - Native tween removal requires object_id";
            #endif
            return false;
        }

        itemIndex = resolveItemIndex(frame, response);
        if (itemIndex < 0)
            return false;

        TupGraphicObject *object = frame->graphicAt(itemIndex);
        if (!object)
            return false;

        if (response->getMode() == TupProjectResponse::Undo) {
            const QString xml = response->getState();
            if (xml.isEmpty())
                return false;

            TupItemTweener *tween = new TupItemTweener();
            tween->fromXml(xml);
            tween->setZLevel(itemIndex);
            object->addTween(tween);
            scene->addTweenObject(layerIndex, object);
        } else {
            QList<TupItemTweener *> tweens = object->tweensList();
            const int tweenIndex = tweenIndexByNameAndType(tweens, tweenName, tweenType);
            if (tweenIndex < 0)
                return false;

            if (response->getMode() == TupProjectResponse::Do) {
                const QString xml = tweenSnapshot(tweens.at(tweenIndex));
                if (xml.isEmpty())
                    return false;
                response->setState(xml);
            }

            object->removeTween(tweenIndex);
            if (object->tweensList().isEmpty())
                scene->removeTweenObject(layerIndex, object);
        }
    } else {
        TupSvgItem *svg = frame->svgAt(itemIndex);
        if (!svg)
            return false;

        if (response->getMode() == TupProjectResponse::Undo) {
            const QString xml = response->getState();
            if (xml.isEmpty())
                return false;

            TupItemTweener *tween = new TupItemTweener();
            tween->fromXml(xml);
            tween->setZLevel(itemIndex);
            svg->addTween(tween);
            scene->addTweenObject(layerIndex, svg);
        } else {
            QList<TupItemTweener *> tweens = svg->tweensList();
            const int tweenIndex = tweenIndexByNameAndType(tweens, tweenName, tweenType);
            if (tweenIndex < 0)
                return false;

            if (response->getMode() == TupProjectResponse::Do) {
                const QString xml = tweenSnapshot(tweens.at(tweenIndex));
                if (xml.isEmpty())
                    return false;
                response->setState(xml);
            }

            svg->removeTween(tweenIndex);
            if (svg->tweensList().isEmpty())
                scene->removeTweenObject(layerIndex, svg);
        }
    }

    emit responsed(response);
    return true;
}

bool TupCommandExecutor::updateTweenPath(TupItemResponse *response)
{   
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::updateTweenTween()]";
        SHOW_VAR(response)
    #endif

    emit responsed(response);
    return true;
}

bool TupCommandExecutor::setBrush(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::setBrush()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();
    QString xml = response->getArg().toString();
    TupScene *scene = project->sceneAt(sceneIndex);

    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setBrushAtItem(itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoBrushAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoBrushAction(itemIndex);

                        emit responsed(response);
                        return true;
                    }
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setBrush()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setBrushAtItem(itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoBrushAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoBrushAction(itemIndex);

                        emit responsed(response);
                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setBrush()] - Invalid path item at index -> " << itemIndex;
                        #endif
                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setBrush()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::setBrush()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }

    return false;
}

bool TupCommandExecutor::setPen(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::setPen()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();

    QString xml = response->getArg().toString();
    TupScene *scene = project->sceneAt(sceneIndex);

    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setPenAtItem(itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoPenAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoPenAction(itemIndex);

                        emit responsed(response);
                        return true;
                    }
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setPen()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setPenAtItem(itemIndex, xml);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoPenAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoPenAction(itemIndex);

                        emit responsed(response);
                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setPen()] - Invalid path item at index -> " << itemIndex;
                        #endif
                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setPen()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::setPen()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }

    return false;
}

bool TupCommandExecutor::setTextColor(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::setTextColor()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();

    QStringList params = response->getArg().toString().split("|");
    QString textColor = params.at(0);
    TupScene *scene = project->sceneAt(sceneIndex);

    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setTextColorAtItem(itemIndex, textColor);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoTextColorAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoTextColorAction(itemIndex);

                        emit responsed(response);
                        return true;
                    }
                }
            }
        } else {
            TupBackground *bg = scene->sceneBackground();
            if (bg) {
                TupFrame *frame = nullptr;
                if (mode == TupProject::VECTOR_STATIC_BG_MODE) {
                    frame = bg->vectorStaticFrame();
                } else if (mode == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                    frame = bg->vectorDynamicFrame();
                } else if (mode == TupProject::VECTOR_FG_MODE) {
                    frame = bg->vectorForegroundFrame();
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setTextColor()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    itemIndex = resolveItemIndex(frame, response);
                    if (itemIndex < 0)
                        return false;

                    QGraphicsItem *item = frame->item(itemIndex);
                    if (item) {
                        if (response->getMode() == TupProjectResponse::Do)
                            frame->setTextColorAtItem(itemIndex, textColor);

                        if (response->getMode() == TupProjectResponse::Redo)
                            frame->redoTextColorAction(itemIndex);

                        if (response->getMode() == TupProjectResponse::Undo)
                            frame->undoTextColorAction(itemIndex);

                        emit responsed(response);
                        return true;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setTextColor()] - Invalid path item at index -> " << itemIndex;
                        #endif
                        return false;
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::setTextColor()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::setTextColor()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }

    return false;
}

bool TupCommandExecutor::createRasterPath(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::createRasterPath()]";
    #endif

    emit responsed(response);
    return true;
}

bool TupCommandExecutor::clearRasterCanvas(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::clearRasterCanvas()]";
    #endif

    emit responsed(response);
    return true;
}
