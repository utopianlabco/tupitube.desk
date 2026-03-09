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

#include "buckettool.h"

#include "tconfig.h"
#include "tupsvgitem.h"
#include "tuptextitem.h"
#include "tupserializer.h"
#include "tupitemconverter.h"
#include "tuprequestbuilder.h"
#include "tupscene.h"
#include "tupframe.h"
#include "tupgraphicobject.h"
#include "tupinputdeviceinformation.h"
#include "tupgraphicsscene.h"
#include "tupprojectrequest.h"
#include "tupbrushmanager.h"
#include "tupgraphiclibraryitem.h"
#include "tupitemgroup.h"
#include "tosd.h"

#include <QPainter>
#include <QQueue>

/*
SQA: Pay attention to this tip
You can use the QGraphicsItem::shape () which returns a QPainterPath to retrieve the shape of an item. 
For taking the intersection path this can be used :

QPainterPath QPainterPath::intersected ( const QPainterPath & p ) const;

So you can get the intersection path of two items like:

QPainterPath intersectedPath = item1->shape()->intersected(item2->shape());

Now you can fill the intersected area by :

painter->setBrush(QColor(122, 163, 39));
painter->drawPath(intersectedPath);
*/

BucketTool::BucketTool()
{
    setupActions();
}

BucketTool::~BucketTool()
{
}

void BucketTool::init(TupGraphicsScene *gScene)
{
    scene = gScene;

    TCONFIG->beginGroup("ColorPalette");
    int colorMode = TCONFIG->value("CurrentColorMode", 0).toInt();
    mode = TColorCell::FillType(colorMode);
}

QList<TAction::ActionId> BucketTool::keys() const
{
    return QList<TAction::ActionId>() << TAction::PaintBucket;
}

void BucketTool::setupActions()
{
    fillCursor = QCursor(CURSORS_DIR + "bucket_fill.png", 0, 11);
    borderCursor = QCursor(CURSORS_DIR + "bucket_border.png", 0, 13);

    TAction *action1 = new TAction(QIcon(ICONS_DIR + "paint_bucket.png"), tr("Paint Bucket"), this);
    action1->setShortcut(QKeySequence(tr("F")));
    action1->setToolTip(tr("Paint Bucket") + " - " + "F");
    action1->setCursor(fillCursor);
    bucketActions.insert(TAction::PaintBucket, action1);
}

// Helper function to identify flood fill items
// Flood fill items are QGraphicsPathItems with a brush but no pen (NoPen)
static bool isFloodFillItem(QGraphicsItem *item)
{
    QGraphicsPathItem *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item);
    if (!pathItem)
        return false;
    
    // Flood fill items have: brush set AND pen is NoPen
    QPen pen = pathItem->pen();
    QBrush brush = pathItem->brush();
    
    return (pen.style() == Qt::NoPen && brush.style() != Qt::NoBrush);
}

/* Helper function removed - TupFrame::indexOf() now uses pointer comparison
static int findItemIndexInFrame(TupFrame *frame, QGraphicsItem *targetItem)
{
    if (!frame || !targetItem)
        return -1;
    
    int count = frame->graphicsCount();
    for (int i = 0; i < count; i++) {
        QGraphicsItem *frameItem = frame->item(i);
        if (frameItem == targetItem) {
            return i;
        }
    }
    return -1;
}
*/

