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

#ifndef TUPITEMGROUP_H
#define TUPITEMGROUP_H

#include "tglobal.h"
#include "tupabstractserializable.h"

#include <QGraphicsItemGroup>
#include <QHash>

class TupGraphicObject;

class TUPITUBE_EXPORT TupItemGroup: public TupAbstractSerializable, public QGraphicsItemGroup
{
    public:
        TupItemGroup(QGraphicsItem *parent = nullptr);
        ~TupItemGroup();

        void addToGroup(QGraphicsItem *item);
        void addToGroup(QGraphicsItem *item, const QString &objectId,
                        const QString &objectName, TupGraphicObject *object = nullptr);
        void setChildMetadata(QGraphicsItem *item, const QString &objectId,
                              const QString &objectName = QString());
        QString childObjectId(QGraphicsItem *item) const;
        QString childObjectName(QGraphicsItem *item) const;
        TupGraphicObject *childGraphicObject(QGraphicsItem *item) const;
        virtual void fromXml(const QString &xml);
        virtual QDomElement toXml(QDomDocument &doc) const;
        void recoverChilds();
        QList<QGraphicsItem *> childItems();

    private:
        QList<QGraphicsItem *> children;
        QHash<QGraphicsItem *, QString> childObjectIds;
        QHash<QGraphicsItem *, QString> childObjectNames;
        QHash<QGraphicsItem *, TupGraphicObject *> childObjects;
};

#endif
