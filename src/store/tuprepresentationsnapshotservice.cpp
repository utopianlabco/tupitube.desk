/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tuprepresentationsnapshotservice.h"
#include "tupabstractserializable.h"
#include "tupitemfactory.h"

#include <QDomDocument>
#include <QGraphicsItem>

bool TupRepresentationSnapshotService::capture(QGraphicsItem *item, TupRepresentationSnapshot *snapshot)
{
    if (!item || !snapshot)
        return false;

    TupAbstractSerializable *serializable = dynamic_cast<TupAbstractSerializable *>(item);
    if (!serializable)
        return false;

    QDomDocument document;
    const QDomElement representation = serializable->toXml(document);
    if (representation.isNull())
        return false;

    document.appendChild(representation);

    const TupRepresentationSnapshot captured(representation.tagName(), document.toString(-1),
                                              item->zValue(), item->opacity(), item->isVisible());
    if (!captured.isValid())
        return false;

    *snapshot = captured;
    return true;
}

QGraphicsItem *TupRepresentationSnapshotService::createItem(const TupRepresentationSnapshot &snapshot,
                                                             const TupLibrary *library,
                                                             QGraphicsItem *parent)
{
    if (!snapshot.isValid())
        return nullptr;

    TupItemFactory factory;
    factory.setLibrary(library);

    QGraphicsItem *item = factory.create(snapshot.payload());
    if (!item)
        return nullptr;

    item->setParentItem(parent);
    item->setZValue(snapshot.zValue());
    item->setOpacity(snapshot.opacity());
    item->setVisible(snapshot.isVisible());

    return item;
}