void BucketTool::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::press()]";
        qDebug() << "[BucketTool::press()] - Color mode:" << (mode == TColorCell::Inner ? "Inner (Flood Fill)" : "Contour (Fill Shape)");
    #endif

    if (input->buttons() == Qt::LeftButton) {
        // Inner fill mode uses Flood Fill algorithm
        // Contour (border) mode uses original item-based approach
        if (mode == TColorCell::Inner) {
            #ifdef TUP_DEBUG
                qDebug() << "[BucketTool::press()] - Calling performFloodFill()";
            #endif
            performFloodFill(input->pos(), brushManager, gScene);
            return;
        }

        #ifdef TUP_DEBUG
            qDebug() << "[BucketTool::press()] - Using original shape fill mode (Contour)";
        #endif

        // Original shape fill behavior for Contour mode
        // SQA: Enhance this plugin to support several items with one click 
        QList<QGraphicsItem *> list = scene->items(input->pos(), Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        foreach(QGraphicsItem *item, list) {
            // QGraphicsItem *item = gScene->itemAt(input->pos(), QTransform());
            if (item) {
                int itemIndex = -1;
                int currentLayer;
                int currentFrame;
                TupFrame *frame = new TupFrame;

                if (gScene->getSpaceContext() == TupProject::FRAMES_MODE) {
                    frame = gScene->currentFrame();
                    if (frame) {
                        itemIndex = frame->indexOf(item);
                        currentLayer = gScene->currentLayerIndex();
                        currentFrame = gScene->currentFrameIndex();
                    }
                } else {
                    currentLayer = -1;
                    currentFrame = -1;
                    TupBackground *bg = gScene->currentScene()->sceneBackground();
                    if (gScene->getSpaceContext() == TupProject::VECTOR_STATIC_BG_MODE) {
                        frame = bg->vectorStaticFrame();
                        itemIndex = frame->indexOf(item);
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                        frame = bg->vectorDynamicFrame();
                        itemIndex = frame->indexOf(item);
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_FG_MODE) {
                        frame = bg->vectorForegroundFrame();
                        itemIndex = frame->indexOf(item);
                    }
                }

                if (itemIndex >= 0) {
                    if (TupGraphicLibraryItem *libraryItem = qgraphicsitem_cast<TupGraphicLibraryItem *>(item)) {
                        // This condition only applies for images
                        if (libraryItem->type() != TupLibraryObject::Item) {
                            TOsd::self()->display(TOsd::Error, tr("Sorry, only native objects can be filled"));
                            #ifdef TUP_DEBUG
                                qWarning() << "[BucketTool::press()] - Warning: item is a RASTER object!";
                            #endif
                            return;
                        }
                    }

                    // Testing if object is a SVG file
                    TupSvgItem *svg = qgraphicsitem_cast<TupSvgItem *>(item);
                    if (svg) {
                        TOsd::self()->display(TOsd::Error, tr("Sorry, only native objects can be filled"));
                        #ifdef TUP_DEBUG
                            qWarning() << "[BucketTool::press()] - Warning: item is a SVG object!";
                        #endif
                        return;
                    }

                    if (qgraphicsitem_cast<TupItemGroup *>(item)) {
                        TOsd::self()->display(TOsd::Error, tr("Sorry, Groups can't be filled yet"));
                        return;
                    }

                    if (qgraphicsitem_cast<TupTextItem *>(item)) {
                        QColor textColor = "";
                        frame->checkTextColorStatus(itemIndex);
                        if (mode == TColorCell::Inner) {
                            textColor = brushManager->brush().color();
                        } else if (mode == TColorCell::Contour) {
                            textColor = brushManager->pen().color();
                        }

                        TupProjectRequest event = TupRequestBuilder::createItemRequest(
                                                  gScene->currentSceneIndex(), currentLayer,
                                                  currentFrame, itemIndex, QPointF(),
                                                  gScene->getSpaceContext(), TupLibraryObject::Item,
                                                  TupProjectRequest::TextColor,
                                                  textColor.name(QColor::HexArgb));
                        emit requested(&event);
                        return;
                    } else if (qgraphicsitem_cast<QAbstractGraphicsShapeItem *>(item)) {
                        QDomDocument doc;
                        TupProjectRequest::Action action = TupProjectRequest::Brush;
                        if (mode == TColorCell::Inner) {
                            frame->checkBrushStatus(itemIndex);
                            QBrush brush = brushManager->brush();
                            doc.appendChild(TupSerializer::brush(&brush, doc));
                        } else if (mode == TColorCell::Contour) {
                            frame->checkPenStatus(itemIndex);
                            QPen pen = brushManager->pen();
                            action = TupProjectRequest::Pen;
                            doc.appendChild(TupSerializer::pen(&pen, doc));
                        }

                        TupProjectRequest event = TupRequestBuilder::createItemRequest(
                                                  gScene->currentSceneIndex(), currentLayer,
                                                  currentFrame, itemIndex, QPointF(),
                                                  gScene->getSpaceContext(), TupLibraryObject::Item,
                                                  action, doc.toString());

                        emit requested(&event);
                        return;
                    } else {
                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool::press()] - Fatal Error: QAbstractGraphicsShapeItem cast has failed!";
                        #endif
                    }
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "[BucketTool::press()] - Error: item is not available at the current frame";
                    #endif
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[BucketTool::press()] - No item found";
                #endif
                return;
            }
        }
    }
}

