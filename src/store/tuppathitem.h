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

#ifndef TUPPATHITEM_H
#define TUPPATHITEM_H

#include "tupabstractserializable.h"

#include <QGraphicsPathItem>
#include <QGraphicsSceneDragDropEvent>
#include <QPainter>

class TUPITUBE_EXPORT TupPathItem : public TupAbstractSerializable, public QGraphicsPathItem
{
    public:
        TupPathItem(QGraphicsItem *parent = nullptr);
        ~TupPathItem();
        
        virtual void fromXml(const QString &xml);
        virtual QDomElement toXml(QDomDocument &doc) const;
        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget = nullptr);
        bool contains(const QPointF &point) const;
        bool isNotEdited();
        void saveOriginalPath();

        QString pathToString() const;
        void setPathFromString(const QString &route);

        void undoPath();
        void redoPath();

        QString refactoringPath(NodeLocation policy, int nodesTotal);
        QString pathRestored(int nodesTotal) const;
        void resetPathHistory();

        QString removeNodeFromPath(int index);
        QString changeNodeTypeFromPath(int index);
        bool pointMatchesPath(QPointF pos, double tolerance, PenTool tool);
        QString addInnerNode(int tolerance, NodeType node);
        bool pointIsContainedBetweenRange(const QPointF &point1, const QPointF &point2,
                                          const QPointF &newPoint, double tolerance);
        QString appendNode(const QPointF &pos);
        QPair<QPointF,QPointF> calculateEndPathCPoints(const QPointF &pos);

        QList<QPointF> keyNodes();
        QList<QColor> nodeColors();
        QList<QString> nodeTips();
        QList<QPair<QPointF,QPointF>> getCPoints();

        QPainterPath clearPath(int tolerance);
        static QPainterPath brushPath(const QPainterPath &route, int tolerance);
        int nodesCount();

        QPair<QString, QString> recalculatePath(const QPointF &limitPoint, int tolerance);
        bool isBlocked();
        void updateBlockingFlag(bool flag);

        // Method for debugging
        QList<QPointF> debuggingCurvePoints(int tolerance);

    protected:
        virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
        virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
        virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
        
    private:
        enum CuttingPointType { None, Begin, End, Middle };
        enum PathSegment { FirstSegment, SecondSegment };

        QPair<QPointF, QPointF> getCurveElements(QPointF initPos, QPointF endPos);
        bool isPointContainedBetweenCurveNodes(const QPointF &node1, const QPointF &node2,
                                               const QPointF &point, double tolerance);
        bool isPointContainedBetweenFlatNodes(const QPointF &node1, const QPointF &node2,
                                              const QPointF &point, int tolerance, bool eraserMode = false);
        QPair<QPointF,QPointF> calculateCPoints(const QPointF &pos1, const QPointF &pos2);
        QPair<QPointF,QPointF> calculatePlainCPoints(const QPointF &pos1, const QPointF &pos2);
        bool isPointPartOfStraightLine(const QPainterPath &route, const QPointF &point, int tolerance,
                                       PenTool tool);
        bool findPointAtStraightLine(const QPointF &point1, const QPointF &point2, const QPointF &cuttingPoint,
                                     int tolerance, PenTool tool);
        QPointF getNewC1Element(const QPointF &c1, const QPointF &c2, double tolerance);
        QPointF pointAtPercentage(const QPointF &startPoint, const QPointF &endPoint, int percentage);

        QList<QPointF> generateCurvePoints(const QPointF &cInit, const QPointF &cEnd);

        bool findPointAtCurve(const QPointF &cuttingPoint, int tolerance, QList<QPointF> curvePoints);
        bool pathIsTooShort(const QPointF &cuttingPoint, int tolerance);

        QString mToString(PathSegment segment, QPointF point);
        QString lineToString(PathSegment segment, QPointF point);
        QString curveToString(PathSegment segment, QList<QPointF> curvePoints);

        QList<QPointF> shortenCurveFromStart(
            const QPointF &cuttingPoint, double tolerance, QList<QList<QPointF>> &bezierPaths);

        QList<QPointF> shortenCurveFromEnd(
            const QPointF &cuttingPoint, double tolerance, QList<QList<QPointF>> &bezierPaths);

        QList<QPointF> firstCurveOfSplit(const QPointF &cuttingPoint, double tolerance,
                                         QList<QList<QPointF>> &bezierPaths);

        QList<QPointF> secondCurveOfSplit(const QPointF &cuttingPoint, double tolerance,
                                          QList<QList<QPointF>> &bezierPaths);

        QPair<QList<QPointF>, QList<QPointF>>
                       splitStraightLine(const QPointF &startPoint, const QPointF &endPoint,
                                         const QPointF &cuttingPoint, double tolerance);        
        void generatePathPoints(QPainterPath route, int tolerance);

        QList<QList<QPointF>> calculateBezierPaths(QPointF c0, QPointF c1,
                                                   QPointF c2, QPointF c3, bool reverse = false);

        QChar t1, t2;
        bool dragOver;
        QList<QString> undoList;
        QList<QString> doList;
        QHash<int, QString> pathCollection;

        bool straightLineFlag;
        bool flatCurveFlag;
        QPointF curvePointFound;
        QPointF newNode;

        QList<QColor> colors;
        QList<QString> tips;

        QList<QPair<QPointF,QPointF>> curvePoints;
        QPair<QString, QString> pathSegments;                

        QPolygonF pathPoints;
        bool pathBlocked;
};

#endif
