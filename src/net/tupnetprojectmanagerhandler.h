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

#ifndef TUPNETPROJECTMANAGERHANDLER_H
#define TUPNETPROJECTMANAGERHANDLER_H

#include "tglobal.h"
#include "tupabstractprojectmanagerhandler.h"
#include "tupstoryboard.h"

#include "tupnetprojectmanagerparams.h"
#include "tupprojectresponse.h"
#include "tosd.h"
#include "tupprojectcommand.h"
#include "tupcommandexecutor.h"
#include "tupnetsocket.h"
#include "tupprojectrequest.h"
#include "tupnewprojectpackage.h"
#include "tupconnectpackage.h"
#include "tupimageexportpackage.h"
#include "tupvideoexportpackage.h"
#include "tupstoryboardupdatepackage.h"
#include "tupstoryboardexportpackage.h"
#include "tupstoryboardparser.h"
#include "tupnetfilemanager.h"
#include "tupopenpackage.h"
#include "tupchatpackage.h"
#include "tupnotificationparser.h"
#include "tupprojectlistparser.h"
#include "tupprojectparser.h"
#include "tuprequestparser.h"
#include "tupackparser.h"
#include "tupcommunicationparser.h"
#include "tuprequestbuilder.h"
#include "tupproject.h"
#include "tupprojectlistdialog.h"
#include "tupchat.h"
#include "tupnotice.h"
#include "tupcollaboratorslist.h"
#include "tupcommandresultparser.h"
#include "tupcommandtracker.h"

#include <QDomDocument>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QTabWidget>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QSplitter>
#include <QTimer>
#include <QSet>

class TupNetSocket;

class TUPITUBE_EXPORT TupNetProjectManagerHandler : public TupAbstractProjectHandler
{
    Q_OBJECT

    public:
        TupNetProjectManagerHandler(QObject *parent = nullptr);
        ~TupNetProjectManagerHandler();

        virtual void initialize(TupProjectManagerParams *params);
        virtual bool setupNewProject(TupProjectManagerParams *params);
        virtual bool closeProject();

        virtual void handleProjectRequest(const TupProjectRequest* event);
        virtual bool commandExecuted(TupProjectResponse *response);
        virtual bool saveProject(const QString &fileName, TupProject *project);
        virtual bool loadProject(const QString &fileName, TupProject *project);

        void handlePackage(const QString &root, const QString &package);
        virtual bool isValid() const;
        void sendPackage(const QDomDocument &doc);

        QTabWidget *communicationWidget();
        QWidget *communicationPanel();
        void updateCollaboratorStatus(const QString &login, int state);
        void setCollaborators(const QStringList &users);
        void closeConnection();

    signals:
        void projectPathChanged();
        void soundPathsChanged();
        void savingSuccessful();
        void savingFailed();
        void postOperationDone();
        void connectionHasBeenLost(DisconnectReason reason = DisconnectReason::UnknownDisconnectReason);
        void collaborationRecoveryStarted();
        void recoverySnapshotAboutToLoad();
        void recoverySnapshotUiReady();
        void collaborationRecoveryFinished();
        void authenticationFailed();
        void newMessageReceived(int messageType);
        void commandResultReceived(const QString &commandId,
                                   const QString &status,
                                   const QString &errorCode,
                                   const QString &message);

    public slots:
        void sendExportImageRequest(int frameIndex, int sceneIndex, const QString &title, const QString &topics, const QString &description);
        void updateStoryboardRequest(TupStoryboard *storyboard, int sceneIndex);
        void postStoryboardRequest(int sceneIndex);
        void sendVideoRequest(const QString &title, const QString &topics, const QString &description, int fps, const QList<int> sceneIndexes);

    private slots:
        void sendChatMessage(const QString &message);
        void connectionLost();
        void retryTimedOutCommands();
        void attemptReconnect();
        void connectionRestored();
        void heartbeatTick();
        void recoveryWatchdogExpired();

    private:
        enum class CollaborationState
        {
            Disconnected,
            Connected,
            Recovering,
            Closing
        };

        void loadProjectFromServer(const QString &projectID, const QString &owner);
        void emitRequest(TupProjectRequest *request, bool toStack);
        void setProject(TupProject *project);
        void resumePendingCommands();
        bool reapplyPendingCommandAfterSnapshot(const QString &commandId);
        void handleProjectEvent(const QString &package);
        void beginProjectEventGapRecovery();
        void requestProjectSync(bool forceSnapshot = false);
        void handleProjectSyncResponse(const QString &package);
        void finishCollaborationRecovery();
        void startHeartbeat();
        void stopHeartbeat();
        void scheduleReconnect(int delayMs);

        TupNetProjectManagerParams *params;
        TupNetSocket *socket;
        TupCommandTracker *commandTracker;
        QTimer *commandRetryTimer;
        QTimer *reconnectTimer;
        QTimer *heartbeatTimer;
        QTimer *recoveryWatchdogTimer;
        QString projectName;
        QString username;
        TupProject *project;

        QString sign;
        bool ownPackage;
        bool doAction;

        QTabWidget *communicationModule;
        TupChat *chat;
        TupNotice *notices;
        TupCollaboratorsList *collaboratorsList;
        QSplitter *communicationContainer;

        bool projectIsOpen;
        bool dialogIsOpen;
        bool intentionalClose;
        bool reconnecting;
        int reconnectAttempts;
        int reconnectDelayMs;
        int missedHeartbeats;
        CollaborationState collaborationState;
        QString currentProjectId;
        QString currentProjectOwner;
        qint64 lastObservedProjectRevision;
        int lastObservedEventIndex;
        bool recoverySnapshotLoaded;
        qint64 snapshotRecoveryRevision;
        QSet<QString> snapshotReconciliationCommands;
        TupProjectListDialog *dialog;
        DisconnectReason m_disconnectReason = DisconnectReason::UnknownDisconnectReason;
};

#endif
