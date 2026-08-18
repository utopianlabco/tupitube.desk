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

#include "tupprojectmanager.h"
#include "tupproject.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tupframe.h"

#include "tupprojectrequest.h"
#include "tupprojectcommand.h"
#include "tupcommandexecutor.h"
#include "tupprojectmanagerparams.h"
#include "tupabstractprojectmanagerhandler.h"
#include "tupprojectresponse.h"
#include "tuprequestbuilder.h"
#include "tuprequestparser.h"
#include "talgorithm.h"
#include "tosd.h"

// This class handles the current animation project 

TupProjectManager::TupProjectManager(QObject *parent) : QObject(parent)
{	
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager()]";
    #endif

    modified = false;
    handler = nullptr;
    macroInProgress = false;
    pendingConvertRestoreCommandId.clear();

    project = new TupProject(this);
    undoStack = new QUndoStack(this);
    commandExecutor = new TupCommandExecutor(project);

    connect(commandExecutor, SIGNAL(responsed(TupProjectResponse*)), this, SLOT(emitResponse(TupProjectResponse *)));
    connect(project, SIGNAL(responsed(TupProjectResponse*)), this, SIGNAL(responsed(TupProjectResponse *)));
}

TupProjectManager::~TupProjectManager()
{
    #ifdef TUP_DEBUG
        qDebug() << "[~TupProjectManager()]";
    #endif

    delete handler;
    delete undoStack;
    delete params;
    delete commandExecutor;
}

void TupProjectManager::setParams(TupProjectManagerParams *parameters)
{
    params = parameters;
    handler->initialize(params);
}

TupProjectManagerParams *TupProjectManager::getParams() const
{
    return params;
}

void TupProjectManager::setHandler(TupAbstractProjectHandler *pHandler, bool networked)
{
    if (handler) {
        try {
            disconnect(handler, SIGNAL(sendCommand(const TupProjectRequest *, bool)),
                    this, SLOT(createCommand(const TupProjectRequest *, bool)));
            disconnect(handler, SIGNAL(sendLocalCommand(const TupProjectRequest *)),
                    this, SLOT(handleLocalRequest(const TupProjectRequest *)));
            disconnect(handler, SIGNAL(projectPathChanged()), this, SIGNAL(projectPathChanged()));
            disconnect(handler, SIGNAL(soundPathsChanged()), this, SIGNAL(soundPathsChanged()));
        } catch(...) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupProjectManager::setHandler] - Error: Exception during handler disconnect/delete";
            #endif            
        }

        delete handler;
        handler = nullptr;
    }

    handler = pHandler;
    handler->setParent(this);
    handler->setProject(project);

    connect(handler, SIGNAL(sendCommand(const TupProjectRequest *, bool)),
            this, SLOT(createCommand(const TupProjectRequest *, bool)));
    connect(handler, SIGNAL(sendLocalCommand(const TupProjectRequest *)),
            this, SLOT(handleLocalRequest(const TupProjectRequest *)));
    connect(handler, SIGNAL(projectPathChanged()), this, SIGNAL(projectPathChanged()));
    connect(handler, SIGNAL(soundPathsChanged()), this, SIGNAL(soundPathsChanged()));

    if (networked) {
        // Keep TupProjectManager independent of the concrete network handler.
        // The old-style signal/slot connection is resolved by Qt's meta-object
        // system at runtime, so this layer does not need the network header.
        connect(handler, SIGNAL(authoritativeModifiedStateChanged(bool)),
                this, SLOT(setModificationStatus(bool)));
        connect(handler, SIGNAL(convertRestoreStackAdvanceRequested(const QString &, bool)),
                this, SLOT(advanceAuthoritativeConvertRestore(const QString &, bool)));
        connect(handler, SIGNAL(convertRestoreRequestFinished(const QString &)),
                this, SLOT(finishAuthoritativeConvertRestore(const QString &)));
        connect(handler, SIGNAL(editNodesRestoreStackAdvanceRequested(const QString &, bool)),
                this, SLOT(advanceAuthoritativeEditNodesRestore(const QString &, bool)));
        connect(handler, SIGNAL(editNodesRestoreRequestFinished(const QString &)),
                this, SLOT(finishAuthoritativeEditNodesRestore(const QString &)));
        connect(handler, SIGNAL(transformRestoreStackAdvanceRequested(const QString &, bool)),
                this, SLOT(advanceAuthoritativeTransformRestore(const QString &, bool)));
        connect(handler, SIGNAL(transformRestoreRequestFinished(const QString &)),
                this, SLOT(finishAuthoritativeTransformRestore(const QString &)));
        connect(handler, SIGNAL(authoritativeRestoreConflict(const QString &, bool)),
                this, SLOT(markAuthoritativeRestoreConflict(const QString &, bool)));
        connect(handler, SIGNAL(authoritativeCreatedObjectIdAssigned(const QString &, const QString &)),
                this, SLOT(reconcileAuthoritativeCreatedObjectId(const QString &, const QString &)));
    }

    isNetworked = networked;
}

