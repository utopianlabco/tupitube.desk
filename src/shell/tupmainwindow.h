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

#ifndef TUPMAINWINDOW_H
#define TUPMAINWINDOW_H

#include "tactionmanager.h"
#include "tupdocumentview.h"
#include "tupanimationspace.h"
#include "tuppreferencesdialog.h"

// Modules
#include "tupexposuresheet.h"
#include "tupbrushwidget.h"
#include "tupcolorpalettewidget.h"
// #include "tupsceneswidget.h"
#include "tuplibrarywidget.h"
#include "tuptimeline.h"
#include "tupcamerawidget.h"
#include "tupnewsdialog.h"
#include "tupexportwidget.h"

#include "tabbedmainwindow.h"
#include "tupstatusbar.h"
#include "tosd.h"
#include "toolview.h"

// Projects management 
#include "tupprojectmanager.h"
#include "tupnetprojectmanagerhandler.h"

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QUndoStack>
#include <QKeySequence>
#include <QTextBrowser>
#include <QToolBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPointer>

class TupProjectManagerParams;
class TupNetProjectManagerParams;
class TupProjectResponse;
class TupCommandCoordinator;
class QMessageBox;

class TupMainWindow : public TabbedMainWindow
{
    Q_OBJECT

    public:

        /*
        enum Perspective {
             Animation = 0x01,
             Player = 0x02,
             All = Animation | Player
        };
        enum RequestType {
             None = 0,
             NewLocalProject,
             OpenLocalProject,
             NewNetProject,
             OpenNetProject,
             UploadLocalProjectToNet
        };
        */

        TupMainWindow(const QString &winKey, const QString &sourceFile);
        ~TupMainWindow();

    signals:
        void responsed(TupProjectResponse *);
        void updateAnimationModule(TupProject *, int, int, int);
        void activeDockChanged(TupDocumentView::DockType);
        void imagePosted();
        void imageExported();
        void storyboardCalled();

    private:
        void createGUI();
        void setupFileActions();
        void setPreferenceActions();
        void setupToolBar();
        void setupMenu();

        void setupHelpActions();
        void setMenuItemsContext(bool flag);

        void connectWidgetToManager(QWidget *widget);
        void disconnectWidgetToManager(QWidget *widget);
        void connectUndoMacroSignals(QWidget *widget);
        void disconnectUndoMacroSignals(QWidget *widget);
        void connectWidgetToPaintArea(QWidget *widget);
        void connectWidgetToLocalManager(QWidget *widget);

        void showCollaborativeConnectionDialog();
        void setupCollaborativeProject();
        void setupCollaborativeProject(TupProjectManagerParams *params);
        void setupLocalProject(TupProjectManagerParams *params);
        void setUndoRedoActions();
        void resetUI();
        void updateRecentProjectList();
        void saveDefaultPath(const QString &dir);
        bool cancelChanges();
        bool storeProcedure();
        void setupCameraConnections();
        void disconnectCameraConnections();

    protected:
        #if defined(Q_OS_MAC)
            bool event(QEvent *event);
        #endif
        void closeEvent(QCloseEvent *event);
        void updateOpenRecentMenu(QMenu *menu, QStringList recents);
        void dragEnterEvent(QDragEnterEvent *event);
        void dropEvent(QDropEvent *event);

    public slots:
        void openProject(const QString &path);
        void openExample();
        void updateColor(TColorCell::FillType type, const QColor &color);
        void updatePenThickness(int thickness);
        void exportProject();
        void postProject();
        void postFrame(const QString &imagePath);
        void releaseSoundRecord(ModuleSource source, const QString &soundKey);
        void releaseAudioResources();
        void enableVisibilityControls();

    private slots:
        void closeCollaborativeProjectIfOpen();
        void enableUpdatesDialog();
        void setWorkSpace(const QStringList &users = QStringList());
        void createNewLocalProject();
        void newProject();
        void closeInterface();
        bool closeProject();
        void unexpectedClose(DisconnectReason reason);
        void openProject();
        void openProjectFromServer();
        void uploadProjectToServer();

        void importLibrary();
        void importProject();
        void importSourceFile(bool onlyLibrary = false);
        bool saveAs();
        bool saveProject();

        void showAnimationMenu(const QPoint &point);

        void changePerspective(QAction *action);
        void changePerspective(UIView tabType);

