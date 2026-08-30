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

#include "tupmainwindow.h"
#include "tapptheme.h"
#include "tupnewscollector.h"
#include "tupnewproject.h"
#include "tupsigndialog.h"
#include "tupabout.h"
#include "tuppackagehandler.h"
#include "tuppaletteimporter.h"
#include "tuppaintareaevent.h"
#include "tuppaintareacommand.h"
#include "tupfilemanager.h"
#include "tupgeneralpreferences.h"

// TupiTube Framework
#include "timagedialog.h"
#include "tosd.h"

#include "tupapplication.h"
#include "tuppluginmanager.h"
#include "tupprojectcommand.h"
#include "tupcommandcoordinator.h"
#include "tuplocalprojectmanagerhandler.h"

// Network support
#include "tupnetprojectmanagerparams.h"
#include "tupconnectdialog.h"
#include "tuplistpackage.h"
#include "tupimportprojectpackage.h"
#include "tuplistprojectspackage.h"
#include "tupsavepackage.h"

// Qt Framework
#include <QImage>
#include <QPixmap>
#include <QResizeEvent>
#include <QMenu>
#include <QCloseEvent>
#include <QTextEdit>
#include <QFileDialog>
#include <QDomDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QThread>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileOpenEvent>
#include <QTabBar>
#include <QTimer>

TupMainWindow::TupMainWindow(const QString &winKey, const QString &sourceFile) :
                             TabbedMainWindow(winKey, AnimationView), m_projectManager(nullptr),
                             animationTab(nullptr), playerTab(nullptr),
                             netProjectManager(nullptr), commandCoordinator(nullptr),
                             m_viewChat(nullptr), m_chatTabWidget(nullptr),
                             m_chatTabHighlighted(false), m_noticesTabHighlighted(false),
                             m_exposureSheet(nullptr), isSaveDialogOpen(false)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow()]";
    #endif

    // Naming the main window
    appTitle = "TupiTube Desk";
    setWindowTitle(appTitle);
    setWindowIcon(QIcon(THEME_DIR + "icons/tupitube.png"));
    setObjectName("TupMainWindow_");
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::NoContextMenu);

    QPair<int, int> dimension = TAlgorithm::screenDimension();
    screenWidth = dimension.first;
    screenHeight = dimension.second;
    isNetworked = false;
    collaborationRecovering = false;
    exportWidget = nullptr;

    uiStyleSheet = TAppTheme::themeStyles();
    setStyleSheet(uiStyleSheet);

    #ifdef Q_OS_WIN
        examplePath = SHARE_DIR + "html/examples/example.tup";
    #else
        examplePath = SHARE_DIR + "data/html/examples/example.tup";
    #endif

    // Calling out the project manager
    m_projectManager = new TupProjectManager(this);

    // Generic dependency coordinator for collaborative commands.
    commandCoordinator = new TupCommandCoordinator(this);
    connect(commandCoordinator,
            SIGNAL(commandReady(const TupProjectRequest*)),
            m_projectManager,
            SLOT(handleProjectRequest(const TupProjectRequest*)),
            Qt::DirectConnection);

    // Calling out the events/actions manager
    m_actionManager = new TActionManager(this);

    // Setting up all the GUI...
    createGUI(); // This method is called from the tupmainwindow_gui class

    // The library widget is created by createGUI(), so inject the coordinator
    // only after the GUI components exist.
    m_libraryWidget->setCommandCoordinator(commandCoordinator);

    setupMenu();
    setupToolBar();

    // SQA: Web announcement comes here
    QString webMsgPath = QDir::homePath() + "/." + QCoreApplication::applicationName() + "/webmsg.html";
    QFile webMsgFile(webMsgPath);
    QString fileContent = "";
    if (webMsgFile.exists()) {
        if (webMsgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&webMsgFile);
            while (!in.atEnd())
                fileContent += in.readLine();
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[TupMainWindow()] - Fatal Error: Can't read news msg file ->" << webMsgPath;
            #endif
        }
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow()] - Warning: News msg file doesn't exist ->" << webMsgPath;
        #endif
    }

    if (TCONFIG->firstTime()) {
        TOptionalDialog dialog(tr("Would you like to allow TupiTube to collect anonymous performance data?")
                               + "<br/>" +
                               tr("This information helps us enhance the app and provide a better experience for you.")
                               + "<br/>" +
                               tr("It will be handled in accordance with Utopian Lab's data policy.")
                               + "<br/>" +
                               tr("You can enable or disable this option anytime in the Preferences dialog."),
                               tr("Help Us Improve TupiTube!"), false, false, true, this);
        dialog.setModal(true);
        dialog.move(static_cast<int> ((screenWidth - dialog.sizeHint().width()) / 2),
                    static_cast<int> ((screenHeight - dialog.sizeHint().height()) / 2));
        dialog.exec();

        TOptionalDialog::Result result = dialog.getResult();
        TCONFIG->beginGroup("General");
        bool isEnabled = false;
        if (result == TOptionalDialog::Accepted)
            isEnabled = true;
        TCONFIG->setValue("EnableStatistics", isEnabled);
    } else {
        // Processing web msg content
        bool showWebMsg = false;
        if (!fileContent.isEmpty()) {
            QDomDocument doc;
            if (doc.setContent(fileContent)) {
                QDomElement root = doc.documentElement();
                QDomNode n = root.firstChild();
                while (!n.isNull()) {
                    QDomElement e = n.toElement();
                    if (e.tagName() == "show") {
                        QString flag = e.text();
                        if (flag.compare("true") == 0)
                            showWebMsg = true;
                        else
                            break;
                    } else if (e.tagName() == "url") {
                        msgUrl = e.text();
                    } else if (e.tagName() == "image") {
                        msgImageName = e.text();
                    }

                    n = n.nextSibling();
                }
            } else {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupMainWindow()] - Fatal Error: XML file seems to be corrupted ->" << webMsgPath;
                #endif
            }
        }

        if (showWebMsg) {
            QTimer::singleShot(0, this, SLOT(showNewsMessage()));
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[TupMainWindow()] - Warning: News message has been disabled!";
            #endif
        }
    }

    /* SQA: Check this code for the future
    TCONFIG->beginGroup("General");
    bool update = TCONFIG->value("NotifyUpdate", false).toBool();
    if (update)
        QDesktopServices::openUrl(QString(UPDATES_URL) + QString("downloads"));
    */

    // Time to load plugins... 
    TupPluginManager::instance()->loadPlugins();

    if (TCONFIG->firstTime()) {
        TCONFIG->beginGroup("General");
        TCONFIG->setValue("NotifyUpdate", false);
        TCONFIG->setValue("OpenLastProject", false);
        TCONFIG->setValue("ShowTipOfDay", true);
        TCONFIG->setValue("ConfirmRemoveFrame", true); 
        TCONFIG->setValue("ConfirmRemoveLayer", true); 
        TCONFIG->setValue("ConfirmRemoveScene", true); 
        TCONFIG->setValue("ConfirmRemoveObject", true);

        TCONFIG->beginGroup("Theme");
        TCONFIG->setValue("ColorRow", 0);
        TCONFIG->setValue("ColorPos", 0);
        TCONFIG->setValue("BgColor", "#a0a0a0");
        TCONFIG->setValue("UITheme", LIGHT_THEME);

        TCONFIG->beginGroup("PaintArea");
        TCONFIG->setValue("GridColor", "#0000b4");
        TCONFIG->setValue("GridSeparation", 20);
        TCONFIG->setValue("GridLineThickness", 1);
        TCONFIG->setValue("ROTColor", "#ff0000");
        TCONFIG->setValue("ROTLineThickness", 1);
        TCONFIG->setValue("SafeAreaRectColor", "#008700");
        TCONFIG->setValue("SafeAreaLineColor", "#969696");
        TCONFIG->setValue("SafeLineThickness", 1);

        TCONFIG->beginGroup("AnimationParameters");
        TCONFIG->setValue("AutoPlay", true);
    }

    TCONFIG->beginGroup("General");
    TCONFIG->setValue("AssetsPath", CACHE_DIR + TAlgorithm::randomString(8) + "/");
    requestType = NoRequest;
    lastSave = false;
    pendingCloseAction = NoPendingClose;

    if (!sourceFile.isEmpty()) {
        openProject(sourceFile);
    } else {
        // Recovery is offered only after normal startup initialization completes.
        QTimer::singleShot(0, this, SLOT(checkForRecoveryProject()));
    }
}

void TupMainWindow::checkForRecoveryProject()
{
    if (m_projectManager->isOpen())
        return;

    TCONFIG->beginGroup("General");
    const QString recoveryPath = TCONFIG->value("RecoveryDir", "").toString().trimmed();
    if (recoveryPath.isEmpty())
        return;

    QDir recoveryDir(recoveryPath);
    if (!recoveryDir.exists()) {
        TCONFIG->setValue("RecoveryDir", "");
        TCONFIG->sync();
        return;
    }

    TOptionalDialog dialog(tr("TupiTube preserved a project after a previous save failure.")
                           + "<br/><br/>"
                           + tr("Would you like to recover it now?")
                           + "<br/><br/><b>" + recoveryPath + "</b>",
                           tr("Recover Unsaved Project"), false, false, false, this);
    dialog.setButtonMode(TOptionalDialog::TextButtons);
    dialog.setAcceptText(tr("Recover Project"));
    dialog.setCancelText(tr("Not Now"));
    dialog.setModal(true);
    dialog.move(static_cast<int> ((screenWidth - dialog.sizeHint().width()) / 2),
                static_cast<int> ((screenHeight - dialog.sizeHint().height()) / 2));
    dialog.exec();

    if (dialog.getResult() != TOptionalDialog::Accepted)
        return;

    m_projectManager->setHandler(new TupLocalProjectManagerHandler, false);
    TupFileManager fileManager;
    if (!fileManager.loadRecovery(recoveryPath, m_projectManager->getProject())) {
        QMessageBox::critical(this, tr("Recovery Failed"),
                              tr("The recovery snapshot could not be loaded. TupiTube will keep it available for another recovery attempt."));
        return;
    }

    isNetworked = false;
    activeRecoveryDir = recoveryPath;
    m_filename.clear();
    requestType = OpenLocalProject;
    projectName = m_projectManager->getProject()->getName();
    author = m_projectManager->getProject()->getAuthor();
    if (author.isEmpty())
        author = "Anonymous";

    setWindowTitle(appTitle + " - " + projectName + " " + tr("[ recovered | save required ]"));
    enableToolViews(true);
    setMenuItemsContext(true);
    setUpdatesEnabled(true);

    m_exposureSheet->updateFramesState();
    m_timeLine->updateFramesState();
    m_exposureSheet->updateSceneAudioButtons();
    m_timeLine->updateSceneAudioButtons();
    m_exposureSheet->updateLayerOpacity(0, 0);
    m_exposureSheet->initLayerVisibility();
    m_timeLine->initLayerVisibility();
    m_colorPalette->setBgColor(m_projectManager->getSceneBgColor(0));

    setWorkSpace();
    m_libraryWidget->updateSoundItems();

    // setWorkSpace() clears the modified flag for a normal open. A recovered
    // snapshot must remain dirty until a fresh .tup save succeeds.
    m_projectManager->setModificationStatus(true);

    TOsd::self()->display(TOsd::Warning,
                          tr("Recovered project opened. Please save it to a new .tup file."));
}

