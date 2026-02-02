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

#include "tuppathitem.h"
#include "tupsvg2qt.h"
#include "tupgraphicalgorithm.h"
#include "tupserializer.h"
#include "talgorithm.h"

#include <QBrush>
#include <QMimeData>
#include <QPainterPath>

TupPathItem::TupPathItem(QGraphicsItem *parent) : QGraphicsPathItem(parent), dragOver(false)
{
    setAcceptDrops(true);
    pathBlocked = false;
}

TupPathItem::~TupPathItem()
{
}

void TupPathItem::fromXml(const QString &xml)
{
    Q_UNUSED(xml)
}

QDomElement TupPathItem::toXml(QDomDocument &doc) const
{
    QDomElement root = doc.createElement("path");

    QString strPath = pathToString();
    root.setAttribute("coords", strPath);

    root.appendChild(TupSerializer::properties(this, doc));

    QBrush brush = this->brush();
    root.appendChild(TupSerializer::brush(&brush, doc));

    QPen pen = this->pen();
    root.appendChild(TupSerializer::pen(&pen, doc));

    return root;
}

void TupPathItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsPathItem::paint(painter, option, widget);
}

bool TupPathItem::contains(const QPointF &point) const
{
    return QGraphicsPathItem::contains(point);
}

void TupPathItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasColor()) {
        event->setAccepted(true);
        dragOver = true;
        update();
    } else {
        event->setAccepted(false);
    }
}

void TupPathItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event)

    dragOver = false;
    update();
}

void TupPathItem::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    dragOver = false;

    if (event->mimeData()->hasColor()) {
        QVariant color = event->mimeData()->colorData();
        setBrush(QBrush(color.value<QColor>()));
    } else if (event->mimeData()->hasImage()) {
        QVariant pixmap = event->mimeData()->imageData();
        setBrush(QBrush(pixmap.value<QPixmap>()));
    }

    update();
}

// Converts object into its string representation

QString TupPathItem::pathToString() const
{
    QPainterPath route = path();
    QString strPath = "";
    QChar t;
    int total = route.elementCount();

    for(int i=0; i < total; i++) {
        QPainterPath::Element e = route.elementAt(i);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                if (t != 'M') {
                    t = 'M';
                    strPath += "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                } else {
                    strPath += QString::number(e.x) + " " + QString::number(e.y) + " ";
                }
            }
            break;
            case QPainterPath::LineToElement:
            {
                if (t != 'L') {
                    t = 'L';
                    strPath += " L " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                } else {
                    strPath += QString::number(e.x) + " " + QString::number(e.y) + " ";
                }
            }
            break;
            case QPainterPath::CurveToElement:
            {
                if (t != 'C') {
                    t = 'C';
                    strPath += " C " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                } else {
                    strPath += "  " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                }
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                if (t == 'C')
                    strPath +=  " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";
            }
            break;
        }
    }

    return strPath;
}

bool TupPathItem::isNotEdited()
{
    return doList.isEmpty() && undoList.isEmpty();
}

void TupPathItem::saveOriginalPath()
{
    QString original = pathToString();
    doList << original;
}

void TupPathItem::setPathFromString(const QString &route)
{
    QPainterPath qPath;
    TupSvg2Qt::svgpath2qtpath(route, qPath);
    setPath(qPath);
    doList << route;
}

void TupPathItem::undoPath()
{
    if (doList.count() > 1) {
        undoList << doList.takeLast();
        if (!doList.isEmpty()) {
            QString route = doList.last();
            QPainterPath qPath;
            TupSvg2Qt::svgpath2qtpath(route, qPath);
            setPath(qPath);
        }
    }
}

void TupPathItem::redoPath()
{
    if (!undoList.isEmpty()) {
        QString route = undoList.takeLast();
        doList << route;
        QPainterPath qPath;
        TupSvg2Qt::svgpath2qtpath(route, qPath);
        setPath(qPath);
    }
}

// This method is used to simplify complex paths by removing nodes from
// the beginning, the middle, random position or the end of the path

QString TupPathItem::refactoringPath(NodeLocation policy, int nodesTotal)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::refactoringPath()] - policy ->" << policy;
        qDebug() << "[TupPathItem::refactoringPath()] - Tracking nodesTotal ->" << nodesTotal;
    #endif

    // Store current structure of the path
    if (!pathCollection.contains(nodesTotal)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::refactoringPath()] - Storing path for nodesTotal ->" << nodesTotal;
            qDebug() << "[TupPathItem::refactoringPath()] - path ->" << pathToString();
        #endif
        pathCollection[nodesTotal] = pathToString();
    }

    // Return the new version of the path if it is already calculated and stored
    if (pathCollection.contains(nodesTotal-1)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::refactoringPath()] - Returning saved path at ->" << (nodesTotal-1);
        #endif

        return pathCollection[nodesTotal-1];
    }

    QChar t;
    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    QString pathStr = "";
    QList<int> curveIndexes;

    // Capture the indexes of all the significant elements of the path
    bool lookingData = false;
    int dataCounter = 0;
    int curvesCounter = 0;
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        if (e.type == QPainterPath::CurveToElement || e.type == QPainterPath::MoveToElement
            || e.type == QPainterPath::LineToElement) {
            curveIndexes << curvesCounter;
            curvesCounter++;
        }
    }

    // Variable "mark" contains the index of the node to be removed
    int mark = curveIndexes.at(0); // FirstNode
    if (curvesCounter > 1) {
        if (policy == MiddleNode) {
            if (curvesCounter % 2 == 0) { // Remove the middle node
                mark = curveIndexes.at(curvesCounter/2);
            } else {
                if (curvesCounter == 3)
                    mark = curveIndexes.at(1);
                else
                    mark = curveIndexes.at((curvesCounter/2) + 1);
            }
        } else if (policy == RandomNode) {
            int index = TAlgorithm::randomNumber(curvesCounter);
            if (index == 0)
                index = 1;
            if (index == (curvesCounter - 1))
                index = curvesCounter - 2;

            mark = curveIndexes.at(index);
        } else if (policy == LastNode) {
            mark = curveIndexes.at(curvesCounter - 1);
        }
    }

    // Rebuild the path structure but ignoring the node selected previously,
    // store it and return it

    bool replace = false;
    curvesCounter = 0;
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);

        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                if (policy == FirstNode && curvesCounter == mark) {
                    replace = true;
                } else {
                    if (t != 'M') {
                        t = 'M';
                        pathStr += "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    } else {
                        pathStr += QString::number(e.x) + " " + QString::number(e.y) + " ";
                    }
                }

                curvesCounter++;
            }
            break;
            case QPainterPath::LineToElement:
            {
                if (replace) {
                    if (t != 'M') {
                        t = 'M';
                        pathStr += "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    } else {
                        pathStr += QString::number(e.x) + " " + QString::number(e.y) + " ";
                    }
                    replace = false;
                } else if (curvesCounter != mark) {
                    if (t != 'L') {
                        t = 'L';
                        pathStr += " L " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    } else {
                        pathStr += QString::number(e.x) + " " + QString::number(e.y) + " ";
                    }
                }

                curvesCounter++;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                if (!replace && curvesCounter != mark) {
                    if (t != 'C') {
                        t = 'C';
                        pathStr += " C " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    } else {
                        pathStr += "  " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    }
                } else {
                    lookingData = true;
                }
                curvesCounter++;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                if (!lookingData) {
                    if (t == 'C')
                        pathStr +=  " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";
                } else {
                    dataCounter++;

                    if (dataCounter == 2) {
                        lookingData = false;
                        dataCounter = 0;

                        if (replace) {
                            replace = false;
                            pathStr += "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                        }
                    }
                }
            }
            break;
        }
    }

    pathCollection[nodesTotal-1] = pathStr;

    return pathStr;
}

QString TupPathItem::pathRestored(int nodesTotal) const
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pathRestored()] - nodesTotal ->" << nodesTotal;
    #endif

    if (pathCollection.contains(nodesTotal))
        return pathCollection[nodesTotal];

    return "";
}

void TupPathItem::resetPathHistory()
{
    pathCollection.clear();
}

QString TupPathItem::removeNodeFromPath(int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::removeNodeFromPath()] - index ->" << index;
    #endif

    QChar t;
    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    QString pathStr = "";

    int dataCounter = 0;
    int nodesCounter = 0;
    bool found = false;

    QStringList parts;
    bool replace = false;
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                nodesCounter++;

                if (nodesCounter != index) {
                    if (t != 'M') {
                        t = 'M';
                        parts << "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    } else {
                        parts << QString::number(e.x) + " " + QString::number(e.y) + " ";
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::removeNodeFromPath()] - MoveToElement - Node was FOUND!";
                    #endif

                    replace = true;
                    found = true;
                }
            }
            break;
            case QPainterPath::LineToElement:
            {
                nodesCounter++;

                if (replace) {
                    parts << "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                    replace = false;
                } else {
                    if (nodesCounter != index) {
                        if (t != 'L') {
                            t = 'L';
                            parts << " L " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                        } else {
                            parts << QString::number(e.x) + " " + QString::number(e.y) + " ";
                        }
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::removeNodeFromPath()] - LineToElement - Node was FOUND!";
                        #endif
                    }
                }
            }
            break;
            case QPainterPath::CurveToElement:
            {
                if (t != 'C') {
                    t = 'C';
                    parts << " C " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                } else {
                    parts << "  " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                }

                dataCounter = 0;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                dataCounter++;
                if (dataCounter == 2)
                    nodesCounter++;

                if ((index == nodesCounter && dataCounter == 2) || (replace && dataCounter == 2)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::removeNodeFromPath()] - *** Node was FOUND!";
                    #endif

                    found = true;
                    parts.removeLast();
                    parts.removeLast();

                    if (replace) {
                        t = 'M';
                        parts << "M " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                        replace = false;
                    } else {
                        t = ' ';
                    }
                } else {
                    parts <<  " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";
                }
            }
            break;
        }
    }

    if (!found) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::removeNodeFromPath()] - Warning: Node was NOT found! ->" << index;
        #endif
    }

    foreach(QString line, parts)
        pathStr += line;

    return pathStr;
}

QString TupPathItem::changeNodeTypeFromPath(int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::changeNodeTypeFromPath()] - index ->" << index;
    #endif

    QChar t;
    QPainterPath route = path();
    int elementsTotal = route.elementCount();

    QStringList parts;
    bool replace = false;
    int dataCounter = 0;
    bool found = false;
    int nodesCounter = 0;
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        QPointF pathPoint = QPointF(e.x, e.y);

        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                nodesCounter++;
                if (nodesCounter == index)
                    replace = true;

                if (t != 'M') {
                    t = 'M';
                    parts << "M " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                } else {
                    parts << QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                }
            }
            break;
            case QPainterPath::LineToElement:
            {
                nodesCounter++;

                if ((nodesCounter == index) || replace) {
                    // Replacing a line node for a curve node
                    if (t != 'C') {
                        t = 'C';
                        parts << " C " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    } else {
                        parts << "  " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    }
                    parts <<  " " + QString::number(pathPoint.x()) + "  " + QString::number(pathPoint.y()) + " ";
                    parts <<  " " + QString::number(pathPoint.x()) + "  " + QString::number(pathPoint.y()) + " ";

                    if (replace)
                        replace = false;

                    found = true;
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::changeNodeTypeFromPath()] - LineToElement - Node was FOUND!";
                    #endif
                } else {
                    if (t != 'L') {
                        t = 'L';
                        parts << " L " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    } else {
                        parts << QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    }
                }
            }
            break;
            case QPainterPath::CurveToElement:
            {
                if (t != 'C') {
                    t = 'C';
                    parts << " C " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                } else {
                    parts << "  " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                }

                dataCounter = 0;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                dataCounter++;
                if (dataCounter == 2)
                    nodesCounter++;

                if ((nodesCounter == index) || replace) {
                    if (dataCounter == 2) {
                        parts.removeLast(); // Removing first CurveToDataElement item
                        parts.removeLast(); // Removing CurveToElement item

                        // Replacing a curve node for a line node
                        t = 'L';
                        parts << " L " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";

                        if (replace)
                            replace = false;

                        found = true;
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::changeNodeTypeFromPath()] - CurveToDataElement - Node was FOUND!";
                        #endif
                    } else {
                        if (t == 'C')
                            parts <<  " " + QString::number(pathPoint.x()) + "  " + QString::number(pathPoint.y()) + " ";
                    }
                } else {
                    if (t == 'C')
                        parts <<  " " + QString::number(pathPoint.x()) + "  " + QString::number(pathPoint.y()) + " ";
                }
            }

            break;
        }
    }

    if (!found) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::changeNodeTypeFromPath()] - Warning: Node was NOT found ->" << (index +1);
        #endif
    }

    QString pathStr = "";
    foreach(QString line, parts)
        pathStr += line;

    return pathStr;
}

