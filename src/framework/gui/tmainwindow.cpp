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

#include "tmainwindow.h"
#include "tresponsiveui.h"

class T_GUI_EXPORT DefaultSettings : public TMainWindowAbstractSettings
{
    public:
        DefaultSettings(QObject *parent);
        ~DefaultSettings();

        void save(const QString &winKey, TMainWindow *window);
        void restore(const QString &winKey, TMainWindow *window);
};

DefaultSettings::DefaultSettings(QObject *parent) : TMainWindowAbstractSettings(parent)
{
}

DefaultSettings::~DefaultSettings()
{
}

void DefaultSettings::save(const QString &winKey, TMainWindow *window)
{
    #ifdef TUP_DEBUG
        qWarning() << "[TMainWindow::DefaultSettings::save()] - Saving UI settings ->"
                   << qApp->applicationName();
    #endif

    QSettings settings(qApp->applicationName(), winKey, this);

    QHash<Qt::ToolBarArea, TButtonBar *> buttonBars = window->buttonBars();
    QHash<TButtonBar *, QList<ToolView*> > toolViews = window->toolViews();

    foreach (TButtonBar *bar, buttonBars.values()) {
        foreach (ToolView *view, toolViews[bar]) {
            settings.beginGroup(view->objectName());

            settings.setValue("area", int(view->button()->area()));
            settings.setValue("style", view->button()->toolButtonStyle());
            settings.setValue("visible", view->isVisible());
            settings.setValue("floating", view->isFloating());
            settings.setValue("position", view->pos());

            settings.endGroup();
        }
    }
}

void DefaultSettings::restore(const QString &winKey, TMainWindow *window)
{
    #ifdef TUP_DEBUG
        qWarning() << "[TMainWindow::DefaultSettings::restore()] - Restoring UI settings ->"
                   << qApp->applicationName();
    #endif

    QSettings settings(qApp->applicationName(), winKey, this);

    QHash<Qt::ToolBarArea, TButtonBar*> buttonBars = window->buttonBars();
    QHash<TButtonBar*, QList<ToolView*>> toolViews = window->toolViews();

    QList<ToolView*> toHide;

    foreach (TButtonBar *bar, buttonBars.values()) {
        foreach (ToolView *view, toolViews[bar]) {
            settings.beginGroup(view->objectName());
            view->button()->setToolButtonStyle(Qt::ToolButtonStyle(settings.value("style", 
                                               int(view->button()->toolButtonStyle())).toInt()));

            bool visible = settings.value("visible", false).toBool();
            if (visible && view->button()->isVisible()) {
                view->button()->setChecked(true);
                view->show();
            } else {
                toHide << view;
            }

            settings.endGroup();
        }
    }
	
    foreach (ToolView *view, toHide) {
        view->button()->setChecked(false);
        view->setVisible(false);
        view->close();
    }

    window->showMaximized();
}

TMainWindow::TMainWindow(const QString &key, UIView defaultPerspective, QWidget *parent): QMainWindow(parent)
{
    windowKey = key;
    perspective = defaultPerspective;
    viewForRelayout = nullptr;
    autoRestore = false;

    setObjectName(key);
    settings = new DefaultSettings(this);    

    specialToolBar = new QToolBar(tr("Show Top Panel"), this);
    int iconSize = TResponsiveUI::fitToolViewIconSize();
    specialToolBar->setIconSize(QSize(iconSize, iconSize/2));
    specialToolBar->setMovable(false);

    addToolBar(Qt::LeftToolBarArea, specialToolBar);
    addButtonBar(Qt::LeftToolBarArea);
    addButtonBar(Qt::RightToolBarArea);
    addButtonBar(Qt::TopToolBarArea);
    addButtonBar(Qt::BottomToolBarArea);

    setDockNestingEnabled(false);
}

TMainWindow::~TMainWindow()
{
}

void TMainWindow::addButtonBar(Qt::ToolBarArea area)
{
    TButtonBar *bar = new TButtonBar(area, this);
    addToolBar(area, bar);
    buttonBarsHash.insert(area, bar);
}

void TMainWindow::enableSpecialBar(bool flag)
{
    specialToolBar->setVisible(flag);
}

void TMainWindow::addSpecialButton(TAction *action)
{
    specialToolBar->addAction(action);
}

