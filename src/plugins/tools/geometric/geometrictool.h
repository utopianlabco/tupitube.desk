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

#ifndef GEOMETRICTOOLPLUGIN_H
#define GEOMETRICTOOLPLUGIN_H

#include "tglobal.h"
#include "tuptoolplugin.h"
#include "tuprectitem.h"
#include "tupellipseitem.h"
#include "tuppathitem.h"
#include "tuplineitem.h"
#include "geometricsettings.h"

#include <QObject>
#include <QLabel>
#include <QKeyEvent>
#include <cmath>
#include <QKeySequence>
#include <QImage>
#include <QPaintDevice>
#include <QGraphicsView>

class TUPITUBE_PLUGIN GeometricTool : public TupToolPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "co.utopianlab.tupi.TupToolInterface" FILE "geometrictool.json")
    
    public:
        GeometricTool();
        ~GeometricTool();

        virtual QList<TAction::ActionId> keys() const;
        virtual void init(TupGraphicsScene *scene);
        virtual void press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);
        virtual void move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);
        virtual void release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene);
        virtual void keyPressEvent(QKeyEvent *event);
        virtual void keyReleaseEvent(QKeyEvent *event);
        // virtual void doubleClick(const TupInputDeviceInformation *input, TupGraphicsScene *scene);

        virtual void sceneResponse(const TupSceneResponse *event);
        virtual void layerResponse(const TupLayerResponse *event);
        virtual void frameResponse(const TupFrameResponse *event);
        virtual void itemResponse(const TupItemResponse *event);

        virtual QMap<TAction::ActionId, TAction *> actions() const;
        TAction * getAction(TAction::ActionId toolId);

        int toolType() const;
        
        virtual QWidget *configurator();
        void aboutToChangeScene(TupGraphicsScene *scene);
        virtual void aboutToChangeTool();
        virtual void saveConfig();
        virtual QCursor toolCursor(); // const;
        void updatePos(QPointF pos);

    public slots:
        void endItem();

    private slots:
        void updateToolMode(ToolMode tool);
        void updateLineMode(GeometricSettings::LineType type);
        void updateTriangleType(GeometricSettings::TriangleType type);
        void updateHexagonType(GeometricSettings::HexagonType type);
        void updateEraserSize(int size);

    signals:
        void closeHugeCanvas();
        void callForPlugin(int, int);
        void toolModeUpdated(ToolMode);

    private:
        void setupActions();

    private:
        QBrush setLiteBrush(QColor color, Qt::BrushStyle style);
        void saveLineSettings();
        void storePathItems();
        TupFrame* getCurrentFrame();
        void runEraser(const QPointF &point);
        void setZValueReferences();

        ToolMode currentToolMode;
        QBrush fillBrush;
        TupRectItem *rect;
        TupEllipseItem *ellipse;
        TupLineItem *guideLine;
        TupPathItem *triangle;        
        QPainterPath trianglePath;
        TupPathItem *linePath;
        TupPathItem *hexagon;
        QPainterPath hexagonPath;

        TupGraphicsScene *scene;
        TupBrushManager *brushManager;

        GeometricSettings *settings;
        bool added;
        QPointF currentPoint;
        QPointF lastPoint;
        QMap<TAction::ActionId, TAction *> geoActions;

        bool proportion;
        bool side;
        int xSideLength;
        int ySideLength;

        QGraphicsItem *item;
        QCursor squareCursor;
        QCursor circleCursor;
        QCursor lineCursor;
        QCursor triangleCursor;
        QCursor hexagonCursor;
        QCursor eraserCursor;

        QPen eraserPen;
        QGraphicsEllipseItem *eraserCircle;
        QPointF eraserDistance;

        GeometricSettings::TriangleType triangleType;
        GeometricSettings::HexagonType hexagonType;

        bool straightMode;
        int eraserSize;
        QList<TupPathItem *> lineItems;

        int baseZValue;
        int topZValue;
        int circleZValue;

        int currentLayer;
        int currentFrame;
};

#endif
