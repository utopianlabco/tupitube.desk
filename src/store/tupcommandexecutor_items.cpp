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
    int action = response->getArg().toInt();
    TupLibraryObject::ObjectType type = response->getItemType();
    TupProject::Mode mode = response->spaceMode();

    // Validate indices
    if (mode == TupProject::FRAMES_MODE && !validateIndices(sceneIndex, layerIndex, frameIndex))
        return true; // Silent skip for invalid indices

    if (mode != TupProject::FRAMES_MODE && !validateIndices(sceneIndex))
        return true;

    if (response->getMode() == TupProjectResponse::Undo) {
        // SQA: Recalculate the variable values based on the action code 
        // objectIndex = ???;
        // action = ???;
    }
    
    TupScene *scene = project->sceneAt(sceneIndex);
    
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    if (type != TupLibraryObject::Svg) {
                        objectIndex = resolveItemIndex(frame, response);
                        if (objectIndex < 0)
                            return false;
                    }

                    if (frame->moveItem(type, objectIndex, action)) {
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
                        qDebug() << "[TupCommandExecutor::moveItem()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    if (type != TupLibraryObject::Svg) {
                        objectIndex = resolveItemIndex(frame, response);
                        if (objectIndex < 0)
                            return false;
                    }

                    if (frame->moveItem(type, objectIndex, action)) {
                        emit responsed(response);
                        return true;
                    }
                } else {                    
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::moveItem()] - Error: Invalid background frame!";
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
    }

    return false;
}

bool TupCommandExecutor::groupItems(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::groupItems()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();
    QString strList = response->getArg().toString();

    TupScene *scene = project->sceneAt(sceneIndex);
    
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    QString::const_iterator itr = strList.constBegin();
                    QList<int> positions = TupSvg2Qt::parseIntList(++itr);

                    // qSort(positions.begin(), positions.end());
                    std::sort(positions.begin(), positions.end()); 

                    int position = frame->createItemGroup(itemIndex, positions);
                    response->setItemIndex(position);
                
                    emit responsed(response);
                    return true;
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
                        qDebug() << "[TupCommandExecutor::groupItems()] - Error: Invalid mode!";
                    #endif                    
                    return false;
                }

                if (frame) {
                    QString::const_iterator itr = strList.constBegin();
                    QList<int> positions = TupSvg2Qt::parseIntList(++itr);

                    // qSort(positions.begin(), positions.end());
                    std::sort(positions.begin(), positions.end());

                    int position = frame->createItemGroup(itemIndex, positions);
                    response->setItemIndex(position);

                    emit responsed(response);
                    return true;
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::groupItems()] - Error: Invalid background frame!";
                    #endif 
                    return false;
                }

            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::groupItems()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }
    
    return false;
}

bool TupCommandExecutor::ungroupItems(TupItemResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::ungroupItems()]";
    #endif

    int sceneIndex = response->getSceneIndex();
    int layerIndex = response->getLayerIndex();
    int frameIndex = response->getFrameIndex();
    int itemIndex = response->getItemIndex();
    TupProject::Mode mode = response->spaceMode();
    
    TupScene *scene = project->sceneAt(sceneIndex);
    if (scene) {
        if (mode == TupProject::FRAMES_MODE) {
            TupLayer *layer = scene->layerAt(layerIndex);
            if (layer) {
                TupFrame *frame = layer->frameAt(frameIndex);
                if (frame) {
                    QString strItems = "";
                    QList<QGraphicsItem *> items = frame->splitGroup(itemIndex);
                    foreach (QGraphicsItem *item, items) {
                         if (frame->indexOf(item) != -1) {
                             if (strItems.isEmpty())
                                 strItems += "("+ QString::number(frame->indexOf(item));
                             else
                                 strItems += " , "+ QString::number(frame->indexOf(item));
                         } else {
                             #ifdef TUP_DEBUG
                                 qDebug() << "[TupCommandExecutor::ungroupItems()] - Error: Item wasn't found at frame!";
                             #endif
                         }
                    }
                    strItems+= ")";
                    response->setArg(strItems);
                    emit responsed(response);

                    return true;
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
                        qDebug() << "[TupCommandExecutor::ungroupItems()] - Error: Invalid mode!";
                    #endif
                    return false;
                }

                if (frame) {
                    QString strItems = "";
                    QList<QGraphicsItem *> items = frame->splitGroup(itemIndex);
                    foreach (QGraphicsItem *item, items) {
                         if (frame->indexOf(item) != -1) {
                             if (strItems.isEmpty())
                                 strItems += "("+ QString::number(frame->indexOf(item));
                             else
                                 strItems += " , "+ QString::number(frame->indexOf(item));
                         } else {
                             #ifdef TUP_DEBUG
                                 qDebug() << "[TupCommandExecutor::ungroupItems()] - Error: Item wasn't found at static/dynamic frame!";
                             #endif
                         }
                    }
                    strItems+= ")";
                    response->setArg(strItems);
                    emit responsed(response);
                    return true;
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::ungroupItems()] - Error: Invalid background frame!";
                    #endif
                    return false;
                }

            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::ungroupItems()] - Error: Invalid background data structure!";
                #endif
                return false;
            }
        }
    }

    return false;
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
                tween->setZLevel(itemIndex);
                if (itemType == TupLibraryObject::Item) {
                    TupGraphicObject *object = frame->graphicAt(itemIndex);
                    if (object) {
                        object->addTween(tween);
                        scene->addTweenObject(layerIndex, object);
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::setTween()] - Error: Invalid graphic index -> "
                                     << itemIndex;
                        #endif
                        return false;
                    }
                } else {
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
