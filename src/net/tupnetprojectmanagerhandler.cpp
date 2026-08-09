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

#include "tupnetprojectmanagerhandler.h"

namespace {
    const int COMMAND_RETRY_SCAN_INTERVAL_MS = 1000;
    const qint64 COMMAND_ACK_TIMEOUT_MS = 5000;
    const int COMMAND_MAX_RETRIES = 3;
    const int RECONNECT_INTERVAL_MS = 2000;
    const int MAX_RECONNECT_ATTEMPTS = 5;
}

TupNetProjectManagerHandler::TupNetProjectManagerHandler(QObject *parent) : TupAbstractProjectHandler(parent)
{    
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler()]";
    #endif

    socket = new TupNetSocket(this);
    commandTracker = new TupCommandTracker(this);
    commandRetryTimer = new QTimer(this);
    commandRetryTimer->setInterval(COMMAND_RETRY_SCAN_INTERVAL_MS);
    connect(commandRetryTimer, SIGNAL(timeout()),
            this, SLOT(retryTimedOutCommands()));
    commandRetryTimer->start();
    reconnectTimer = new QTimer(this);
    reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(attemptReconnect()));
    // Enable OS-level TCP Keep-Alive to prevent NAT/firewall from dropping idle sockets
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(socket, SIGNAL(disconnected()), this, SLOT(connectionLost()));
    connect(socket, SIGNAL(connected()), this, SLOT(connectionRestored()));

    project = nullptr;
    params = nullptr;
    ownPackage = false;
    doAction = true;
    projectIsOpen = false;
    dialogIsOpen = false;
    intentionalClose = false;
    reconnecting = false;
    reconnectAttempts = 0;
    collaborationState = CollaborationState::Disconnected;
    lastObservedProjectRevision = -1;
    lastObservedEventIndex = -1;
    
    communicationModule = new QTabWidget;

    chat = new TupChat;
    communicationModule->addTab(chat, tr("Chat"));
    
    connect(chat, SIGNAL(requestSendMessage(const QString&)), this, SLOT(sendChatMessage(const QString&)));
    
    notices = new TupNotice;
    communicationModule->addTab(notices, tr("Notices"));
    
    // Create collaborators list
    collaboratorsList = new TupCollaboratorsList;
    
    // Create container with splitter
    communicationContainer = new QSplitter(Qt::Horizontal);
    communicationContainer->setWindowTitle(tr("Communications"));
    communicationContainer->setWindowIcon(QPixmap(THEME_DIR + "icons/chat.png"));
    communicationContainer->addWidget(collaboratorsList);
    communicationContainer->addWidget(communicationModule);
    communicationContainer->setStretchFactor(0, 0); // Collaborators list doesn't stretch
    communicationContainer->setStretchFactor(1, 1); // Tab widget stretches    
}

TupNetProjectManagerHandler::~TupNetProjectManagerHandler()
{
    #ifdef TUP_DEBUG
        qDebug() << "[~TupNetProjectManagerHandler()]";
    #endif

    if (commandRetryTimer)
        commandRetryTimer->stop();
    if (reconnectTimer)
        reconnectTimer->stop();

    if (commandTracker)
        commandTracker->clear();

    // Robustly disconnect and delete the socket to avoid use-after-free and queued event crashes
    if (socket) {
        socket->blockSignals(true);
        socket->disconnect();
        socket->close();
        socket->setParent(nullptr);
        socket->deleteLater();
        socket = nullptr;
    }

    if (chat) {
        chat->close();
    }
}

void TupNetProjectManagerHandler::handleProjectRequest(const TupProjectRequest *request)
{
#ifdef TUP_DEBUG
    qDebug() << "[TupNetProjectManagerHandler::handleProjectRequest()]";
#endif

    if (!request) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Null request.";
        return;
    }

    if (!request->isValid()) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Invalid request."
            << "Action:" << request->getActionId();
#endif
        return;
    }

    if (request->getCommandId().isEmpty()) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Request has no command ID."
            << "Action:" << request->getActionId();
        return;
    }

    if (collaborationState != CollaborationState::Connected) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Command blocked because collaborative editing is unavailable."
            << "Command:" << request->getCommandId();
#endif
        return;
    }

    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Socket is not connected."
            << "Command:" << request->getCommandId();
