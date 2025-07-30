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

TabbedMainWindow::TabbedMainWindow(const QString &winKey, QWidget *parent): TMainWindow(winKey, parent)
{
    currentTab = new QTabWidget;
    connect(currentTab, SIGNAL(currentChanged(int)), this, SLOT(emitWidgetChanged(int)));
    setCentralWidget(currentTab);
}

TabbedMainWindow::~TabbedMainWindow()
{
}

void TabbedMainWindow::addWidget(QWidget *widget, bool persistant, int perspective)
{
    if (widget) {
        if (perspective == currentPerspective())
            currentTab->addTab(widget, widget->windowIcon(), widget->windowTitle());

        if (persistant)
            persistentWidgets << widget;

        pages << widget;
        tabs[widget] = perspective;
    }
}

void TabbedMainWindow::removeWidget(QWidget *widget, bool force)
{
    if (widget) {
        if (force) 
            persistentWidgets.removeAll(widget);

        if (persistentWidgets.contains(widget))
            return;

        int index = currentTab->indexOf(widget);
        if (index == ANIMATION_TAB || index == PLAYER_TAB)
            currentTab->removeTab(index);

        tabs.remove(widget);
        pages.removeAll(widget);
    }
}

void TabbedMainWindow::removeAllWidgets()
{
    persistentWidgets.clear();
    currentTab->clear();
    tabs.clear();
    pages.clear();
}

int TabbedMainWindow::tabCount()
{
    return pages.count();
}

// Close the current tab.
void TabbedMainWindow::closeCurrentTab()
{
    int index = currentTab->currentIndex();
    if (index == ANIMATION_TAB || index == PLAYER_TAB)
        removeWidget(currentTab->widget(index));
}

// Return the current tab widget.
QTabWidget *TabbedMainWindow::tabWidget() const
{
    return currentTab;
}

void TabbedMainWindow::emitWidgetChanged(int index)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TabbedMainWindow::emitWidgetChanged()] - index ->" << index;
    #endif
    */

    if (index == ANIMATION_TAB || index == PLAYER_TAB) {
        setCurrentPerspective(index);
        emit tabHasChanged(index);
    }
}

void TabbedMainWindow::setCurrentTab(int index)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TabbedMainWindow::setCurrentTab()] - index: " << index;
    #endif
    */

    if (index == 0 || index == 1)
        currentTab->setCurrentIndex(index);
}