void TupMainWindow::showNewsMessage()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::showNewsMessage()]";
    #endif

    TCONFIG->beginGroup("General");
    const QString recoveryPath = TCONFIG->value("RecoveryDir", "").toString().trimmed();
    if (!recoveryPath.isEmpty() && QDir(recoveryPath).exists()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::showNewsMessage()] - Skipping news message while recovery is pending ->"
                     << recoveryPath;
        #endif
        return;
    }

    TImageDialog *msgDialog = new TImageDialog(msgUrl, msgImageName, this);
    msgDialog->show();

    msgDialog->move(static_cast<int> ((screenWidth - msgDialog->width()) / 2),
                    static_cast<int> ((screenHeight - msgDialog->height()) / 2));
}

TupMainWindow::~TupMainWindow()
{
    #ifdef TUP_DEBUG
        qDebug() << "[~TupMainWindow()]";
    #endif

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear(QClipboard::Clipboard);

    delete TupPluginManager::instance();
    delete TOsd::self();
 
    delete m_projectManager;
    delete penView;
}

void TupMainWindow::createNewLocalProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupMainWindow::createNewLocalProject()]";
    #endif

    TupMainWindow::requestType = NewLocalProject;
    m_projectManager->setupNewProject();
    m_projectManager->setOpen(true);
 
    enableToolViews(true);
    setMenuItemsContext(true);

    setWorkSpace();
}

void TupMainWindow::createNewNetProject(const QString &title, const QStringList &users)
{
    isNetworked = true;
    projectName = title;
    setWindowTitle(appTitle + " - " + projectName + " " + tr("[ connected as %1 | collaboration mode ]").arg(netUser));

    if (m_viewChat) {
        removeToolView(m_viewChat);
        delete m_viewChat;
    }

    m_chatTabWidget = netProjectManager->communicationWidget();
    m_viewChat = addToolView(netProjectManager->communicationPanel(), Qt::BottomDockWidgetArea, AnimationView, "Chat");
    m_viewChat->setVisible(false);
    connect(m_viewChat, SIGNAL(visibilityChanged(bool)), this, SLOT(handleChatVisibilityChanged(bool)));
    connect(m_chatTabWidget, SIGNAL(currentChanged(int)), this, SLOT(handleChatTabChanged(int)));
    
    // Initialize collaborators list with the initial users
    netProjectManager->setCollaborators(users);

    enableToolViews(true);
    setMenuItemsContext(true);
    m_exposureSheet->updateFramesState();
    m_exposureSheet->updateSceneAudioButtons();
    m_timeLine->updateSceneAudioButtons();
    m_projectManager->setOpen(true);

    setWorkSpace(users);
}

void TupMainWindow::setupCameraConnections()
{
    connect(cameraWidget, SIGNAL(projectAuthorUpdated(const QString&)), this, SLOT(updateProjectAuthor(const QString&)));
    connect(cameraWidget, SIGNAL(exportRequested()), this, SLOT(exportProject()));
    connect(cameraWidget, SIGNAL(postRequested()), this, SLOT(postProject()));
    connect(cameraWidget, SIGNAL(projectHasChanged(bool)), m_projectManager, SLOT(setModificationStatus(bool)));
    connect(cameraWidget, SIGNAL(fpsUpdated(int)), m_exposureSheet, SLOT(updateFPS(int)));
    connect(cameraWidget, SIGNAL(fpsUpdated(int)), m_timeLine, SLOT(updateFPS(int)));
    connectWidgetToManager(cameraWidget);
}

void TupMainWindow::disconnectCameraConnections()
{
    disconnect(cameraWidget, SIGNAL(projectAuthorUpdated(const QString&)), this, SLOT(updateProjectAuthor(const QString&)));
    disconnect(cameraWidget, SIGNAL(exportRequested()), this, SLOT(exportProject()));
    disconnect(cameraWidget, SIGNAL(postRequested()), this, SLOT(postProject()));
    disconnect(cameraWidget, SIGNAL(projectHasChanged(bool)), m_projectManager, SLOT(setModificationStatus(bool)));
    disconnect(cameraWidget, SIGNAL(fpsUpdated(int)), m_exposureSheet, SLOT(updateFPS(int)));
    disconnect(cameraWidget, SIGNAL(fpsUpdated(int)), m_timeLine, SLOT(updateFPS(int)));
    disconnectWidgetToManager(cameraWidget);
}

void TupMainWindow::setWorkSpace(const QStringList &users)
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupMainWindow::setWorkSpace()]";
    #endif

    // Downloading TupiTube news
    TupNewsCollector *newsCollector = new TupNewsCollector();
    newsCollector->start();
    connect(newsCollector, SIGNAL(pageReady()), this, SLOT(enableUpdatesDialog()));
    connect(newsCollector, SIGNAL(newUpdate(bool)), this, SLOT(setUpdateFlag(bool)));
    connect(newsCollector, SIGNAL(downloadsFinished()), newsCollector, SLOT(deleteLater()));

    if (m_projectManager->isOpen()) {
        if (requestType == NewLocalProject || requestType == NewNetProject)
            TOsd::self()->display(TOsd::Info, tr("Opening a new document..."));

        contextMode = TupProject::FRAMES_MODE;

        // Setting undo/redo actions
        setUndoRedoActions();

        // animationTab = new TupDocumentView(m_projectManager->getProject(), isNetworked, users, this);
        animationTab = new TupDocumentView(m_projectManager->getProject(), m_actionManager, isNetworked, users, this);

        TCONFIG->beginGroup("CollabServer");
        QString server = TCONFIG->value("Server").toString();
        if (isNetworked && server.compare("tupitu.be") == 0) {
            connect(animationTab, SIGNAL(requestExportImageToServer(int, int, const QString &, const QString &, const QString &)),                         
                    netProjectManager, SLOT(sendExportImageRequest(int, int, const QString &, const QString &, const QString &)));
            connect(animationTab, SIGNAL(updateStoryboard(TupStoryboard *, int)), netProjectManager,
                    SLOT(updateStoryboardRequest(TupStoryboard *, int)));
            connect(animationTab, SIGNAL(postStoryboard(int)), netProjectManager, SLOT(postStoryboardRequest(int))); 
        }

        QWidget *animationWidget = new QWidget();
        animationWidget->setWindowTitle(tr("Animation"));
        animationWidget->setWindowIcon(QPixmap(THEME_DIR + "icons/animation_mode.png"));
        QBoxLayout *tabLayout = new QBoxLayout(QBoxLayout::TopToBottom, animationWidget);
        tabLayout->addWidget(animationTab);
        addTabComponent(animationWidget);

        connectWidgetToManager(animationTab);
        connectUndoMacroSignals(animationTab);
        connectWidgetToLocalManager(animationTab);
        connectWidgetToPaintArea(animationTab);
        connect(animationTab, SIGNAL(modeHasChanged(TupProject::Mode)), this, SLOT(restoreFramesMode(TupProject::Mode)));
        connect(animationTab, SIGNAL(projectSizeHasChanged(const QSize)), this, SLOT(resizePlayerCameraDimension(const QSize)));
        connect(animationTab, SIGNAL(newPerspective(UIView)), this, SLOT(changePerspective(UIView)));

        connect(animationTab, SIGNAL(colorChanged(TColorCell::FillType, const QColor &)),
                this, SLOT(updateColor(TColorCell::FillType, const QColor &)));

        connect(animationTab, SIGNAL(contourColorChanged(const QColor &)), m_colorPalette, SLOT(updateContourColor(const QColor &))); 
        connect(animationTab, SIGNAL(fillColorChanged(const QColor &)), m_colorPalette, SLOT(updateFillColor(const QColor &)));
        connect(animationTab, SIGNAL(bgColorChanged(const QColor &)), m_colorPalette, SLOT(updateBgColor(const QColor &)));

        connect(animationTab, SIGNAL(colorModeChanged(TColorCell::FillType)), m_colorPalette,
                SLOT(checkColorButton(TColorCell::FillType)));

        connect(animationTab, SIGNAL(penWidthChanged(int)), this, SLOT(updatePenThickness(int)));
        connect(animationTab, SIGNAL(projectHasChanged()), this, SLOT(requestSaveAction()));
        connect(animationTab, SIGNAL(imagePostRequested(const QString &)), this, SLOT(postFrame(const QString &)));        
        connect(animationTab, SIGNAL(soundRemoved(ModuleSource, const QString &)),
                this, SLOT(releaseSoundRecord(ModuleSource, const QString &)));

        connect(this, SIGNAL(activeDockChanged(TupDocumentView::DockType)), animationTab,
                SLOT(updateActiveDock(TupDocumentView::DockType)));

        connect(m_colorPalette, SIGNAL(eyeDropperActivated(TColorCell::FillType)),
                animationTab, SLOT(enableEyeDropperTool(TColorCell::FillType)));        

        connect(m_libraryWidget, SIGNAL(lipsyncModuleCalled(PapagayoAppMode, const QString&)),
                animationTab, SLOT(launchLipsyncModule(PapagayoAppMode, const QString&)));

        connect(this, SIGNAL(imageExported()), animationTab, SLOT(exportImage()));
        connect(this, SIGNAL(imagePosted()), animationTab, SLOT(postImage()));
        connect(this, SIGNAL(storyboardCalled()), animationTab, SLOT(storyboardSettings()));

        connect(animationTab, SIGNAL(localAssetDropped(const QString &, TupLibraryObject::ObjectType)),
                m_libraryWidget, SLOT(importLocalDroppedAsset(const QString &, TupLibraryObject::ObjectType)));

        connect(animationTab, SIGNAL(libraryAssetImported(const QString &, TupLibraryObject::ObjectType, const QString &)),
                m_libraryWidget, SLOT(importExternalLibraryAsset(const QString &, TupLibraryObject::ObjectType, const QString &)));
        connect(animationTab, SIGNAL(webAssetDropped(const QString &, const QString &, TupLibraryObject::ObjectType, QByteArray)),
                m_libraryWidget, SLOT(importWebDroppedAsset(const QString &, const QString &, TupLibraryObject::ObjectType, QByteArray)));
        connect(animationTab, SIGNAL(libraryAssetDragged()), m_libraryWidget, SLOT(insertObjectInWorkspace()));
        connect(animationTab, SIGNAL(sceneCreated(int)), m_exposureSheet, SLOT(updateSceneFramesState(int)));
        connect(animationTab, SIGNAL(sceneCreated(int)), this, SLOT(updateBgColorInPalette(int)));
        connect(animationTab, SIGNAL(eyeDropperLaunched()), colorView, SLOT(expandDock()));
        // connect(animationTab, SIGNAL(brushSizeChanged(int)), m_brushWidget, SLOT(setPenThickness(int)));

        animationTab->setAntialiasing(true);
        int width = animationTab->workSpaceSize().width();
        int height = animationTab->workSpaceSize().height();
        animationTab->setWorkSpaceSize(width, height);

        TupProject *project = m_projectManager->getProject();
        int pWidth = project->getDimension().width();
        int pHeight = project->getDimension().height();

        double proportion = 1;
        if (pWidth >= pHeight)
            proportion = static_cast<double>(width) / static_cast<double>(pWidth);
        else
            proportion = static_cast<double>(height) / static_cast<double>(pHeight);

        if (proportion <= 0.5) {
            animationTab->setZoomPercent("20");
        } else if (proportion > 0.5 && proportion <= 0.75) {
            animationTab->setZoomPercent("25");
        } else if (proportion > 0.75 && proportion <= 1.7) {
            animationTab->setZoomPercent("50");
        } else if (proportion > 1.7 && proportion < 2) {
            animationTab->setZoomPercent("75");
        } else {
            animationTab->setZoomPercent("85");
        }

        // TupCamera Widget
        cameraWidget = new TupCameraWidget(m_projectManager->getProject());
        setupCameraConnections();
        // Player widget must be hidden while the Player tab is not visible
        cameraWidget->setVisible(false);

        connect(m_libraryWidget, SIGNAL(soundUpdated()), this, SLOT(updateSoundItems()));
        // Sync volume between Library and Player
        connect(m_libraryWidget, SIGNAL(volumeUpdated(int)), cameraWidget, SLOT(setVolume(int)));
        connect(cameraWidget, SIGNAL(volumeChanged(int)), m_libraryWidget, SLOT(setVolume(int)));
        // Enable/disable audio controls based on audio file presence
        connect(m_libraryWidget, SIGNAL(audioControlEnabled(bool)), cameraWidget, SLOT(enableAudioControls(bool)));
        // Set initial audio control state based on project content
        cameraWidget->enableAudioControls(m_projectManager->getProject()->hasLibrarySounds());

        m_libraryWidget->setNetworking(isNetworked);
        // Save event
        connect(animationTab, SIGNAL(saveRequested()), this, SLOT(callSaveProcedure()));

        // Audio scrubbing in Exposure Sheet panel
        connect(m_exposureSheet, SIGNAL(playSoundAt(int, int)), cameraWidget, SLOT(playSoundAtFrame(int, int)));

        // Audio scrubbing in Timeline panel
        connect(m_timeLine, SIGNAL(playSoundAt(int, int)), cameraWidget, SLOT(playSoundAtFrame(int, int)));

        if (isNetworked) {
            connect(cameraWidget, SIGNAL(requestForExportVideoToServer(const QString &, const QString &, const QString &, int, const QList<int>)), 
                    netProjectManager, SLOT(sendVideoRequest(const QString &, const QString &, const QString &, int, const QList<int>)));
        }

        playerTab = new TupAnimationSpace(cameraWidget);
        playerTab->setWindowIcon(QIcon(THEME_DIR + "icons/play_small.png"));
        playerTab->setWindowTitle(tr("Player"));                    
        connect(playerTab, SIGNAL(newPerspective(UIView)), this, SLOT(changePerspective(UIView)));
        addTabComponent(playerTab);

        connect(animationTab, SIGNAL(fpsUpdated(int)), cameraWidget, SLOT(setFpsStatus(int)));
        connect(animationTab, SIGNAL(fpsUpdated(int)), m_exposureSheet, SLOT(updateFPS(int)));
        connect(animationTab, SIGNAL(fpsUpdated(int)), m_timeLine, SLOT(updateFPS(int)));
        connect(animationTab, SIGNAL(pluginsLoaded()), this, SLOT(enableVisibilityControls()));

        // SQA: Implement the Preferences option to choose between the Exposure view and the Timeline view
        exposureView->expandDock(true);
        // timeView->expandDock(true);
        currentDock = TupDocumentView::ExposureSheet;

        m_projectManager->setModificationStatus(false);
        m_colorPalette->init();
        m_colorPalette->setBgColor(project->getCurrentBgColor());

        TCONFIG->beginGroup("BrushParameters");
        int thickness = TCONFIG->value("Thickness", 3).toInt();
        m_brushWidget->init(thickness);

        if (requestType == OpenLocalProject || requestType == OpenNetProject)
            TOsd::self()->display(TOsd::Info, tr("Project <b>%1</b> opened!").arg(m_projectManager->getProject()->getName()));

        m_exposureSheet->setCurrentScene(0);
        m_libraryWidget->initCurrentFrame();
        connect(this, SIGNAL(tabHasChanged(UIView)), this, SLOT(updateCurrentTab(UIView)));

        m_projectManager->clearUndoStack();

        QApplication::restoreOverrideCursor();
    }
}

