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

        // Updates one native Motion tween member by durable tween_id/object_id.
        // The returned snapshots are exact serialized tween states used for
        // rollback, Undo/Redo, and authoritative-result delivery.
        static Result updateMotionTweenPath(TupScene *scene,
                                            const QString &tweenId,
                                            const QString &objectId,
                                            const QString &route);

        static bool restoreMotionTweenMemberSnapshot(TupScene *scene,
                                                     const QString &objectId,
                                                     const QString &snapshot,
                                                     QString *error = nullptr);

        // Restores an exact snapshot captured by rebaseMotionTween(). Used by
        // Undo/Redo; no inverse rebase is computed.
        static bool restoreMotionTweenSnapshot(TupScene *scene,
                                               const QString &snapshot,
                                               QString *error = nullptr);

        // Rebuilds the current authoritative state using the identity/membership
        // described by a previously captured snapshot. This is used for
        // optimistic-concurrency validation before an authoritative restore.
        static QString currentMotionTweenSnapshot(TupScene *scene,
                                                  const QString &referenceSnapshot,
                                                  QString *error = nullptr);

        static QString packSnapshots(const QString &sourceSnapshot,
                                     const QString &targetSnapshot);
        static bool unpackSnapshots(const QString &state,
                                    QString *sourceSnapshot,
                                    QString *targetSnapshot);
};

#endif
