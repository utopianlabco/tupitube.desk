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

#ifndef TUPREPRESENTATIONSNAPSHOTSERVICE_H
#define TUPREPRESENTATIONSNAPSHOTSERVICE_H

#include "tglobal.h"
#include "tuprepresentationsnapshot.h"

class QGraphicsItem;
class TupLibrary;

class TUPITUBE_EXPORT TupRepresentationSnapshotService
{
    private:
        TupRepresentationSnapshotService();

    public:
        static bool capture(QGraphicsItem *item, TupRepresentationSnapshot *snapshot);
        static QGraphicsItem *createItem(const TupRepresentationSnapshot &snapshot,
                                         const TupLibrary *library = nullptr,
                                         QGraphicsItem *parent = nullptr);
};

#endif
