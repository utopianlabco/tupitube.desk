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

#ifndef TUPEXPOSURESCENETABWIDGET_H
#define TUPEXPOSURESCENETABWIDGET_H

#include "tglobal.h"
#include "tupexposuretable.h"
#include "tapplicationproperties.h"

#include <QTabWidget>
#include <QList>
#include <QLabel>
#include <QFrame>
#include <QWheelEvent>
#include <QTabBar>
#include <QVBoxLayout>
#include <QDoubleSpinBox>

/**
 * @author Gustav Gonzalez 
*/

class T_GUI_EXPORT TupExposureSceneTabWidget : public QFrame
{
    Q_OBJECT

    public:
        TupExposureSceneTabWidget(QWidget *parent = nullptr);
        ~TupExposureSceneTabWidget();

        void addScene(int index, const QString &name, TupExposureTable *table = nullptr);
        void restoreScene(int index, const QString &name);
        void removeScene(int index, bool withBackup);
        void renameScene(int index, const QString &name);
        void moveScene(int pos, int newPos);
        void swapExposureTables(int pos, int newPos);

        TupExposureTable* getCurrentExposureTable();
        TupExposureTable* getExposureTable(int index);
        void setCurrentSceneIndex(int index);
        int currentSceneIndex();
        QString currentSceneName() const;
        bool isExposureTableIndexValid(int index);
        int count();
        void setLayerOpacity(int sceneIndex, double opacity);
        void setLayerVisibility(int sceneIndex, int layerIndex, bool visibility);

    public slots:
        void removeAllTabs();

    signals:
        void currentChanged(int index);
        void layerOpacityChanged(double opacity);
        void sceneRenameRequested(int index);
        void sceneMoved(int from, int to);

    private:
        QTabWidget *scenesTabber;

        QList<QFrame *> sceneContainers;
        QList<QFrame *> undoContainers;

        QList<TupExposureTable *> sceneTables;
        QList<TupExposureTable *> undoTables;

        QList<QDoubleSpinBox *> opacityControl;
        QList<QDoubleSpinBox *> undoOpacities;
};

#endif
