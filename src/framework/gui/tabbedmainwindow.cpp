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

#include "tabbedmainwindow.h"

// TabbedMainWindow

TabbedMainWindow::TabbedMainWindow(const QString &winKey, UIView defaultPerspective, QWidget *parent):
                                   TMainWindow(winKey, defaultPerspective, parent)
{
    tabsContainer = new QTabWidget;
    connect(tabsContainer, SIGNAL(currentChanged(int)), this, SLOT(emitWidgetChanged(int)));
    setCentralWidget(tabsContainer);
}

TabbedMainWindow::~TabbedMainWindow()
{
}

void TabbedMainWindow::addTabComponent(QWidget *widget, bool persistant)
{
    if (widget) {
        tabsContainer->addTab(widget, widget->windowIcon(), widget->windowTitle());

        if (persistant)
            persistentWidgets << widget;

        pages << widget;
    }
}

void TabbedMainWindow::removeWidget(QWidget *widget, bool force)
{
    if (widget) {
        if (force) 
            persistentWidgets.removeAll(widget);

        if (persistentWidgets.contains(widget))
            return;

        int index = tabsContainer->indexOf(widget);
        if (index != -1)
            tabsContainer->removeTab(index);

        pages.removeAll(widget);
    }
}

void TabbedMainWindow::removeAllWidgets()
{
    persistentWidgets.clear();
    tabsContainer->clear();
    pages.clear();
}

int TabbedMainWindow::tabCount()
{
    return pages.count();
}

void TabbedMainWindow::closeCurrentTab()
{
    int index = tabsContainer->currentIndex();
    if (index != 1)
        removeWidget(tabsContainer->widget(index));
}

QTabWidget *TabbedMainWindow::tabWidget() const
{
    return tabsContainer;
}

void TabbedMainWindow::emitWidgetChanged(int index)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TabbedMainWindow::emitWidgetChanged()] - index ->" << index;
    #endif
    */

    UIView perspective = AnimationView;
    if (index == 1)
        perspective = PlayerView;

    if (currentPerspective() != perspective)
        setCurrentPerspective(perspective);
    emit tabHasChanged(perspective);
}

void TabbedMainWindow::setCurrentTab(int index)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TabbedMainWindow::setCurrentTab()] - index ->" << index;
    #endif
    */

    if (index > -1 && index < tabsContainer->count())
        tabsContainer->setCurrentIndex(index);
}
