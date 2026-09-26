/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GPL v2 or later                                              *
 ***************************************************************************/

#ifndef TUPOBJECTRELOCATIONSERVICE_H
#define TUPOBJECTRELOCATIONSERVICE_H

#include "tglobal.h"

#include <QString>

class TupFrame;
class TupGraphicObject;

class TUPITUBE_EXPORT TupObjectRelocationService
{
    public:
        struct Result {
            Result();

            bool success;
            int sourcePosition;
            int targetPosition;
            QString error;
        };

        // Relocates the same native TupGraphicObject wrapper between regular
        // frames in the same layer. No object is serialized/recreated and no
        // frame deletion Undo stack is touched.
        static Result relocate(TupGraphicObject *object, TupFrame *targetFrame,
                               int targetPosition = -1);
};

#endif