void BucketTool::move(const TupInputDeviceInformation *, TupBrushManager *, TupGraphicsScene *)
{
}

void BucketTool::release(const TupInputDeviceInformation *, TupBrushManager *, TupGraphicsScene *)
{
}

QMap<TAction::ActionId, TAction *> BucketTool::actions() const
{
    return bucketActions;
}

TAction * BucketTool::getAction(TAction::ActionId toolId)
{
    return bucketActions[toolId];
}

int BucketTool::toolType() const
{
    return TupToolInterface::Bucket;
}
        
QWidget *BucketTool::configurator()
{
    return nullptr;
}

void BucketTool::aboutToChangeScene(TupGraphicsScene *)
{
}

void BucketTool::aboutToChangeTool() 
{
    foreach (QGraphicsItem *item, scene->items()) {
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        item->setFlag(QGraphicsItem::ItemIsFocusable, false);
    }
}

QPainterPath BucketTool::mapPath(const QPainterPath &path, const QPointF &pos)
{
    QTransform transform;
    transform.translate(pos.x(), pos.y());
    
    QPainterPath painter = transform.map(path);
    painter.closeSubpath();
    
    return painter;
}

QPainterPath BucketTool::mapPath(const QGraphicsPathItem *item)
{
    return mapPath(item->path(), item->pos());
}

void BucketTool::saveConfig()
{
}

void BucketTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11 || event->key() == Qt::Key_Escape) {
        emit closeHugeCanvas();
    } else {
        QPair<int, int> flags = TAction::setKeyAction(event->key(), event->modifiers());
        if (flags.first != -1 && flags.second != -1)
            emit callForPlugin(flags.first, flags.second);
    }
}

QCursor BucketTool::toolCursor() // const
{
    if (mode == TColorCell::Inner) {
        return fillCursor;
    } else if (mode == TColorCell::Contour) {
        return borderCursor;
    }

    return QCursor(Qt::ArrowCursor);
}

void BucketTool::setColorMode(TColorCell::FillType colorMode)
{
    mode = colorMode;
}

void BucketTool::itemResponse(const TupItemResponse *event)
{
    Q_UNUSED(event)
    
    // Redraw the scene after item is added/removed
    if (scene)
        scene->drawCurrentPhotogram();
}

// Flood Fill Implementation

QImage BucketTool::renderSceneToImage(TupGraphicsScene *gScene)
{
    QRectF sceneRect = gScene->sceneRect();
    QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    // Disable anti-aliasing for cleaner line boundaries in flood fill
    painter.setRenderHint(QPainter::Antialiasing, false);
    gScene->render(&painter, QRectF(), sceneRect);
    painter.end();

    return image;
}

bool BucketTool::colorsMatch(QRgb c1, QRgb c2, int tolerance)
{
    Q_UNUSED(c2)
    
    // For flood fill, we use brightness-based boundary detection
    // A pixel is fillable if it's bright enough (not a line)
    int gray = qGray(c1);
    
    // The tolerance represents how dark a pixel can be and still be filled
    // tolerance 0 = only pure white (255), tolerance 32 = gray >= 223, etc.
    int brightnessThreshold = 255 - tolerance;
    
    // Only fill if pixel is bright (not a line pixel)
    return (gray >= brightnessThreshold);
}