void TupPathItem::generatePathPoints(QPainterPath route, int toleranceValue)
{
    double tolerance = static_cast<double>(toleranceValue);
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::generatePathPoints()] - path string ->" << pathToString();
        qDebug() << "[TupPathItem::generatePathPoints()] - tolerance ->" << tolerance;
    #endif

    // Getting all the points of the path
    QPolygonF dots = route.toFillPolygon();
    if (!dots.isEmpty())
        dots.removeLast(); // Last dot is equal to the first one

    // Detect all the points closer to the cutting point and store them in pathPoints
    pathPoints.clear();
    int total = dots.size() - 1;
    for(int i=0; i<total; i++) {
        double distance = TAlgorithm::distance(dots.at(i), dots.at(i+1));
        pathPoints << dots.at(i);

        if (distance > tolerance) {
            double x = dots.at(i+1).x() - dots.at(i).x();
            double y = dots.at(i+1).y() - dots.at(i).y();
            double segment = sqrt(pow(x, 2) + pow(y, 2));
            double step = (segment/(tolerance/2)) - 1;

            if (!std::isnan(step)) {
                double stepX = x/step;
                double stepY = y/step;
                for(int j=1; j<step; j++)
                    pathPoints << (dots.at(i) + QPointF(stepX*j, stepY*j));
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPathItem::generatePathPoints()] - Warning: Division by zero (step variable)";
                #endif
            }
        }

        pathPoints << dots.at(i+1);
    }
}

QList<QPointF> TupPathItem::debuggingCurvePoints(int tolerance)
{
    QList<QPointF> pathPoints;

    // Getting all the points of the path
    QPolygonF dots = path().toFillPolygon();
    if (!dots.isEmpty())
        dots.removeLast(); // Last dot is equal to the first one

    // Detect all the points closer to the cutting point and store them in pathPoints
    int total = dots.size() - 1;
    for(int i=0; i<total; i++) {
        double distance = TAlgorithm::distance(dots.at(i), dots.at(i+1));
        pathPoints << dots.at(i);

        if (distance > tolerance) {
            double x = dots.at(i+1).x() - dots.at(i).x();
            double y = dots.at(i+1).y() - dots.at(i).y();
            double segment = sqrt(pow(x, 2) + pow(y, 2));
            double step = (segment/(tolerance/2)) - 1;

            double stepX = x/step;
            double stepY = y/step;
            for(int j=1; j<step; j++)
                pathPoints << (dots.at(i) + QPointF(stepX*j, stepY*j));
        }

        pathPoints << dots.at(i+1);
    }

    return pathPoints;
}

bool TupPathItem::pointMatchesPath(QPointF pos, double tolerance, ToolMode tool)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pointMatchesPath()] - pos ->" << pos;
        qDebug() << "[TupPathItem::pointMatchesPath()] - tolerance ->" << tolerance;
    #endif
    */

    int shift = 0;
    if (tool == EraserMode)
        shift = tolerance;

    QRectF rect = boundingRect();
    QPointF topLeft = rect.topLeft();
    QPointF bottomRight = rect.bottomRight();

    // Checking if point is part of the path area (rect)
    bool contained = ((topLeft.x() - shift) <= pos.x() && (topLeft.y() - shift) <= pos.y())
                     && (pos.x() <= (bottomRight.x() + shift) && pos.y() <= (bottomRight.y() + shift));
    if (!contained) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::pointMatchesPath()] - Warning: pos is out of scope! ->" << pos;
            qDebug() << "[TupPathItem::pointMatchesPath()] - boundingRect() - top left ->" << boundingRect().topLeft();
            qDebug() << "[TupPathItem::pointMatchesPath()] - boundingRect() - bottom right ->" << boundingRect().bottomRight();
        #endif

        return false;
    }

    /*
    #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::pointMatchesPath()] - Point is part of the item rect!";
            qDebug() << "---";
    #endif
    */

    double minimum;
    straightLineFlag = false; // global variable
    flatCurveFlag = false;
    QPainterPath route = path();

    // Checking if point is part of a straight line segment
    if (isPointPartOfStraightLine(route, pos, tolerance, tool))
        return true;

    generatePathPoints(route, tolerance);

    // Look for the closer point to the cutting point
    double found = false;
    int total = pathPoints.size();
    for(int i=0; i<total-1; i++) {
        double distance = TAlgorithm::distance(pos, pathPoints.at(i));
        if (i == 0) {
            minimum = distance;
            if (distance <= tolerance) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPathItem::pointMatchesPath()] - Point was found!";
                    qDebug() << "---";
                #endif
                newNode = pathPoints.at(i);

                return true;
            }
        }

        // If point is closer than tolerance value, store it
        if (distance <= tolerance) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::pointMatchesPath()] - Point is part of the path!";
            #endif

            if (distance < minimum) {
                newNode = pathPoints.at(i);
                minimum = distance;
                found = true;
            }
        }
    }

    if (found) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::pointMatchesPath()] - Point was found!";
            qDebug() << "---";
        #endif

        return true;
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pointMatchesPath()] - Warning: Input point is out of the path!";
        qDebug() << "[TupPathItem::pointMatchesPath()] - pos ->" << pos;
    #endif

    return false;
}

// This method checks if target is part of a line segment of the path
bool TupPathItem::isPointPartOfStraightLine(const QPainterPath &route, const QPointF &cuttingPoint,
                                            int tolerance, ToolMode tool)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::IsPointPartOfStraightLine()] - point ->" << cuttingPoint;
    #endif

    QList<QPointF> previewPoint;
    QPointF curvePoint;
    QPair<QPointF, QPointF> dataPoints;
    int curveDataCounter = 0;
    int elementsTotal = route.elementCount();
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        QPointF pathPoint = QPointF(e.x, e.y);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                previewPoint << pathPoint;
            }
            break;
            case QPainterPath::LineToElement:
            {
                if (findPointAtStraightLine(previewPoint.last(), pathPoint, cuttingPoint, tolerance, tool)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::IsPointPartOfStraightLine()] - Point was found in line!";
                    #endif
                    straightLineFlag = true;

                    return true;
                }

                previewPoint << pathPoint;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                curveDataCounter = 0;
                curvePoint = pathPoint;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                curveDataCounter++;
                if (curveDataCounter == 1)
                    dataPoints.first = pathPoint;
                if (curveDataCounter == 2) {
                    dataPoints.second = pathPoint;
                    previewPoint << pathPoint;
                }

                if (curveDataCounter == 2) {
                    // The segment is part of a curve, but is flat!
                    if (curvePoint == dataPoints.first && curvePoint == dataPoints.second) {
                        int index = previewPoint.size() - 2;
                        if (findPointAtStraightLine(previewPoint.at(index), pathPoint, cuttingPoint, tolerance, tool)) {
                            flatCurveFlag = true;

                            return true;
                        }
                    }
                }
            }
            break;
            default:
            break;
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::IsPointPartOfStraightLine()] - Undetermined!";
    #endif

    return false;
}

// This method finds the point in the straight line closer to the cutting point
bool TupPathItem::findPointAtStraightLine(const QPointF &point1, const QPointF &point2,
                                          const QPointF &cuttingPoint, int tolerance, ToolMode tool)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::findPointAtStraightLine()]";
    #endif
    */

    QPointF bestPoint;
    double distance = TAlgorithm::distance(point1, point2);
    double min = distance;
    if (distance > tolerance) { // Line is bigger than tolerance
        int times = distance / tolerance;
        double deltaX = (point2.x() - point1.x()) / times;
        double deltaY = (point2.y() - point1.y()) / times;

        int k=1;
        if (tool == EraserMode)
            k=0;
        for (; k<=times; k++) {
            QPointF point(point1.x() + (deltaX * k), point1.y() + (deltaY * k));
            distance = TAlgorithm::distance(point, cuttingPoint);
            // qDebug() << "DISTANCE ->" << distance;
            if (distance < min) {
                min = distance;
                bestPoint = point;
            }
        }

        // The closer point of the line is in the range of the eraser
        if (min < tolerance) {
            newNode = bestPoint;

            return true;
        }

        if (tool == EraserMode) {
            if (TAlgorithm::distance(point1, cuttingPoint) <= tolerance) {                
                newNode = point1;

                return true;
            }

            if (TAlgorithm::distance(point2, cuttingPoint) <= tolerance) {
                newNode = point2;

                return true;
            }
        }
    } else { // Segment is too short (distance < tolerance)
        double deltaX = (point2.x() - point1.x()) / 2;
        double deltaY = (point2.y() - point1.y()) / 2;
        QPointF middlePoint(point1.x() + deltaX, point1.y() + deltaY);
        QList<QPointF> pointList;
        pointList << point1 << middlePoint << point2;
        for(int i=0; i< 3; i++) {
            QPointF point = pointList.at(i);
            distance = TAlgorithm::distance(point, cuttingPoint);
            if (i == 0) {
                min = distance;
                newNode = point;
            } else if (distance < min) {
                min = distance;
                newNode = point;
            }
        }

        if (min <= tolerance)
            return true;
    }

    return false;
}