ToolView *TMainWindow::addToolView(QWidget *widget, Qt::DockWidgetArea area,
                                   UIView perspective, const QString &code, QKeySequence shortcut)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::addToolView()] - area ->" << area;
        qDebug() << "[TMainWindow::addToolView()] - perspective ->" << perspective;
        qDebug() << "[TMainWindow::addToolView()] - code ->" << code;
    #endif
    */

    ToolView *toolView = new ToolView(widget->windowTitle(), widget->windowIcon(), code);
    toolView->setShortcut(shortcut);
    toolView->setWidget(widget);
    toolView->setPerspective(perspective);
    toolView->button()->setArea(toToolBarArea(area));

    buttonBarsHash[toToolBarArea(area)]->addButton(toolView->button());
    toolViewsHash[buttonBarsHash[toToolBarArea(area)]] << toolView;

    addDockWidget(area, toolView);
    // SQA: This line is a hack to avoid self-resizing docks issue
    // resizeDocks({toolView}, {200}, Qt::Horizontal);

    return toolView;
}

void TMainWindow::removeToolView(ToolView *view)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::removeToolView()]";
    #endif
    */

    bool findIt = false;

    foreach (TButtonBar *bar, buttonBarsHash.values()) {
        QList<ToolView *> &views = toolViewsHash[bar];
        QList<ToolView *>::iterator it = views.begin();

        while (it != views.end()) {
            ToolView *toolView = *it;
            if (toolView == view) {
                it = views.erase(it);
                bar->removeButton(view->button());
                findIt = true;
                break;
            }
            ++it;
        }

        if (findIt) 
            break;
    }

    if (findIt)
        removeDockWidget(view);
}

void TMainWindow::enableToolViews(bool isEnabled)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::enableToolViews()] - enable ->" << isEnabled;
    #endif
    */

    foreach (TButtonBar *bar, buttonBarsHash.values()) {
        QList<ToolView *> views = toolViewsHash[bar];
        QList<ToolView *>::iterator it = views.begin();

        while (it != views.end()) {
            ToolView *view = *it;
            view->enableButton(isEnabled);
            ++it;
        }
    }
}

void TMainWindow::addToPerspective(QWidget *widget, UIView workSpace)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::addToPerspective()] - workSpace ->" << workSpace;
        qDebug() << "[TMainWindow::addToPerspective()] - perspective ->" << perspective;
    #endif
    */

    if (QToolBar *bar = dynamic_cast<QToolBar*>(widget)) {
        if (toolBarArea(bar) == 0)
            addToolBar(bar);
    }

    if (!managedWidgetsHash.contains(widget)) {
        managedWidgetsHash.insert(widget, workSpace);

        if (workSpace != perspective) {
            #ifdef TUP_DEBUG
                qDebug() << "[TMainWindow::addToPerspective()] - "
                            "Warning: Widget doesn't belong to current perspective ->"
                         << workSpace;
            #endif
            widget->hide();
        }
    }
}

void TMainWindow::removeFromPerspective(QWidget *widget)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::removeFromPerspective()]";
    #endif
    */

    managedWidgetsHash.remove(widget);
}

// Add action list to perspective
void TMainWindow::addToPerspective(const QList<QAction *> &actions, UIView workSpace)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::addToPerspective()]";
    #endif
    */

    foreach (QAction *action, actions)
        addToPerspective(action, workSpace);
}

// Add action to perspective
void TMainWindow::addToPerspective(QAction *action, UIView workSpace)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::addToFromPerspective()]";
    #endif
    */

    if (!managedActionsHash.contains(action)) {
        managedActionsHash.insert(action, workSpace);

        if (workSpace != perspective)
            action->setVisible(false);
    }
}

// Remove action from perspective
void TMainWindow::removeFromPerspective(QAction *action)
{
    managedActionsHash.remove(action);
}

Qt::DockWidgetArea TMainWindow::toDockWidgetArea(Qt::ToolBarArea area)
{
    switch (area) {
        case Qt::LeftToolBarArea:
           {
             return Qt::LeftDockWidgetArea;
           }
        case Qt::RightToolBarArea:
           {
             return Qt::RightDockWidgetArea;
           }
        case Qt::TopToolBarArea:
           {
             return Qt::TopDockWidgetArea;
           }
        case Qt::BottomToolBarArea:
           {
             return Qt::BottomDockWidgetArea;
           }
        default: 
           {
              #ifdef TUP_DEBUG
                  qWarning() << "[TMainWindow::toDockWidgetArea()] - Floating ->" << area;
              #endif
           }
           break;
    }

    return Qt::LeftDockWidgetArea;
}

