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

#include "tuptimelinescenecontainer.h"

TupTimelineSceneContainer::TupTimelineSceneContainer(QWidget *parent) : QTabWidget(parent)
{
    setMovable(true);
    connect(tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SIGNAL(sceneRenameRequested(int)));
}

TupTimelineSceneContainer::~TupTimelineSceneContainer()
{
}

void TupTimelineSceneContainer::addScene(int sceneIndex, TupTimeLineTable *framesTable, const QString &sceneName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::addScene()] - sceneIndex ->" << sceneIndex;
        qDebug() << "[TupTimelineSceneContainer::addScene()] - sceneName ->" << sceneName;
    #endif

    scenes << framesTable;
    QTabWidget::insertTab(sceneIndex, framesTable, sceneName);
    connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SIGNAL(sceneMoved(int,int)));
}

void TupTimelineSceneContainer::restoreScene(int sceneIndex, const QString &sceneName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::restoreScene()] - sceneIndex ->" << sceneIndex;
        qDebug() << "[TupTimelineSceneContainer::restoreScene()] - sceneName ->" << sceneName;
    #endif

    TupTimeLineTable *framesTable = undoScenes.takeLast();
    scenes << framesTable;
    QTabWidget::insertTab(sceneIndex, framesTable, sceneName);
    // connect(tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SLOT(editSceneLabel(int)));
}

void TupTimelineSceneContainer::moveScene(int sceneIndex, int newIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::moveScene()] - sceneIndex ->" << sceneIndex;
        qDebug() << "[TupTimelineSceneContainer::moveScene()] - newIndex ->" << newIndex;
    #endif

    blockSignals(true);
    QTabWidget::tabBar()->moveTab(sceneIndex, newIndex);
    blockSignals(false);
}

void TupTimelineSceneContainer::removeScene(int sceneIndex, bool withBackup)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::removeScene()]";
    #endif

    if (withBackup)
        undoScenes << scenes.takeAt(sceneIndex);
    else
        scenes.takeAt(sceneIndex);

    QTabWidget::removeTab(sceneIndex);
}

void TupTimelineSceneContainer::renameScene(int index, const QString &sceneName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::renameScene()] - index ->" << index;
        qDebug() << "[TupTimelineSceneContainer::renameScene()] - sceneName ->" << sceneName;
    #endif

    setTabText(index, sceneName);
}

void TupTimelineSceneContainer::removeAllScenes()
{
    blockSignals(true);
    clear();
    scenes.clear();
    undoScenes.clear();
    blockSignals(false);
}

TupTimeLineTable * TupTimelineSceneContainer::currentScene()
{
    int index = currentIndex();
    TupTimeLineTable *framesTable = nullptr;

    if (index < scenes.size())
        framesTable = scenes.at(index);

    return framesTable;
}

TupTimeLineTable * TupTimelineSceneContainer::getTable(int index)
{
    TupTimeLineTable *framesTable = scenes.at(index);

    return framesTable;
}

int TupTimelineSceneContainer::count()
{
    return scenes.count();
}

bool TupTimelineSceneContainer::isTableIndexValid(int index)
{
    if (index > -1 && index < scenes.count())
        return true;

    return false;
}

#ifndef QT_NO_WHEELEVENT
void TupTimelineSceneContainer::wheelEvent(QWheelEvent *ev)
{
    QRect rect = tabBar()->rect();
    rect.setWidth(width());

    // SQA: Evaluate this replacement (delta)
    if (rect.contains(ev->position().toPoint()))
        wheelMove(ev->angleDelta().y());
}

void TupTimelineSceneContainer::wheelMove(int delta)
{
    if (count() > 1) {
        int current = currentIndex();
        if (delta < 0) {
            current = (current + 1) % count();
        } else {
            current--;
            if (current < 0)
                current = count() - 1;
        }
        setCurrentIndex(current);
    }
}
#endif

QString TupTimelineSceneContainer::currentSceneName() const
{
    int sceneIndex = tabBar()->currentIndex();
    if (sceneIndex > -1)
        return tabBar()->tabText(sceneIndex);

    #ifdef TUP_DEBUG
        qDebug() << "[TupTimelineSceneContainer::currentSceneName()] - Fatal Error: No scene selected!";
    #endif

    return "";
}