QImage BucketTool::floodFillMask(const QImage &image, const QPoint &seedPoint, int tolerance, int maxPixels, bool *aborted)
{
    int width = image.width();
    int height = image.height();

    if (aborted)
        *aborted = false;

    // Use ARGB32 format for reliable pixel operations
    QImage mask(width, height, QImage::Format_ARGB32);
    mask.fill(qRgba(0, 0, 0, 255));  // Fill with black

    if (seedPoint.x() < 0 || seedPoint.x() >= width || 
        seedPoint.y() < 0 || seedPoint.y() >= height) {
        #ifdef TUP_DEBUG
            qDebug() << "[BucketTool::floodFillMask()] - Seed point out of bounds";
        #endif
        return mask;
    }

    QRgb targetColor = image.pixel(seedPoint);
    QRgb fillColor = qRgba(255, 255, 255, 255);  // White for filled pixels

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::floodFillMask()] - Target color:" << QColor(targetColor).name();
        qDebug() << "[BucketTool::floodFillMask()] - Seed point:" << seedPoint;
        qDebug() << "[BucketTool::floodFillMask()] - Tolerance:" << tolerance;
        qDebug() << "[BucketTool::floodFillMask()] - Max pixels limit:" << maxPixels;
    #endif

    // Use scanline flood fill algorithm for efficiency
    QQueue<QPoint> queue;
    queue.enqueue(seedPoint);

    QVector<bool> visited(width * height, false);
    int filledPixels = 0;

    while (!queue.isEmpty()) {
        // Early termination: abort if we've filled too many pixels (region not enclosed)
        if (maxPixels > 0 && filledPixels > maxPixels) {
            #ifdef TUP_DEBUG
                qDebug() << "[BucketTool::floodFillMask()] - EARLY ABORT: filled" << filledPixels << "pixels, exceeds limit" << maxPixels;
            #endif
            if (aborted)
                *aborted = true;
            return mask;  // Return partial mask, caller will handle abort
        }
        QPoint p = queue.dequeue();
        int x = p.x();
        int y = p.y();

        if (x < 0 || x >= width || y < 0 || y >= height)
            continue;

        int idx = y * width + x;
        if (visited[idx])
            continue;

        if (!colorsMatch(image.pixel(x, y), targetColor, tolerance))
            continue;

        // Scan left
        int leftX = x;
        while (leftX > 0 && colorsMatch(image.pixel(leftX - 1, y), targetColor, tolerance)) {
            leftX--;
        }

        // Scan right
        int rightX = x;
        while (rightX < width - 1 && colorsMatch(image.pixel(rightX + 1, y), targetColor, tolerance)) {
            rightX++;
        }

        // Fill the scanline and check above/below
        for (int fillX = leftX; fillX <= rightX; fillX++) {
            int fillIdx = y * width + fillX;
            if (!visited[fillIdx]) {
                visited[fillIdx] = true;
                mask.setPixel(fillX, y, fillColor);  // Set white for filled pixels
                filledPixels++;

                // Check pixel above
                if (y > 0) {
                    int aboveIdx = (y - 1) * width + fillX;
                    if (!visited[aboveIdx] && colorsMatch(image.pixel(fillX, y - 1), targetColor, tolerance)) {
                        queue.enqueue(QPoint(fillX, y - 1));
                    }
                }

                // Check pixel below
                if (y < height - 1) {
                    int belowIdx = (y + 1) * width + fillX;
                    if (!visited[belowIdx] && colorsMatch(image.pixel(fillX, y + 1), targetColor, tolerance)) {
                        queue.enqueue(QPoint(fillX, y + 1));
                    }
                }
            }
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::floodFillMask()] - Filled pixels:" << filledPixels;
    #endif

    return mask;
}

QVector<QPoint> BucketTool::traceBoundary(const QImage &mask)
{
    // Marching squares algorithm to trace boundary
    QVector<QPoint> boundary;

    int width = mask.width();
    int height = mask.height();

    // Helper lambda to check if pixel is filled (white pixel = filled)
    auto isFilled = [&mask, width, height](int x, int y) -> bool {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return false;
        QRgb pixel = mask.pixel(x, y);
        return qRed(pixel) > 128;  // Check red channel (white = 255, black = 0)
    };

    // Find starting point on boundary
    QPoint start(-1, -1);
    for (int y = 0; y < height && start.x() < 0; y++) {
        for (int x = 0; x < width; x++) {
            if (isFilled(x, y)) {
                // Check if it's on the boundary (has at least one empty neighbor)
                bool onBoundary = (x == 0 || !isFilled(x - 1, y) ||
                                   y == 0 || !isFilled(x, y - 1) ||
                                   x == width - 1 || !isFilled(x + 1, y) ||
                                   y == height - 1 || !isFilled(x, y + 1));
                if (onBoundary) {
                    start = QPoint(x, y);
                    break;
                }
            }
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::traceBoundary()] - Start point:" << start;
    #endif

    if (start.x() < 0)
        return boundary;

    // Direction vectors for 8-connectivity
    const int dx[] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int dy[] = {0, 1, 1, 1, 0, -1, -1, -1};

    QPoint current = start;
    int dir = 0; // Start direction

    do {
        boundary.append(current);

        // Find next boundary pixel (Moore neighborhood tracing)
        int startDir = (dir + 5) % 8; // Turn back and search clockwise
        bool found = false;

        for (int i = 0; i < 8; i++) {
            int checkDir = (startDir + i) % 8;
            int nx = current.x() + dx[checkDir];
            int ny = current.y() + dy[checkDir];

            if (isFilled(nx, ny)) {
                current = QPoint(nx, ny);
                dir = checkDir;
                found = true;
                break;
            }
        }

        if (!found)
            break;

        // Prevent infinite loops
        if (boundary.size() > width * height)
            break;

    } while (current != start);

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::traceBoundary()] - Boundary points:" << boundary.size();
    #endif

    return boundary;
}