void TupMainWindow::enableVisibilityControls()
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupMainWindow::enableVisibilityControls()]";
    #endif

    connect(exposureView, SIGNAL(visibilityChanged(bool)), this, SLOT(checkTimeLineVisibility(bool)));
    connect(timeView, SIGNAL(visibilityChanged(bool)), this, SLOT(checkExposureVisibility(bool)));
}

void TupMainWindow::updateSoundItems()
{
    cameraWidget->updateSoundItems();
    m_exposureSheet->updateSceneAudioButtons();
    m_timeLine->updateSceneAudioButtons();
    m_projectManager->setModificationStatus(true);
}

void TupMainWindow::enableUpdatesDialog()
{
    QString mainPath = QDir::homePath() + "/." + QCoreApplication::applicationName();
    QString releasePath = mainPath + "/release.html";
    QString newsPath = mainPath + "/news.html";
    if (QFile::exists(releasePath) && QFile::exists(newsPath)) {
        updatesAction->setEnabled(true);
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::enableUpdatesDialog()] - Warning! Can't find these files:";
            qDebug() << "Release file -> " << releasePath;
            qDebug() << "News file -> " << newsPath;
        #endif
    }
}

void TupMainWindow::newProject()
{
    #ifdef TUP_DEBUG
        qWarning() << "---";
        qWarning() << "[TupMainWindow::newProject()]";
    #endif

    if (m_projectManager->isOpen()) {
        if (playerTab)
            cameraWidget->doStop();
    }

    if (cancelChanges())
        return;

    TupNewProject *wizard = new TupNewProject(this);
    wizard->show();

    wizard->move(static_cast<int> ((screenWidth - wizard->width()) / 2),
                 static_cast<int> ((screenHeight - wizard->height()) / 2));

    wizard->focusProjectLabel();

    if (wizard->exec() != QDialog::Rejected) {
        if (wizard->useNetwork()) {
            TupMainWindow::requestType = NewNetProject;
            setupCollaborativeProject(wizard->parameters());
            netUser = wizard->login();
        } else {
            setupLocalProject(wizard->parameters());
            createNewLocalProject();
        }
    }

    delete wizard;
}

bool TupMainWindow::cancelChanges(PendingCloseAction action)
{
    if (isNetworked) {
        const int pendingCount = netProjectManager
            ? netProjectManager->pendingCommandCount()
            : 0;

        if (pendingCount > 0) {
            if (action != NoPendingClose)
                pendingCloseAction = action;

#ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::cancelChanges()] Collaborative close deferred."
                       << "Pending commands:" << pendingCount
                       << "Action:" << static_cast<int>(action);
#endif

            TOsd::self()->display(
                TOsd::Warning,
                tr("Waiting for %1 collaborative edit(s) to be confirmed by the server...")
                    .arg(pendingCount));
            return true;
        }

        // Collaborative edits that have no pending local commands are already
        // durable in the server-authoritative history. The saved revision is an
        // explicit checkpoint only, so closing must not offer Save/Discard for
        // already committed collaborative work.
        return false;
    }

    if (m_projectManager->projectWasModified()) {
        QMessageBox msgBox;
        msgBox.setStyleSheet(uiStyleSheet);
        msgBox.setWindowTitle(tr("Confirmation Required"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(tr("The document has been modified."));
        msgBox.setInformativeText(tr("Do you want to save the project?"));

        msgBox.addButton(QString(tr("Save")), QMessageBox::AcceptRole);
        msgBox.addButton(QString(tr("Discard")), QMessageBox::NoRole);
        msgBox.addButton(QString(tr("Cancel")), QMessageBox::DestructiveRole);
        msgBox.show();

        msgBox.move(static_cast<int> ((screenWidth - msgBox.width()) / 2),
                    static_cast<int> ((screenHeight - msgBox.height()) / 2));

        int ret = msgBox.exec();
        switch (ret) {
            case QMessageBox::AcceptRole:
                lastSave = false;
                if (!saveProject())
                    return true;
                return false;
            case QMessageBox::DestructiveRole:
                return true;
        }
    }

    return false;
}

void TupMainWindow::closeInterface()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::closeInterface()]";
    #endif

    if (pendingCloseAction != NoPendingClose)
        return;

    if (cancelChanges(CloseProjectAfterSave))
        return;

    closeProject();
}

void TupMainWindow::closeCollaborativeProjectIfOpen()
{
    if (isNetworked) {
        closeProject();
    }
}

bool TupMainWindow::closeProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::closeProject()]";
    #endif

    if (!m_projectManager->isOpen()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::closeProject()] - No project is open!";
        #endif
        // Defensive: always cleanup netProjectManager
        if (netProjectManager) {
            netProjectManager->disconnect();
            netProjectManager->deleteLater();
            netProjectManager = nullptr;
        }
        return true;
    }

    if (!mainToolBar->isVisible())
        hideTopPanels();

    resetUI();

    // Defensive: always cleanup netProjectManager
    if (netProjectManager) {
        netProjectManager->disconnect();
        netProjectManager->deleteLater();
        netProjectManager = nullptr;
    }

    return true;
}

void TupMainWindow::requestSaveAction()
{
    m_projectManager->setModificationStatus(true);
}