Qt::ToolBarArea TMainWindow::toToolBarArea(Qt::DockWidgetArea area)
{
    switch (area) {
        case Qt::LeftDockWidgetArea:
           {
             return Qt::LeftToolBarArea;
           }
        case Qt::RightDockWidgetArea:
           {
             return Qt::RightToolBarArea;
           }
        case Qt::TopDockWidgetArea:
           {
             return Qt::TopToolBarArea;
           }
        case Qt::BottomDockWidgetArea:
           {
             return Qt::BottomToolBarArea;
           }
        default: 
           {
             #ifdef TUP_DEBUG
                 qWarning() << "TMainWindow::toToolBarArea() - area ->" << area;
             #endif
           }
    }

    return Qt::LeftToolBarArea;
}

void TMainWindow::setCurrentPerspective(UIView workSpace)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::setCurrentPerspective()]";
    #endif
    */

    if (perspective == workSpace) {
        #ifdef TUP_DEBUG
                 qDebug() << "[TMainWindow::setCurrentPerspective()] - Warning: Perspective is already loaded ->"
                          << perspective;
        #endif
        return;
    }

    if (workSpace != AnimationView)
        specialToolBar->setVisible(false);
    else
        specialToolBar->setVisible(true);

    typedef QList<ToolView *> Views;
    QList<Views > viewsList = toolViewsHash.values();

    QHash<TButtonBar *, int> hideButtonCount;
    foreach (Views views, viewsList) {
        foreach (ToolView *view, views) {
            TButtonBar *bar = buttonBarsHash[view->button()->area()];

            if (view->perspective() == workSpace) { 
                bar->enable(view->button());
                if (view->isExpanded()) {
                    view->blockSignals(true);
                    view->show(); 
                    view->blockSignals(false);
                }
            } else {
                bar->disable(view->button());
                if (view->isExpanded()) {
                    view->blockSignals(true);
                    view->close();
                    view->blockSignals(false);
                }
                hideButtonCount[bar]++;
            }

            if (bar->isEmpty() && bar->isVisible()) {
                bar->hide();
            } else {
                if (!bar->isVisible())
                    bar->show();
            }
        }
    }

    QHashIterator<TButtonBar *, int> barIt(hideButtonCount);
    // This loop hides the bars with no buttons
    while (barIt.hasNext()) {
        barIt.next();
        if (barIt.key()->count() == barIt.value())
            barIt.key()->hide();
    }

    perspective = workSpace;
    emit perspectiveChanged(perspective);
}

UIView TMainWindow::currentPerspective()
{
    return perspective;
}

// if autoRestore is true, the widgets will be loaded when main window is showed (position and properties)
void TMainWindow::setAutoRestoreProperty(bool autoRestoreUI)
{
    autoRestore = autoRestoreUI;
}

bool TMainWindow::autoRestoreProperty() const
{
    return autoRestore;
}

void TMainWindow::setSettingsHandler(TMainWindowAbstractSettings *config)
{
    delete settings;

    settings = config;
    settings->setParent(this);
}

void TMainWindow::closeEvent(QCloseEvent *e)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::closeEvent()]";
    #endif
    */

    QMainWindow::closeEvent(e);
}

void TMainWindow::showEvent(QShowEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::showEvent()] - autoRestore ->" << autoRestore;
    #endif

    if (!autoRestore) {
        autoRestore = true;
        restoreGUI();
        UIView workspace = perspective;
        if (workspace != UndefinedView) {
            perspective = UndefinedView;
            setCurrentPerspective(workspace);
        }
    }

    QMainWindow::showEvent(event);
}

void TMainWindow::saveGUI()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::saveGUI()]";
    #endif

    settings->save(windowKey, this);
}

void TMainWindow::restoreGUI()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TMainWindow::restoreGUI()]";
    #endif

    setUpdatesEnabled(false);
    settings->restore(windowKey, this);
    setUpdatesEnabled(true);
}

QHash<Qt::ToolBarArea, TButtonBar *> TMainWindow::buttonBars() const
{
    return buttonBarsHash;
}

QHash<TButtonBar *, QList<ToolView*> > TMainWindow::toolViews() const
{
    return toolViewsHash;
}
