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

#ifndef TUPTIMELINESCENECONTAINER_H
#define TUPTIMELINESCENECONTAINER_H

#include "tglobal.h"
#include "tuptimelinetable.h"

#include <QTabWidget>
#include <QWheelEvent>
#include <QTabBar>

class T_GUI_EXPORT TupTimelineSceneContainer : public QTabWidget
{
    Q_OBJECT

    public:
        TupTimelineSceneContainer(QWidget *parent = nullptr);
        ~TupTimelineSceneContainer();

        void addScene(int sceneIndex, TupTimeLineTable *framesTable, const QString &title, bool preserveSelection = false);
        void restoreScene(int sceneIndex, const QString &title);
        void moveScene(int pos, int newPos);
        void removeScene(int sceneIndex, bool withBackup);
        void renameScene(int index, const QString &name);
        void swapTables(int pos, int newPos);

        TupTimeLineTable * currentScene();
        TupTimeLineTable * getTable(int index);
        int count();
        void removeAllScenes();
        bool isTableIndexValid(int index);
        QString currentSceneName() const;

    signals:
        void sceneRenameRequested(int sceneIndex);
        void sceneMoved(int from, int to);

    protected:
    #ifndef QT_NO_WHEELEVENT
        virtual void wheelEvent(QWheelEvent *e);
    #endif

    protected slots:
    #ifndef QT_NO_WHEELEVENT
        virtual void wheelMove(int delta);
    #endif

    private:
        QList<TupTimeLineTable *> scenes;
        QList<TupTimeLineTable *> undoScenes;
};

#endif
