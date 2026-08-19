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

#include "tupitemgroup.h"
#include "tupserializer.h"
#include "tupgraphicobject.h"

TupItemGroup::TupItemGroup(QGraphicsItem *parent) : QGraphicsItemGroup(parent)
{
}

TupItemGroup::~TupItemGroup()
{
}

void TupItemGroup::addToGroup(QGraphicsItem *item)
{
    if (!item)
        return;

    if (!children.contains(item))
        children << item;
    QGraphicsItemGroup::addToGroup(item);
}

void TupItemGroup::addToGroup(QGraphicsItem *item, const QString &objectId,
                              const QString &objectName, TupGraphicObject *object)
{
    if (!item)
        return;

    setChildMetadata(item, objectId, objectName);
    if (object)
        childObjects.insert(item, object);
    addToGroup(item);
}

void TupItemGroup::setChildMetadata(QGraphicsItem *item, const QString &objectId,
                                    const QString &objectName)
{
    if (!item)
        return;

    const QString normalizedObjectId = objectId.trimmed();
    if (!normalizedObjectId.isEmpty())
        childObjectIds.insert(item, normalizedObjectId);

    if (!objectName.isEmpty())
        childObjectNames.insert(item, objectName);
}

QString TupItemGroup::childObjectId(QGraphicsItem *item) const
{
    return childObjectIds.value(item);
}

QString TupItemGroup::childObjectName(QGraphicsItem *item) const
{
    return childObjectNames.value(item);
}

TupGraphicObject *TupItemGroup::childGraphicObject(QGraphicsItem *item) const
{
    return childObjects.value(item, nullptr);
}

void TupItemGroup::recoverChilds()
{
    int total = children.count();
    for(int i=0; i<total; i++) {
        QGraphicsItem *item = children.at(i);
        item->setZValue(i);

        if (TupItemGroup *child = qgraphicsitem_cast<TupItemGroup *>(item))
            child->recoverChilds();
        
        if (item->parentItem() != this)
            item->setParentItem(this);
    }
}

QList<QGraphicsItem *> TupItemGroup::childItems()
{
    return children;
}

void TupItemGroup::fromXml(const QString &)
{
}

QDomElement TupItemGroup::toXml(QDomDocument &doc) const
{
    QDomElement root = doc.createElement("group");
    int total = children.count();
    for(int i=0; i<total; i++) {
        QGraphicsItem *item = children.at(i);
        TupAbstractSerializable *serializable = dynamic_cast<TupAbstractSerializable *>(item);
        if (!serializable)
            continue;

        QDomElement child = serializable->toXml(doc);
        const QString objectId = childObjectIds.value(item).trimmed();
        if (!objectId.isEmpty())
            child.setAttribute(QStringLiteral("object_id"), objectId);

        const QString objectName = childObjectNames.value(item);
        if (!objectName.isEmpty())
            child.setAttribute(QStringLiteral("object_name"), objectName);

        root.appendChild(child);
    }

    /*
    QPointF point = this->scenePos();
    QString pos = "(" + QString::number(point.x()) + ", " + QString::number(point.y()) + ")";
    root.setAttribute("pos", pos);
    */

    root.appendChild(TupSerializer::properties(this, doc));
    
    return root;
}