void TupMainWindow::resetUI()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::resetUI()]";
    #endif

    disconnect(this, SIGNAL(tabHasChanged(UIView)), this, SLOT(updateCurrentTab(UIView)));
    disconnect(exposureView, SIGNAL(visibilityChanged(bool)), this, SLOT(checkTimeLineVisibility(bool)));
    disconnect(timeView, SIGNAL(visibilityChanged(bool)), this, SLOT(checkExposureVisibility(bool)));

    colorView->expandDock(false);
    penView->expandDock(false);
    libraryView->expandDock(false);
    exposureView->expandDock(false);
    timeView->expandDock(false);

    setUpdatesEnabled(false);
    setMenuItemsContext(false);
    updateOpenRecentMenu(m_recentProjectsMenu, m_recentProjects);

    if (animationTab)
        animationTab->closeInterface();

    removeAllWidgets();

    if (playerTab) {
        playerTab->clearInterface();

        delete playerTab;
        playerTab = nullptr;
    }

    if (animationTab) {
        delete animationTab;
        animationTab = nullptr;
    }

    if (exportWidget) {
        delete exportWidget;
        exportWidget = nullptr;
    }

    m_exposureSheet->closeAllScenes();
    m_timeLine->closeAllScenes();
    m_libraryWidget->resetGUI();

    if (commandCoordinator)
        commandCoordinator->clear();

    m_filename = QString();
    // Detach the in-memory project from any recovery snapshot. The persistent
    // RecoveryDir marker is cleared only after a verified successful .tup save.
    activeRecoveryDir.clear();

    enableToolViews(false);
    setUpdatesEnabled(true);
    setWindowTitle(appTitle);

    if (isNetworked) {
        if (m_viewChat)
            m_viewChat->expandDock(false);
        if (netProjectManager) {
            netProjectManager->closeProject();
            netProjectManager->disconnect();
            netProjectManager->deleteLater();
            netProjectManager = nullptr;
        }
    }

    m_projectManager->closeProject();
    if (!projectName.isEmpty()) {
        QString projectCache(CACHE_DIR + projectName);
        QDir projectPath(projectCache);
        if (projectPath.exists()) {
            if (projectPath.removeRecursively()) {
                #ifdef TUP_DEBUG
                    qDebug() << "---";
                    qDebug() << "[TupMainWindow::resetUI()] - "
                                "*** CACHE project path removed successfully! -> " << projectCache;
                    qDebug() << "---";
                #endif
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupMainWindow::resetUI()] - "
                                "Fatal Error: Can't remove CACHE project path! -> " << projectCache;
                #endif
                TOsd::self()->display(TOsd::Error, tr("Error while clearing cache!"));
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::resetUI()] - Warning: project name is empty!";
        #endif
    }

    resetMousePointer();
}

void TupMainWindow::showCollaborativeConnectionDialog()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::showCollaborativeConnectionDialog()]";
    #endif

    TupConnectDialog *netDialog = new TupConnectDialog(this);
    netDialog->show();

    netDialog->move(static_cast<int> ((screenWidth - netDialog->width()) / 2),
                    static_cast<int> ((screenHeight - netDialog->height()) / 2));

    if (netDialog->exec() == QDialog::Accepted) {
        TupNetProjectManagerParams *params = new TupNetProjectManagerParams();
        params->setLogin(netDialog->login());
        params->setPassword(netDialog->password());
        params->setServer(netDialog->server());
        params->setPort(netDialog->port());
        params->setWindowRecordID(netDialog->windowRecordID());

        delete netDialog;
        setupCollaborativeProject(params);
    } else {
        delete netDialog;
    }
}

void TupMainWindow::setupCollaborativeProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::setupCollaborativeProject()]";
    #endif

    // Check if all connection credentials are already stored
    TCONFIG->beginGroup("CollabServer");
    QString server = TCONFIG->value("Server", "").toString();
    int port = TCONFIG->value("Port", 8080).toInt();
    QString login = TCONFIG->value("Login", "").toString();
    QString password = TCONFIG->value("Password", "").toString();
    bool storePassword = TCONFIG->value("StorePassword", "false").toBool();
    QString windowRecordID = TAlgorithm::windowRecordID();

    // If all credentials are available and password is stored, connect directly
    if (!server.isEmpty() && !login.isEmpty() && !password.isEmpty() && storePassword) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::setupCollaborativeProject()] - Using stored credentials for server ->" << server;
        #endif

        TupNetProjectManagerParams *params = new TupNetProjectManagerParams();
        netUser = login;
        params->setLogin(netUser);
        params->setPassword(password);
        params->setServer(server);
        params->setPort(port);
        params->setWindowRecordID(windowRecordID);

        setupCollaborativeProject(params);
        return;
    }

    // Otherwise, show the dialog to get/confirm credentials
    showCollaborativeConnectionDialog();
}

void TupMainWindow::setupCollaborativeProject(TupProjectManagerParams *params)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::setupCollaborativeProject(TupProjectManagerParams *)]";
    #endif

    if (closeProject()) {
        // Clean up previous netProjectManager if it exists
        if (netProjectManager) {
            // Disconnect all signals from netProjectManager
            netProjectManager->disconnect();
            // If netProjectManager has a socket() method, disconnect its signals too
            QObject *socketObj = nullptr;
            int methodIndex = netProjectManager->metaObject()->indexOfMethod("socket()");
            if (methodIndex != -1) {
                QVariant returnedValue;
                QMetaObject::invokeMethod(netProjectManager, "socket", Q_RETURN_ARG(QVariant, returnedValue));
                socketObj = returnedValue.value<QObject*>();
            } else {
                QVariant socketVar = netProjectManager->property("socket");
                if (socketVar.isValid() && socketVar.canConvert<QObject*>()) {
                    socketObj = socketVar.value<QObject*>();
                }
            }

            if (socketObj)
                socketObj->disconnect();

            delete netProjectManager;
            netProjectManager = nullptr;
        }

        netProjectManager = new TupNetProjectManagerHandler;
        connect(netProjectManager, SIGNAL(authenticationSuccessful()), this, SLOT(requestProject()));
        connect(netProjectManager, SIGNAL(authenticationFailed()), this, SLOT(handleCollaborativeAuthenticationFailure()));
        connect(netProjectManager, SIGNAL(openNewArea(const QString &, const QStringList &)), 
                this, SLOT(createNewNetProject(const QString &, const QStringList &)));
        connect(netProjectManager, SIGNAL(updateUsersList(const QString &, int)),
                this, SLOT(updateUsersOnLine(const QString &, int)));

        connect(netProjectManager, SIGNAL(connectionHasBeenLost(DisconnectReason)),
                this, SLOT(unexpectedClose(DisconnectReason)));
        connect(netProjectManager, SIGNAL(collaborationRecoveryStarted()),
                this, SLOT(collaborationRecoveryStarted()));
        connect(netProjectManager, SIGNAL(recoverySnapshotAboutToLoad()),
                this, SLOT(prepareRecoverySnapshot()));
        connect(netProjectManager, SIGNAL(recoverySnapshotUiReady()),
                this, SLOT(completeRecoverySnapshotUi()));
        connect(netProjectManager, SIGNAL(collaborationRecoveryFinished()),
                this, SLOT(collaborationRecoveryFinished()));
        connect(netProjectManager, SIGNAL(pendingCommandCountChanged(int)),
                this, SLOT(collaborativePendingCommandCountChanged(int)));

        connect(netProjectManager, SIGNAL(savingSuccessful()), this, SLOT(netProjectSaved()));
        connect(netProjectManager, SIGNAL(savingFailed()), this, SLOT(netProjectSaveFailed()));
        connect(netProjectManager, SIGNAL(postOperationDone()), this, SLOT(resetMousePointer()));
        connect(netProjectManager, SIGNAL(newMessageReceived(int)), this, SLOT(notifyChatMessage(int)));
        // Milestone 3: route every terminal server result through the generic
        // command coordinator. It ignores results without registered dependents.
        connect(netProjectManager,
                SIGNAL(commandResultReceived(QString, QString, QString, QString)),
                commandCoordinator,
                SLOT(handleCommandResult(QString, QString, QString, QString)),
                Qt::UniqueConnection);

        m_projectManager->setHandler(netProjectManager, true);
        m_projectManager->setParams(params);
        author = params->getAuthor();
    }
}

void TupMainWindow::setupLocalProject(TupProjectManagerParams *params)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::setupLocalProject()]";
    #endif

    if (closeProject()) {
        isNetworked = false;
        m_projectManager->setHandler(new TupLocalProjectManagerHandler, false);
        m_projectManager->setParams(params);
        projectName = params->getProjectManager();
        author = params->getAuthor();
        setWindowTitle(appTitle +  " - " + projectName + " [ " + tr("by") + " " + author + " ]");
        kAppProp->setProjectDir(projectName);
    }
}

void TupMainWindow::openProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::openProject()]";
    #endif

    if (m_projectManager->isOpen()) {
        if (playerTab)
            cameraWidget->doStop();
    }

    if (cancelChanges())
        return;

    TCONFIG->beginGroup("General");
    QString path = TCONFIG->value("DefaultPath", QDir::homePath()).toString();

    QString package = QFileDialog::getOpenFileName(this, tr("Open TupiTube project"), path,
                      tr("TupiTube Project Package (*.tup)"));

    if (package.isEmpty() || !package.endsWith(".tup")) 
        return;

    openProject(package);
}

void TupMainWindow::openExample()
{
    if (m_projectManager->isOpen()) {
        if (playerTab)
            cameraWidget->doStop();
    }

    if (cancelChanges())
        return;

    if (QFile::exists(examplePath)) {
        if (m_filename.compare(examplePath) != 0)
            openProject(examplePath);
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::openExample()] - "
                          "Fatal Error: Couldn't open example file ->" << examplePath;
        #endif
        TOsd::self()->display(TOsd::Error, tr("Cannot open project!"));
    }
}

void TupMainWindow::openProject(const QString &path)
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupMainWindow::openProject()] - Opening project ->" << path;
    #endif

    if (path.isEmpty() || !path.endsWith(".tup")) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::openProject()] - Fatal Error: Invalid TUP source file path! ->" << path;
        #endif
        return;
    }

    m_projectManager->setHandler(new TupLocalProjectManagerHandler, false);
    isNetworked = false;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_actionManager->enable("open_project", false);
    if (closeProject()) {
        setUpdatesEnabled(false);
        tabWidget()->setCurrentWidget(animationTab);

        if (m_projectManager->loadProject(path)) {
            if (QDir::isRelativePath(path))
                m_filename = QDir::currentPath() + "/" + path;
            else
                m_filename = path;

            requestType = OpenLocalProject;
            projectName = m_projectManager->getProject()->getName();
            updateRecentProjectList();
            updateOpenRecentMenu(m_recentProjectsMenu, m_recentProjects);

            author = m_projectManager->getProject()->getAuthor();
            if (author.length() <= 0)
                author = "Anonymous";
            setWindowTitle(appTitle + " - " + projectName + " [ " + tr("by") + " " + author + " ]");

            enableToolViews(true);
            setMenuItemsContext(true);
            setUpdatesEnabled(true);

            m_exposureSheet->updateFramesState();
            m_timeLine->updateFramesState();
            m_exposureSheet->updateSceneAudioButtons();
            m_timeLine->updateSceneAudioButtons();

            m_exposureSheet->updateLayerOpacity(0, 0);
            m_exposureSheet->initLayerVisibility();
            m_timeLine->initLayerVisibility();
            m_colorPalette->setBgColor(m_projectManager->getSceneBgColor(0));

            int last = path.lastIndexOf("/");
            QString dir = path.left(last);
            saveDefaultPath(dir);

            setWorkSpace();

            // Update sound items AFTER setWorkSpace() so volume signal connections exist
            m_libraryWidget->updateSoundItems();
        } else {
            setUpdatesEnabled(true);
            TOsd::self()->display(TOsd::Error, tr("Cannot open project!"));
        }
    }

    m_actionManager->enable("open_project", true);

    QApplication::restoreOverrideCursor();
}