        void updateCurrentTab(UIView tabType);

        void requestProject();
        void createNewNetProject(const QString &title, const QStringList &users);
        void netProjectSaved();
        void notifyChatMessage(int messageType);
        void handleChatVisibilityChanged(bool visible);
        void handleChatTabChanged(int index);
        void updatePlayer();
        void updatePlayer(bool removeAction);

        void resizePlayerCameraDimension(const QSize size);
        void resizeCanvasDimension(const QSize size);
        void updateProjectAuthor(const QString &artist);
        void editProjectSize();

    private slots:
        void preferences();
        // void showHelp();
        void checkTupiTubeUpdates();
        void aboutTupiTube();
        void openYouTubeChannel();
        void importPalettes();
        void openRecentProject();
        void createPaintCommand(const TupPaintAreaEvent *event);
        bool callSaveProcedure();
        void restoreFramesMode(TupProject::Mode contextMode);
        void resetMousePointer();
        void updateUsersOnLine(const QString &login, int state);
        void importPapagayoLipSync();
        void hideTopPanels();
        void showNewsMessage();
        void setUpdateFlag(bool flag);
        void checkTimeLineVisibility(bool visible);
        void checkExposureVisibility(bool visible);
        void updateBucketTool(TColorCell::FillType type);
        void openTupiTubeNetwork();
        void handleCollaborativeAuthenticationFailure();
        void collaborationRecoveryStarted();
        void prepareRecoverySnapshot();
        void completeRecoverySnapshotUi();
        void collaborationRecoveryFinished();
        void updateColorPanelStatus(bool flag);
        void updatePenPanelStatus(bool flag);
        void updateLibraryPanelStatus(bool flag);
        void updateScenesPanelStatus(bool flag);
        void doPlay();
        void requestSaveAction();
        void updateSoundsPath();
        void updateBgColorInPalette(int sceneIndex);
        void updateSoundItems();

    private:
        QString appTitle;
        TupProjectManager *m_projectManager;
        QString m_filename;
        bool lastSave;

    private:
        int screenWidth;
        int screenHeight;
        TupDocumentView *animationTab;
        TupAnimationSpace *playerTab;
        TupNewsDialog *newsDialog;
        TupStatusBar *m_statusBar;
        TActionManager *m_actionManager;
        QMenu *m_fileMenu;
        QMenu *exportMenu;
        QMenu *postMenu;
        QMenu *m_settingsMenu;
        QMenu *m_viewMenu;
        QMenu *m_insertMenu;
        QMenu *m_toolsMenu; 
        QMenu *m_windowMenu;
        QMenu *m_helpMenu;

        QStringList m_recentProjects;
        QMenu *m_recentProjectsMenu;

    // Network variables
    private:
        QPointer<TupNetProjectManagerHandler> netProjectManager;
        TupCommandCoordinator *commandCoordinator;
        bool isNetworked;
        bool collaborationRecovering;
        QPointer<QMessageBox> collaborationRecoveryDialog;
        bool recoverySnapshotConsumersSuspended = false;
        ToolView *m_viewChat;
        QTabWidget *m_chatTabWidget;
        bool m_chatTabHighlighted;
        bool m_noticesTabHighlighted;

    // Components
    private:
        QString uiStyleSheet;
        QToolBar *mainToolBar;
        QToolBar *alternativeToolBar;
        TupExposureSheet *m_exposureSheet;
        // TupScenesWidget *m_scenes;
        TupTimeLine *m_timeLine;

        TupLibraryWidget *m_libraryWidget;
        TupColorPaletteWidget *m_colorPalette;
        TupBrushWidget *m_brushWidget;
        ToolView *exposureView;
        ToolView *colorView;
        ToolView *penView;
        ToolView *libraryView;
        // ToolView *scenesView;  
        ToolView *helpView;
        ToolView *timeView;
        ToolView *exportView;
        TupExportWidget *exportWidget;

        TupCameraWidget *cameraWidget;
        bool isSaveDialogOpen; 
        // bool internetOn;
        UIView lastTab;
        TupProject::Mode contextMode;
        RequestType requestType;
        QString projectName;
        QString author;
        QString netUser;
        QString msgImageName;
        QString msgUrl;
        TAction *updatesAction;
        TupDocumentView::DockType currentDock;
        QString examplePath;       
};

#endif