QPainterPath BucketTool::boundaryToPath(const QVector<QPoint> &boundary, double smoothness)
{
    QPainterPath path;

    if (boundary.isEmpty())
        return path;

    // Convert boundary points to path
    path.moveTo(boundary.first());

    if (smoothness <= 0) {
        // No smoothing - just connect the points
        for (int i = 1; i < boundary.size(); i++) {
            path.lineTo(boundary[i]);
        }
    } else {
        // Apply smoothing using Catmull-Rom spline
        int n = boundary.size();
        if (n < 4) {
            for (int i = 1; i < n; i++)
                path.lineTo(boundary[i]);
        } else {
            // Sample points based on smoothness (higher = fewer points)
            int step = qMax(1, static_cast<int>(smoothness * 3));
            QVector<QPointF> sampledPoints;
            
            for (int i = 0; i < n; i += step) {
                sampledPoints.append(boundary[i]);
            }
            
            // Ensure last point is included
            if (sampledPoints.last() != boundary.last())
                sampledPoints.append(boundary.last());

            // Convert to smooth curves using quadratic bezier
            if (sampledPoints.size() >= 3) {
                for (int i = 1; i < sampledPoints.size() - 1; i++) {
                    QPointF mid = (sampledPoints[i] + sampledPoints[i + 1]) / 2.0;
                    path.quadTo(sampledPoints[i], mid);
                }
                path.lineTo(sampledPoints.last());
            } else {
                for (int i = 1; i < sampledPoints.size(); i++)
                    path.lineTo(sampledPoints[i]);
            }
        }
    }

    path.closeSubpath();
    return path;
}