void TupMainWindow::updateRecentProjectList()
{
    int pos = m_recentProjects.indexOf(m_filename);
    if (pos == -1) {
        m_recentProjects.push_front(m_filename);
        if (m_recentProjects.count() > 5)
            m_recentProjects.removeLast();
    } else {
        m_recentProjects.push_front(m_recentProjects.takeAt(pos));
    }
}

void TupMainWindow::importSourceFile(bool onlyLibrary)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::importSourceFile()]";
    #endif

    TCONFIG->beginGroup("General");
    QString path = TCONFIG->value("DefaultPath", QDir::homePath()).toString();

    QString packagePath = QFileDialog::getOpenFileName(this, tr("Select TupiTube project"), path,
                      tr("TupiTube Project Package (*.tup)"));

    if (packagePath.isEmpty() || !packagePath.endsWith(".tup"))
        return;

    animationTab->importLocalProject(packagePath, onlyLibrary);
}

void TupMainWindow::importLibrary()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::importLibrary()]";
    #endif

    importSourceFile(true);
}

void TupMainWindow::importProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::importProject()]";
    #endif

    importSourceFile();
}

void TupMainWindow::openProjectFromServer()
{
    TupMainWindow::requestType = OpenNetProject;
    setupCollaborativeProject();
}

void TupMainWindow::uploadProjectToServer()
{
    TupMainWindow::requestType = UploadLocalProjectToNet;
    setupCollaborativeProject();
}

void TupMainWindow::preferences()
{
    TupPreferencesDialog *dialog = new TupPreferencesDialog(this);
    dialog->show();

    dialog->move(static_cast<int> ((screenWidth - dialog->width()) / 2),
                 static_cast<int> ((screenHeight - dialog->height()) / 2));

    if (dialog->exec() == QDialog::Accepted) {
        if (animationTab)
            animationTab->updateWorkspace();
    }
}

/*
void TupMainWindow::showHelp()
{
    QDesktopServices::openUrl(QString("https://tupitube.com/wiki"));
}
*/

void TupMainWindow::aboutTupiTube()
{
    TupAbout *about = new TupAbout(this);
    about->show();
}

void TupMainWindow::openYouTubeChannel()
{
    QDesktopServices::openUrl(QString("https://www.youtube.com/tupitube"));
}

void TupMainWindow::importPalettes()
{
    TCONFIG->beginGroup("General");
    QString path = TCONFIG->value("DefaultPath", QDir::homePath()).toString();
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Import Gimp Palettes"),
                                                      path, tr("Gimp Palette (*.gpl *.txt *.css)"));

    if (files.count() > 0) { 
        QStringList::ConstIterator file = files.begin();
        bool isOk = true;
        while (file != files.end()) {
            TupPaletteImporter importer;
            bool ok = importer.import(*file, TupPaletteImporter::Gimp);
            if (ok) {
                QString home = getenv("HOME");
                QString path = home + "/.tupitube/palettes";
                ok = importer.saveFile(path);
                if (ok) {
                    m_colorPalette->parsePaletteFile(importer.getFilePath());
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupMainWindow::importPalettes()] - Fatal Error: Couldn't import file -> " << QString(*file);
                    #endif
                    isOk = false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupMainWindow::importPalettes()] - Fatal Error: Couldn't import palette -> " << QString(*file);
                #endif
                isOk = false;
            }
            file++;
        }

        if (isOk) {
            path = files.at(0);
            int last = path.lastIndexOf("/");
            QString dir = path.left(last);
            saveDefaultPath(dir);

            TOsd::self()->display(TOsd::Info, tr("Gimp palette import was successful"));
        } else {
            TOsd::self()->display(TOsd::Error, tr("Gimp palette import was unsuccessful"));
        }
    }
}

void TupMainWindow::connectWidgetToManager(QWidget *widget)
{
    connect(widget, SIGNAL(requestTriggered(const TupProjectRequest*)), m_projectManager,
            SLOT(handleProjectRequest(const TupProjectRequest*)));

    connect(m_projectManager, SIGNAL(responsed(TupProjectResponse*)), widget, 
            SLOT(handleProjectResponse(TupProjectResponse*)));
}

void TupMainWindow::connectUndoMacroSignals(QWidget *widget)
{
    connect(widget, SIGNAL(beginUndoMacroRequested(const QString&)), m_projectManager,
            SLOT(beginUndoMacro(const QString&)));
    connect(widget, SIGNAL(endUndoMacroRequested()), m_projectManager,
            SLOT(endUndoMacro()));
}

void TupMainWindow::connectWidgetToLocalManager(QWidget *widget)
{
    connect(widget, SIGNAL(localRequestTriggered(const TupProjectRequest*)),
            m_projectManager, SLOT(handleLocalRequest(const TupProjectRequest*)));
}

void TupMainWindow::disconnectWidgetToManager(QWidget *widget)
{
    disconnect(widget, SIGNAL(requestTriggered(const TupProjectRequest*)), m_projectManager,
            SLOT(handleProjectRequest(const TupProjectRequest*)));

    disconnect(m_projectManager, SIGNAL(responsed(TupProjectResponse*)), widget,
            SLOT(handleProjectResponse(TupProjectResponse*)));
}

void TupMainWindow::disconnectUndoMacroSignals(QWidget *widget)
{
    disconnect(widget, SIGNAL(beginUndoMacroRequested(const QString&)), m_projectManager,
            SLOT(beginUndoMacro(const QString&)));
    disconnect(widget, SIGNAL(endUndoMacroRequested()), m_projectManager,
            SLOT(endUndoMacro()));
}

void TupMainWindow::connectWidgetToPaintArea(QWidget *widget)
{
    connect(widget, SIGNAL(paintAreaEventTriggered(const TupPaintAreaEvent*)),
            this, SLOT(createPaintCommand(const TupPaintAreaEvent*)));
}

bool TupMainWindow::saveAs()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::saveAs()]";
    #endif

    if (m_projectManager->isOpen()) {
        if (cameraWidget)
            cameraWidget->doStop();
    }

    TCONFIG->beginGroup("General");
    QString home = TCONFIG->value("DefaultPath", QDir::homePath()).toString();
    home.append("/" + projectName);
    isSaveDialogOpen = true;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Project As"), home,
                       tr("TupiTube Project Package (*.tup)"));
    if (fileName.isEmpty()) {
        isSaveDialogOpen = false;
        return false;
    }

    if (!fileName.endsWith(".tup", Qt::CaseInsensitive))
        fileName += ".tup";

    isSaveDialogOpen = false;
    int indexPath = fileName.lastIndexOf("/");
    int indexFile = fileName.length() - indexPath;
    QString name = fileName.right(indexFile - 1);
    QString path = fileName.left(indexPath + 1);

    QDir directory(path);
    if (!directory.exists()) {
        TOsd::self()->display(TOsd::Error, tr("Directory does not exist! Please, choose another path."));
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::saveAs()] - Fatal Error: Directory doesn't exist! -> " + path.toLocal8Bit();
        #endif
        return false;
    } else {
        QFile file(directory.filePath(name));
        if (!file.open(QIODevice::ReadWrite)) {
            file.remove();
            TOsd::self()->display(TOsd::Error, tr("Insufficient permissions. Please, pick another path"));
            return false;
        }
        file.remove();
    }

    int dotIndex = name.lastIndexOf(".tup");
    projectName = name.left(dotIndex);

    m_filename = fileName;

    if (isNetworked) {
        isNetworked = false;
        m_projectManager->setHandler(new TupLocalProjectManagerHandler, false);
        setWindowTitle(appTitle + " - " + projectName + " [ " + tr("by") + " " + author + " ]");
    }

    return storeProcedure();
}

bool TupMainWindow::saveProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupMainWindow::saveProject()] - file path ->" << m_filename;
    #endif

    if (!isNetworked) {
        if (isSaveDialogOpen)
            return false;

        if (m_filename.isEmpty())
            return saveAs();

        if (m_filename.contains(SHARE_DIR)) {
            TCONFIG->beginGroup("General");
            TCONFIG->setValue("DefaultPath", QDir::homePath());
            return saveAs();
        }

        return storeProcedure();
    } else {
        if (!netProjectManager)
            return false;

        TupSavePackage package(lastSave);
        netProjectManager->sendPackage(package);

        // Collaborative saves complete only after the server acknowledges them.
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        lastSave = false;
    }

    #ifdef TUP_DEBUG
        qDebug() << "---";
    #endif

    return true;
}

