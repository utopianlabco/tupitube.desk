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
#include "tupproject.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tupframe.h"
#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tupprojectresponse.h"

TupCommandExecutor::TupCommandExecutor(TupProject *animation) : QObject(animation), project(animation)
{
}

TupCommandExecutor::~TupCommandExecutor()
{
}

bool TupCommandExecutor::validateIndices(int sceneIdx, int layerIdx, int frameIdx, int itemIdx)
{
    if (sceneIdx >= 0) {
        if (sceneIdx >= project->scenesCount()) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupCommandExecutor::validateIndices()] - Invalid scene index:" << sceneIdx;
            #endif
            return false;
        }

        TupScene *scene = project->sceneAt(sceneIdx);
        if (!scene)
            return false;

        if (layerIdx >= 0) {
            if (layerIdx >= scene->layersCount()) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupCommandExecutor::validateIndices()] - Invalid layer index:" << layerIdx;
                #endif
                return false;
            }

            TupLayer *layer = scene->layerAt(layerIdx);
            if (!layer)
                return false;

            if (frameIdx >= 0) {
                if (frameIdx >= layer->framesCount()) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupCommandExecutor::validateIndices()] - Invalid frame index:" << frameIdx;
                    #endif
                    return false;
                }

                if (itemIdx >= 0) {
                    TupFrame *frame = layer->frameAt(frameIdx);
                    if (!frame || itemIdx >= frame->graphicsCount()) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupCommandExecutor::validateIndices()] - Invalid item index:" << itemIdx;
                        #endif
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void TupCommandExecutor::getScenes(TupSceneResponse *response)
{
    response->setScenes(project->getScenes());
    emit responsed(response);
}

bool TupCommandExecutor::createScene(TupSceneResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupCommandExecutor::createScene()]";
    #endif

    int pos = response->getSceneIndex();
    QString name = response->getArg().toString();
    if (pos < 0)
        return false;

    if (response->getMode() == TupProjectResponse::Do) {
        TupScene *scene = project->createScene(name, pos);
        if (!scene) 
            return false;
    }

    if (response->getMode() == TupProjectResponse::Redo || response->getMode() == TupProjectResponse::Undo) { 
        bool success = project->restoreScene(pos);
        if (!success)
            return false;
    }

    emit responsed(response);
    return true;
}

bool TupCommandExecutor::duplicateScene(TupSceneResponse *response)
{
    int sceneIndex = response->getSceneIndex();

    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupCommandExecutor::duplicateScene()] - sceneIndex ->" << sceneIndex;
        qDebug() << "[TupCommandExecutor::duplicateScene()] - scene name ->" << response->getArg().toString();
    #endif

    if (sceneIndex < 0)
        return false;

    if (!validateIndices(sceneIndex))
        return true; // Silent skip for invalid scene

    if (response->getMode() == TupProjectResponse::Do) {
        bool result = project->duplicateScene(sceneIndex, response->getArg().toString());
        if (!result)
            return false;
        emit responsed(response);

        return true;
    }

    if (response->getMode() == TupProjectResponse::Redo) {
        bool success = project->restoreScene(sceneIndex + 1);
        if (!success)
            return false;
    }

    if (response->getMode() == TupProjectResponse::Undo) {
        bool success = project->removeScene(sceneIndex + 1);
        if (!success)
            return false;
    }

    emit responsed(response);

    return true;
}

bool TupCommandExecutor::removeScene(TupSceneResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::removeScene()]";
    #endif

    int pos = response->getSceneIndex();

    if (!validateIndices(pos))
        return true; // Already removed - not an error

    TupScene *scene = project->sceneAt(pos);
    if (scene) {
        QDomDocument document;
        document.appendChild(scene->toXml(document));
        response->setState(document.toString());
        response->setArg(scene->getSceneName());

        if (project->removeScene(pos)) {
            emit responsed(response);

            return true;
        } 
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::removeScene()] - Fatal Error: No scene at index ->" << pos;
        #endif
    }

    return false;
}