#endif
        return;
    }

#ifdef TUP_DEBUG
    qDebug()
        << "[TupNetProjectManagerHandler::handleProjectRequest()]"
        << "Sending command:" << request->getCommandId()
        << "Action:" << request->getActionId();

    qDebug() << request->getXml();
#endif

    if (!commandTracker || !commandTracker->track(*request)) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectRequest()]"
            << "Unable to track command:"
            << request->getCommandId();
        return;
    }

    // Preserve the existing optimistic local execution behavior.
    emit sendCommand(request, true);
    socket->send(request->getXml());
}

bool TupNetProjectManagerHandler::commandExecuted(TupProjectResponse *response)
{
    if (collaborationState != CollaborationState::Connected) {
#ifdef TUP_DEBUG
        qWarning() << "[TupNetProjectManagerHandler::commandExecuted()] Collaborative editing is suspended.";
#endif
        return false;
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::commandExecuted()]";
    #endif

    if (response->getMode() == TupProjectResponse::Do) {
        doAction = true;
        return true;
    } 

    TupProjectRequest request = TupRequestBuilder::fromResponse(response, false);
    doAction = false;

    if (response->getMode() != TupProjectResponse::Undo && response->getMode() != TupProjectResponse::Redo) {
        handleProjectRequest(&request);
    } else { 
        if (socket->state() == QAbstractSocket::ConnectedState) {
            if (request.isValid())
                socket->send(request.getXml());
        }
    }

    return true;
}

bool TupNetProjectManagerHandler::saveProject(const QString &fileName, TupProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::saveProject()]";
    #endif

    Q_UNUSED(fileName)
    Q_UNUSED(project)

    return true;
}

bool TupNetProjectManagerHandler::loadProject(const QString &fileName, TupProject *project)
{
    Q_UNUSED(fileName)
    Q_UNUSED(project)

    return true;
}

void TupNetProjectManagerHandler::loadProjectFromServer(const QString &projectID, const QString &owner)
{
    currentProjectId = projectID;
    currentProjectOwner = owner;
    lastObservedProjectRevision = -1;
    lastObservedEventIndex = -1;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    TupOpenPackage package(projectID, owner);
    socket->send(package);
}

void TupNetProjectManagerHandler::initialize(TupProjectManagerParams *parameters)
{
    TupNetProjectManagerParams *netParams = dynamic_cast<TupNetProjectManagerParams*>(parameters);

    if (!netParams)
        return;
    
    params = netParams;
    intentionalClose = false;
    reconnecting = false;
    reconnectAttempts = 0;
    collaborationState = CollaborationState::Disconnected;

    #ifdef TUP_DEBUG
        QString server = netParams->server() + ":" + QString::number(netParams->port());
        qDebug() << "[TupNetProjectManagerHandler::initialize()] - Connecting to ->" << server;
    #endif

    socket->connectToHost(netParams->server(), netParams->port());
    bool connected = socket->waitForConnected(1000);
    if (connected) {
        TupConnectPackage connectPackage(netParams->server(), netParams->login(), netParams->windowRecordID());
        socket->send(connectPackage);
        username = netParams->login();
    } else {
        TOsd::self()->display(TOsd::Error, tr("Unable to connect to server"));
        emit authenticationFailed();
    }
}

bool TupNetProjectManagerHandler::setupNewProject(TupProjectManagerParams *params)
{
    TupNetProjectManagerParams *netParams = dynamic_cast<TupNetProjectManagerParams*>(params);
    
    if (!netParams)
        return false;
    
    #ifdef TUP_DEBUG
        qWarning() << "netParams->projectName() : " << netParams->getProjectManager();
        SHOW_VAR(netParams->getProjectManager());
    #endif    

    projectName = netParams->getProjectManager();
    QString dimension = QString::number(netParams->getDimension().width()) + "," + QString::number(netParams->getDimension().height());

    TupNewProjectPackage newProjectPackage(netParams->getProjectManager(), netParams->getAuthor(), netParams->getDescription(),
                                           netParams->getBgColor().name(), dimension, QString::number(netParams->getFPS()));
    socket->send(newProjectPackage);
    
    return true;
}

bool TupNetProjectManagerHandler::closeProject()
{
    projectIsOpen = false;
    closeConnection();

    return TupAbstractProjectHandler::closeProject();
}