bool TupMainWindow::storeProcedure()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::storeProcedure()] - m_filename ->" << m_filename;
    #endif

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (playerTab)
        cameraWidget->doStop();

    m_actionManager->enable("save_project", false);
    m_actionManager->enable("save_project_as", false);

    connect(m_projectManager, SIGNAL(projectPathChanged()),
            this, SLOT(updateSoundsPath()));
    connect(m_projectManager, SIGNAL(soundPathsChanged()),
            m_libraryWidget, SLOT(updateCurrentSoundPath()));

    TCONFIG->beginGroup("General");
    const QString recoveryPathBeforeSave = TCONFIG->value("RecoveryDir").toString();
    TCONFIG->sync();

    if (m_projectManager->saveProject(m_filename)) {
        if (!activeRecoveryDir.isEmpty()) {
            QDir recoveryDir(activeRecoveryDir);
            if (recoveryDir.exists() && !recoveryDir.removeRecursively()) {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupMainWindow::storeProcedure()] - Warning: Can't remove completed recovery snapshot ->"
                               << activeRecoveryDir;
                #endif
            }
            TCONFIG->beginGroup("General");
            TCONFIG->setValue("RecoveryDir", "");
            TCONFIG->sync();
            activeRecoveryDir.clear();
        }

        updateRecentProjectList();

        TOsd::self()->display(TOsd::Info, tr("Project <b>%1</b> saved").arg(projectName));
        int indexPath = m_filename.lastIndexOf("/");
        int indexFile = m_filename.length() - indexPath;
        QString name = m_filename.right(indexFile - 1);
        int indexDot = name.lastIndexOf(".");
        name = name.left(indexDot);

        setWindowTitle(appTitle + " - " + name + " [ " + tr("by") +  " " +  author + " ]");

        int last = m_filename.lastIndexOf("/");
        QString dir = m_filename.left(last);
        saveDefaultPath(dir);

        disconnect(m_projectManager, SIGNAL(projectPathChanged()),
                   this, SLOT(updateSoundsPath()));
        // disconnect(m_projectManager, SIGNAL(soundPathsChanged()),
        //            m_libraryWidget, SLOT(updateSoundPlayer()));

        disconnect(m_projectManager, SIGNAL(soundPathsChanged()),
                   m_libraryWidget, SLOT(updateCurrentSoundPath()));
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "TupMainWindow::saveProject() - Error: Can't save project -> " << m_filename;
        #endif
        disconnect(m_projectManager, SIGNAL(projectPathChanged()),
                   this, SLOT(updateSoundsPath()));
        disconnect(m_projectManager, SIGNAL(soundPathsChanged()),
                   m_libraryWidget, SLOT(updateCurrentSoundPath()));

        m_actionManager->enable("save_project", true);
        m_actionManager->enable("save_project_as", true);
        if (isSaveDialogOpen)
            isSaveDialogOpen = false;
        QApplication::restoreOverrideCursor();

        TCONFIG->beginGroup("General");
        const QString recoveryPathAfterSave = TCONFIG->value("RecoveryDir").toString();
        TCONFIG->sync();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Project Save Failed"));
        const bool recoveryCreated = !recoveryPathAfterSave.isEmpty()
                && recoveryPathAfterSave != recoveryPathBeforeSave
                && QDir(recoveryPathAfterSave).exists();

        if (recoveryCreated) {
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("The normal .tup file could not be created, but TupiTube preserved a recovery copy of your project."));
            msgBox.setInformativeText(tr("Recovery copy:<br/><b>%1</b><br/><br/>"
                                         "TupiTube must restart before you continue editing. "
                                         "On the next start, it will offer to recover this project automatically.")
                                      .arg(recoveryPathAfterSave));
            msgBox.setStandardButtons(QMessageBox::NoButton);
            msgBox.addButton(tr("Exit TupiTube"), QMessageBox::AcceptRole);

            // Package generation failed after the unpacked project had been
            // serialized and a validated recovery snapshot was created. Every
            // way of dismissing this blocking dialog, including its title-bar X,
            // must take the same terminal path. Queue the main-window close from
            // the dialog's finished signal so the close cannot depend on which
            // QMessageBox exit path ended exec().
            connect(&msgBox, &QMessageBox::finished, this, [this](int) {
                recoveryExitPending = true;
                QTimer::singleShot(0, this, SLOT(close()));
            });
            msgBox.exec();
        } else {
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("The project could not be saved."));
            msgBox.setInformativeText(tr("Your project is still open with its unsaved changes. "
                                         "Please check the destination, available disk space, and write permissions, then try again."));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }

        return false;
    }

    m_actionManager->enable("save_project", true);
    m_actionManager->enable("save_project_as", true);
    if (isSaveDialogOpen)
        isSaveDialogOpen = false;
    QApplication::restoreOverrideCursor();

    return true;
}

void TupMainWindow::updateSoundsPath()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::updateSoundsPath()]";
    #endif

    if (cameraWidget)
        cameraWidget->loadSoundRecords();
}

void TupMainWindow::releaseSoundRecord(ModuleSource source, const QString &soundKey)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::releaseSoundRecord()] - soundKey -> " << soundKey;
    #endif

    if (m_libraryWidget) {
        if (source == Library)
            m_libraryWidget->removeSoundItem(soundKey);
        else
            m_libraryWidget->resetSoundPlayer();
    }

    if (cameraWidget)
        cameraWidget->removeSoundTrack(soundKey);
}

void TupMainWindow::releaseAudioResources()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::releaseAudioResources()]";
    #endif

    if (m_libraryWidget)
        m_libraryWidget->resetSoundPlayer();

    if (cameraWidget)
        cameraWidget->releaseAudioResources();
}

void TupMainWindow::openRecentProject()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        QString recentProjectPath = action->text();
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::openRecentProject()] - recent project path ->" << recentProjectPath;
        #endif
        if (recentProjectPath.compare(m_filename) != 0)
            openProject(recentProjectPath);
        else
            TOsd::self()->display(TOsd::Warning, tr("Project is already opened!"));
    }
}

// SQA: Check if this method is still used for something
void TupMainWindow::showAnimationMenu(const QPoint &point)
{
    QMenu *menu = new QMenu(tr("Animation"), playerTab);
    menu->addAction(tr("New camera"), this, SLOT(newViewCamera()));
    menu->exec(point);
    delete menu;
}

#if defined(Q_OS_MAC)
bool TupMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        openProject(openEvent->file());
        return false;
    }

    return QMainWindow::event(event); 
}
#endif

void TupMainWindow::closeEvent(QCloseEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::closeEvent(QCloseEvent)]";
    #endif

    if (!recoveryExitPending && pendingCloseAction != NoPendingClose) {
        event->ignore();
        return;
    }

    if (!recoveryExitPending && cancelChanges(ExitApplicationAfterSave)) {
        event->ignore();
        return;
    } else {
        QString newsPath = QDir::homePath() + "/." + QCoreApplication::applicationName() + "/twitter.html";
        if (QFile::exists(newsPath)) {
            QFile file(newsPath);
            file.remove();
        }

        // Removing assets path
        TCONFIG->beginGroup("General");
        QString assetsPath = TCONFIG->value("AssetsPath", CACHE_DIR + "assets").toString();
        QDir assetsDir(assetsPath);
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::closeEvent()] - Removing assets path ->" << assetsPath;
        #endif
        if (assetsDir.exists()) {
            if (!assetsDir.removeRecursively()) {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupMainWindow::closeEvent()] - Error: Can't remove assets path ->" << assetsPath;
                #endif
            }
        }

        TCONFIG->beginGroup("General");
        TCONFIG->setValue("Recents", m_recentProjects);
        resetUI();

        TMainWindow::closeEvent(event);
    }
}

void TupMainWindow::createPaintCommand(const TupPaintAreaEvent *event)
{
    if (isNetworked && collaborationRecovering) {
#ifdef TUP_DEBUG
        qWarning() << "[TupMainWindow::createPaintCommand()] Ignoring paint mutation while collaborative session is recovering.";
#endif
        return;
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::createPaintCommand()]";
    #endif

    if (!animationTab) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::createPaintCommand()] - No animation tab... aborting!";
        #endif
        return;
    }

    TupPaintAreaCommand *command = animationTab->createPaintCommand(event);
    if (command) { 
        // SQA: Implement Undo procedure for "Color" actions 
        // SQA: Refactor pointer cast
        m_projectManager->createCommand((TupProjectCommand *) command);

        // Updating color on the Pen module interface
        if (event->getAction() == TupPaintAreaEvent::ChangePenColor) {
            QColor color = qvariant_cast<QColor>(event->getData());
            m_brushWidget->setPenColor(color);
            animationTab->updateColorOnSelection(TupProjectRequest::Pen, color);
            return;
        }

        if (event->getAction() == TupPaintAreaEvent::ChangeBrush) {
            QColor color = qvariant_cast<QColor>(event->getData());
            animationTab->updateColorOnSelection(TupProjectRequest::Brush, color);
            return;
        }

        if (event->getAction() == TupPaintAreaEvent::ChangePenThickness) {
            int size = qvariant_cast<int>(event->getData());
            m_brushWidget->setPenThickness(size);
            return;
        }

        if (event->getAction() == TupPaintAreaEvent::ChangePen) {
            QPen pen = qvariant_cast<QPen>(event->getData());
            animationTab->updatePenOnSelection(pen);
            return;
        }

        if (event->getAction() == TupPaintAreaEvent::ChangeBgColor) {
            m_projectManager->setModificationStatus(true);
            QColor color = qvariant_cast<QColor>(event->getData());
            int sceneIndex = animationTab->currentSceneIndex();
            m_projectManager->setSceneBgColor(sceneIndex, color);
        }
    }
}

void TupMainWindow::updateColor(TColorCell::FillType type, const QColor &color)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::updateColor()]";
    #endif

    TupPaintAreaEvent::Action action = TupPaintAreaEvent::ChangePenColor;

    if (type == TColorCell::Inner)
        action = TupPaintAreaEvent::ChangeBrush;

    if (type == TColorCell::Background)
        action = TupPaintAreaEvent::ChangeBgColor;

    createPaintCommand(new TupPaintAreaEvent(action, color));
}

void TupMainWindow::updatePenThickness(int thickness)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::updatePenThickness()] - thickness -> " << thickness;
    #endif

    TupPaintAreaEvent *event = new TupPaintAreaEvent(TupPaintAreaEvent::ChangePenThickness, thickness);
    createPaintCommand(event);
}

void TupMainWindow::updateCurrentTab(UIView tabType)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::updateCurrentTab()] - tabType ->" << tabType;
    #endif

    if (tabType == PlayerView) { // Player mode
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::updateCurrentTab()] - Setting player mode...";
        #endif

        lastTab = PlayerView;
        updatePlayer();
        cameraWidget->setVisible(true);
        cameraWidget->updateFirstFrame();
        cameraWidget->setFocus();

        TCONFIG->beginGroup("AnimationParameters");
        bool autoPlay = TCONFIG->value("AutoPlay", true).toBool();
        if (autoPlay)
            QTimer::singleShot(0, this, SLOT(doPlay()));
    } else {
        if (tabType == AnimationView) { // Animation mode
            if (playerTab)
                cameraWidget->setVisible(false);

            animationTab->updatePerspective(); // Just for Papagayo UI
            if (lastTab == PlayerView)
                cameraWidget->doStop();

            if (contextMode != TupProject::FRAMES_MODE) {
                if (exposureView->isExpanded()) {
                    exposureView->expandDock(false);
                    exposureView->enableButton(false);
                } else if (timeView->isExpanded()) {
                    timeView->expandDock(false);
                    timeView->enableButton(false);
                }
            }

            animationTab->updatePaintArea();
            lastTab = AnimationView;
        }
    }
}

void TupMainWindow::exportProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::exportProject()]";
    #endif

    if (callSaveProcedure()) {
        exportWidget = new TupExportWidget(m_projectManager->getProject(), this);
        connect(exportWidget, SIGNAL(isDone()), animationTab, SLOT(updatePaintArea()));
        exportWidget->show();

        exportWidget->move(static_cast<int> ((screenWidth - exportWidget->width()) / 2),
                           static_cast<int> ((screenHeight - exportWidget->height()) / 2));

        exportWidget->exec();
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::exportProject()] - Warning: callSaveProcedure() couldn't be called!";
        #endif
    }
}