TupAbstractProjectHandler *TupProjectManager::getHandler() const
{
    return handler;
}

void TupProjectManager::setupNewProject()
{	
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::setupNewProject()]";
    #endif

    if (!handler || !params) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::setupNewProject()] - Error: No handler available or no params!";
        #endif
        return;
    }

    closeProject();

    project->setProjectName(params->getProjectManager());
    project->setAuthor(params->getAuthor());
    project->setDescription(params->getDescription());
    project->setCurrentBgColor(params->getBgColor());
    project->setDimension(params->getDimension());
    project->setFPS(params->getFPS());

    if (!handler->setupNewProject(params)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::setupNewProject()] - Error: Project params misconfiguration";
        #endif
        return;
    }

    if (!isNetworked) {
        QString projectPath = CACHE_DIR + params->getProjectManager();
        QDir projectDir(projectPath); 
        if (projectDir.exists()) {
            if (!projectDir.removeRecursively()) {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupProjectManager::setupNewProject()] - Error: Can't remove CACHE path ->" << projectPath;
                #endif
            }
        }
        project->setDataDir(projectPath);

        TupProjectRequest request = TupRequestBuilder::createSceneRequest(0, TupProjectRequest::Add, tr("Scene %1").arg(1));
        handleProjectRequest(&request);

        request = TupRequestBuilder::createLayerRequest(0, 0, TupProjectRequest::Add, tr("Layer %1").arg(1));
        handleProjectRequest(&request);

        request = TupRequestBuilder::createFrameRequest(0, 0, 0, TupProjectRequest::Add, tr("Frame"));
        handleProjectRequest(&request);
    }
}

void TupProjectManager::closeProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::closeProject()]";
    #endif

    if (!handler) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::closeProject()] - Fatal Error: Project handler is NULL!";
        #endif
        return;
    }

    if (project->isProjectOpen()) {
        if (!handler->closeProject()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupProjectManager::closeProject()] - Fatal Error: Handler can't close project!";
            #endif
            return;
        }
        project->clear();
    }

    project->setOpen(false);
    modified = false;
    undoStack->clear();
}

bool TupProjectManager::saveProject(const QString &fileName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::saveProject()]";
    #endif

    bool result = handler->saveProject(fileName, project);
    modified = !result;

    return result;
}

bool TupProjectManager::loadProject(const QString &fileName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::loadProject()] - fileName ->" << fileName;
    #endif

    if (!handler) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::loadProject()] - Fatal Error: No project handler available!";
        #endif
        return false;
    }

    bool ok = handler->loadProject(fileName, project);

    if (ok) {
        project->setOpen(true);
        modified = false;
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::loadProject()] - Fatal Error: Can't load project ->" << fileName;
        #endif
    }

    return ok;
}

// Returns true if project is open

bool TupProjectManager::isOpen() const
{
    return project->isProjectOpen();
}

bool TupProjectManager::projectWasModified() const
{
    return modified;
}

void TupProjectManager::setModificationStatus(bool changed)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::setModificationStatus()] - changed ->" << changed;
    #endif

    modified = changed;
}

bool TupProjectManager::isValid() const
{
    if (!handler) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::isValid()] - Error: No project handler available!";
        #endif
        return false;
    }

    return handler->isValid();
}

/**
 *  This function is called when some event is triggered by the project
 *  It must be re-implemented if you want to deal with the event in another way, i.ex: send it through the net.
 *  By default, it sends the event through the signal commandExecuted
 *  @param event 
 **/

void TupProjectManager::handleProjectRequest(const TupProjectRequest *request)
{	
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::handleProjectRequest()]";
        // qDebug() << "[TupProjectManager::handleProjectRequest()] - Package:";
        // qDebug() << request->getXml();
    #endif

    // SQA: the handler must advise when to build the command    
    if (handler) {
        handler->handleProjectRequest(request);
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::handleProjectRequest()] - Error: Project handler is NULL! Cannot process request.";
        #endif
        TOsd::self()->display(TOsd::Error, tr("Project must be closed"));
        closeProject();
    }
}

