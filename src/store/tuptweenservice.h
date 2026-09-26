/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GPL v2 or later                                              *
 ***************************************************************************/

#ifndef TUPTWEENSERVICE_H
#define TUPTWEENSERVICE_H

#include "tglobal.h"

#include <QString>

class TupScene;

class TUPITUBE_EXPORT TupTweenService
{
    public:
        struct Result {
            Result();

            bool success;
            QString sourceSnapshot;
            QString targetSnapshot;
            QString error;
        };

        // Applies one semantic Motion rebase for all native members carried
        // by the payload. The same TupGraphicObject wrappers and object_ids
        // survive the operation. On failure the exact source snapshot is
        // restored before returning false.
        static Result rebaseMotionTween(TupScene *scene, const QString &payload);

        // Restores an exact snapshot captured by rebaseMotionTween(). Used by
        // Undo/Redo; no inverse rebase is computed.
        static bool restoreMotionTweenSnapshot(TupScene *scene,
                                               const QString &snapshot,
                                               QString *error = nullptr);

        static QString packSnapshots(const QString &sourceSnapshot,
                                     const QString &targetSnapshot);
        static bool unpackSnapshots(const QString &state,
                                    QString *sourceSnapshot,
                                    QString *targetSnapshot);
};

#endif