void BucketTool::performFloodFill(const QPointF &pos, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::performFloodFill()] - pos:" << pos;
    #endif

    // Check if the selected color is transparent - if so, try to remove item under click
    QColor fillColor;
    if (mode == TColorCell::Inner) {
        fillColor = brushManager->brush().color();
    } else {
        fillColor = brushManager->pen().color();
    }

    // First check if user clicked on a Qt shape item (rectangle, ellipse, polygon)
    // If so, directly set the brush on that item instead of running flood fill
    if (fillColor.alpha() > 0) {
        QList<QGraphicsItem *> itemsAtPos = gScene->items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        for (QGraphicsItem *item : itemsAtPos) {
            // Check for QAbstractGraphicsShapeItem (rect, ellipse, polygon, etc.)
            QAbstractGraphicsShapeItem *shapeItem = qgraphicsitem_cast<QAbstractGraphicsShapeItem *>(item);
            if (shapeItem) {
                // Check if user clicked on an existing flood fill item - update its color
                if (isFloodFillItem(item)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[BucketTool::performFloodFill()] - Clicked on existing flood fill item, updating color";
                    #endif

                    int itemIndex = -1;
                    int currentLayer = -1;
                    int currentFrame = -1;
                    TupFrame *frame = nullptr;

                    if (gScene->getSpaceContext() == TupProject::FRAMES_MODE) {
                        frame = gScene->currentFrame();
                        if (frame) {
                            itemIndex = frame->indexOf(item);
                            currentLayer = gScene->currentLayerIndex();
                            currentFrame = gScene->currentFrameIndex();
                        }
                    } else {
                        TupBackground *bg = gScene->currentScene()->sceneBackground();
                        if (gScene->getSpaceContext() == TupProject::VECTOR_STATIC_BG_MODE) {
                            frame = bg->vectorStaticFrame();
                        } else if (gScene->getSpaceContext() == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                            frame = bg->vectorDynamicFrame();
                        } else if (gScene->getSpaceContext() == TupProject::VECTOR_FG_MODE) {
                            frame = bg->vectorForegroundFrame();
                        }
                        if (frame)
                            itemIndex = frame->indexOf(item);
                    }

                    if (itemIndex >= 0) {
                        frame->checkBrushStatus(itemIndex);
                        QBrush brush = brushManager->brush();

                        QDomDocument doc;
                        doc.appendChild(TupSerializer::brush(&brush, doc));

                        TupProjectRequest event = TupRequestBuilder::createItemRequest(
                                                  gScene->currentSceneIndex(), currentLayer,
                                                  currentFrame, itemIndex, QPointF(),
                                                  gScene->getSpaceContext(), TupLibraryObject::Item,
                                                  TupProjectRequest::Brush, doc.toString());
                        emit requested(&event);
                        return;
                    }
                    continue;
                }

                // Check for path items - only fill if path is closed
                if (QGraphicsPathItem *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item)) {
                    QPainterPath itemPath = pathItem->path();
                    // Check if path is closed (first and last points approximately equal)
                    bool isClosed = false;
                    if (itemPath.elementCount() >= 3) {
                        QPainterPath::Element first = itemPath.elementAt(0);
                        QPainterPath::Element last = itemPath.elementAt(itemPath.elementCount() - 1);
                        QPointF firstPt(first.x, first.y);
                        QPointF lastPt(last.x, last.y);
                        // Check if points are close (within 5 pixels)
                        qreal dist = QLineF(firstPt, lastPt).length();
                        isClosed = (dist < 5.0);
                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool::performFloodFill()] - Path first:" << firstPt 
                                     << "last:" << lastPt << "dist:" << dist << "closed:" << isClosed;
                        #endif
                    }
                    
                    if (!isClosed) {
                        // Open path - skip it, can't be filled directly
                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool::performFloodFill()] - Skipping open path item";
                        #endif
                        continue;
                    }
                    // Closed path - continue to fill it directly
                    #ifdef TUP_DEBUG
                        qDebug() << "[BucketTool::performFloodFill()] - Found closed path, will fill directly";
                    #endif
                }

                #ifdef TUP_DEBUG
                    qDebug() << "[BucketTool::performFloodFill()] - Found fillable shape item, filling directly";
                #endif

                int itemIndex = -1;
                int currentLayer = -1;
                int currentFrame = -1;
                TupFrame *frame = nullptr;

                if (gScene->getSpaceContext() == TupProject::FRAMES_MODE) {
                    frame = gScene->currentFrame();
                    if (frame) {
                        itemIndex = frame->indexOf(item);
                        currentLayer = gScene->currentLayerIndex();
                        currentFrame = gScene->currentFrameIndex();
                    }
                } else {
                    TupBackground *bg = gScene->currentScene()->sceneBackground();
                    if (gScene->getSpaceContext() == TupProject::VECTOR_STATIC_BG_MODE) {
                        frame = bg->vectorStaticFrame();
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                        frame = bg->vectorDynamicFrame();
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_FG_MODE) {
                        frame = bg->vectorForegroundFrame();
                    }
                    if (frame)
                        itemIndex = frame->indexOf(item);
                }

                if (itemIndex >= 0) {
                    frame->checkBrushStatus(itemIndex);
                    QBrush brush = brushManager->brush();

                    QDomDocument doc;
                    doc.appendChild(TupSerializer::brush(&brush, doc));

                    TupProjectRequest event = TupRequestBuilder::createItemRequest(
                                              gScene->currentSceneIndex(), currentLayer,
                                              currentFrame, itemIndex, QPointF(),
                                              gScene->getSpaceContext(), TupLibraryObject::Item,
                                              TupProjectRequest::Brush, doc.toString());
                    emit requested(&event);
                    return;
                }
            }
        }
    }

    if (fillColor.alpha() == 0) {
        // Transparent color selected - remove flood fill items or clear native shape fills
        #ifdef TUP_DEBUG
            qDebug() << "[BucketTool::performFloodFill()] - Transparent color selected, entering REMOVE mode";
        #endif

        QList<QGraphicsItem *> itemsAtPos = gScene->items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        
        #ifdef TUP_DEBUG
            qDebug() << "[BucketTool::performFloodFill()] - Items at position:" << itemsAtPos.size();
        #endif

        for (QGraphicsItem *item : itemsAtPos) {
            #ifdef TUP_DEBUG
                qDebug() << "[BucketTool::performFloodFill()] - Checking item type:" << item->type();
            #endif

            // Check if it's a QAbstractGraphicsShapeItem (includes path, rect, ellipse, polygon)
            QAbstractGraphicsShapeItem *shapeItem = qgraphicsitem_cast<QAbstractGraphicsShapeItem *>(item);
            if (shapeItem) {
                int itemIndex = -1;
                int layerIndex = -1;
                int frameIndex = -1;
                TupFrame *frame = nullptr;

                if (gScene->getSpaceContext() == TupProject::FRAMES_MODE) {
                    frame = gScene->currentFrame();
                    if (frame) {
                        itemIndex = frame->indexOf(item);
                        layerIndex = gScene->currentLayerIndex();
                        frameIndex = gScene->currentFrameIndex();
                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool::performFloodFill()] - Item index in frame:" << itemIndex;
                        #endif
                    }
                } else {
                    TupBackground *bg = gScene->currentScene()->sceneBackground();
                    if (gScene->getSpaceContext() == TupProject::VECTOR_STATIC_BG_MODE) {
                        frame = bg->vectorStaticFrame();
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                        frame = bg->vectorDynamicFrame();
                    } else if (gScene->getSpaceContext() == TupProject::VECTOR_FG_MODE) {
                        frame = bg->vectorForegroundFrame();
                    }
                    if (frame)
                        itemIndex = frame->indexOf(item);
                }

                if (itemIndex >= 0) {
                    // For flood fill items, remove them entirely
                    if (isFloodFillItem(item)) {
                        TupProjectRequest request = TupRequestBuilder::createItemRequest(
                            gScene->currentSceneIndex(), layerIndex, frameIndex,
                            itemIndex, QPointF(), gScene->getSpaceContext(),
                            TupLibraryObject::Item, TupProjectRequest::Remove);
                        emit requested(&request);

                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool] - REMOVED flood fill item at index:" << itemIndex;
                        #endif
                        TOsd::self()->display(TOsd::Info, tr("Fill removed"));
                        return;
                    } else {
                        // For native shapes, set brush to transparent (NoBrush)
                        frame->checkBrushStatus(itemIndex);
                        QBrush transparentBrush(Qt::NoBrush);

                        QDomDocument doc;
                        doc.appendChild(TupSerializer::brush(&transparentBrush, doc));

                        TupProjectRequest event = TupRequestBuilder::createItemRequest(
                                                  gScene->currentSceneIndex(), layerIndex,
                                                  frameIndex, itemIndex, QPointF(),
                                                  gScene->getSpaceContext(), TupLibraryObject::Item,
                                                  TupProjectRequest::Brush, doc.toString());
                        emit requested(&event);

                        #ifdef TUP_DEBUG
                            qDebug() << "[BucketTool] - Cleared brush on native shape at index:" << itemIndex;
                        #endif
                        TOsd::self()->display(TOsd::Info, tr("Fill cleared"));
                        return;
                    }
                }
            }
        }

        qWarning() << "[BucketTool] - No fill item found to remove at click position";
        TOsd::self()->display(TOsd::Warning, tr("No fill item found to remove"));
        return;
    }

    // Render scene to image
    QImage sceneImage = renderSceneToImage(gScene);

    // Convert scene coordinates to image coordinates
    QRectF sceneRect = gScene->sceneRect();
    QPoint imagePos(static_cast<int>(pos.x() - sceneRect.left()),
                    static_cast<int>(pos.y() - sceneRect.top()));

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::performFloodFill()] - Scene rect:" << sceneRect;
        qDebug() << "[BucketTool::performFloodFill()] - Image pos:" << imagePos;
        qDebug() << "[BucketTool::performFloodFill()] - Image size:" << sceneImage.size();
    #endif

    if (imagePos.x() < 0 || imagePos.x() >= sceneImage.width() ||
        imagePos.y() < 0 || imagePos.y() >= sceneImage.height()) {
        qWarning() << "[BucketTool] - NOT FILLABLE: Click position is outside the canvas";
        TOsd::self()->display(TOsd::Error, tr("Click position is outside the canvas"));
        return;
    }

    // Check if clicking on a line (non-white pixel)
    int tolerance = 20;  // Default tolerance for flood fill
    int brightnessThreshold = 255 - tolerance;
    QRgb clickedColor = sceneImage.pixel(imagePos);
    if (qGray(clickedColor) < brightnessThreshold) {  // Clicked on a dark pixel (line)
        qWarning() << "[BucketTool] - NOT FILLABLE: Clicked on a line (brightness:" << qGray(clickedColor) << "< threshold:" << brightnessThreshold << ")";
        TOsd::self()->display(TOsd::Warning, tr("Click inside an enclosed area, not on a line"));
        return;
    }

    // Calculate max allowed fill pixels (80% of canvas = not enclosed)
    int totalPixels = sceneImage.width() * sceneImage.height();
    int maxFillPixels = static_cast<int>(totalPixels * 0.8);

    // Perform flood fill with early termination
    bool fillAborted = false;
    QImage mask = floodFillMask(sceneImage, imagePos, tolerance, maxFillPixels, &fillAborted);

    // Check if fill was aborted due to region not being enclosed
    if (fillAborted) {
        qWarning() << "[BucketTool] - NOT FILLABLE: Region leaked beyond 80% of canvas - lines don't form a closed shape";
        TOsd::self()->display(TOsd::Warning, tr("Region is not enclosed - lines must form a closed shape"));
        return;
    }

    // Trace boundary
    QVector<QPoint> boundary = traceBoundary(mask);

    if (boundary.size() < 3) {
        qWarning() << "[BucketTool] - NOT FILLABLE: Boundary has less than 3 points (" << boundary.size() << ")";
        TOsd::self()->display(TOsd::Warning, tr("No fillable region found"));
        return;
    }

    // Convert boundary to path
    double smoothness = 1.0;  // Default smoothness for flood fill
    QPainterPath fillPath = boundaryToPath(boundary, smoothness);

    // Check if path is valid
    if (fillPath.isEmpty()) {
        qWarning() << "[BucketTool] - NOT FILLABLE: Could not create path from boundary";
        TOsd::self()->display(TOsd::Warning, tr("Could not create fill path"));
        return;
    }

    // *** FILLABLE AREA FOUND ***
    qWarning() << "[BucketTool] - FILLABLE AREA FOUND! Boundary points:" << boundary.size();

    // Offset path back to scene coordinates
    fillPath.translate(sceneRect.left(), sceneRect.top());

    // Get current frame info for z-level management
    int sceneIndex = gScene->currentSceneIndex();
    int layerIndex = gScene->currentLayerIndex();
    int frameIndex = gScene->currentFrameIndex();
    TupFrame *frame = gScene->currentFrame();
    
    // Count existing fills and strokes before adding
    // Flood fill items are identified by having a brush but no pen (NoPen)
    int fillCount = 0;
    int totalItems = frame ? frame->graphicsCount() : 0;
    if (frame) {
        for (int i = 0; i < totalItems; i++) {
            QGraphicsItem *item = frame->item(i);
            if (item && isFloodFillItem(item)) {
                fillCount++;
            }
        }
    }
    int strokeCount = totalItems - fillCount;

    // Create the fill item
    TupPathItem *fillItem = new TupPathItem();
    fillItem->setPath(fillPath);
    
    // Set brush for the fill (flood fill only applies to Inner mode)
    // Flood fill items have: brush set AND pen is NoPen (this is how they're identified)
    fillItem->setBrush(brushManager->brush());
    fillItem->setPen(Qt::NoPen);

    // Create project request to add the item
    // Note: Do NOT call gScene->includeObject() - let the request handler
    // add the item to both frame and scene to keep them synchronized
    QDomDocument doc;
    doc.appendChild(fillItem->toXml(doc));

    TupProjectRequest addRequest = TupRequestBuilder::createItemRequest(
        sceneIndex,
        layerIndex,
        frameIndex,
        0,
        QPointF(),
        gScene->getSpaceContext(),
        TupLibraryObject::Item,
        TupProjectRequest::Add,
        doc.toString()
    );

    emit requested(&addRequest);

    // Move the new fill to the correct position: after existing fills, before strokes
    // After Add, the new item is at position totalItems (the end)
    // We need strokeCount MoveOneLevelBack operations to place it at position fillCount
    for (int i = 0; i < strokeCount; i++) {
        int currentPos = totalItems - i;  // Position decreases with each move
        TupProjectRequest moveRequest = TupRequestBuilder::createItemRequest(
            sceneIndex,
            layerIndex,
            frameIndex,
            currentPos,
            QPointF(),
            gScene->getSpaceContext(),
            TupLibraryObject::Item,
            TupProjectRequest::Move,
            QString::number(TupFrame::MoveOneLevelBack)
        );
        emit requested(&moveRequest);
    }

    TOsd::self()->display(TOsd::Info, tr("Region filled successfully"));

    #ifdef TUP_DEBUG
        qDebug() << "[BucketTool::performFloodFill()] - Fill path created with" << boundary.size() << "boundary points";
    #endif
}