void TupNetProjectManagerHandler::emitRequest(TupProjectRequest *request, bool toStack)
{
    emit sendCommand(request, toStack);
}

void TupNetProjectManagerHandler::handlePackage(const QString &root, const QString &package)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::handlePackage()] - PKG:";
        qDebug() << package;
    #endif

    if (root == "user_denied") {
        closeConnection();
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Fatal Error"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("User \"%1\" is disabled.\nPlease, contact the TupiTube server admin to get access.").arg(params->login()));
        msgBox.exec();
    } else if (root == QStringLiteral("project_event")) {
        handleProjectEvent(package);
    } else if (root == "project_request") {
        TupRequestParser parser;

        if (!parser.parse(package)) {
#ifdef TUP_DEBUG
            qWarning()
                << "[TupNetProjectManagerHandler::handlePackage()]"
                << "Error parsing project request.";
#endif
            return;
        }

        TupProjectResponse *response = parser.getResponse();

        if (!response) {
            qWarning()
                << "[TupNetProjectManagerHandler::handlePackage()]"
                << "No response was generated.";
            return;
        }

        const QString commandId = response->getCommandId();

        if (commandId.isEmpty()) {
            qWarning()
                << "[TupNetProjectManagerHandler::handlePackage()]"
                << "Received command without an ID.";
            return;
        }

#ifdef TUP_DEBUG
        qDebug()
            << "[TupNetProjectManagerHandler::handlePackage()]"
            << "Received command:" << commandId
            << "Action:" << response->getAction();
#endif

        ownPackage = (parser.getSign() == sign);

        if (ownPackage && !doAction) {
            if (response->getPart() == TupProjectRequest::Item) {
                TupItemResponse *itemResponse =
                    static_cast<TupItemResponse *>(response);

                TupProjectRequest selectionRequest =
                    TupRequestBuilder::createFrameRequest(
                        itemResponse->getSceneIndex(),
                        itemResponse->getLayerIndex(),
                        itemResponse->getFrameIndex(),
                        TupProjectRequest::Select,
                        QVariant(),
                        QByteArray(),
                        QString());

                selectionRequest.setExternal(false);
                emit sendLocalCommand(&selectionRequest);
            }

            return;
        }

        // This is a replay of the command received from the server, so its
        // original command ID must be preserved.
        TupProjectRequest request =
            TupRequestBuilder::fromResponse(response, true);

        request.setExternal(!ownPackage);
        emitRequest(&request, doAction && ownPackage);
    } else if (root == "server_ack") {
               // Checking the package
               TupAckParser parser(package);
               if (parser.parse()) {
                   sign = parser.sign();

                   if (collaborationState == CollaborationState::Recovering) {
#ifdef TUP_DEBUG
                       qDebug() << "[TupNetProjectManagerHandler::handlePackage()] Recovery authentication successful.";
#endif
                       if (!currentProjectId.isEmpty()) {
                           loadProjectFromServer(currentProjectId, currentProjectOwner);
                       } else {
                           qWarning() << "[TupNetProjectManagerHandler::handlePackage()] Cannot restore collaborative project: project identity is unknown.";
                           reconnecting = false;
                           collaborationState = CollaborationState::Disconnected;
                           if (commandTracker)
                               commandTracker->clear();
                           emit connectionHasBeenLost(DisconnectReason::NetworkError);
                       }
                   } else {
                       collaborationState = CollaborationState::Connected;
                       // Initial login successful.
                       emit authenticationSuccessful();
                   }
               }
    } else if (root == "server_project") {
               TupProjectParser parser(package);
               if (parser.parse()) {
                   QTemporaryFile file;
                   if (file.open()) {
                       file.write(parser.data());
                       file.flush();
            
                       if (project) {
                           TupFileManager *manager = new TupFileManager;
                           bool isOk = manager->load(file.fileName(), project);
                           if (isOk) {
                               projectIsOpen = true;

                               if (collaborationState == CollaborationState::Recovering) {
#ifdef TUP_DEBUG
                                   qDebug() << "[TupNetProjectManagerHandler::handlePackage()] Collaborative project restored after reconnect.";
#endif
                                   delete manager;
                                   resumePendingCommands();
                                   collaborationState = CollaborationState::Connected;
                                   emit collaborationRecoveryFinished();
                                   QApplication::restoreOverrideCursor();
                               } else {
                                   collaborationState = CollaborationState::Connected;
                                   emit openNewArea(project->getName(), parser.partners());
                                   delete manager;
                               }
                           } else {
                               #ifdef TUP_DEBUG
                                   qWarning() << "[TupNetProjectManagerHandler::handlePackage()] - Error: Net project can't be opened";
                               #endif

                               delete manager;
                               QApplication::restoreOverrideCursor();
                               TOsd::self()->display(TOsd::Error, tr("The project may be corrupt or incomplete."));
                               emit connectionHasBeenLost();
                               return;
                           }
                       } else {
                           #ifdef TUP_DEBUG
                               qWarning() << "[TupNetProjectManagerHandler::handlePackage()] - Error: Can't open project";
                           #endif
                           QApplication::restoreOverrideCursor();
                           TOsd::self()->display(TOsd::Error, tr("The project may be corrupt or incomplete."));
                           emit connectionHasBeenLost();
                           return;
                       }
                   }
               }
    } else if (root == "server_projectlist") {
               TupProjectListParser parser(package);
               if (parser.parse()) {
                   int works = parser.workSize();
                   int contributions = parser.contributionSize();
                   if ((works + contributions) > 0) {
                       dialog = new TupProjectListDialog(works, contributions, params->server());
                       QDesktopWidget desktop;
                       dialog->show();

                       QPair<int, int> dimension = TAlgorithm::screenDimension();
                       int screenWidth = dimension.first;
                       int screenHeight = dimension.second;
                       dialog->move(static_cast<int> ((screenWidth - dialog->width()) / 2),
                                    static_cast<int> ((screenHeight - dialog->height()) / 2));

                       dialogIsOpen = true;

                       foreach (TupProjectListParser::ProjectInfo info, parser.works())
                                dialog->addWork(info.file, info.name, info.description, info.date);

                       foreach (TupProjectListParser::ProjectInfo info, parser.contributions())
                                dialog->addContribution(info.file, info.name, info.author, info.description, info.date);

                       int dialogResult = dialog->exec();
                       if (dialogResult == QDialog::Accepted && !dialog->projectID().isEmpty()) {
                           #ifdef TUP_DEBUG
                               qDebug() << "[TupNetProjectManagerHandler::handlePackage()] - Opening project ->" << dialog->projectID();
                           #endif
                           dialogIsOpen = false;
                           if (dialog->workIsMine())
                               loadProjectFromServer(dialog->projectID(), username);
                           else
                               loadProjectFromServer(dialog->projectID(), dialog->owner());
                       } else {
                           dialogIsOpen = false;
                           closeConnection();
                       }
                   } else {
                       TOsd::self()->display(TOsd::Warning, tr("User has no available projects in the server"));
                       #ifdef TUP_DEBUG
                           qDebug() << "[TupNetProjectManagerHandler::handlePackage()] - Info: User has no available projects in the server";
                       #endif
                       closeConnection();
                   }
               }
    } else if (root == "communication_notification") {
               TupNotificationParser parser(package);
               if (parser.parse()) {
                   int code = parser.notification().code;
                   switch(code) {
                          case 400:
                               emit authenticationFailed();
                               return;
                          break;
                          case 380:
                               emit savingSuccessful();
                          break;
                          case 100:
                          case 101:
                          case 102:
                          case 382:
                          case 383:
                          case 384:
                               emit postOperationDone();
                          break;
                   }

                   TOsd::Level level = TOsd::Level(parser.notification().level);
                   TOsd::self()->display(level, parser.notification().message);                   
               }
    } else if (root == "communication_chat") {
               TupCommunicationParser parser(package);
               if (parser.parse()) {
                   chat->addMessage(parser.login(), parser.message());
                   emit newMessageReceived(0); // 0 = chat message
               }
    } else if (root == "communication_notice") {
               TupCommunicationParser parser(package);
               if (parser.parse()) {
                   QString login = parser.login();
                   int state = parser.state();

                   emit updateUsersList(login, state);

                   QString message = "<b>" + login + "</b>" + " has left the project"; 
                   if (state == 1)
                       message = "<b>" + login + "</b>" + " has joined the project";

                   TOsd::self()->display(TOsd::Info, message);
                   notices->addMessage(message);
                   emit newMessageReceived(1); // 1 = notice message
               } 
    } else if (root == "communication_wall") {
               TupCommunicationParser parser(package);
               if (parser.parse()) {
                   QString message = QObject::tr("Message from") + " <b>" + parser.login() + "</b>:<br>" + parser.message();
                   TOsd::self()->display(TOsd::Info, message);
                   notices->addMessage(message);
                   emit newMessageReceived(1); // 1 = notice message
               }
    } else if (root == "project_storyboard_update") {
               // qDebug() << "TupNetProjectManagerHandler::handlePackage() - Updating the storyboard...";
               TupStoryboardParser parser(package);

               if (parser.checksum()) {
                   if ((parser.sceneIndex() >= 0) && (parser.storyboardXml().length() > 0)) {
                       TupStoryboard *storyboard = new TupStoryboard;
                       storyboard->fromXml(parser.storyboardXml());
                       project->sceneAt(parser.sceneIndex())->setStoryboard(storyboard);
                   } else {
                       #ifdef TUP_DEBUG
                           qWarning() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_storyboard package";
                       #endif
                   }
               } else {
                   #ifdef TUP_DEBUG
                       qWarning() << "[ProjectManager::handlePackage()] - Fatal Error: Can't parse project_storyboard package"; 
                   #endif

               }
    } else if (root == "storyboard_update") {
               // SQA: storyboard package must be parsed and the related scene must be updated
    } else if (root == "disconnect") {
               // Server is gracefully telling us why it's disconnecting
               QDomDocument doc;
               if (doc.setContent(package)) {
                       QString reason = doc.documentElement().attribute("reason");
                       if (reason == "inactivity") {
                           m_disconnectReason = DisconnectReason::UserInactivity;
                       }
               }
               // Initiate graceful socket closure
               if (socket) {
                       socket->disconnectFromHost();
               }
               return;
    } else if (root == QStringLiteral("command_result")) {
        TupCommandResultParser parser;

        if (!parser.parse(package)) {
            qWarning()
                << "[TupNetProjectManagerHandler::handlePackage()]"
                << "Unable to parse command result:"
                << parser.errorString();
            return;
        }

        QString status;

        switch (parser.status()) {
            case TupCommandResultParser::Committed:
                status = QStringLiteral("committed");
#ifdef TUP_DEBUG
                qDebug()
                    << "[TupNetProjectManagerHandler::handlePackage()]"
                    << "Command committed:"
                    << parser.commandId();
#endif
                break;

            case TupCommandResultParser::Rejected:
                status = QStringLiteral("rejected");
                qWarning()
                    << "[TupNetProjectManagerHandler::handlePackage()]"
                    << "Command rejected:"
                    << parser.commandId()
                    << "Error:"
                    << parser.errorCode()
                    << "Message:"
                    << parser.message();
                break;

            case TupCommandResultParser::Failed:
                status = QStringLiteral("failed");
                qWarning()
                    << "[TupNetProjectManagerHandler::handlePackage()]"
                    << "Command failed:"
                    << parser.commandId()
                    << "Error:"
                    << parser.errorCode()
                    << "Message:"
                    << parser.message();
                break;

            case TupCommandResultParser::Invalid:
            default:
                qWarning()
                    << "[TupNetProjectManagerHandler::handlePackage()]"
                    << "Invalid command result status.";
                return;
        }

        if (commandTracker)
            commandTracker->complete(parser.commandId());

        emit commandResultReceived(
            parser.commandId(),
            status,
            parser.errorCode(),
            parser.message());
    } else {
      #ifdef TUP_DEBUG
          qWarning() << "[TupNetProjectManagerHandler::handlePackage()] - Error: Unknown package ->" << root;
      #endif
    }
}