QString TupPathItem::addInnerNode(int tolerance, NodeType nodeType)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::addInnerNode()]";
    #endif

    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    QPointF previewPoint;

    int dataCounter = 0;
    int nodesCounter = 0;
    QPointF initPoint;
    QPointF endPoint;
    bool ignoreData = false;
    bool isEarlyNode = false;
    bool found = false;
    int foundInLine = false;
    int lineIndex = 0;

    QStringList parts;
    QString pathStr = "";

    // Getting the key points for the new point (initPoint - endPoint)
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        QPointF pathPoint = QPointF(e.x, e.y);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                nodesCounter++;
                previewPoint = pathPoint;
            }
            break;
            case QPainterPath::LineToElement:
            {
                nodesCounter++;
                lineIndex = nodesCounter;

                // Check if new node is contained in this straight segment
                if (isPointContainedBetweenFlatNodes(previewPoint, pathPoint, newNode, tolerance)) {
                    found = true;
                    foundInLine = true;
                    initPoint = previewPoint;
                    endPoint = pathPoint;
                    goto next;
                }

                previewPoint = pathPoint;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                dataCounter = 0;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                if (dataCounter == 1) { // This is the node of the curve
                    nodesCounter++;

                    if (!straightLineFlag) {
                        if (flatCurveFlag) { // Flat curve segment
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - CurveToDataElement - Evaluating FLAT nodes segment";
                            #endif
                            if (isPointContainedBetweenFlatNodes(previewPoint, pathPoint, newNode, tolerance)) {
                                found = true;
                                initPoint = previewPoint;
                                endPoint = pathPoint;
                                if ((nodesCounter - 1) == 1)
                                    isEarlyNode = true;
                                goto next;
                            }
                        } else { // Curve Segment
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - CurveToDataElement - Evaluating CURVE nodes segment";
                            #endif
                            if (isPointContainedBetweenCurveNodes(previewPoint, pathPoint, newNode, tolerance)) {
                                found = true;
                                initPoint = previewPoint;
                                endPoint = pathPoint;
                                if ((nodesCounter - 1) == 1)
                                    isEarlyNode = true;
                                goto next;
                            }
                        }
                    }

                    previewPoint = pathPoint;
                }

                dataCounter++;
            }
            break;
            default:
            break;
        }
    }

    next:

    if (!found) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::addInnerNode()] - Node is out of the path. Appending... ->" << newNode;
        #endif

        QString pathStr = appendNode(newNode);
        setPathFromString(pathStr);

        return pathStr;
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::addInnerNode()] - Segment was found. Time to generate the path string and insert the node...";
        qDebug() << "[TupPathItem::addInnerNode()] - initPoint ->" << initPoint;
        qDebug() << "[TupPathItem::addInnerNode()] - endPoint ->" << endPoint;
    #endif

    QChar t;
    nodesCounter = 0;
    bool nodeAdded = false;

    // Time to generate the path
    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        QPointF pathPoint = QPointF(e.x, e.y);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                nodesCounter++;

                if (t != 'M') {
                    t = 'M';
                    parts << "M " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                } else {
                    parts << QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                }

                if (isEarlyNode) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::addInnerNode()] - Adding node at the beginning of the path...";
                    #endif

                    if (flatCurveFlag) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding node at FLAT CURVE segment";
                        #endif

                        if (nodeType == CurveNode) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                            #endif

                            if (t != 'C') {
                                t = 'C';
                                parts << " C " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                            } else {
                                parts << "  " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                            }

                            parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                            // Adding NewNode element (CurveToDataElement)
                            parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                        } else { // Adding LineNode
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding LINE node";
                            #endif

                            t = 'L';
                            parts << " L " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        }
                    } else { // Curve Segment
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding node at CURVE segment";
                        #endif

                        ignoreData = true;

                        QPair<QPointF, QPointF> elements;
                        QPointF curveToElement;
                        QPointF dataToElement;
                        if (nodeType == CurveNode) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                                qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for newNode";
                            #endif

                            // Calculating new segment and insert it here
                            elements = getCurveElements(initPoint, newNode);
                            // Adding CurveToElement
                            curveToElement = elements.first;
                            if (t != 'C') {
                                t = 'C';
                                parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                            } else {
                                parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                            }

                            // Adding CurveToDataElement
                            dataToElement = elements.second;
                            parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";

                            // Adding NewNode element (CurveToDataElement)
                            parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                        } else { // Adding LineNode
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding LINE node";
                            #endif

                            t = 'L';
                            parts << " L " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        }

                        // Calculating curve positions of the next node
                        #ifdef TUP_DEBUG
                            qDebug() << "---";
                            qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for the node after the newNode";
                        #endif
                        elements = getCurveElements(newNode, endPoint);
                        curveToElement = elements.first;
                        // Adding CurveToElement
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Creating CurveToDataElement
                        dataToElement = elements.second;
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";
                    }
                    nodeAdded = true;
                }
            }
            break;
            case QPainterPath::LineToElement:
            {
                nodesCounter++;

                if (foundInLine && nodesCounter == lineIndex) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::addInnerNode()] - Adding node at line segment of the path...";
                    #endif

                    if (nodeType == CurveNode) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                        #endif

                        QPointF segment = newNode - initPoint;
                        double stepX = segment.x()/3;
                        double stepY = segment.y()/3;

                        // Calculating new segment and insert it here

                        QPointF curveToElement = initPoint + QPointF(stepX, stepY);
                        QPointF dataToElement = initPoint + QPointF(stepX*2, stepY*2);

                        // Adding CurveToElement
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << " " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Adding CurveToDataElement
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";

                        // Adding NewNode element (CurveToDataElement)
                        parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                    } else { // Adding LineNode
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding LINE node";
                        #endif

                        if (t != 'L') {
                            t = 'L';
                            parts << " L " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        } else {
                            parts << QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        }
                    }
                    nodeAdded = true;
                }

                if (t != 'L') {
                    t = 'L';
                    parts << " L " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                } else {
                    parts << QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                }

                if (!foundInLine && pathPoint == initPoint) { // New node was added between LINE point and CURVE node
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                        qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for newNode";
                    #endif

                    ignoreData = true;

                    QPair<QPointF, QPointF> elements;
                    QPointF curveToElement;
                    QPointF dataToElement;
                    if (nodeType == CurveNode) { // Adding CURVE node
                        // Calculating new C1/C2 values for the new node
                        elements = getCurveElements(initPoint, newNode);
                        // Adding CurveToElement
                        curveToElement = elements.first;
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Adding CurveToDataElement
                        dataToElement = elements.second;
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";

                        // Adding NewNode element (CurveToDataElement)
                        parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";

                        // ---
                        // *** After Node
                        // Recalculating C1/C2 values for the node after the new one
                        #ifdef TUP_DEBUG
                            qDebug() << "---";
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                            qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for node after the newNode";
                        #endif

                        elements = getCurveElements(newNode, endPoint);
                        curveToElement = elements.first;
                        // Adding CurveToElement
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Creating CurveToDataElement
                        dataToElement = elements.second;
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";
                    } else { // Adding LineNode
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding LINE node";
                        #endif

                        if (t != 'L') {
                            t = 'L';
                            parts << " L " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        } else {
                            parts << QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        }

                        // ---
                        // *** After Node
                        // Recalculating C1/C2 values for the node after the new one
                        elements = getCurveElements(newNode, endPoint);
                        curveToElement = elements.first;
                        // Adding CurveToElement
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Creating CurveToDataElement
                        dataToElement = elements.second;
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";

                    }
                    nodeAdded = true;
                }
            }
            break;
            case QPainterPath::CurveToElement:
            {
                dataCounter = 0;

                if (!ignoreData) {
                    if (t != 'C') {
                        t = 'C';
                        parts << " C " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    } else {
                        parts << "  " + QString::number(pathPoint.x()) + " " + QString::number(pathPoint.y()) + " ";
                    }
                    previewPoint = pathPoint;
                }
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                if (!foundInLine && !flatCurveFlag) {
                    if (pathPoint == initPoint && dataCounter == 1) { // Insert the new node in this segment
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding node at CURVE segment of the path...";
                        #endif

                        ignoreData = true;

                        // *** Previous Node
                        // Adding the second CurveToDataElement component of the previous node
                        if (t == 'C')
                            parts << " " + QString::number(pathPoint.x()) + "  " + QString::number(pathPoint.y()) + " ";

                        QPair<QPointF, QPointF> elements;
                        QPointF curveToElement;
                        QPointF dataToElement;
                        if (nodeType == CurveNode) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                                qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for newNode";
                            #endif

                            // Calculating new C1/C2 values for the new node
                            elements = getCurveElements(initPoint, newNode);
                            // Adding CurveToElement
                            curveToElement = elements.first;
                            if (t != 'C') {
                                t = 'C';
                                parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                            } else {
                                parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                            }

                            // Adding CurveToDataElement
                            dataToElement = elements.second;
                            parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";

                            // Adding NewNode element (CurveToDataElement)
                            parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                        } else { // Adding LineNode
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Adding LINE node";
                            #endif

                            if (t != 'L') {
                                t = 'L';
                                parts << " L " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                            } else {
                                parts << QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                            }
                        }
                        nodeAdded = true;

                        // ---
                        // *** After Node
                        // Recalculating C1/C2 values for the node after the new one
                        #ifdef TUP_DEBUG
                            qDebug() << "---";
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding CURVE node";
                            qDebug() << "[TupPathItem::addInnerNode()] - Getting C1/C2 for node after the newNode";
                        #endif

                        elements = getCurveElements(newNode, endPoint);
                        curveToElement = elements.first;
                        // Adding CurveToElement
                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        } else {
                            parts << "  " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
                        }

                        // Creating CurveToDataElement
                        dataToElement = elements.second;
                        parts << " " + QString::number(dataToElement.x()) + "  " + QString::number(dataToElement.y()) + " ";
                    } else if (pathPoint == endPoint) { // Adding the node after the new node
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding node after the new node";
                        #endif

                        if (t == 'C')
                            parts << " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";

                        ignoreData = false;
                    } else {
                        if (!ignoreData) { // Saving data from other segments
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupPathItem::addInnerNode()] - Saving data from other segments";
                            #endif

                            if (t == 'C')
                                parts << " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";
                            if (dataCounter == 1)
                                nodesCounter++;
                        }
                    }
                } else { // It's flat curve or it's part of other segment
                    if (flatCurveFlag && pathPoint == initPoint && dataCounter == 1) { // Flat curve
                        parts << "  " + QString::number(e.x) + " " + QString::number(e.y) + " ";
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Adding node at FLAT CURVE segment of the path...";
                        #endif

                        if (t != 'C') {
                            t = 'C';
                            parts << " C " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        } else {
                            parts << "  " + QString::number(newNode.x()) + " " + QString::number(newNode.y()) + " ";
                        }

                        parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";
                        // Adding NewNode element (CurveToDataElement)
                        parts << " " + QString::number(newNode.x()) + "  " + QString::number(newNode.y()) + " ";

                        nodeAdded = true;
                    } else { // Saving data from other segments
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupPathItem::addInnerNode()] - Saving data from other segments";
                        #endif

                        if (t == 'C')
                            parts << " " + QString::number(e.x) + "  " + QString::number(e.y) + " ";
                        if (dataCounter == 1)
                            nodesCounter++;
                    }
                }

                dataCounter++;
            }
            break;
        }
    }

    if (!nodeAdded) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::addInnerNode()] - Warning: Node wasn't added at ->" << newNode;
        #endif
    }

    foreach(QString line, parts)
        pathStr += line;

    setPathFromString(pathStr);

    return pathStr;
}

// This method calculates C1/C2 for a curve

QPair<QPointF, QPointF> TupPathItem::getCurveElements(QPointF initPos, QPointF endPos)
{
    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupPathItem::getCurveElements()] - initPos ->" << initPos;
        qDebug() << "[TupPathItem::getCurveElements()] - endPos ->" << endPos;
    #endif

    QPainterPath route = path();
    QPolygonF points = route.toFillPolygon();

    QPointF firstPoint;
    QPointF secondPoint;

    bool startSaving = false;
    QList<QPointF> range;
    int total = points.size();
    for(int i=0; i<total-1; i++) {
        if (pointIsContainedBetweenRange(points.at(i), points.at(i+1), initPos, 0)) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::getCurveElements()] - initPos found ->" << initPos;
            #endif
            startSaving = true;
        } else {
            if (pointIsContainedBetweenRange(points.at(i), points.at(i+1), endPos, 0)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPathItem::getCurveElements()] - endPos found ->" << endPos;
                #endif
                startSaving = false;
            } else {
                if (startSaving)
                    range << points.at(i);
            }
        }
    }

    if (!range.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::getCurveElements()] - range.size() ->" << range.size();
        #endif

        int size = range.size();
        if (size == 1) {
            firstPoint = range.at(0);
            secondPoint = range.at(0);
        } else if (size == 2) {
            firstPoint = range.at(0);
            secondPoint = range.at(1);
        } else {
            int step = size / 3;
            firstPoint = range.at(step);
            secondPoint = range.at(step*2);
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::getCurveElements()] - Warning: range.size() -> 0";
        #endif
        // Making up the missing points
        QPointF segment = endPos - initPos;
        double stepX = segment.x()/3;
        double stepY = segment.y()/3;

        firstPoint = initPos + QPointF(stepX, stepY);
        secondPoint = initPos + QPointF(stepX*2, stepY*2);
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::getCurveElements()] - range ->" << range;
        qDebug() << "[TupPathItem::getCurveElements()] - firstPoint ->" << firstPoint;
        qDebug() << "[TupPathItem::getCurveElements()] - secondPoint ->" << secondPoint;
    #endif

    return QPair<QPointF, QPointF>(firstPoint, secondPoint);
}

