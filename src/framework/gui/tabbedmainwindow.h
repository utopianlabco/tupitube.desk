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

#ifndef TABBEDMAINWINDOW_H
#define TABBEDMAINWINDOW_H

#include "tmainwindow.h"

#include <QTabWidget>

class T_GUI_EXPORT TabbedMainWindow : public TMainWindow
{
    Q_OBJECT

    public:
        TabbedMainWindow(const QString &winKey, QWidget *parent = nullptr);
        ~TabbedMainWindow();

        void addWidget(QWidget *widget, bool persistant = true, int perspective = ANIMATION_TAB);
        void removeWidget(QWidget *widget, bool force = false);
        void removeAllWidgets();

        QTabWidget *tabWidget() const;
        void setCurrentTab(int index);
        int tabCount(); 

    protected slots:
        void closeCurrentTab();

    signals:
        void widgetChanged(QWidget *widget);
        void tabHasChanged(int);

    private slots:
        void emitWidgetChanged(int index);

    private:
        QTabWidget *currentTab;
        QWidgetList persistentWidgets;
        QHash<QWidget *, int> tabs;
        QWidgetList pages;
};

#endif