void TupProjectManager::handleLocalRequest(const TupProjectRequest *request)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::handleLocalRequest()]";
    #endif	

    TupRequestParser parser;

    if (parser.parse(request->getXml())) {
        if (TupFrameResponse *response = static_cast<TupFrameResponse *>(parser.getResponse())) {
            sceneIndex = response->getSceneIndex();
            layerIndex = response->getLayerIndex();
            frameIndex = response->getFrameIndex();
        }

        parser.getResponse()->setExternal(request->isRequestExternal());
        emit responsed(parser.getResponse());
    }
}

/*
 *  This function creates a command to execute an action, i.e. add a frame. 
 *  The command has the information necessary to undo its effect.
 *  Usually this command must be added into the commands stack.
 *  The command created is not deleted by this class, this task depends on the user.
 *  @param event 
 *  @return 
 */

void TupProjectManager::createCommand(const TupProjectRequest *request, bool addToStack)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::createCommand()]";
        // qDebug() << request->getXml();
    #endif

    if (request->isValid()) {
        TupProjectCommand *command = new TupProjectCommand(commandExecutor, request);
        if (command) {
            if (addToStack) {
                undoStack->push(command);
                /*
                #ifdef TUP_DEBUG
                    QString msg = "TupProjectManager::createCommand() * command counter: " + QString::number(undoStack->count());
                    qWarning() << msg;
                #endif
                */
            } else {
                command->redo();
            }
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[TupProjectManager::createCommand()] - Invalid command";
            #endif
        }
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::createCommand()] - Invalid request";
        #endif
    }
}

void TupProjectManager::createCommand(TupProjectCommand *command)
{
    undoStack->push(command);
    /*
    #ifdef TUP_DEBUG
        qWarning() << "TupProjectManager::createCommand() - command counter ->" << QString::number(undoStack->count());
    #endif
    */
}

TupProject *TupProjectManager::getProject()
{
    return project;
}

void TupProjectManager::beginUndoMacro(const QString &text)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::beginUndoMacro()] - text ->" << text;
    #endif
    if (macroInProgress) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::beginUndoMacro()] - Macro already in progress, ignoring!";
        #endif
        return;
    }
    macroInProgress = true;
    undoStack->beginMacro(text);
}

void TupProjectManager::endUndoMacro()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::endUndoMacro()]";
    #endif

    if (!macroInProgress) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::endUndoMacro()] - No macro in progress, ignoring!";
        #endif
        return;
    }
    macroInProgress = false;
    undoStack->endMacro();
}

void TupProjectManager::undo()
{
    if (!undoStack || undoStack->count() == 0)
        return;

    if (isNetworked && (!pendingConvertRestoreCommandId.isEmpty()
            || !pendingEditNodesRestoreCommandId.isEmpty()
            || !pendingTransformRestoreCommandId.isEmpty())) {
        return;
    }

    while (undoStack->canUndo()) {
        const int commandIndex = undoStack->index() - 1;
        const TupProjectCommand *constCommand = commandIndex >= 0
            ? dynamic_cast<const TupProjectCommand *>(undoStack->command(commandIndex))
            : nullptr;

        if (isNetworked && constCommand && constCommand->isUndoBlocked()) {
            TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
            command->skipNextStackExecution();
            undoStack->undo();
            continue;
        }

        if (isNetworked && constCommand && constCommand->isItemConvert()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingConvertRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(
                        handler,
                        "requestAuthoritativeConvertRestore",
                        Qt::DirectConnection,
                        Q_ARG(QString, commandId),
                        Q_ARG(bool, true))) {
                    return;
                }
                pendingConvertRestoreCommandId.clear();
            }
        } else if (isNetworked && constCommand && constCommand->isItemEditNodes()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingEditNodesRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(
                        handler,
                        "requestAuthoritativeEditNodesRestore",
                        Qt::DirectConnection,
                        Q_ARG(QString, commandId),
                        Q_ARG(bool, true))) {
                    return;
                }
                pendingEditNodesRestoreCommandId.clear();
            }
        } else if (isNetworked && constCommand && constCommand->isItemTransform()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingTransformRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(handler, "requestAuthoritativeTransformRestore",
                        Qt::DirectConnection, Q_ARG(QString, commandId), Q_ARG(bool, true)))
                    return;
                pendingTransformRestoreCommandId.clear();
            }
        }

        undoStack->undo();
        return;
    }