// Method for short distances

bool TupPathItem::pointIsContainedBetweenRange(const QPointF &point1, const QPointF &point2,
                                               const QPointF &newPoint, double tolerance)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pointIsContainedBetweenRange()]";
    #endif
    */

    if (point1.x() <= newPoint.x() && newPoint.x() < point2.x()) {
        if (point1.y() <= newPoint.y() && newPoint.y() < point2.y())
            return true;

        if (point2.y() < newPoint.y() && newPoint.y() <= point1.y())
            return true;

        // Measuring distance between newPoint and path
        qreal distance = TAlgorithm::distanceFromLine(point1, point2, newPoint);
        if (distance <= tolerance)
            return true;
    }

    if (point2.x() < newPoint.x() && newPoint.x() <= point1.x()) {
        if (point1.y() <= newPoint.y() && newPoint.y() < point2.y())
            return true;

        if (point2.y() < newPoint.y() && newPoint.y() <= point1.y())
            return true;

        // Measuring distance between newPoint and path
        qreal distance = TAlgorithm::distanceFromLine(point1, point2, newPoint);
        if (distance <= tolerance)
            return true;
    }

    // Line is vertical
    if (point1.y() <= newPoint.y() && newPoint.y() < point2.y()) {
        if (point1.x() <= newPoint.x() && newPoint.x() < point2.x())
            return true;

        if (point2.x() < newPoint.x() && newPoint.x() <= point1.x())
            return true;

        // Measuring distance between newPoint and path
        qreal distance = TAlgorithm::distanceFromLine(point1, point2, newPoint);
        if (distance <= tolerance)
            return true;
    }

    if (point2.y() < newPoint.y() && newPoint.y() <= point1.y()) {
        if (point1.x() <= newPoint.x() && newPoint.x() < point2.x())
            return true;

        if (point2.x() < newPoint.x() && newPoint.x() <= point1.x())
            return true;

        // Measuring distance between newPoint and path
        qreal distance = TAlgorithm::distanceFromLine(point1, point2, newPoint);
        if (distance <= tolerance)
            return true;
    }

    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pointIsContainedBetweenRange()] - Warning: Point is out of scope!";
    #endif
    */

    return false;
}

// This method verifies if the cutting point is between nodes of a curve

bool TupPathItem::isPointContainedBetweenCurveNodes(const QPointF &node1, const QPointF &node2,
                                                    const QPointF &cuttingPoint, double tolerance)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::isPointContainedBetweenCurveNodes()]";
    #endif

    QPainterPath route = path();
    QPolygonF points = route.toFillPolygon();

    double distance;
    bool startSaving = false;
    QList<QPointF> range;
    int total = points.size();
    for(int i=0; i<total-1; i++) {
        if (node1 == points.at(i)) {
            startSaving = true;
        } else {
            if (node2 == points.at(i)) {
                startSaving = false;
            } else {
                if (startSaving) {
                    if (i>0) {
                        double innerDistance = TAlgorithm::distance(points.at(i-1), points.at(i));
                        if (innerDistance > tolerance) {
                            int times = innerDistance / tolerance;
                            double deltaX = (points.at(i).x() - points.at(i-1).x()) / times;
                            double deltaY = (points.at(i).y() - points.at(i-1).y()) / times;
                            for (int k=1; k<=times; k++) {
                                QPointF middlePoint(points.at(i-1).x() + (deltaX * k), points.at(i-1).y() + (deltaY * k));
                                range << middlePoint;
                            }
                        }
                    }
                    range << points.at(i);
                }
            }
        }
    }

    total = range.size();
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::isPointContainedBetweenCurveNodes()] - range.size() ->" << total;
    #endif
    if (total == 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::isPointContainedBetweenCurveNodes()] - Warning: range of points between nodes is empty!";
        #endif

        return false;
    }

    for(int i=0; i<total; i++) {
        distance = TAlgorithm::distance(range.at(i), cuttingPoint);
        if (distance <= tolerance) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::isPointContainedBetweenCurveNodes()] - Point was FOUND! ->" << cuttingPoint;
            #endif

            return true;
        }
    }

    return false;
}

bool TupPathItem::isPointContainedBetweenFlatNodes(const QPointF &linePoint1, const QPointF &linePoint2,
                                                   const QPointF &cuttingPoint, int tolerance, bool eraserMode)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()]";
    #endif

    QPointF node1 = linePoint1;
    QPointF node2 = linePoint2;

    if (eraserMode) {
        node1 = linePoint1 - QPointF(tolerance, tolerance);
        node2 = linePoint2 + QPointF(tolerance, tolerance);
    }

    if (node1.x() <= cuttingPoint.x() && cuttingPoint.x() <= node2.x()) { // Node1.x < Node2.x
        if (node1.y() <= cuttingPoint.y() && cuttingPoint.y() <= node2.y()) { // Node1.y < Node2.y
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()] - cuttingPoint is contained!";
            #endif

            // Measuring distance between point and line (linePoint1 - linePoint2)
            qreal distance = TAlgorithm::distanceFromLine(linePoint1, linePoint2, cuttingPoint);
            if (distance <= tolerance)
                return true;
        }

        if (eraserMode) {
            node1 = linePoint1 + QPointF(tolerance, tolerance);
            node2 = linePoint2 - QPointF(tolerance, tolerance);
        }

        if (node2.y() <= cuttingPoint.y() && cuttingPoint.y() <= node1.y()) { // Node2.y < Node1.y
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()] - cuttingPoint is contained!";
            #endif

            // Measuring distance between point and line (point1-point2)
            qreal distance = TAlgorithm::distanceFromLine(linePoint1, linePoint2, cuttingPoint);
            if (distance <= tolerance)
                return true;
        }
    }

    if (eraserMode) {
        node1 = linePoint1 + QPointF(tolerance, tolerance);
        node2 = linePoint2 - QPointF(tolerance, tolerance);
    }

    if (node2.x() <= cuttingPoint.x() && cuttingPoint.x() <= node1.x()) { // Node2.x < Node1.x
        if (node2.y() <= cuttingPoint.y() && cuttingPoint.y() <= node1.y()) { // Node2.y < Node1.y
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()] - Point is contained!";
            #endif

            // Measuring distance between point and line (point1-point2)
            qreal distance = TAlgorithm::distanceFromLine(linePoint1, linePoint2, cuttingPoint);
            if (distance <= tolerance)
                return true;
        }

        if (eraserMode) {
            node1 = linePoint1 - QPointF(tolerance, tolerance);
            node2 = linePoint2 + QPointF(tolerance, tolerance);
        }

        if (node1.y() <= cuttingPoint.y() && cuttingPoint.y() <= node2.y()) { // Node1.y < Node2.y
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()] - Point is contained!";
            #endif

            // Measuring distance between point and line (point1-point2)
            qreal distance = TAlgorithm::distanceFromLine(linePoint1, linePoint2, cuttingPoint);
            if (distance <= tolerance)
                return true;
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::isPointContainedBetweenFlatNodes()] - Warning: Point is NOT contained!";
    #endif

    return false;
}

// This method is for debugging tasks
QList<QPointF> TupPathItem::keyNodes()
{
    QPainterPath route = path();
    int total = route.elementCount();
    QList<QPointF> points;
    int dataCounter = -1;

    colors.clear();
    tips.clear();

    for(int i=0; i < total; i++) {
        QPainterPath::Element e = route.elementAt(i);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                points << QPointF(e.x, e.y);
                colors << QColor(255, 0, 0);
                tips << "MoveToElement - Red";
            }
            break;
            case QPainterPath::LineToElement:
            {
                points << QPointF(e.x, e.y);
                colors << QColor(0, 0, 0);
                tips << "LineToElement - Black";
            }
            break;
            case QPainterPath::CurveToElement:
            {
                points << QPointF(e.x, e.y);
                colors << QColor(0, 255, 0);
                tips << "CurveToElement - Green";
                dataCounter = 0;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                points << QPointF(e.x, e.y);
                if (dataCounter == 0) {
                    colors << QColor(144, 194, 231);
                    tips << "CurveToDataElement - Blue";
                } else {
                    colors << QColor(230, 150, 0);
                    tips << "Curve Node - Orange";
                }
                dataCounter++;
            }
            break;
        }
    }

    return points;
}

// This method is for debugging tasks
QList<QColor> TupPathItem::nodeColors()
{
    return colors;
}

// This method is for debugging tasks
QList<QString> TupPathItem::nodeTips()
{
    return tips;
}

QString TupPathItem::appendNode(const QPointF &pos)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::appendNode()] - pos ->" << pos;
    #endif

    QPainterPath qPath = path();
    QPointF lastPoint = qPath.pointAtPercent(1);
    QString pathStr = pathToString();

    double stepX = (pos.x() - lastPoint.x()) / 3;
    double stepY = (pos.y() - lastPoint.y()) / 3;

    QPointF curveToElement = QPointF(lastPoint.x() + stepX, lastPoint.y() + stepY);
    QPointF c1 = QPointF(lastPoint.x() + (stepX*2), lastPoint.y() + (stepY*2));

    pathStr += " C " + QString::number(curveToElement.x()) + " " + QString::number(curveToElement.y()) + " ";
    pathStr += "  " + QString::number(c1.x()) + " " + QString::number(c1.y()) + " ";
    pathStr += "  " + QString::number(pos.x()) + " " + QString::number(pos.y()) + " ";

    setPathFromString(pathStr);

    return pathStr;
}

QPair<QPointF,QPointF> TupPathItem::calculateEndPathCPoints(const QPointF &pos)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::calculateEndPathCPoints()] - pos ->" << pos;
    #endif

    QPainterPath qPath = path();
    QPointF lastPoint = qPath.pointAtPercent(1);

    return calculateCPoints(lastPoint, pos);
}

QPair<QPointF,QPointF> TupPathItem::calculateCPoints(const QPointF &pos1, const QPointF &pos2)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::calculateCPoints()] - pos1 ->" << pos1;
        qDebug() << "[TupPathItem::calculateCPoints()] - pos2 ->" << pos2;
    #endif

    QPointF c1;
    QPointF c2;

    double distance = TAlgorithm::distance(pos1, pos2);
    double delta = distance * 0.2;

    double x = pos1.x() + ((pos2.x() - pos1.x()) / 2);
    double y = pos1.y() + ((pos2.y() - pos1.y()) / 2);

    QPointF middlePoint = QPointF(x, y);
    double slope = TAlgorithm::inverseSlope(pos1, pos2);

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::calculateCPoints()] - delta ->" << delta;
        qDebug() << "[TupPathItem::calculateCPoints()] - slope ->" << slope;
    #endif

    if (fabs(slope) < 10) {
        double b = TAlgorithm::calculateBFromLine(middlePoint, slope);

        double x1 = middlePoint.x() - delta;
        double x2 = middlePoint.x() + delta;

        double y1 = TAlgorithm::calculateYFromLine(x1, slope, b);
        double y2 = TAlgorithm::calculateYFromLine(x2, slope, b);

        c1 = QPointF(x1, y1);
        c2 = QPointF(x2, y2);
    } else {
        double deltaX = (pos2.x() - pos1.x())/3;
        double y1 = pos1.y() - delta;
        double y2 = pos1.y() + delta;

        c1 = QPointF(pos1.x() + deltaX, y1);
        c2 = QPointF(pos1.x() + (deltaX*2), y2);
    }

    return QPair<QPointF, QPointF>(c1, c2);
}