void TupNetProjectManagerHandler::handleProjectEvent(const QString &package)
{
    QDomDocument document;

    if (!document.setContent(package)) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Unable to parse project_event XML.";
        return;
    }

    const QDomElement root = document.documentElement();
    if (root.isNull() || root.tagName() != QStringLiteral("project_event")) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Invalid project_event root element.";
        return;
    }

    bool versionOk = false;
    const int version = root.attribute(QStringLiteral("version")).toInt(&versionOk);
    if (!versionOk || version != 1) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Unsupported project_event version:"
            << root.attribute(QStringLiteral("version"));
        return;
    }

    const QString eventId =
        root.attribute(QStringLiteral("event_id")).trimmed();
    const QString causedBy =
        root.attribute(QStringLiteral("caused_by")).trimmed();
    const QString eventProjectId =
        root.attribute(QStringLiteral("project_id")).trimmed();
    const QString eventType =
        root.attribute(QStringLiteral("event_type")).trimmed();

    bool revisionOk = false;
    const qint64 revision =
        root.attribute(QStringLiteral("revision")).toLongLong(&revisionOk);

    bool eventIndexOk = false;
    const int eventIndex =
        root.attribute(QStringLiteral("event_index")).toInt(&eventIndexOk);

    if (eventId.isEmpty()
            || causedBy.isEmpty()
            || eventProjectId.isEmpty()
            || eventType.isEmpty()
            || !revisionOk
            || revision <= 0
            || !eventIndexOk
            || eventIndex < 0) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Incomplete project_event metadata.";
        return;
    }

    if (!currentProjectId.isEmpty()
            && eventProjectId != currentProjectId) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Ignoring event for a different project."
            << "Current:" << currentProjectId
            << "Event project:" << eventProjectId;
        return;
    }

    // TCP preserves ordering for a live connection. Validate the complete
    // authoritative ordering key (revision, event_index), so a future
    // command may safely emit more than one event for the same revision.
    if (lastObservedProjectRevision < 0) {
        if (eventIndex != 0) {
            qCritical()
                << "[TupNetProjectManagerHandler::handleProjectEvent()]"
                << "First observed event does not start at event_index 0."
                << "Revision:" << revision
                << "Index:" << eventIndex
                << "Event:" << eventId;
            return;
        }
    } else if (revision < lastObservedProjectRevision
               || (revision == lastObservedProjectRevision
                   && eventIndex <= lastObservedEventIndex)) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Ignoring duplicate/out-of-order event."
            << "Event:" << eventId
            << "Revision:" << revision
            << "Index:" << eventIndex
            << "Last revision:" << lastObservedProjectRevision
            << "Last index:" << lastObservedEventIndex;