void TupMainWindow::postProject()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::postProject()]";
    #endif

    if (animationTab) {
        int sceneIndex = animationTab->currentSceneIndex();
        int framesCount = m_projectManager->framesCount(sceneIndex);
        if (framesCount < 2) {
            TOsd::self()->display(TOsd::Error, tr("To post video add more frames!"));
            #ifdef TUP_DEBUG
                qWarning() << "[TupMainWindow::postProject()] - Error: Too few frames!";
            #endif
            return;
        }
    }

    if (callSaveProcedure()) {
        QFile file(m_filename);
        double fileSize = static_cast<double>(file.size()) / static_cast<double>(1000000);
        if (fileSize < 10) {
            TCONFIG->beginGroup("Website");
            QString username = TCONFIG->value("Username", "").toString();
            QString password = TCONFIG->value("Password", "").toString();
            bool isAnonymous = TCONFIG->value("Anonymous", false).toBool();
            bool storePasswd = TCONFIG->value("StorePassword", "false").toBool();

            if (username.isEmpty() || password.isEmpty() || !storePasswd) {
                TupSignDialog *dialog = new TupSignDialog(this);
                dialog->show();
                dialog->move(static_cast<int> ((screenWidth - dialog->width()) / 2),
                                static_cast<int> ((screenHeight - dialog->height()) / 2));

                if (dialog->exec() != QDialog::Rejected) {
                    if (dialog->isAnonymous()) {
                        username = "tupitube";
                        password = "tupitube";
                    } else {
                        username = dialog->getUsername();
                        password = dialog->getMetadata();
                    }
                } else {
                    // User cancelled action
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupMainWindow::postProject()] - Action canceled by user!";
                    #endif
                    TOsd::self()->display(TOsd::Info, tr("Post canceled by user!"));
                    return;
                }
            } else {
                if (isAnonymous) {
                    username = "tupitube";
                    password = "tupitube";
                }        
            }

            exportWidget = new TupExportWidget(m_projectManager->getProject(), this, TupExportWidget::Scene, username, password);
            exportWidget->setProjectParams(username, password, m_filename);
            connect(exportWidget, SIGNAL(isDone()), animationTab, SLOT(updatePaintArea()));
            exportWidget->show();

            exportWidget->move(static_cast<int> ((screenWidth - exportWidget->width()) / 2),
                               static_cast<int> ((screenHeight - exportWidget->height()) / 2));

            exportWidget->exec();
        } else {
            TOsd::self()->display(TOsd::Error, tr("Project is larger than 10 MB. Too big!"));
        }
    }
}

void TupMainWindow::postFrame(const QString &imagePath)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::postFrame()] - imagePath ->" << imagePath;
    #endif

    if (callSaveProcedure()) {
        TCONFIG->beginGroup("Website");
        QString username = TCONFIG->value("Username").toString();
        QString password = TCONFIG->value("Password").toString();
        bool storePasswd = (TCONFIG->value("StorePassword", "false").toString() == "true");
        bool anonymous = TCONFIG->value("Anonymous").toBool();

        if (!anonymous) {
            if (username.isEmpty() || password.isEmpty() || !storePasswd) {
                TupSignDialog *dialog = new TupSignDialog(this);
                dialog->show();
                dialog->move(static_cast<int> ((screenWidth - dialog->width()) / 2),
                             static_cast<int> ((screenHeight - dialog->height()) / 2));

                if (dialog->exec() != QDialog::Rejected) {
                    username = dialog->getUsername();
                    password = dialog->getMetadata();
                } else {
                    // User cancelled action
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupMainWindow::postProject()] - Action canceled by user!";
                    #endif
                    TOsd::self()->display(TOsd::Info, tr("Post canceled by user!"));
                    return;
                }
            }
        } else { // Anonymous Post
            username = "tupitube";
            password = "tupitube";
        }

        QString projectCode = TAlgorithm::randomString(8);
        TupFileManager *manager = new TupFileManager;
        bool saveDone = manager->createImageProject(projectCode, imagePath, m_projectManager->getProject());
        if (saveDone) {
            QString fileName = CACHE_DIR + projectCode + ".tup";
            QFile file(fileName);
            double fileSize = static_cast<double>(file.size()) / static_cast<double>(1000000);
            if (fileSize < 10) {
                exportWidget = new TupExportWidget(m_projectManager->getProject(), this, TupExportWidget::Frame);
                exportWidget->setProjectParams(username, password, fileName);
                connect(exportWidget, SIGNAL(isDone()), animationTab, SLOT(updatePaintArea()));
                exportWidget->show();

                exportWidget->move(static_cast<int> ((screenWidth - exportWidget->width()) / 2),
                                   static_cast<int> ((screenHeight - exportWidget->height()) / 2));

                exportWidget->exec();
            } else {
                TOsd::self()->display(TOsd::Error, tr("Error while posting image. File is too big!"));
            }
        }

        QFile imgFile(imagePath);
        if (!imgFile.remove()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupMainWindow::postFrame()] - Error: Can't remove image file ->" << imagePath;
            #endif
        }
    }
}

bool TupMainWindow::callSaveProcedure()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::callSaveProcedure()]";
    #endif

    if (m_filename.compare(examplePath) != 0) {
        if (m_projectManager->projectWasModified()) {
            return saveProject();
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupMainWindow::callSaveProcedure()] - Warning: No changes to save!";
            #endif
        }
    }

    return true;
}

void TupMainWindow::restoreFramesMode(TupProject::Mode mode)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::restoreFramesMode()] " << mode << " - currentDock: " << currentDock;
    #endif

    contextMode = mode;
    if (contextMode == TupProject::FRAMES_MODE) {
        if (currentDock == TupDocumentView::ExposureSheet) {
            exposureView->expandDock(true);
        } else if (currentDock == TupDocumentView::TimeLine) {
            timeView->expandDock(true);
        }
        exposureView->enableButton(true);
        timeView->enableButton(true);
    } else { // BG / FG modes
        if (exposureView->isExpanded()) {
            currentDock = TupDocumentView::ExposureSheet;
            exposureView->expandDock(false);
        } else if (timeView->isExpanded()) {
            currentDock = TupDocumentView::TimeLine;
            timeView->expandDock(false);
        }

        exposureView->enableButton(false);
        timeView->enableButton(false);
    }

    if (m_libraryWidget)
        m_libraryWidget->updateSpaceContext(mode);
}

void TupMainWindow::requestProject()
{
    if (requestType == NewNetProject) {
        m_projectManager->setupNewProject();
    } else if (TupMainWindow::requestType == OpenNetProject) {
        TupListProjectsPackage package;
        netProjectManager->sendPackage(package);
    } else if (TupMainWindow::requestType == UploadLocalProjectToNet) {
        const char *home = getenv("HOME");
        QString file = QFileDialog::getOpenFileName(this, tr("Upload project package"),
                                                    home, tr("TupiTube Project Package (*.tup)"));
        if (file.length() > 0) {
            QFile project(file);
            if (project.exists()) {
                if (project.size() > 0) {
                    TupImportProjectPackage package(file);
                    netProjectManager->sendPackage(package);
                 } else {
                    TOsd::self()->display(TOsd::Error, tr("Can't import project. File is empty!"));
                    netProjectManager->closeProject();
                 }
            } else {
                 TOsd::self()->display(TOsd::Error, tr("Can't save the project. File doesn't exist!"));
                 netProjectManager->closeProject();
            }
        } else {
            netProjectManager->closeProject();
        }
    }
}

void TupMainWindow::collaborationRecoveryStarted()
{
    collaborationRecovering = true;

#ifdef TUP_DEBUG
    qWarning() << "[TupMainWindow::collaborationRecoveryStarted()] Collaborative editing suspended.";
#endif

    if (animationTab)
        animationTab->setEnabled(false);
    if (m_exposureSheet)
        m_exposureSheet->setEnabled(false);
    if (m_timeLine)
        m_timeLine->setEnabled(false);
    if (m_libraryWidget)
        m_libraryWidget->setEnabled(false);
    if (m_brushWidget)
        m_brushWidget->setEnabled(false);
    if (m_colorPalette)
        m_colorPalette->setEnabled(false);

    m_actionManager->enable("save_project", false);
    m_actionManager->enable("save_project_as", false);
    m_actionManager->enable("import_project", false);
    m_actionManager->enable("importImageGroup", false);
    m_actionManager->enable("importImageSequence", false);
    m_actionManager->enable("importSvg", false);
    m_actionManager->enable("importSvgSequence", false);
    m_actionManager->enable("importAudioFile", false);
    m_actionManager->enable("importVideoFile", false);

    setWindowTitle(appTitle + " - " + projectName + " " + tr("[ reconnecting | collaboration mode ]"));
    TOsd::self()->display(TOsd::Warning, tr("Connection lost. Reconnecting... Editing is temporarily disabled."));

    if (!collaborationRecoveryDialog) {
        collaborationRecoveryDialog = new QMessageBox(this);
        collaborationRecoveryDialog->setAttribute(Qt::WA_DeleteOnClose, false);
        collaborationRecoveryDialog->setWindowTitle(tr("Connection Lost"));
        collaborationRecoveryDialog->setIcon(QMessageBox::Warning);
        collaborationRecoveryDialog->setText(tr("Connection to the server lost. Waiting for reconnection..."));
        collaborationRecoveryDialog->setInformativeText(
                    tr("TupiTube will keep trying to reconnect automatically. "
                       "Your collaborative project will remain open while editing is suspended."));
        collaborationRecoveryDialog->setWindowModality(Qt::ApplicationModal);
        collaborationRecoveryDialog->setWindowFlag(Qt::WindowCloseButtonHint, false);

        QPushButton *exitButton = collaborationRecoveryDialog->addButton(
                    tr("Exit TupiTube"), QMessageBox::DestructiveRole);
        collaborationRecoveryDialog->setEscapeButton(nullptr);

        connect(exitButton, &QPushButton::clicked, this, []() {
            QApplication::quit();
        });
    }

    collaborationRecoveryDialog->show();
    collaborationRecoveryDialog->raise();
    collaborationRecoveryDialog->activateWindow();
}

void TupMainWindow::prepareRecoverySnapshot()
{


    // The loader emits project responses while rebuilding the snapshot.
    // Exposure Sheet/Timeline need those responses, but the paint area and
    // camera own state tied to the old scene objects and must not process a
    // partially rebuilt model. Suspend only those two consumers.
    if (animationTab) {
        disconnectWidgetToManager(animationTab);
        animationTab->prepareRecoverySnapshot();
    }

    if (cameraWidget) {
        disconnectWidgetToManager(cameraWidget);
        cameraWidget->prepareRecoverySnapshot();
    }

    recoverySnapshotConsumersSuspended = true;

    if (m_exposureSheet)
        m_exposureSheet->closeAllScenes();
    if (m_timeLine)
        m_timeLine->closeAllScenes();

    if (m_projectManager)
        m_projectManager->clearUndoStack();
}

void TupMainWindow::completeRecoverySnapshotUi()
{


    if (!recoverySnapshotConsumersSuspended)
        return;

    // Rebind only after TupFileManager::load() has reconstructed the complete
    // project. This prevents stale pointers and partial scene/layer state.
    //
    // Camera rendering uses TupGraphicObject::item() in its own QGraphicsScene.
    // A QGraphicsItem can belong to only one scene, so camera recovery must run
    // before the editor's final recovery render. The animation view is restored
    // last so it deterministically owns the live editable items when recovery
    // finishes.
    if (cameraWidget) {
        cameraWidget->completeRecoverySnapshot();
        connectWidgetToManager(cameraWidget);
    }

    if (animationTab) {
        animationTab->completeRecoverySnapshot();
        connectWidgetToManager(animationTab);
    }

    recoverySnapshotConsumersSuspended = false;
}