QPair<QPointF,QPointF> TupPathItem::calculatePlainCPoints(const QPointF &pos1, const QPointF &pos2)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::calculatePlainCPoints()] - pos1 ->" << pos1;
        qDebug() << "[TupPathItem::calculatePlainCPoints()] - pos2 ->" << pos2;
    #endif

    double xDelta = (pos2.x() - pos1.x()) / 3;
    double yDelta = (pos2.x() - pos1.x()) / 3;

    double x1 = pos1.x() + xDelta;
    double y1 = pos1.y() + yDelta;

    double x2 = pos1.x() + (xDelta * 2);
    double y2 = pos1.y() + (yDelta * 2);

    QPointF c1(x1, y1);
    QPointF c2(x2, y2);

    return QPair<QPointF, QPointF>(c1, c2);
}

QPainterPath TupPathItem::clearPath(int tolerance)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::clearPath()] - tolerance ->" << tolerance;
    #else
        Q_UNUSED(tolerance)
    #endif

    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    int step = elementsTotal/50;

    QPointF previewPoint;
    QPointF currentPoint;
    QPointF nextPoint;

    int curveCounter = 0;
    bool enableData = false;

    // First clearing process
    QPainterPath::Element e = route.elementAt(0);
    QList<QPainterPath::Element> elements;
    elements << e;

    for(int i=2; i<elementsTotal-1; i++) {
        QPainterPath::Element e = route.elementAt(i);
        currentPoint = QPointF(e.x, e.y);

        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                // qDebug() << "QPainterPath::MoveToElement ->" << i;
            }
            break;
            case QPainterPath::LineToElement:
            {
                if (i % step == 0)
                    elements << e;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                if (i % step == 0) {
                    elements << e;
                    enableData = true;
                    curveCounter = 0;
                }
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                if (enableData) {
                    elements << e;
                    curveCounter++;
                    if (curveCounter == 2)
                        enableData = false;
                }
            }
            break;
        }
    }

    elements << route.elementAt(route.elementCount()-1);

    // Second clearing process

    QChar t;
    e = elements.at(0);
    QPainterPath qPath(QPointF(e.x, e.y));
    previewPoint = QPointF(e.x, e.y);

    double slope;
    double newSlope;
    double diff;

    elementsTotal = elements.size();
    for(int i=1; i<elementsTotal-1; i++) {
        e = elements.at(i);
        currentPoint = QPointF(e.x, e.y);
        e = elements.at(i+1);
        nextPoint = QPointF(e.x, e.y);

        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                // qDebug() << "QPainterPath::MoveToElement ->" << i;
            }
            break;
            case QPainterPath::LineToElement:
            {
                slope = TAlgorithm::slope(previewPoint, currentPoint);
                newSlope = TAlgorithm::slope(currentPoint, nextPoint);
                diff = fabs(newSlope - slope);

                if (diff > 0.2) {
                    QPair<QPointF,QPointF> cPoints = calculateCPoints(previewPoint, currentPoint);
                    curvePoints << cPoints;
                    qPath.cubicTo(currentPoint, currentPoint, currentPoint);
                }
            }
            break;
            case QPainterPath::CurveToElement:
            {
                // qDebug() << "QPainterPath::CurveToElement ->" << i;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                // qDebug() << "QPainterPath::CurveToDataElement -> " << i;
            }
            break;
        }

        previewPoint = currentPoint;
    }

    return qPath;
}

QPainterPath TupPathItem::brushPath(const QPainterPath &route, int tolerance)
{
    QPolygonF points = route.toFillPolygon();
    points.removeLast();
    int total = points.size();

    QPainterPath qPath(points.at(0));
    qPath.lineTo(points.at(1));
    double slope = TAlgorithm::slope(points.at(0), points.at(1));

    for(int i=2; i<total-1; i++) {
        double newSlope = TAlgorithm::slope(points.at(i-1), points.at(i));
        double distance = TAlgorithm::distance(points.at(i-1), points.at(i));
        double diff = fabs(newSlope - slope);

        if ((diff > 0.2) && (distance > (tolerance*2))) {
            qPath.lineTo(points.at(i));
            slope = newSlope;
        }
    }

    qPath.lineTo(points.last());

    return qPath;
}

int TupPathItem::nodesCount()
{
    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    int counter = 0;

    for(int i=0; i<elementsTotal; i++) {
        QPainterPath::Element e = route.elementAt(i);
        switch (e.type) {
            case QPainterPath::MoveToElement:
            {
                counter++;
            }
            break;
            case QPainterPath::LineToElement:
            {
                counter++;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                counter++;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
            }
            break;
        }
    }

    return counter;
}

QList<QPair<QPointF,QPointF>> TupPathItem::getCPoints()
{
    return curvePoints;
}

bool TupPathItem::pathIsTooShort(const QPointF &cuttingPoint, int tolerance)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::pathIsTooShort()] - cuttingPoint ->" << cuttingPoint;
        qDebug() << "[TupPathItem::pathIsTooShort()] - tolerance ->" << tolerance;
    #endif

    if (pathPoints.size() == 2) { // Segment only has two nodes
        double dx = (pathPoints.first().x() - pathPoints.last().x()) / 2;
        double dy = (pathPoints.first().y() - pathPoints.last().y()) / 2;
        QPointF middlePoint = QPointF(pathPoints.first().x() + dx, pathPoints.first().y() + dy);

        // If line is too short, must be removed
        if (TAlgorithm::distance(cuttingPoint, middlePoint) <= tolerance &&
            TAlgorithm::distance(pathPoints.first(), pathPoints.last()) <= tolerance) {
            // Segment is too short, must be removed
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::pathIsTooShort()] - Path is too short. Must be removed!";
            #endif
            return true;
        }
    }

    return false;
}

