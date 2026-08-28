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

#ifndef COLORINGTWEENER_H
#define COLORINGTWEENER_H

#include "tglobal.h"
#include "tuptoolplugin.h"
#include "coloringsettings.h"
#include "configurator.h"

/**
 * @author Gustav Gonzalez 
 * 
*/

class TUPITUBE_PLUGIN ColoringTweener : public TupToolPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "co.utopianlab.tupi.TupToolInterface" FILE "coloringtool.json")

    public:
        ColoringTweener();
        virtual ~ColoringTweener();

        virtual void init(TupGraphicsScene *scene);
        virtual QList<TAction::ActionId> keys() const;
        virtual void press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);
        virtual void move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);
        virtual void release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);

        virtual QMap<TAction::ActionId, TAction *>actions() const;
        virtual TAction * getAction(TAction::ActionId toolId);

        int toolType() const;
        virtual QWidget *configurator();

        void aboutToChangeScene(TupGraphicsScene *scene);
        virtual void aboutToChangeTool();

        virtual void updateScene(TupGraphicsScene *scene);
        virtual void saveConfig();

        virtual void sceneResponse(const TupSceneResponse *event);
        virtual void layerResponse(const TupLayerResponse *event);
        virtual void frameResponse(const TupFrameResponse *event);

    signals:
        void tweenRemoved();

    private slots:
        void setCurrentTween(const QString &name);
        void setSelection();
        void setPropertiesMode(); 
        void updateMode(TupToolPlugin::Mode mode);
        void updateStartPoint(int index);
        void applyReset();
        void applyTween();
        void removeTween(const QString &name);

    private:
        void setupActions();
        int framesCount();
        void clearSelection();
        void disableSelection();
        void removeTweenFromProject(const QString &name);

        QMap<TAction::ActionId, TAction *> colorActions;
        Configurator *configPanel;

        TupGraphicsScene *scene;
        QList<QGraphicsItem *> objects;

        TupItemTweener *currentTween;
        int initFrame;
        int initLayer;
        int initScene;

        TupToolPlugin::Mode mode;
        TupToolPlugin::EditMode editMode;
};

#endif