#endif
        return;
    } else {
        const bool nextSameRevision =
            revision == lastObservedProjectRevision
            && eventIndex == lastObservedEventIndex + 1;
        const bool nextRevision =
            revision == lastObservedProjectRevision + 1
            && eventIndex == 0;

        if (!nextSameRevision && !nextRevision) {
            qCritical()
                << "[TupNetProjectManagerHandler::handleProjectEvent()]"
                << "Authoritative event sequence gap detected."
                << "Last:" << lastObservedProjectRevision
                << lastObservedEventIndex
                << "Received:" << revision
                << eventIndex
                << "Event:" << eventId;
            return;
        }
    }

    const QDomElement payloadElement =
        root.firstChildElement(QStringLiteral("payload"));
    if (payloadElement.isNull()) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "project_event has no payload.";
        return;
    }

    const QString payloadXml = payloadElement.text().trimmed();
    if (payloadXml.isEmpty()) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "project_event payload is empty.";
        return;
    }

    TupRequestParser parser;
    if (!parser.parse(payloadXml)) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Unable to parse authoritative event payload."
            << "Event:" << eventId;
        return;
    }

    TupProjectResponse *response = parser.getResponse();
    if (!response) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Event payload produced no project response."
            << "Event:" << eventId;
        return;
    }

    if (response->getCommandId() != causedBy) {
        qWarning()
            << "[TupNetProjectManagerHandler::handleProjectEvent()]"
            << "Event cause does not match payload command ID."
            << "Event:" << eventId
            << "caused_by:" << causedBy
            << "payload command:" << response->getCommandId();
        return;
    }

    TupProjectRequest request =
        TupRequestBuilder::fromResponse(response, true);
    request.setExternal(true);