QPair<QString, QString> TupPathItem::recalculatePath(const QPointF &cuttingPoint, int tolerance)
{
    QPainterPath route = path();
    int elementsTotal = route.elementCount();
    QPointF previewPoint;

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::recalculatePath()] - path string ->" << pathToString();
        qDebug() << "[TupPathItem::recalculatePath()] - cuttingPoint ->" << cuttingPoint;
    #endif

    generatePathPoints(route, tolerance);

    int nodeIndex = 0;
    int curveDataCounter = 0;
    CuttingPointType cuttingType = None;

    QPointF c0, c1, c2;
    t1 = 'X';
    t2 = 'X';
    QStringList parts1;
    QStringList parts2;
    QString pathStr1 = "";
    QString pathStr2 = "";
    QPointF newPoint;
    bool M_IsBeingRemovedAtBeginningOfNotFlatCurve = false;

    // Detecting the coordinates where eraser intercepts the path
    for(int elementCounter=0; elementCounter<elementsTotal; elementCounter++) {
        // qDebug() << "[TupPathItem::recalculatePath()] - Processing next element...";
        QPainterPath::Element e = route.elementAt(elementCounter);
        QPointF pathPoint = QPointF(e.x, e.y);
        nodeIndex++;
        switch (e.type) {                
            case QPainterPath::MoveToElement: // Starting point of the path
            {
                #ifdef TUP_DEBUG
                    qDebug() << "---";
                    qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - nodeIndex ->" << nodeIndex;
                    qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - pathPoint ->" << pathPoint;
                #endif
                previewPoint = pathPoint;

                if (cuttingType == None) {
                    // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Looking for cutting point";
                    double distance = TAlgorithm::distance(pathPoint, cuttingPoint);
                    if (distance <= tolerance) {
                        // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Cutting point is at the beginning of the path (M)";
                        // If path is shorter than eraser size must be removed
                        if (pathIsTooShort(cuttingPoint, tolerance)) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Returning empty strings!";
                            return QPair<QString, QString>("", "");
                        }

                        QPainterPath::Element eNext1 = route.elementAt(elementCounter+1);
                        // Checking if next node is a curve
                        if (eNext1.type == QPainterPath::CurveToElement) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Next element is a CURVE!";
                            QPainterPath::Element eNext2 = route.elementAt(elementCounter+2);
                            QPainterPath::Element eNext3 = route.elementAt(elementCounter+3);
                            QPointF eNextPoint1 = QPointF(eNext1.x, eNext1.y);
                            QPointF eNextPoint2 = QPointF(eNext2.x, eNext2.y);
                            QPointF eNextPoint3 = QPointF(eNext3.x, eNext3.y);
                            // Checking if next node is a flat curve
                            if (eNextPoint1 == eNextPoint2 && eNextPoint2 == eNextPoint3) {
                                // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Next element is a FLAT curve!";
                                int total = pathPoints.size()-1;

                                bool found = false;
                                for (int pathPointIndex=0; pathPointIndex<total; pathPointIndex++) {
                                    if (TAlgorithm::distance(pathPoints.at(pathPointIndex), cuttingPoint) > tolerance) {                                        
                                        cuttingType = Begin;
                                        pathBlocked = true;
                                        found = true;

                                        // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Cutting point WAS found as part of a flat curve!";
                                        // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Adding M to the path (A)";

                                        parts1 << mToString(FirstSegment, pathPoints.at(pathPointIndex));
                                        break;
                                    }
                                }

                                if (!found) {
                                    #ifdef TUP_DEBUG
                                        qDebug() << "[TupPathItem::recalculatePath()] - WARNING: Cutting point couldn't be FOUND at the beginning of path (flat curve)";
                                    #endif
                                    parts1 << mToString(FirstSegment, pathPoint);
                                }
                            } else { // M is the first coord of the curve (no flat)
                                // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - M is the first coord of the CURVE (no flat)";
                                M_IsBeingRemovedAtBeginningOfNotFlatCurve = true;
                            }
                        } else { // Next element is a line
                            // The starting point of the path has been removed
                            // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - The starting point of the path has been removed";
                            cuttingType = Begin;
                            pathBlocked = true;

                            bool found = false;
                            // Beginning of the line has been removed. Finding new first point.
                            for (int pathPointIndex=0; pathPointIndex<pathPoints.size(); pathPointIndex++) {
                                if (TAlgorithm::distance(pathPoints.at(pathPointIndex), cuttingPoint) > tolerance) {
                                    newPoint = pathPoints.at(pathPointIndex);
                                    // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Adding M to the path (B)";
                                    parts1 << mToString(FirstSegment, pathPoints.at(pathPointIndex));
                                    found = true;
                                    break;
                                }
                            }

                            if (!found) {
                                // qDebug() << "[TupPathItem::recalculatePath()] - WARNING: Cutting point couldn't be FOUND at the beginning of path (line)";
                                // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Adding original M point";
                                parts1 << mToString(FirstSegment, pathPoint);
                            }
                        }
                    } else { // Storing original element
                        // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Storing original element (A)";
                        parts1 << mToString(FirstSegment, pathPoint);
                    }
                } else { // Storing original element
                    // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - Storing original element (B)";
                    parts1 << mToString(FirstSegment, pathPoint);
                }

                // qDebug() << "[TupPathItem::recalculatePath()] - MoveToElement - M_IsBeingRemovedAtBeginningOfNoFlatCurve ->" << M_IsBeingRemovedAtBeginningOfNotFlatCurve;
            }
            break;
            case QPainterPath::LineToElement:
            {
                #ifdef TUP_DEBUG
                    qDebug() << "---";
                    qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - nodeIndex ->" << nodeIndex;
                    qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - pathPoint ->" << pathPoint;
                #endif

                if (cuttingType == None) {
                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Looking for cutting point";
                    // Check if limit point is contained in this straight segment
                    if (isPointContainedBetweenFlatNodes(previewPoint, pathPoint, cuttingPoint,
                                                         tolerance, true)) {
                        // If path is shorter than eraser size must be removed
                        if (pathIsTooShort(cuttingPoint, tolerance)) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Returning empty strings!";
                            return QPair<QString, QString>("", "");
                        }

                        if (nodeIndex == elementsTotal) {
                            if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                // Ending segment of the path is being removed
                                int total = pathPoints.size()-1;

                                bool found = false;
                                for (int pathPointCounter=total; pathPointCounter>=0; pathPointCounter--) {
                                    if (TAlgorithm::distance(pathPoints.at(pathPointCounter), cuttingPoint) > tolerance) {
                                        // The last point of the line is being removed
                                        if (pathPointCounter == 0) // The distance of the path is zero
                                            return QPair<QString, QString>("", "");

                                        cuttingType = End;
                                        pathBlocked = true;
                                        found = true;
                                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding LINE to path - part1";
                                        parts1 << lineToString(FirstSegment, pathPoints.at(pathPointCounter));

                                        break;
                                    }
                                }

                                if (!found) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - WARNING: Cutting point couldn't be FOUND at the beginning of path (line)";
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding original line to part1";
                                    parts1 << lineToString(FirstSegment, pathPoint);
                                }
                            } else {
                                // Middle point of the segment is being removed
                                QPair<QList<QPointF>, QList<QPointF>>
                                    lines = splitStraightLine(previewPoint, pathPoint, cuttingPoint, tolerance);

                                cuttingType = Middle;
                                pathBlocked = true;

                                QList<QPointF> line1 = lines.first;
                                QList<QPointF> line2 = lines.second;

                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - line1.size() ->" << line1.size();
                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - line2.size() ->" << line2.size();

                                if (line1.size() == 2) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding LINE to path - part1 - line1";
                                    parts1 << lineToString(FirstSegment, line1.at(1));
                                } else {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Warning: path 1 line-component wasn't added!";
                                }

                                if (line2.size() == 2) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding LINE to path - part2 - line2";
                                    parts2 << mToString(SecondSegment, line2.at(0));
                                    parts2 << lineToString(SecondSegment, line2.at(1));
                                } else {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Warning: path 2 line-component wasn't added!";
                                }
                            }
                        } else {
                            // Eraser is closer to the key point of the line
                            if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                cuttingType = Middle;
                                pathBlocked = true;

                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding LINE to path - part1 - newNode";
                                parts1 << lineToString(FirstSegment, newNode);
                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding LINE to path - part2 - pathPoint";
                                parts2 << mToString(SecondSegment, pathPoint);
                            } else { // Middle point of the segment is being removed
                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Middle point of the segment is being removed";

                                QPair<QList<QPointF>, QList<QPointF>>
                                    lines = splitStraightLine(previewPoint, pathPoint, cuttingPoint, tolerance);

                                cuttingType = Middle;
                                pathBlocked = true;

                                QList<QPointF> line1 = lines.first;
                                QList<QPointF> line2 = lines.second;

                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - line1.size() ->" << line1.size();
                                // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - line2.size() ->" << line2.size();

                                if (line1.size() == 2) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts1 - Adding lineToString()";
                                    parts1 << lineToString(FirstSegment, line1.at(1));
                                } else {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Warning: path 1 line-component wasn't added!";
                                }

                                if (line2.size() == 0) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts2 - Adding M - pathPoint";
                                    parts2 << mToString(SecondSegment, pathPoint);
                                } else if (line2.size() == 1) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts2 - Adding M - line2.at(0)";
                                    parts2 << mToString(SecondSegment, line2.at(0));
                                } else if (line2.size() == 2) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts2 - Adding M - line2.at(0)";
                                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts2 - Adding lineToString - line2.at(1)";
                                    parts2 << mToString(SecondSegment, line2.at(0));
                                    parts2 << lineToString(SecondSegment, line2.at(1));
                                }
                            }
                        }
                    } else { // Line segment wasn't affected by the eraser
                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Storing original element (A)";
                        parts1 << lineToString(FirstSegment, pathPoint);
                    }
                } else { // Line segment wasn't affected by the eraser
                    // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Storing original element (B)";

                    if (cuttingType == Begin || cuttingType == End) {
                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding part1 - Begin/End";
                        parts1 << lineToString(FirstSegment, pathPoint);
                    } else { // Middle cut                        
                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding part2 - Middle Cut";
                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - parts2.size() ->" << parts2.size();
                        if (parts2.size() == 0) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding M";
                            QPainterPath::Element previousE = route.elementAt(elementCounter-1);
                            QPointF previousPoint = QPointF(previousE.x, previousE.y);
                            parts2 << mToString(SecondSegment, previousPoint);
                        }

                        // qDebug() << "[TupPathItem::recalculatePath()] - LineToElement - Adding lineToString - pathPoint";
                        parts2 << lineToString(SecondSegment, pathPoint);
                    }
                }

                previewPoint = pathPoint;
            }
            break;
            case QPainterPath::CurveToElement:
            {
                #ifdef TUP_DEBUG
                    qDebug() << "---";
                    qDebug() << "[TupPathItem::recalculatePath()] - CurveToElement - nodeIndex ->" << nodeIndex;
                    qDebug() << "[TupPathItem::recalculatePath()] - CurveToElement - pathPoint ->" << pathPoint;
                #endif
                curveDataCounter = 0;
                c0 = pathPoint;
            }
            break;
            case QPainterPath::CurveToDataElement:
            {
                #ifdef TUP_DEBUG
                    qDebug() << "---";
                    qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - nodeIndex ->" << nodeIndex;
                    qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - pathPoint ->" << pathPoint;
                #endif

                if (curveDataCounter == 0) {
                    // First curve data point
                    c1 = pathPoint;
                } else {
                    // Second curve data point
                    c2 = pathPoint;
                    previewPoint = pathPoint;

                    if (cuttingType == None || M_IsBeingRemovedAtBeginningOfNotFlatCurve) {
                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Looking for cutting point";

                        if (pathIsTooShort(cuttingPoint, tolerance)) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Returning empty strings!";
                            return QPair<QString, QString>("", "");
                        }

                        // Curve is a FLAT curve (straight line)
                        if (c0 == c1 && c1 == c2) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Curve is flat";
                            // Checking if point is close to the FLAT curve end (C1 == C2 == C3)
                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Checking if point is close to the flat curve end";
                            if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                // Ending segment of the FLAT curve is being removed
                                int total = pathPoints.size()-1;
                                if (nodeIndex == elementsTotal) {
                                    // Removing FLAT curve segment at the end of the path
                                    bool found = false;
                                    for (int pathPointCounter=total; pathPointCounter>=0; pathPointCounter--) {                                        
                                        if (TAlgorithm::distance(pathPoints.at(pathPointCounter), cuttingPoint) > tolerance) {
                                            if (pathPointCounter == 0) // The distance of the path is zero
                                                return QPair<QString, QString>("", "");

                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Cutting point FOUND at the end of the flat curve!";
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - c0 ->" << c0;
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - c1 ->" << c1;
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - c2 ->" << c2;

                                            cuttingType = End;
                                            pathBlocked = true;
                                            found = true;
                                            parts1 << curveToString(FirstSegment, {pathPoints.at(pathPointCounter), pathPoints.at(pathPointCounter), pathPoints.at(pathPointCounter)});

                                            break;
                                        }                                        
                                    }

                                    if (!found) {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - Fatal Error: Cutting point couldn't be FOUND at the beginning of path (curve)";
                                    }
                                } else {
                                    QPainterPath::Element previewE = route.elementAt(elementCounter-3);
                                    QPointF firstCurvePoint = QPointF(previewE.x, previewE.y);
                                    if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                        // Removing a NO FLAT curve node in the MIDDLE of the path!
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Removing a NO FLAT curve node in the MIDDLE of the path!";
                                        cuttingType = Middle;
                                        pathBlocked = true;

                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding M element / first line for part2";
                                        QPair<QList<QPointF>, QList<QPointF>> lines = splitStraightLine(firstCurvePoint,
                                                                                                        pathPoint,
                                                                                                        cuttingPoint,
                                                                                                        tolerance);
                                        // qDebug() << "[TupPathItem::recalculatePath()] - lines ->" << lines;
                                        QList<QPointF> line = lines.first;
                                        if (line.size() == 2) {
                                            // Adding FLAT CURVE element for the beginning of the first path
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - M";
                                            parts1 << curveToString(FirstSegment, {line.at(1), line.at(1), line.at(1)});

                                            QPainterPath::Element nextElement = route.elementAt(elementCounter+1);
                                            QPointF nextPoint = QPointF(nextElement.x, nextElement.y);
                                            if (nextElement.type == QPainterPath::CurveToElement) {
                                                // Adding curve at the beginning of the second path
                                                QPainterPath::Element nextC1 = route.elementAt(elementCounter+2);
                                                QPointF pointC1 = QPointF(nextC1.x, nextC1.y);
                                                QPainterPath::Element nextC2 = route.elementAt(elementCounter+3);
                                                QPointF pointC2 = QPointF(nextC2.x, nextC2.y);

                                                QList<QList<QPointF>> bezierPaths = calculateBezierPaths(pathPoint, nextPoint, pointC1, pointC2, true);
                                                QList<QPointF> startingCurve = shortenCurveFromStart(cuttingPoint, tolerance, bezierPaths);

                                                if (startingCurve.size() == 4) {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - startingCurve";
                                                    parts2 << mToString(SecondSegment, startingCurve.at(0));
                                                    parts2 << curveToString(SecondSegment, {startingCurve.at(1), startingCurve.at(2), startingCurve.at(3)});
                                                } else {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Curve at parts2 wasn't added!";
                                                }
                                                elementCounter += 3;
                                            } else { // Next point is a line
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding M element / first line for part2";
                                                QPair<QList<QPointF>, QList<QPointF>> lines = splitStraightLine(pathPoint,
                                                                                                                nextPoint,
                                                                                                                cuttingPoint,
                                                                                                                tolerance);

                                                // qDebug() << "[TupPathItem::recalculatePath()] - lines ->" << lines;
                                                QList<QPointF> line = lines.second;
                                                if (line.size() == 2) {
                                                    // Adding M element for the beginning of the second path
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - M";
                                                    parts2 << mToString(FirstSegment, line.at(0));
                                                } else {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Warning: Could NOT add M element!";
                                                }
                                            }
                                        } else {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Warning: Could NOT add M element!";
                                        }
                                    } else {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Cutting point is too far from the FLAT curve point";
                                    }
                                }
                            } else {
                                QPainterPath::Element previewE = route.elementAt(elementCounter-3);
                                QPointF firstCurvePoint = QPointF(previewE.x, previewE.y);

                                // qDebug() << "[TupPathItem::recalculatePath()] - Checking if the cutting point is part of the flat curve and the previous point";
                                // Checking if the cutting point is part of the flat curve and the previous point
                                if (isPointContainedBetweenFlatNodes(firstCurvePoint, pathPoint, cuttingPoint,
                                                                     tolerance, true)) {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Point was FOUND!";

                                    QPair<QList<QPointF>, QList<QPointF>>
                                        lines = splitStraightLine(firstCurvePoint, pathPoint, cuttingPoint, tolerance);

                                    cuttingType = Middle;
                                    pathBlocked = true;

                                    QList<QPointF> line1 = lines.first;
                                    QList<QPointF> line2 = lines.second;

                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - line1.size() ->" << line1.size();
                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - line2.size() ->" << line2.size();

                                    if (line1.size() == 2) {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - line1";
                                        parts1 << lineToString(FirstSegment, line1.at(1));
                                    }

                                    if (line2.size() == 2) {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - line2";
                                        parts2 << mToString(SecondSegment, line2.at(0));
                                        parts2 << lineToString(SecondSegment, line2.at(1));
                                    } else {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Warning: part2 wasn't added!";
                                    }
                                } else {
                                    // The flat curve wasn't affected by eraser
                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - The flat curve wasn't affected by eraser";
                                    parts1 << curveToString(FirstSegment, { c0, c1, c2 });
                                }
                            }
                        } else { // Curve is NOT flat
                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Curve is not FLAT";
                            if (M_IsBeingRemovedAtBeginningOfNotFlatCurve) {
                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Curve at the beginning of the path";
                                // Looking for cutting point at the beginning of the curve
                                QPainterPath::Element previewE = route.elementAt(elementCounter-3);
                                QPointF firstCurvePoint = QPointF(previewE.x, previewE.y);

                                QList<QList<QPointF>> bezierPaths = calculateBezierPaths(firstCurvePoint, c0, c1, c2, true);
                                cuttingType = Begin;
                                pathBlocked = true;
                                QList<QPointF> curve = shortenCurveFromStart(cuttingPoint, tolerance,
                                                                             bezierPaths);
                                if (curve.isEmpty()) {
                                    if (elementCounter < (elementsTotal-1)) {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Not LAST element";
                                        QString mString = mToString(FirstSegment, firstCurvePoint);
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Returning string path ->" << mString;
                                        // Returning last point of the curve as M
                                        parts1 << mString;
                                    } else {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Returning EMPTY paths!";
                                        return QPair<QString, QString>("", "");
                                    }
                                } else {
                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Curve NO empty";
                                    parts1 << mToString(FirstSegment, curve.at(0));
                                    parts1 << curveToString(FirstSegment, {curve.at(1), curve.at(2), curve.at(3)});
                                }
                                M_IsBeingRemovedAtBeginningOfNotFlatCurve = false;
                            } else {
                                QPainterPath::Element previewE = route.elementAt(elementCounter-3);
                                QPointF firstCurvePoint = QPointF(previewE.x, previewE.y);
                                if (nodeIndex == elementsTotal) {
                                    // Looking for cutting point in the end of the curve
                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Looking for cutting point in the end of the curve";
                                    if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                        // cuttingPoint is at the end of the curve
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - FOUND! cuttingPoint is at the end of the curve";

                                        cuttingType = End;
                                        pathBlocked = true;

                                        QList<QList<QPointF>> bezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                 c0, c1, c2);
                                        QList<QPointF> curve = shortenCurveFromEnd(cuttingPoint, tolerance,
                                                                                   bezierPaths);

                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - curve.size() ->" << curve.size();

                                        if (elementsTotal == 4) {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - elementsTotal == 4 - Adding M component";
                                            if (!parts1.at(0).startsWith("M"))
                                                parts1 << mToString(FirstSegment, curve.at(0));
                                        }

                                        if (curve.size() == 4) {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding curve component!";
                                            parts1 << curveToString(FirstSegment, {curve.at(1), curve.at(2), curve.at(3)});
                                        } else {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Fatal Error: Couldn't add curve element at part1!";
                                        }
                                    } else {
                                        // Looking for cutting point at some part of the last segment of the path
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement "
                                        //             "- Looking for cutting point at some part of the last segment of the path";

                                        QList<QPointF> setOfPoints = generateCurvePoints(firstCurvePoint, pathPoint);
                                        if (isPointPartOfTheCurve(cuttingPoint, tolerance, setOfPoints)) {
                                            cuttingType = End;
                                            pathBlocked = true;

                                            QList<QList<QPointF>> curve1BezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                           c0, c1, c2);
                                            QList<QPointF> curve1 = firstCurveOfSplit(cuttingPoint, tolerance,
                                                                                      curve1BezierPaths);

                                            QList<QList<QPointF>> curve2BezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                           c0, c1, c2, true);
                                            QList<QPointF> curve2 = secondCurveOfSplit(cuttingPoint, tolerance,
                                                                                       curve2BezierPaths);

                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - curve1.size() ->" << curve1.size();
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - curve2.size() ->" << curve2.size();

                                            if (curve1.size() == 4) {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - curve1";
                                                parts1 << curveToString(FirstSegment, {curve1.at(1), curve1.at(2), curve1.at(3)});
                                            } else {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Couldn't add curve component at parts1!";
                                            }

                                            if (curve2.size() == 4) {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - curve2";
                                                parts2 << mToString(SecondSegment, curve2.at(0));
                                                parts2 << curveToString(SecondSegment, {curve2.at(1), curve2.at(2), curve2.at(3)});
                                            } else {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Couldn't add curve component at parts2!";
                                            }

                                            if (curve1.size() == 0 && curve2.size() == 0) {
                                                // Cutting point is not part of the curve
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING! Cutting point is NOT part of the curve!";
                                                parts1 << curveToString(FirstSegment, { c0, c1, c2 });
                                            }
                                        } else {
                                            // Cutting point is not close to the curve
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Cutting point is not close to the curve!";
                                            cuttingType = None;
                                            pathBlocked = false;
                                            parts1 << curveToString(FirstSegment, {c0, c1, c2});
                                        }
                                    }
                                } else {
                                    // cuttingPoint is at the middle of a curve in the middle of the path
                                    if (TAlgorithm::distance(pathPoint, cuttingPoint) <= tolerance) {
                                        cuttingType = Middle;
                                        pathBlocked = true;

                                        QList<QList<QPointF>> bezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                 c0, c1, c2);
                                        QList<QPointF> curve1 = shortenCurveFromEnd(cuttingPoint, tolerance, bezierPaths);

                                        if (curve1.size() == 4) {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - curve1";
                                            parts1 << curveToString(FirstSegment, {curve1.at(1), curve1.at(2), curve1.at(3)});
                                        } else {
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Curve at parts1 wasn't added!";
                                        }

                                        QPainterPath::Element nextElement = route.elementAt(elementCounter+1);
                                        QPointF nextPoint = QPointF(nextElement.x, nextElement.y);
                                        if (nextElement.type == QPainterPath::CurveToElement) {
                                            // Adding curve at the beginning of the second path
                                            QPainterPath::Element nextC1 = route.elementAt(elementCounter+2);
                                            QPointF pointC1 = QPointF(nextC1.x, nextC1.y);
                                            QPainterPath::Element nextC2 = route.elementAt(elementCounter+3);
                                            QPointF pointC2 = QPointF(nextC2.x, nextC2.y);

                                            bezierPaths = calculateBezierPaths(pathPoint, nextPoint, pointC1, pointC2, true);
                                            QList<QPointF> startingCurve = shortenCurveFromStart(cuttingPoint, tolerance, bezierPaths);

                                            if (startingCurve.size() == 4) {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - startingCurve";
                                                parts2 << mToString(SecondSegment, startingCurve.at(0));
                                                parts2 << curveToString(SecondSegment, {startingCurve.at(1), startingCurve.at(2), startingCurve.at(3)});
                                            } else {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Curve at parts2 wasn't added!";
                                            }
                                            elementCounter += 3;
                                        } else { // Next point is a line
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding M element / first line for part2";
                                            QPair<QList<QPointF>, QList<QPointF>> lines = splitStraightLine(pathPoint,
                                                                                                            nextPoint,
                                                                                                            cuttingPoint,
                                                                                                            tolerance);

                                            // qDebug() << "[TupPathItem::recalculatePath()] - lines ->" << lines;
                                            QList<QPointF> line = lines.second;
                                            if (line.size() == 2) {
                                                // Adding M element for the beginning of the second path
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - M";
                                                parts2 << mToString(FirstSegment, line.at(0));
                                            } else {
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Warning: Could NOT add M element!";
                                            }
                                        }
                                    } else {
                                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement "
                                        //             "- cuttingPoint is at the middle of a curve in the middle of the path";
                                        QList<QPointF> setOfPoints = generateCurvePoints(firstCurvePoint, pathPoint);
                                        if (isPointPartOfTheCurve(cuttingPoint, tolerance, setOfPoints)) {
                                            QList<QList<QPointF>> curve1BezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                           c0, c1, c2);
                                            QList<QPointF> curve1 = firstCurveOfSplit(cuttingPoint, tolerance,
                                                                                      curve1BezierPaths);

                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - curve1.size() ->" << curve1.size();

                                            if (!curve1.isEmpty()) {
                                                cuttingType = Middle;
                                                pathBlocked = true;

                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - FOUND in the middle!";

                                                QList<QList<QPointF>> curve2BezierPaths = calculateBezierPaths(firstCurvePoint,
                                                                                                               c0, c1, c2, true);
                                                QList<QPointF> curve2 = secondCurveOfSplit(cuttingPoint, tolerance,
                                                                                           curve2BezierPaths);

                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - curve2.size() ->" << curve2.size();

                                                if (curve1.size() == 4) {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - curve1";
                                                    parts1 << curveToString(FirstSegment, {curve1.at(1), curve1.at(2), curve1.at(3)});
                                                } else {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: Curve at parts1 wasn't added!";
                                                }

                                                if (curve2.size() == 4) {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - curve2";
                                                    parts2 << mToString(SecondSegment, curve2.at(0));
                                                    parts2 << curveToString(SecondSegment, {curve2.at(1), curve2.at(2), curve2.at(3)});
                                                } else {
                                                    // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING: M and curve components wasn't added at parts2!";
                                                }

                                            } else {
                                                // Eraser no action! Middle-Curve
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Fatal Error: Algorithm failed at Middle-Curve!";
                                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - c0-c1-c2";
                                                parts1 << curveToString(FirstSegment, { c0, c1, c2 });
                                            }
                                        } else {
                                            // Cutting point is not part of the curve
                                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - WARNING! Cutting point is NOT part of the curve!";
                                            parts1 << curveToString(FirstSegment, { c0, c1, c2 });
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Curve wasn't affected by the eraser. Storing original data (AA)";
                        // Curve wasn't affected by the eraser
                        if (cuttingType == Begin || cuttingType == End) {
                            // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part1 - c0-c1-c2";
                            parts1 << curveToString(FirstSegment, { c0, c1, c2 });
                        } else { // Middle cut
                            if (parts2.size() == 0) {
                                // qDebug() << "[TupPathItem::recalculatePath()] - CurveToDataElement - Adding part2 - mToString()";
                                QPainterPath::Element previewE = route.elementAt(elementCounter-3);
                                QPointF firstCurvePoint = QPointF(previewE.x, previewE.y);
                                parts2 << mToString(SecondSegment, firstCurvePoint);
                            }
                            parts2 << curveToString(SecondSegment, { c0, c1, c2 });
                        }
                    }                    
                }

                curveDataCounter++;
            }
            break;
            default:
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPathItem::recalculatePath()] - Fatal Error: Path element unknown!";
                #endif
            break;
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::recalculatePath()] - parts1 ->" << parts1;
        qDebug() << "[TupPathItem::recalculatePath()] - parts2 ->" << parts2;
    #endif

    if (parts1.size() == 1) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPathItem::recalculatePath()] - Fatal Error: path 1 string is misconfigured!";
        #endif
        return QPair<QString, QString>("", "");
    }

    if (!parts1.isEmpty()) {
        foreach(QString line, parts1)
            pathStr1 += line;
        pathStr1 = pathStr1.simplified();
    }

    if (!parts2.isEmpty()) {
        if (parts2.size() == 1) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupPathItem::recalculatePath()] - Fatal Error: path 2 string is misconfigured!";
            #endif
            return QPair<QString, QString>("", "");
        }

        foreach(QString line, parts2)
            pathStr2 += line;
        pathStr2 = pathStr2.simplified();
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::recalculatePath()] - pathStr1 ->" << pathStr1;
        qDebug() << "[TupPathItem::recalculatePath()] - pathStr2 ->" << pathStr2;
    #endif

    return QPair<QString, QString>(pathStr1, pathStr2);
}

