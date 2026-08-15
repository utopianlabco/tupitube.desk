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

#ifndef TUPREPRESENTATIONSNAPSHOT_H
#define TUPREPRESENTATIONSNAPSHOT_H

#include "tglobal.h"

#include <QString>

class TUPITUBE_EXPORT TupRepresentationSnapshot
{
    public:
        TupRepresentationSnapshot();
        TupRepresentationSnapshot(const QString &type, const QString &payload,
                                  qreal zValue, qreal opacity, bool visible);

        int version() const;
        QString type() const;
        QString payload() const;
        qreal zValue() const;
        qreal opacity() const;
        bool isVisible() const;

        bool isValid() const;
        QString toXml() const;
        bool fromXml(const QString &xml);

    private:
        static const int FormatVersion = 1;

        int snapshotVersion;
        QString representationType;
        QString representationPayload;
        qreal representationZValue;
        qreal representationOpacity;
        bool representationVisible;
};

#endif