void TupMainWindow::collaborationRecoveryFinished()
{
    collaborationRecovering = false;

    if (collaborationRecoveryDialog) {
        collaborationRecoveryDialog->hide();
        collaborationRecoveryDialog->deleteLater();
        collaborationRecoveryDialog = nullptr;
    }

#ifdef TUP_DEBUG
    qDebug() << "[TupMainWindow::collaborationRecoveryFinished()] Collaborative editing resumed.";
#endif

    if (animationTab)
        animationTab->setEnabled(true);
    if (m_exposureSheet)
        m_exposureSheet->setEnabled(true);
    if (m_timeLine)
        m_timeLine->setEnabled(true);
    if (m_libraryWidget)
        m_libraryWidget->setEnabled(true);
    if (m_brushWidget)
        m_brushWidget->setEnabled(true);
    if (m_colorPalette)
        m_colorPalette->setEnabled(true);

    setMenuItemsContext(true);
    setWindowTitle(appTitle + " - " + projectName + " " + tr("[ connected as %1 | collaboration mode ]").arg(netUser));
    TOsd::self()->display(TOsd::Info, tr("Connection restored. Collaborative editing resumed."));
}

void TupMainWindow::unexpectedClose(DisconnectReason reason)
{
    collaborationRecovering = false;

    if (collaborationRecoveryDialog) {
        collaborationRecoveryDialog->hide();
        collaborationRecoveryDialog->deleteLater();
        collaborationRecoveryDialog = nullptr;
    }

    if (m_projectManager->isOpen()) {
        resetUI();
    }

    QMessageBox msgBox;

    // Dynamically set the UI based on the disconnection reason
    if (reason == DisconnectReason::UserInactivity) {
        msgBox.setWindowTitle(tr("Disconnected"));
        msgBox.setIcon(QMessageBox::Warning); // Less alarming than Critical
        msgBox.setText(tr("You have been disconnected due to inactivity."));
        msgBox.setInformativeText(tr("Please save your work locally and reconnect when you are ready."));
    } else {
        // Default to Network Error / Unknown
        msgBox.setWindowTitle(tr("Connection Lost"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("The connection to the server has been lost due to a network issue."));
        msgBox.setInformativeText(tr("Please check your internet connection and try to connect again."));
    }

    msgBox.addButton(QString(tr("Close")), QMessageBox::DestructiveRole);

    // Keep your custom centering logic
    msgBox.show();
    msgBox.move(static_cast<int>((screenWidth - msgBox.width()) / 2),
                static_cast<int>((screenHeight - msgBox.height()) / 2));

    msgBox.exec();
}

void TupMainWindow::handleCollaborativeAuthenticationFailure()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::handleCollaborativeAuthenticationFailure()]";
    #endif

    TOsd::self()->display(TOsd::Error, tr("Authentication failed! Please, try again."));

    // Show the connection dialog again so user can fix credentials
    showCollaborativeConnectionDialog();
}

void TupMainWindow::netProjectSaved()
{
    // The project_saved metadata from the server is the authority for
    // saved_revision and therefore for collaborative modified state.
    // A generic save notification must not clear pending-command state.
    QApplication::restoreOverrideCursor();
}

void TupMainWindow::netProjectSaveFailed()
{
    // Explicit collaborative Save is only a checkpoint request. Failure does
    // not affect command durability and must not alter deferred close state.
    lastSave = false;
    QApplication::restoreOverrideCursor();
}

void TupMainWindow::collaborativePendingCommandCountChanged(int pendingCount)
{
    if (!isNetworked || pendingCount > 0 || pendingCloseAction == NoPendingClose)
        return;

    const PendingCloseAction action = pendingCloseAction;
    pendingCloseAction = NoPendingClose;

#ifdef TUP_DEBUG
    qDebug() << "[TupMainWindow::collaborativePendingCommandCountChanged()]"
             << "Collaborative commands reconciled. Completing deferred close."
             << "Action:" << static_cast<int>(action);
#endif

    // Finish the close on the next event-loop turn. This avoids tearing down
    // the network handler re-entrantly while it is still completing the
    // command_result or recovery event that reduced the pending count to zero.
    if (action == CloseProjectAfterSave) {
        QTimer::singleShot(0, this, SLOT(closeProject()));
    } else if (action == ExitApplicationAfterSave) {
        QTimer::singleShot(0, this, SLOT(close()));
    }
}

void TupMainWindow::notifyChatMessage(int messageType)
{
    if (m_viewChat && !m_viewChat->isVisible()) {
        m_viewChat->startBlinking();
    }
    
    // If it's a chat message (0) and panel is visible but Chat tab is not selected
    if (messageType == 0 && m_viewChat && m_viewChat->isVisible()) {
        if (m_chatTabWidget && m_chatTabWidget->currentIndex() != 0) {
            // Highlight the Chat tab with blue background
            m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:first { background-color: #308cc6; color: white; }");
            m_chatTabHighlighted = true;
        }
    }
    
    // If it's a notice message (1) and panel is visible but Notices tab is not selected
    if (messageType == 1 && m_viewChat && m_viewChat->isVisible()) {
        if (m_chatTabWidget && m_chatTabWidget->currentIndex() != 1) {
            // Highlight the Notices tab with blue background
            m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:last { background-color: #308cc6; color: white; }");
            m_noticesTabHighlighted = true;
        }
    }
    
    // If panel is not visible, mark for highlighting when opened
    if (m_viewChat && !m_viewChat->isVisible()) {
        if (messageType == 0)
            m_chatTabHighlighted = true;
        else if (messageType == 1)
            m_noticesTabHighlighted = true;
    }
}

void TupMainWindow::handleChatVisibilityChanged(bool visible)
{
    if (visible && m_viewChat && m_viewChat->isBlinking()) {
        m_viewChat->stopBlinking();
    }
    
    // When panel becomes visible, highlight tabs if there are unread messages
    if (visible && m_chatTabWidget) {
        int currentIndex = m_chatTabWidget->currentIndex();
        
        // Handle Chat tab highlighting
        if (m_chatTabHighlighted) {
            if (currentIndex != 0) {
                m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:first { background-color: #308cc6; color: white; }");
            } else {
                m_chatTabHighlighted = false;
            }
        }
        
        // Handle Notices tab highlighting
        if (m_noticesTabHighlighted) {
            if (currentIndex != 1) {
                m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:last { background-color: #308cc6; color: white; }");
            } else {
                m_noticesTabHighlighted = false;
            }
        }
        
        // Handle both tabs highlighted
        if (m_chatTabHighlighted && m_noticesTabHighlighted) {
            m_chatTabWidget->tabBar()->setStyleSheet(
                "QTabBar::tab:first { background-color: #308cc6; color: white; }"
                "QTabBar::tab:last { background-color: #308cc6; color: white; }");
        }
    }
}

void TupMainWindow::handleChatTabChanged(int index)
{
    if (!m_chatTabWidget)
        return;
        
    // When user clicks on Chat tab (index 0), clear its highlighting
    if (index == 0 && m_chatTabHighlighted) {
        m_chatTabHighlighted = false;
        if (m_noticesTabHighlighted) {
            m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:last { background-color: #308cc6; color: white; }");
        } else {
            m_chatTabWidget->tabBar()->setStyleSheet("");
        }
    }
    
    // When user clicks on Notices tab (index 1), clear its highlighting
    if (index == 1 && m_noticesTabHighlighted) {
        m_noticesTabHighlighted = false;
        if (m_chatTabHighlighted) {
            m_chatTabWidget->tabBar()->setStyleSheet("QTabBar::tab:first { background-color: #308cc6; color: white; }");
        } else {
            m_chatTabWidget->tabBar()->setStyleSheet("");
        }
    }
}

void TupMainWindow::updatePlayer(bool removeAction)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::updatePlayer()] - removeAction ->" << removeAction;
    #endif

    if (!removeAction)
        updatePlayer();
}

void TupMainWindow::updatePlayer()
{
    if (animationTab) {
        int sceneIndex = animationTab->currentSceneIndex();
        #ifdef TUP_DEBUG
            qDebug() << "[TupMainWindow::updatePlayer()] - sceneIndex ->" << sceneIndex;
        #endif

        cameraWidget->updateScenes(sceneIndex);
    }
}

void TupMainWindow::resetMousePointer()
{
    QApplication::restoreOverrideCursor();
}

void TupMainWindow::updateUsersOnLine(const QString &login, int state)
{
    animationTab->updateUsersOnLine(login, state);
    
    // Update collaborators list in the chat panel
    if (netProjectManager)
        netProjectManager->updateCollaboratorStatus(login, state);
}

void TupMainWindow::resizePlayerCameraDimension(const QSize dimension)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::resizePlayerCameraDimension()] - dimension ->" << dimension;
    #endif

    m_projectManager->updateProjectDimension(dimension);
    disconnectCameraConnections();

    delete cameraWidget;

    playerTab->setCameraWidget(m_projectManager->getProject());
    cameraWidget = playerTab->getCameraWidget();
    setupCameraConnections();
}

void TupMainWindow::resizeCanvasDimension(const QSize size)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::resizeCanvasDimension()] - size ->" << size;
    #endif

    animationTab->resizeProjectDimension(size);
    resizePlayerCameraDimension(size);
}

void TupMainWindow::saveDefaultPath(const QString &dir)
{
    TCONFIG->beginGroup("General");
    TCONFIG->setValue("DefaultPath", dir);
    TCONFIG->sync();
}

void TupMainWindow::setUpdateFlag(bool update)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::setUpdateFlag()] - update ->" << update;
    #endif

    TCONFIG->beginGroup("General");
    TCONFIG->setValue("NotifyUpdate", update);
    TCONFIG->sync();
}

void TupMainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::dragEnterEvent()]";
    #endif

    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::dragEnterEvent()] - Warning: Mime data has no URL!";
        #endif
    }
}

void TupMainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> list = e->mimeData()->urls();
    QString assetPath = list.at(0).toLocalFile();

    #ifdef TUP_DEBUG
        qDebug() << "[TupMainWindow::dropEvent()] - Assets list size ->" << list.size();
        qDebug() << "[TupMainWindow::dropEvent()] - Object dropped ->" << assetPath;
    #endif

    if (!assetPath.isEmpty()) {
        QString lowercase = assetPath.toLower();
        if (lowercase.endsWith(".tup")) {
            openProject(assetPath);
        } else {                        
            if (!m_projectManager->isOpen()) {
                // Creating a new project first!
                newProject();
                if (lowercase.startsWith("http")) // Web assets
                    animationTab->getWebAsset(assetPath);
                else // Local assets
                    animationTab->getLocalAsset(assetPath);
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupMainWindow::dropEvent()] - Fatal Error: Object filename is empty!";
        #endif
    }
}

void TupMainWindow::updateProjectAuthor(const QString &artist)
{
    author = artist;
    setWindowTitle(appTitle +  " - " + projectName + " [ " + tr("by") + " " + author + " ]");
}