bool TupPathItem::isBlocked()
{
    return pathBlocked;
}

void TupPathItem::updateBlockingFlag(bool flag)
{
    pathBlocked = flag;
}

QString TupPathItem::mToString(PathSegment segment, QPointF point)
{
    QString mString = "";
    if (segment == FirstSegment) {
        if (t1 != 'M') {
            t1 = 'M';
            mString = "M " + QString::number(point.x()) + " "
                      + QString::number(point.y()) + " ";
        } else {
            mString = QString::number(point.x()) + " "
                      + QString::number(point.y()) + " ";
        }
    } else { // Second segment
        if (t2 != 'M') {
            t2 = 'M';
            mString = "M " + QString::number(point.x()) + " "
                      + QString::number(point.y()) + " ";
        } else {
            mString = QString::number(point.x()) + " "
                      + QString::number(point.y()) + " ";
        }
    }

    return mString;
}

QString TupPathItem::lineToString(PathSegment segment, QPointF point)
{
    QString lineString = "";
    if (segment == FirstSegment) {
        if (t1 != 'L') {
            t1 = 'L';
            lineString = " L " + QString::number(point.x()) + " "
                         + QString::number(point.y()) + " ";
        } else {
            lineString = QString::number(point.x()) + " "
                         + QString::number(point.y()) + " ";
        }
    } else {
        if (t2 != 'L') {
            t2 = 'L';
            lineString = " L " + QString::number(point.x()) + " "
                         + QString::number(point.y()) + " ";
        } else {
            lineString = QString::number(point.x()) + " "
                         + QString::number(point.y()) + " ";
        }
    }

    return lineString;
}

