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

#include "tupitemconverter.h"
#include "tuprectitem.h"
#include "tuppathitem.h"
#include "tuplineitem.h"
#include "tupellipseitem.h"
#include "tupproxyitem.h"
#include "tupitemgroup.h"
#include "tupgraphicobject.h"
#include "tupitemfactory.h"
#include "tupabstractserializable.h"

#include <QDomDocument>

TupItemConverter::TupItemConverter()
{
}

TupItemConverter::~TupItemConverter()
{
}

void TupItemConverter::copyProperties(QGraphicsItem *src, QGraphicsItem *dest)
{
    // dest->setMatrix(src->matrix());
    dest->setTransform(src->transform());
    dest->setPos(src->scenePos());
    dest->setFlags(src->flags() );
    dest->setSelected(src->isSelected());
    
    // Shapes
    QAbstractGraphicsShapeItem *shape =  dynamic_cast<QAbstractGraphicsShapeItem*>(src);
    QAbstractGraphicsShapeItem *shapeDst = qgraphicsitem_cast<QAbstractGraphicsShapeItem*>(dest);
    
    if (shape && shapeDst) {
        QBrush shapeBrush = shape->brush();
        
        if (shapeBrush.color().isValid() || shapeBrush.gradient() || 
        !shapeBrush.texture().isNull())
            shapeDst->setBrush( shape->brush());
        
        shapeDst->setPen(shape->pen());
    }
}

TupPathItem *TupItemConverter::convertToPath(QGraphicsItem *item)
{
    if (!item) 
        return 0;
    
    TupPathItem *path = new TupPathItem(item->parentItem());
    
    QPainterPath ppath;
    
    switch (item->type()) {
        case TupPathItem::Type:
        {
            ppath = qgraphicsitem_cast<TupPathItem *>(item)->path();
        }
        break;

        case TupRectItem::Type:
        {
            ppath.addRect(qgraphicsitem_cast<TupRectItem *>(item)->rect());
        }
        break;

        case TupEllipseItem::Type:
        {
            ppath.addEllipse(qgraphicsitem_cast<TupEllipseItem *>(item)->rect());
        }
        break;

        case TupProxyItem::Type:
        {
            QGraphicsItem * data = qgraphicsitem_cast<TupProxyItem*>(item)->item();
            data->setPos(item->scenePos());
            return convertToPath(data);
        }
        break;

        case TupLineItem::Type:
        {
            QLineF line = qgraphicsitem_cast<TupLineItem *>(item)->line();
            ppath.moveTo(line.p1());
            ppath.lineTo(line.p2());
        }
        break;

        case TupItemGroup::Type:
        {
            #ifdef TUP_DEBUG
                qWarning() << "TupItemConverter::convertToPath - Error: Group items are not supported";
            #endif	
            delete path;
            return 0;
        }
        break;

        default:
        {
            #ifdef TUP_DEBUG
                qWarning() << "TupItemConverter::convertToPath - Using default converter...";
            #endif			
            ppath = item->shape(); // TODO
        }
        break;
    }
    
    path->setPath(ppath);
    TupItemConverter::copyProperties(item, path);
    
    return path;
}

static QString serializeRepresentation(QGraphicsItem *item)
{
    if (!item)
        return QString();

    TupAbstractSerializable *serializable = dynamic_cast<TupAbstractSerializable *>(item);
    if (!serializable)
        return QString();

    QDomDocument doc;
    doc.appendChild(serializable->toXml(doc));
    return doc.toString(0);
}

static void setConversionError(QString *errorCode, const QString &value)
{
    if (errorCode)
        *errorCode = value;
}