#ifdef TUP_DEBUG
    qWarning()
        << "[TupNetProjectManagerHandler::handleProjectEvent()]"
        << "Applying authoritative event:"
        << eventId
        << "Type:" << eventType
        << "Revision:" << revision
        << "Command:" << causedBy;
#endif

    emitRequest(&request, false);
    lastObservedProjectRevision = revision;
    lastObservedEventIndex = eventIndex;
}

bool TupNetProjectManagerHandler::isValid() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void TupNetProjectManagerHandler::sendPackage(const QDomDocument &doc)
{
    // #ifdef TUP_DEBUG
    //     qDebug() << "[TupNetProjectManagerHandler::sendPackage()] - xml: " << doc.toString();
    // #endif

    socket->send(doc);
}

QTabWidget *TupNetProjectManagerHandler::communicationWidget()
{
    return communicationModule;
}

QWidget *TupNetProjectManagerHandler::communicationPanel()
{
    return communicationContainer;
}

void TupNetProjectManagerHandler::updateCollaboratorStatus(const QString &login, int state)
{
    collaboratorsList->updateUserStatus(login, state);
}

void TupNetProjectManagerHandler::setCollaborators(const QStringList &users)
{
    collaboratorsList->setCurrentUser(username);
    collaboratorsList->setInitialUsers(users);
}

