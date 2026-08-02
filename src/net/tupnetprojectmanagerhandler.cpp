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

TupNetProjectManagerHandler::TupNetProjectManagerHandler(QObject *parent) : TupAbstractProjectHandler(parent)
{    
    #ifdef TUP_DEBUG
        qDebug() << "[TupNetProjectManagerHandler()]";
    #endif

    socket = new TupNetSocket(this);
    // Enable OS-level TCP Keep-Alive to prevent NAT/firewall from dropping idle sockets
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(socket, SIGNAL(disconnected()), this, SLOT(connectionLost()));

    project = nullptr;
    params = nullptr;
    ownPackage = false;
    doAction = true;
    projectIsOpen = false;
    dialogIsOpen = false;
    
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

    // Milestone 1 keeps the existing optimistic execution behavior.
    emit sendCommand(request, true);
    socket->send(request->getXml());
}

bool TupNetProjectManagerHandler::commandExecuted(TupProjectResponse *response)
{
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
                   // Login successful
                   emit authenticationSuccessful(); 
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
                               emit openNewArea(project->getName(), parser.partners());
                               delete manager;
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

    // If it wasn't explicitly set to "Inactivity" by the server,
    // it means the connection dropped unexpectedly (Network Error).
    if (m_disconnectReason == DisconnectReason::UnknownDisconnectReason) {
        m_disconnectReason = DisconnectReason::NetworkError;
    }

    if (dialogIsOpen) {
        if (dialog) {
            if (dialog->isVisible())
                dialog->close();
        }
        emit connectionHasBeenLost(m_disconnectReason);
    } else if (projectIsOpen) {
               emit connectionHasBeenLost(m_disconnectReason);
    }

    // Reset for the next session
    m_disconnectReason = DisconnectReason::UnknownDisconnectReason;
}

void TupNetProjectManagerHandler::closeConnection()
{
    if (socket->isOpen())
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