bool TupItemConverter::convertToPath(TupGraphicObject *object, TupConversionResult *result,
                                     QString *errorCode)
{
    if (errorCode)
        errorCode->clear();

    if (!object) {
        setConversionError(errorCode, QStringLiteral("missing_object"));
        return false;
    }

    if (!result) {
        setConversionError(errorCode, QStringLiteral("missing_result"));
        return false;
    }

    QGraphicsItem *source = object->item();
    if (!source) {
        setConversionError(errorCode, QStringLiteral("missing_representation"));
        return false;
    }

    if (source->type() == TupPathItem::Type) {
        setConversionError(errorCode, QStringLiteral("already_path"));
        return false;
    }

    QPainterPath pathData;
    switch (source->type()) {
        case TupRectItem::Type:
            pathData.addRect(qgraphicsitem_cast<TupRectItem *>(source)->rect());
            break;
        case TupEllipseItem::Type:
            pathData.addEllipse(qgraphicsitem_cast<TupEllipseItem *>(source)->rect());
            break;
        default:
            setConversionError(errorCode, QStringLiteral("unsupported_source_type"));
            return false;
    }

    const QString sourceSnapshot = serializeRepresentation(source);
    if (sourceSnapshot.isEmpty()) {
        setConversionError(errorCode, QStringLiteral("serialize_source_failed"));
        return false;
    }

    TupPathItem *target = new TupPathItem(source->parentItem());
    target->setPath(pathData);
    copyProperties(source, target);
    target->setZValue(source->zValue());

    const QString targetSnapshot = serializeRepresentation(target);
    if (targetSnapshot.isEmpty()) {
        delete target;
        setConversionError(errorCode, QStringLiteral("serialize_target_failed"));
        return false;
    }

    result->objectId = object->objectId();
    result->sourceRepresentation = sourceSnapshot;
    result->targetRepresentation = targetSnapshot;

    object->setItem(target);
    return true;
}

bool TupItemConverter::applyRepresentation(TupGraphicObject *object, const QString &representationXml,
                                           QString *errorCode)
{
    if (errorCode)
        errorCode->clear();

    if (!object) {
        setConversionError(errorCode, QStringLiteral("missing_object"));
        return false;
    }

    QGraphicsItem *current = object->item();
    if (!current) {
        setConversionError(errorCode, QStringLiteral("missing_representation"));
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(representationXml)) {
        setConversionError(errorCode, QStringLiteral("invalid_snapshot_xml"));
        return false;
    }

    const QString rootName = doc.documentElement().tagName();
    if (rootName != QStringLiteral("path") &&
        rootName != QStringLiteral("rect") &&
        rootName != QStringLiteral("ellipse")) {
        setConversionError(errorCode, QStringLiteral("unsupported_snapshot_type"));
        return false;
    }

    TupItemFactory factory;
    QGraphicsItem *restored = factory.create(representationXml);
    if (!restored) {
        setConversionError(errorCode, QStringLiteral("restore_failed"));
        return false;
    }

    restored->setZValue(current->zValue());
    object->setItem(restored);
    return true;
}

TupEllipseItem *TupItemConverter::convertToEllipse(QGraphicsItem *item)
{
    TupEllipseItem *ellipse = new TupEllipseItem(item->parentItem());
    
    switch (item->type()) {

        case TupPathItem::Type:
        {
            ellipse->setRect(qgraphicsitem_cast<QGraphicsPathItem *>(item)->path().boundingRect());
        }
        break;

        case TupEllipseItem::Type:
        {
            ellipse->setRect(qgraphicsitem_cast<QGraphicsEllipseItem *>(item)->rect());
        }
        break;
        // TODO: default case
    }
    
    TupItemConverter::copyProperties(item, ellipse);
    
    return ellipse;
}

TupRectItem *TupItemConverter::convertToRect(QGraphicsItem *item)
{
    TupRectItem *rect = new TupRectItem(item->parentItem());
    
    switch (item->type()) {

        case TupPathItem::Type:
        {
            rect->setRect(qgraphicsitem_cast<QGraphicsPathItem *>(item)->path().boundingRect());
        }
        break;

        case TupEllipseItem::Type:
        {
            rect->setRect(qgraphicsitem_cast<QGraphicsEllipseItem *>(item)->rect());
        }
        break;
        // TODO: include other kind of objects
    }
    
    TupItemConverter::copyProperties(item, rect);
    
    return rect;
}

TupLineItem *TupItemConverter::convertToLine(QGraphicsItem *item)
{
    TupLineItem *line = new TupLineItem(item->parentItem());

    switch (item->type()) {

        case TupPathItem::Type:
        {
            QRectF rect = qgraphicsitem_cast<QGraphicsPathItem *>(item)->path().boundingRect();
            line->setLine(QLineF(rect.topLeft(), rect.bottomRight()));
        }
        break;

        case TupEllipseItem::Type:
        {
            QRectF rect = qgraphicsitem_cast<QGraphicsEllipseItem *>(item)->rect();
            line->setLine(QLineF(rect.topLeft(), rect.bottomRight()));
        }
        break;
        // TODO: include other kind of objects
    }

    TupItemConverter::copyProperties(item, line);
    
    return line;
}