QString TupPathItem::curveToString(PathSegment segment, QList<QPointF> curvePoints)
{
    /*
     #ifdef TUP_DEBUG
         qDebug() << "[TupPathItem::curveToString()]";
     #endif
    */

    QString curveString = "";
    if (curvePoints.count() == 3) {
        if (segment == FirstSegment) {
            if (t1 != 'C') {
                t1 = 'C';
                curveString = " C " + QString::number(curvePoints.at(0).x())
                              + " " + QString::number(curvePoints.at(0).y()) + " ";
            } else {
                curveString = "  " + QString::number(curvePoints.at(0).x()) + " "
                              + QString::number(curvePoints.at(0).y()) + " ";
            }
        } else { // Second segment
            if (t2 != 'C') {
                t2 = 'C';
                curveString = " C " + QString::number(curvePoints.at(0).x())
                              + " " + QString::number(curvePoints.at(0).y()) + " ";
            } else {
                curveString = "  " + QString::number(curvePoints.at(0).x()) + " "
                              + QString::number(curvePoints.at(0).y()) + " ";
            }
        }

        curveString += "  " + QString::number(curvePoints.at(1).x()) + " "
                      + QString::number(curvePoints.at(1).y()) + " ";

        curveString += "  " + QString::number(curvePoints.at(2).x()) + " "
                      + QString::number(curvePoints.at(2).y()) + " ";
    }

    return curveString;
}

QPointF TupPathItem::getNewC1Element(const QPointF &c0, const QPointF &c1, double advance)
{
    /*
     #ifdef TUP_DEBUG
         qDebug() << "[TupPathItem::getNewC1Element()] - advance ->" << advance;
     #endif
    */

    QPointF point;

    double m = TAlgorithm::slope(c0, c1);
    double b = TAlgorithm::calculateBFromLine(c0, m);
    double x = 0;
    double y = 0;

    if (c0.x() < c1.x()) // Left to Right
        x = c0.x() + advance;
    else // Right to Left
        x = c0.x() - advance;

    y =  TAlgorithm::calculateYFromLine(x, m, b);
    point.setX(x);
    point.setY(y);

    return point;
}

QPointF TupPathItem::pointAtPercentage(const QPointF& startPoint, const QPointF& endPoint, int percentage)
{
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    if (percentage < 100) {
        double t = percentage / 100.0;
        double x = startPoint.x() + t * (endPoint.x() - startPoint.x());
        double y = startPoint.y() + t * (endPoint.y() - startPoint.y());

        return QPointF(x, y);
    }

    return endPoint;
}

QList<QList<QPointF>> TupPathItem::calculateBezierPaths(QPointF c0, QPointF c1,
                                                        QPointF c2, QPointF c3, bool reverse)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::calculateBezierPaths()]";
    #endif

    QList<QList<QPointF>> bezierCurves;
    double delta = 0;

    if (!reverse) {
        for(int i=0; i<199; i++) {
            QPointF point1 = pointAtPercentage(c0, c1, delta);
            QPointF point2 = pointAtPercentage(c1, c2, delta);
            QPointF point3 = pointAtPercentage(c2, c3, delta);

            QPointF pointA = pointAtPercentage(point1, point2, delta);
            QPointF pointB = pointAtPercentage(point2, point3, delta);

            QPointF curvePoint = pointAtPercentage(pointA, pointB, delta);
            QList<QPointF> record = { c0, point1, pointA, curvePoint };
            bezierCurves << record;
            delta += 0.5;
        }

        QList<QPointF> record = { c0, c1, c2, c3 };
        bezierCurves << record;

    } else {
        for(int i=0; i<199; i++) {
            QPointF point1 = pointAtPercentage(c3, c2, delta);
            QPointF point2 = pointAtPercentage(c2, c1, delta);
            QPointF point3 = pointAtPercentage(c1, c0, delta);

            QPointF pointA = pointAtPercentage(point1, point2, delta);
            QPointF pointB = pointAtPercentage(point2, point3, delta);

            QPointF curvePoint = pointAtPercentage(pointA, pointB, delta);
            QList<QPointF> record = { c3, point1, pointA, curvePoint };
            bezierCurves << record;
            delta += 0.5;
        }

        QList<QPointF> record = { c3, c2, c1, c0 };
        bezierCurves << record;
    }

    return bezierCurves;
}

QList<QPointF> TupPathItem::generateCurvePoints(const QPointF &cInit, const QPointF &cEnd)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPathItem::generateCurvePoints()]";
    #endif

    QList<QPointF> curvePoints;
    int total = pathPoints.size();

    bool start = false;
    for (int i=0; i<total; i++) {
        if (cInit == pathPoints.at(i)) {
            start = true;
            curvePoints << pathPoints.at(i);
        } else if (cEnd == pathPoints.at(i)) {
                curvePoints << pathPoints.at(i);
                break;
        } else {
            if (start) {
                curvePoints << pathPoints.at(i);
            }
        }
    }

    return curvePoints;
}

bool TupPathItem::isPointPartOfTheCurve(const QPointF &cuttingPoint, int tolerance,
                                   QList<QPointF> curvePoints)
{
    bool found = false;
    // QPointF curvePointFound;
    // double minimumDistance = 0;
    int i=0;
    foreach(QPointF point, curvePoints) {
        double distance = TAlgorithm::distance(point, cuttingPoint);
        if (distance < tolerance) {
            /*
            if (i==0) {
                minimumDistance = distance;
                // curvePointFound = point;
            } else {
                if (distance < minimumDistance) {
                    minimumDistance = distance;
                    // curvePointFound = point;
                }
            }
            */
            found = true;
        }
        i++;
    }

    return found;
}

QList<QPointF> TupPathItem::shortenCurveFromStart(
    const QPointF &cuttingPoint, double tolerance, QList<QList<QPointF>> &bezierPaths)
{
    int total = bezierPaths.size() - 1;
    for (int i=total; i>=0; i--) {
        QList<QPointF> curve = bezierPaths.at(i);
        if (TAlgorithm::distance(curve.last(), cuttingPoint) > tolerance)
            return { curve.at(3), curve.at(2), curve.at(1), curve.at(0) };
    }

    return QList<QPointF>();
}

QList<QPointF> TupPathItem::shortenCurveFromEnd(
    const QPointF &cuttingPoint, double tolerance, QList<QList<QPointF>> &bezierPaths)
{
    int total = bezierPaths.size() - 1;
    for (int i=total; i>=0; i--) {
        QList<QPointF> curve = bezierPaths.at(i);
        if (TAlgorithm::distance(curve.last(), cuttingPoint) > tolerance)
            return curve;
    }

    return QList<QPointF>();
}

QList<QPointF> TupPathItem::firstCurveOfSplit(const QPointF &cuttingPoint, double tolerance,
                                              QList<QList<QPointF>> &bezierPaths)
{
    int total = bezierPaths.size() - 1;
    for (int i=0; i<total; i++) {
        QList<QPointF> curve = bezierPaths.at(i);
        if (TAlgorithm::distance(curve.last(), cuttingPoint) < tolerance) {
            if (i > 0)
                return bezierPaths.at(i-1);
        }
    }

    return QList<QPointF>();
}

QList<QPointF> TupPathItem::secondCurveOfSplit(const QPointF &cuttingPoint, double tolerance,
                                               QList<QList<QPointF>> &bezierPaths)
{
    int total = bezierPaths.size() - 1;
    for (int i=0; i<total; i++) {
        QList<QPointF> curve = bezierPaths.at(i);
        if (TAlgorithm::distance(curve.last(), cuttingPoint) < tolerance) {
            if (i > 0) {
                QList<QPointF> curve = bezierPaths.at(i-1);
                return { curve.at(3), curve.at(2), curve.at(1), curve.at(0) };
            }
        }
    }

    return QList<QPointF>();
}

QPair<QList<QPointF>, QList<QPointF>> TupPathItem::splitStraightLine(const QPointF& startPoint,
                                                                     const QPointF& endPoint,
                                                                     const QPointF& eraserPosition,
                                                                     double tolerance)
{
    double tStart = -1;
    double tEnd = -1;
    double eraserDiameter = 2 * tolerance;

    double t = 0;
    for (int i = 0; i <= 1000; i++) {
        QPointF pointOnLine = startPoint + t * (endPoint - startPoint);
        double distance = std::hypot(pointOnLine.x() - eraserPosition.x(), pointOnLine.y() - eraserPosition.y());

        if (distance <= tolerance && tStart == -1) {
            tStart = t;
        }
        if (distance > eraserDiameter && tStart != -1) {
            tEnd = t;
            break;
        }
        t += 0.001;
    }

    if (tStart == -1 || tEnd == -1) {
        // The entire line is outside the eraser's effect
        return { { startPoint, endPoint }, {} };
    }

    // Calculate the points where the line is split
    QPointF splitStart = startPoint + tStart * (endPoint - startPoint);
    QPointF splitEnd = startPoint + tEnd * (endPoint - startPoint);

    QList<QPointF> firstSegment = { startPoint, splitStart };
    QList<QPointF> secondSegment = { splitEnd, endPoint };

    return qMakePair(firstSegment, secondSegment);
}