#ifdef TUP_DEBUG
    qWarning() << "[TupProjectManager::undo()] - No undo actions available!";
#endif
}

void TupProjectManager::redo()
{
    if (!undoStack || undoStack->count() == 0)
        return;

    if (isNetworked && (!pendingConvertRestoreCommandId.isEmpty()
            || !pendingEditNodesRestoreCommandId.isEmpty()
            || !pendingTransformRestoreCommandId.isEmpty())) {
        return;
    }

    while (undoStack->canRedo()) {
        const int commandIndex = undoStack->index();
        const TupProjectCommand *constCommand = commandIndex < undoStack->count()
            ? dynamic_cast<const TupProjectCommand *>(undoStack->command(commandIndex))
            : nullptr;

        if (isNetworked && constCommand && constCommand->isRedoBlocked()) {
            TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
            command->skipNextStackExecution();
            undoStack->redo();
            continue;
        }

        if (isNetworked && constCommand && constCommand->isItemConvert()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingConvertRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(
                        handler,
                        "requestAuthoritativeConvertRestore",
                        Qt::DirectConnection,
                        Q_ARG(QString, commandId),
                        Q_ARG(bool, false))) {
                    return;
                }
                pendingConvertRestoreCommandId.clear();
            }
        } else if (isNetworked && constCommand && constCommand->isItemEditNodes()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingEditNodesRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(
                        handler,
                        "requestAuthoritativeEditNodesRestore",
                        Qt::DirectConnection,
                        Q_ARG(QString, commandId),
                        Q_ARG(bool, false))) {
                    return;
                }
                pendingEditNodesRestoreCommandId.clear();
            }
        } else if (isNetworked && constCommand && constCommand->isItemTransform()) {
            const QString commandId = constCommand->commandId().trimmed();
            if (!commandId.isEmpty()) {
                pendingTransformRestoreCommandId = commandId;
                if (QMetaObject::invokeMethod(handler, "requestAuthoritativeTransformRestore",
                        Qt::DirectConnection, Q_ARG(QString, commandId), Q_ARG(bool, false)))
                    return;
                pendingTransformRestoreCommandId.clear();
            }
        }

        undoStack->redo();
        return;
    }

#ifdef TUP_DEBUG
    qWarning() << "[TupProjectManager::redo()] - No redo actions available!";
#endif
}

void TupProjectManager::advanceAuthoritativeConvertRestore(
    const QString &commandId, bool undoRestore)
{
    if (!undoStack || pendingConvertRestoreCommandId != commandId.trimmed())
        return;

    const int commandIndex = undoRestore ? undoStack->index() - 1 : undoStack->index();
    const TupProjectCommand *constCommand = commandIndex >= 0 && commandIndex < undoStack->count()
        ? dynamic_cast<const TupProjectCommand *>(undoStack->command(commandIndex))
        : nullptr;
    if (!constCommand || constCommand->commandId() != pendingConvertRestoreCommandId)
        return;

    TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
    if (undoRestore)
        command->setRedoBlocked(false);
    else
        command->setUndoBlocked(false);
    command->skipNextStackExecution();
    if (undoRestore)
        undoStack->undo();
    else
        undoStack->redo();
}

void TupProjectManager::finishAuthoritativeConvertRestore(const QString &commandId)
{
    if (pendingConvertRestoreCommandId == commandId.trimmed())
        pendingConvertRestoreCommandId.clear();
}

void TupProjectManager::advanceAuthoritativeEditNodesRestore(
    const QString &commandId, bool undoRestore)
{
    if (!undoStack || pendingEditNodesRestoreCommandId != commandId.trimmed())
        return;

    const int commandIndex = undoRestore ? undoStack->index() - 1 : undoStack->index();
    const TupProjectCommand *constCommand = commandIndex >= 0 && commandIndex < undoStack->count()
        ? dynamic_cast<const TupProjectCommand *>(undoStack->command(commandIndex))
        : nullptr;
    if (!constCommand || constCommand->commandId() != pendingEditNodesRestoreCommandId)
        return;

    TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
    if (undoRestore)
        command->setRedoBlocked(false);
    else
        command->setUndoBlocked(false);
    command->skipNextStackExecution();
    if (undoRestore)
        undoStack->undo();
    else
        undoStack->redo();
}