bool TupCommandExecutor::moveScene(TupSceneResponse *response)
{
    int oldPos = response->getSceneIndex();
    int newPos = response->getArg().toInt();

    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::moveScene()] - oldPos ->" << oldPos;
        qDebug() << "[TupCommandExecutor::moveScene()] - newPos ->" << newPos;
    #endif

    if (!validateIndices(oldPos))
        return true; // Silent skip for invalid scene

    if (project->moveScene(oldPos, newPos)) {
        emit responsed(response);

        return true;
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupCommandExecutor::moveScene()] - Fatal Error: Can't move scene at index ->" << oldPos;
        #endif
    }

    return false;
}

bool TupCommandExecutor::lockScene(TupSceneResponse *response)
{
    int pos = response->getSceneIndex();
    bool lock = response->getArg().toBool();

    #ifdef TUP_DEBUG
        qWarning() << "[TupCommandExecutor::lockScene()] - Scene is locked ->" << lock;
    #endif  

    TupScene *scene = project->sceneAt(pos);
    if (scene) {
        scene->setSceneLocked(lock);
        emit responsed(response);

        return true;
    }

    return false;
}

bool TupCommandExecutor::renameScene(TupSceneResponse *response)
{
    int pos = response->getSceneIndex();
    QString newName = response->getArg().toString();

    TupScene *scene = project->sceneAt(pos);
    if (scene) {
        scene->setSceneName(newName);
        emit responsed(response);

        return true;
    }

    return false;
}

void TupCommandExecutor::selectScene(TupSceneResponse *response)
{
    emit responsed(response);
}

bool TupCommandExecutor::setSceneVisibility(TupSceneResponse *response)
{
    int pos = response->getSceneIndex();
    bool view = response->getArg().toBool();
    
    TupScene *scene = project->sceneAt(pos);
    if (scene) {
        scene->setVisibility(view);
        emit responsed(response);

        return true;
    }

    return false;
}

bool TupCommandExecutor::resetScene(TupSceneResponse *response)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupCommandExecutor::resetScene()]";
    #endif

    int index = response->getSceneIndex();
    QString newName = response->getArg().toString();

    TupScene *scene = project->sceneAt(index);
    if (scene) {
        if (response->getMode() == TupProjectResponse::Do || response->getMode() == TupProjectResponse::Redo) {
            if (project->resetScene(index, newName)) {
                emit responsed(response);

                return true;
            }
        }

        if (response->getMode() == TupProjectResponse::Undo) {
            QString oldName = project->recoverScene(index);
            response->setArg(oldName);
            emit responsed(response);

            return true;
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupCommandExecutor::resetScene()] - Fatal Error: No scene at index ->" << index;
        #endif
    }

    return false;
}

void TupCommandExecutor::setBgColor(TupSceneResponse *response)
{
    int index = response->getSceneIndex();
    QString colorName = response->getArg().toString();
    project->setSceneBgColor(index, QColor(colorName));

    emit responsed(response);
}

void TupCommandExecutor::setFps(TupSceneResponse *response)
{
    int index = response->getSceneIndex();

    if (response->getMode() == TupProjectResponse::Undo) {
            // Restore the old FPS stored in the state
            int oldFps = response->getState().toInt();
            project->setFPS(oldFps, index);

            // Swap arg and state so the next Redo knows what the "new" FPS was
            response->setState(response->getArg().toString());
            response->setArg(QString::number(oldFps));
    } else if (response->getMode() == TupProjectResponse::Redo) {
            // Get the "new" FPS that we saved in state during the Undo step
            int newFps = response->getState().toInt();
            project->setFPS(newFps, index);

            // Swap them back for the next Undo
            response->setState(response->getArg().toString());
            response->setArg(QString::number(newFps));
    } else { // Do Mode (Initial execution or receiving from network)
            int newFps = response->getArg().toInt();
            int oldFps = project->getFPS(index);

            // Save the current FPS into state before overwriting it
            response->setState(QString::number(oldFps));
            project->setFPS(newFps, index);
    }

    emit responsed(response);
}
