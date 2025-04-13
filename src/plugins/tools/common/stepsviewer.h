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

#ifndef STEPSVIEWER_H
#define STEPSVIEWER_H

#include "tglobal.h"
#include "tuptweenerstep.h"
#include "tpushbutton.h"

#include <QTableWidget>
#include <cmath>
#include <QGraphicsPathItem>
#include <QBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QKeyEvent>

typedef QList<QPointF> Segment;

class TUPITUBE_EXPORT StepsViewer : public QTableWidget
{
    Q_OBJECT

    public:
        StepsViewer(QWidget *parent = nullptr);
        ~StepsViewer();

        void setPath(const QGraphicsPathItem *pathItem);        
        QVector<TupTweenerStep *> steps();
        int totalSteps();
        void clearInterface();
        QString intervals();
        void loadPath(const QGraphicsPathItem *pathItem, QList<int> intervals);
        QList<QPointF> tweenPoints();

        virtual QSize sizeHint() const;

        void updateSegments();
        void updateSegments(const QPainterPath path);
        void undoSegment(const QPainterPath path);
        void redoSegment(const QPainterPath path);

    signals:
        void totalHasChanged(int total);

    private slots:
        void updatePathSection(int column, int row);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;

    protected slots:
        void commitData(QWidget *editor);
        
    private:
        void calculateKeys();
        void calculateGroups();
        QList<QPointF> calculateSegmentPoints(QPointF begin, QPointF end, int total);
        void addTableRow(int row, int frames);
        void loadTweenPoints();

        QList<int> frameIntervals;
        QList<int> undoFrames;

        QList<Segment> blocksList;

        QList<Segment> segments;
        QList<Segment> undoSegments;

        QPainterPath path;

        int intervalsTotal;
        QList<QPointF> keys;
        QPolygonF pointsList;
        QList<QPointF> tweenPointsList;

        QList<TPushButton*> *plusButton;
        QList<TPushButton*> *minusButton;
        void updateFramesSection(int column, int row);
        void updateColumnSelection(const QPoint &pos);
        void clearSelection();

    private:
        bool selecting;
        int selectedColumn;
        int initialRow;
        QString initialValue; // Store the value of the first selected cell
        QMap<int, QString> originalValues; // Map to store the original values of the selected cells
};
#endif
