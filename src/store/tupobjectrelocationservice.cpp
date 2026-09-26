/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GPL v2 or later                                              *
 ***************************************************************************/

#include "tupobjectrelocationservice.h"
#include "tupframe.h"
#include "tupgraphicobject.h"
#include "tuplayer.h"

TupObjectRelocationService::Result::Result()
    : success(false), sourcePosition(-1), targetPosition(-1)
{
}

TupObjectRelocationService::Result TupObjectRelocationService::relocate(
        TupGraphicObject *object, TupFrame *targetFrame, int requestedTargetPosition)
{
    Result result;

    if (!object) {
        result.error = QStringLiteral("Native relocation requires a graphic object");
        return result;
    }

    TupFrame *sourceFrame = object->frame();
    if (!sourceFrame || !targetFrame) {
        result.error = QStringLiteral("Native relocation requires source and target frames");
        return result;
    }

    if (sourceFrame->frameType() != TupFrame::Regular
            || targetFrame->frameType() != TupFrame::Regular) {
        result.error = QStringLiteral("Native relocation currently supports regular frames only");
        return result;
    }

    TupLayer *sourceLayer = sourceFrame->parentLayer();
    TupLayer *targetLayer = targetFrame->parentLayer();
    if (!sourceLayer || !targetLayer || sourceLayer != targetLayer) {
        result.error = QStringLiteral("Native relocation currently requires frames in the same layer");
        return result;
    }

    const QString objectId = object->objectId().trimmed();
    if (objectId.isEmpty()) {
        result.error = QStringLiteral("Native relocation requires object_id");
        return result;
    }

    const int sourcePosition = sourceFrame->indexOf(object);
    if (sourcePosition < 0 || sourceFrame->graphicById(objectId) != object) {
        result.error = QStringLiteral("Source frame does not own the requested object_id");
        return result;
    }

    result.sourcePosition = sourcePosition;

    if (sourceFrame == targetFrame) {
        result.targetPosition = sourcePosition;
        result.success = true;
        return result;
    }

    if (targetFrame->graphicById(objectId)) {
        result.error = QStringLiteral("Target frame already contains object_id");
        return result;
    }

    const int targetPosition = requestedTargetPosition < 0
            ? qMin(sourcePosition, targetFrame->graphicsCount())
            : qBound(0, requestedTargetPosition, targetFrame->graphicsCount());

    TupGraphicObject *detachedObject = nullptr;
    QString label;
    int sourceZLevel = -1;
    if (!sourceFrame->takeGraphicObjectForRelocation(sourcePosition, &detachedObject,
                                                      &label, &sourceZLevel)
            || detachedObject != object) {
        result.error = QStringLiteral("Failed to detach native object from source frame");
        return result;
    }

    if (targetFrame->insertGraphicObjectForRelocation(targetPosition, detachedObject,
                                                       label, sourceZLevel)) {
        result.targetPosition = targetPosition;
        result.success = true;
        return result;
    }

    // Roll back to the exact wrapper, source list position, label, and z slot.
    // If this fails, keep the error explicit: callers must not continue toward
    // an authoritative commit with a half-relocated object.
    if (!sourceFrame->insertGraphicObjectForRelocation(sourcePosition, detachedObject,
                                                        label, sourceZLevel)) {
        result.error = QStringLiteral("Relocation failed and source rollback also failed");
        return result;
    }

    result.error = QStringLiteral("Failed to attach native object to target frame");
    return result;
}
