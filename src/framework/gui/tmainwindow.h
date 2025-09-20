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

#ifndef TMAINWINDOW_H
#define TMAINWINDOW_H

#include "tglobal.h"
#include "tbuttonbar.h"
#include "toolview.h"
#include "tviewbutton.h"
#include "tmainwindowabstractsettings.h"
#include "taction.h"

#include <QMainWindow>
#include <QApplication>
#include <QMap>
#include <QKeySequence>
#include <QSettings>
#include <QCloseEvent>

class TButtonBar;
class ToolView;
class TMainWindowAbstractSettings;

class T_GUI_EXPORT TMainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        TMainWindow(const QString &key, UIView defaultPerspective, QWidget *parent = nullptr);
        ~TMainWindow();

        ToolView *addToolView(QWidget *widget, Qt::DockWidgetArea area, UIView perspective = AnimationView,
                              const QString &code = QString(), QKeySequence shortcut = QKeySequence(""));

        void removeToolView(ToolView *view);

        void addToPerspective(QWidget *widget, UIView perspective = AnimationView);
        void removeFromPerspective(QWidget *widget);
        void setCurrentPerspective(UIView windowPerspective);
        UIView currentPerspective();

        void addToPerspective(QAction *action, UIView perspective);
        void addToPerspective(const QList<QAction *> &actions, UIView perspective);
        void removeFromPerspective(QAction *action);

        void enableToolViews(bool flag);

        void setAutoRestoreProperty(bool autoRestore);
        bool autoRestoreProperty() const;

        void setSettingsHandler(TMainWindowAbstractSettings *config);
        void restoreGUI();
        void saveGUI();

        QHash<Qt::ToolBarArea, TButtonBar *> buttonBars() const;
        QHash<TButtonBar *, QList<ToolView*> > toolViews() const;

        void enableSpecialBar(bool flag);
        void addSpecialButton(TAction *action);

    private:
        Qt::DockWidgetArea toDockWidgetArea(Qt::ToolBarArea area);
        Qt::ToolBarArea toToolBarArea(Qt::DockWidgetArea area);

    signals:
        void perspectiveChanged(UIView wps);

    protected:
        void addButtonBar(Qt::ToolBarArea area);

    protected:
        virtual void closeEvent(QCloseEvent *event);
        virtual void showEvent(QShowEvent *event);

    private:
        ToolView *viewForRelayout;

    private:
        QString windowKey;
        QHash<Qt::ToolBarArea, TButtonBar *> buttonBarsHash;
        QHash<TButtonBar *, QList<ToolView*> > toolViewsHash;
        QHash<QWidget *, int> managedWidgetsHash;
        QHash<QAction *, int> managedActionsHash;
        QToolBar *specialToolBar;

        UIView perspective;

        TMainWindowAbstractSettings *settings;
        bool autoRestore;
};

#endif