void TupNetProjectManagerHandler::setProject(TupProject *work)
{
    project = work;
}

void TupNetProjectManagerHandler::sendChatMessage(const QString &message)
{
    TupChatPackage package(message);
    sendPackage(package);
}

void TupNetProjectManagerHandler::connectionLost()
{
#ifdef TUP_DEBUG
    qWarning() << "[TupNetProjectManagerHandler::connectionLost()] - The socket has been closed";
#endif

    if (intentionalClose || collaborationState == CollaborationState::Closing) {
        if (commandTracker)
            commandTracker->clear();
        reconnecting = false;
        reconnectAttempts = 0;
        collaborationState = CollaborationState::Disconnected;
        m_disconnectReason = DisconnectReason::UnknownDisconnectReason;
        return;
    }

    if (m_disconnectReason == DisconnectReason::UserInactivity) {
        if (commandTracker)
            commandTracker->clear();
        reconnecting = false;
        collaborationState = CollaborationState::Disconnected;
        emit connectionHasBeenLost(m_disconnectReason);
        m_disconnectReason = DisconnectReason::UnknownDisconnectReason;
        return;
    }

    if (!projectIsOpen) {
        collaborationState = CollaborationState::Disconnected;
        emit connectionHasBeenLost(DisconnectReason::NetworkError);
        return;
    }

    m_disconnectReason = DisconnectReason::NetworkError;
    collaborationState = CollaborationState::Recovering;

    if (commandRetryTimer)
        commandRetryTimer->stop();

    reconnecting = true;
    reconnectAttempts = 0;

#ifdef TUP_DEBUG
    qWarning() << "[TupNetProjectManagerHandler::connectionLost()] Entering recovery mode. Pending commands:"
               << (commandTracker ? commandTracker->pendingCount() : 0);
#endif

    emit collaborationRecoveryStarted();

    if (reconnectTimer && !reconnectTimer->isActive())
        reconnectTimer->start();
}


void TupNetProjectManagerHandler::attemptReconnect()
{
    if (!reconnecting || intentionalClose || !params || !socket)
        return;

    if (socket->state() == QAbstractSocket::ConnectedState ||
            socket->state() == QAbstractSocket::ConnectingState)
        return;

    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        reconnectTimer->stop();
        reconnecting = false;
        collaborationState = CollaborationState::Disconnected;
        qWarning() << "[TupNetProjectManagerHandler::attemptReconnect()] Reconnect limit reached. Closing collaborative session. Pending commands:"
                   << (commandTracker ? commandTracker->pendingCount() : 0);
        if (commandTracker)
            commandTracker->clear();
        emit connectionHasBeenLost(DisconnectReason::NetworkError);
        return;
    }

    ++reconnectAttempts;
#ifdef TUP_DEBUG
    qWarning() << "[TupNetProjectManagerHandler::attemptReconnect()] Attempt" << reconnectAttempts
               << "of" << MAX_RECONNECT_ATTEMPTS;
#endif
    socket->connectToHost(params->server(), params->port());
}