void TupProjectManager::finishAuthoritativeEditNodesRestore(const QString &commandId)
{
    if (pendingEditNodesRestoreCommandId == commandId.trimmed())
        pendingEditNodesRestoreCommandId.clear();
}

void TupProjectManager::advanceAuthoritativeTransformRestore(const QString &commandId, bool undoRestore)
{
    if (!undoStack || pendingTransformRestoreCommandId != commandId.trimmed())
        return;
    const int commandIndex = undoRestore ? undoStack->index() - 1 : undoStack->index();
    const TupProjectCommand *constCommand = commandIndex >= 0 && commandIndex < undoStack->count()
        ? dynamic_cast<const TupProjectCommand *>(undoStack->command(commandIndex)) : nullptr;
    if (!constCommand || constCommand->commandId() != pendingTransformRestoreCommandId)
        return;
    TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
    if (undoRestore) command->setRedoBlocked(false); else command->setUndoBlocked(false);
    command->skipNextStackExecution();
    if (undoRestore) undoStack->undo(); else undoStack->redo();
}

void TupProjectManager::finishAuthoritativeTransformRestore(const QString &commandId)
{
    if (pendingTransformRestoreCommandId == commandId.trimmed())
        pendingTransformRestoreCommandId.clear();
}

void TupProjectManager::markAuthoritativeRestoreConflict(
    const QString &commandId, bool undoRestore)
{
    if (!undoStack)
        return;

    const QString normalizedCommandId = commandId.trimmed();
    if (normalizedCommandId.isEmpty())
        return;

    for (int index = 0; index < undoStack->count(); ++index) {
        const TupProjectCommand *constCommand =
            dynamic_cast<const TupProjectCommand *>(undoStack->command(index));
        if (!constCommand || constCommand->commandId() != normalizedCommandId)
            continue;

        TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
        if (undoRestore)
            command->setUndoBlocked(true);
        else
            command->setRedoBlocked(true);
        return;
    }
}

void TupProjectManager::reconcileAuthoritativeCreatedObjectId(
    const QString &commandId, const QString &objectId)
{
    if (!undoStack)
        return;

    const QString normalizedCommandId = commandId.trimmed();
    const QString normalizedObjectId = objectId.trimmed();
    if (normalizedCommandId.isEmpty() || normalizedObjectId.isEmpty())
        return;

    for (int index = 0; index < undoStack->count(); ++index) {
        const TupProjectCommand *constCommand =
            dynamic_cast<const TupProjectCommand *>(undoStack->command(index));
        if (!constCommand || constCommand->commandId() != normalizedCommandId)
            continue;

        TupProjectCommand *command = const_cast<TupProjectCommand *>(constCommand);
        command->reconcileCreatedObjectId(normalizedObjectId);
        return;
    }
}

void TupProjectManager::clearUndoStack()
{
    pendingConvertRestoreCommandId.clear();
    pendingEditNodesRestoreCommandId.clear();
    pendingTransformRestoreCommandId.clear();
    undoStack->clear();
}

void TupProjectManager::emitResponse(TupProjectResponse *response)
{	
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectManager::emitResponse()] - response->action() ->" << response->getAction();
    #endif

    if (response->getAction() != TupProjectRequest::Select && !response->external())
        modified = true;

   if (handler) {
        if (isNetworked) {
            if (handler->commandExecuted(response))
                emit responsed(response);
        } else { // Local request
            emit responsed(response);
        }
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupProjectManager::emitResponse()] - Error: Project handler is NULL! Cannot process response.";
        #endif
        TOsd::self()->display(TOsd::Error, tr("Project must be closed"));
        closeProject();
    }
}

void TupProjectManager::setOpen(bool isOpen)
{
    project->setOpen(isOpen);
}

void TupProjectManager::updateProjectDimension(const QSize size)
{
    project->setDimension(size);
}

int TupProjectManager::framesCount(int sceneIndex)
{
    int total = 0;
    TupScene *scene = project->sceneAt(sceneIndex);
    if (scene)
        total = scene->framesCount();

    return total;
}

void TupProjectManager::setSceneBgColor(int sceneIndex, const QColor &bgColor)
{
    project->sceneAt(sceneIndex)->setBgColor(bgColor);
}

QColor TupProjectManager::getSceneBgColor(int sceneIndex)
{
    return project->getSceneBgColor(sceneIndex);
}