void TupNetProjectManagerHandler::connectionRestored()
{
    if (!reconnecting || !params || !socket)
        return;

    if (reconnectTimer)
        reconnectTimer->stop();

#ifdef TUP_DEBUG
    qDebug() << "[TupNetProjectManagerHandler::connectionRestored()] TCP connection restored. Re-authenticating.";
#endif

    TupConnectPackage connectPackage(params->server(), params->login(), params->windowRecordID());
    socket->send(connectPackage);
}

void TupNetProjectManagerHandler::resumePendingCommands()
{
    if (!reconnecting || !commandTracker || !socket ||
            socket->state() != QAbstractSocket::ConnectedState)
        return;

    commandTracker->restartTimeoutWindow();
    const QList<QString> pendingIds = commandTracker->pendingCommandIds();

#ifdef TUP_DEBUG
    qDebug() << "[TupNetProjectManagerHandler::resumePendingCommands()] Resuming pending commands:" << pendingIds.count();
#endif

    for (const QString &commandId : pendingIds) {
        const QString xml = commandTracker->commandXml(commandId);
        if (!xml.isEmpty())
            socket->send(xml);
    }

    reconnecting = false;
    reconnectAttempts = 0;
    if (commandRetryTimer && !commandRetryTimer->isActive())
        commandRetryTimer->start();
}

void TupNetProjectManagerHandler::retryTimedOutCommands()
{
    if (!commandTracker || commandTracker->pendingCount() == 0)
        return;

    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;

    const QList<QString> expired =
        commandTracker->expiredCommandIds(COMMAND_ACK_TIMEOUT_MS);

    for (const QString &commandId : expired) {
        if (!commandTracker->contains(commandId))
            continue;

        const int retries = commandTracker->retryCount(commandId);

        if (retries >= COMMAND_MAX_RETRIES) {
            qWarning()
                << "[TupNetProjectManagerHandler::retryTimedOutCommands()]"
                << "Command acknowledgment timed out."
                << "Command:" << commandId
                << "Retries:" << retries;

            commandTracker->complete(commandId);

            emit commandResultReceived(
                commandId,
                QStringLiteral("failed"),
                QStringLiteral("ack_timeout"),
                tr("The server did not acknowledge the command after multiple retries."));
            continue;
        }

        const QString xml = commandTracker->commandXml(commandId);
        if (xml.isEmpty()) {
            qWarning()
                << "[TupNetProjectManagerHandler::retryTimedOutCommands()]"
                << "Cannot retry command because its XML is empty:"
                << commandId;

            commandTracker->complete(commandId);
            continue;
        }

        if (!commandTracker->markRetried(commandId))
            continue;

#ifdef TUP_DEBUG
        qWarning()
            << "[TupNetProjectManagerHandler::retryTimedOutCommands()]"
            << "Resending timed-out command:" << commandId
            << "Retry:" << commandTracker->retryCount(commandId);
#endif

        socket->send(xml);
    }
}

void TupNetProjectManagerHandler::closeConnection()
{
    intentionalClose = true;
    reconnecting = false;
    collaborationState = CollaborationState::Closing;
    if (reconnectTimer)
        reconnectTimer->stop();
    if (commandTracker)
        commandTracker->clear();

    if (socket && socket->isOpen())
        socket->close();
}

void TupNetProjectManagerHandler::sendExportImageRequest(int frameIndex, int sceneIndex, 
                                                         const QString &title, const QString &topics, const QString &description)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::sendExportImageRequest()]";
    #endif

    TupImageExportPackage package(frameIndex, sceneIndex, title, topics, description);
    sendPackage(package);
}

void TupNetProjectManagerHandler::sendVideoRequest(const QString &title, const QString &topics, const QString &description, int fps, const QList<int> sceneIndexes)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::sendVideoRequest()]";
    #endif

    TupVideoExportPackage package(title, topics, description, fps, sceneIndexes);
    sendPackage(package);
}

void TupNetProjectManagerHandler::updateStoryboardRequest(TupStoryboard *storyboard, int sceneIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::updateStoryboardRequest()]";
    #endif

    QDomDocument doc;
    QDomElement story = storyboard->toXml(doc);
    TupStoryboardUpdatePackage package(story, sceneIndex);
    sendPackage(package);
}

void TupNetProjectManagerHandler::postStoryboardRequest(int sceneIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler::postStoryboardRequest()]";
    #endif

    TupStoryboardExportPackage package(sceneIndex);
    sendPackage(package);
}
