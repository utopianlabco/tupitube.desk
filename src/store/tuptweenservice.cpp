/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License: GPL v2 or later                                              *
 ***************************************************************************/

#include "tuptweenservice.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tupframe.h"
#include "tupgraphicobject.h"
#include "tupitemtweener.h"
#include "tupobjectrelocationservice.h"

#include <QDomDocument>
#include <QByteArray>

namespace {

struct MemberState
{
    QString objectId;
    int layerIndex = -1;
    int frameIndex = -1;
    int position = -1;
    QString tweenXml;
};

QString encode(const QString &value)
{
    return QString::fromLatin1(value.toUtf8().toBase64());
}

QString decode(const QString &value)
{
    return QString::fromUtf8(QByteArray::fromBase64(value.toLatin1()));
}

QString tweenXml(TupItemTweener *tween)
{
    if (!tween)
        return QString();

    QDomDocument document;
    document.appendChild(tween->toXml(document));
    return document.toString(0);
}

TupGraphicObject *findGraphicObject(TupScene *scene, const QString &objectId,
                                    int *layerIndex = nullptr,
                                    int *frameIndex = nullptr,
                                    int *position = nullptr)
{
    if (!scene || objectId.trimmed().isEmpty())
        return nullptr;

    for (int layerPos = 0; layerPos < scene->layersCount(); ++layerPos) {
        TupLayer *layer = scene->layerAt(layerPos);
        if (!layer)
            continue;

        for (int framePos = 0; framePos < layer->framesCount(); ++framePos) {
            TupFrame *frame = layer->frameAt(framePos);
            if (!frame)
                continue;

            TupGraphicObject *object = frame->graphicById(objectId);
            if (!object)
                continue;

            if (layerIndex)
                *layerIndex = layerPos;
            if (frameIndex)
                *frameIndex = framePos;
            if (position)
                *position = frame->graphicIndexById(objectId);
            return object;
        }
    }

    return nullptr;
}

bool replaceTween(TupGraphicObject *object, const QString &xml,
                  const QString &expectedTweenId, int zLevel,
                  QString *error)
{
    if (!object) {
        if (error)
            *error = QStringLiteral("Missing native tween member");
        return false;
    }

    TupItemTweener *replacement = new TupItemTweener();
    replacement->fromXml(xml);
    if (replacement->tweenId().trimmed() != expectedTweenId) {
        delete replacement;
        if (error)
            *error = QStringLiteral("Tween snapshot identity mismatch");
        return false;
    }

    replacement->setZLevel(zLevel);
    object->addTween(replacement);
    return true;
}

QString createSnapshot(TupScene *scene, const QString &tweenId,
                       const QList<QString> &objectIds, QString *error)
{
    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("tween_rebase_snapshot"));
    root.setAttribute(QStringLiteral("tween_id"), tweenId);

    int snapshotLayerIndex = -1;
    int snapshotFrameCount = -1;

    for (const QString &objectId : objectIds) {
        int layerIndex = -1;
        int frameIndex = -1;
        int position = -1;
        TupGraphicObject *object = findGraphicObject(scene, objectId,
                                                     &layerIndex, &frameIndex,
                                                     &position);
        if (!object) {
            if (error)
                *error = QStringLiteral("Snapshot member object_id was not found");
            return QString();
        }

        if (snapshotLayerIndex < 0) {
            snapshotLayerIndex = layerIndex;
            TupLayer *snapshotLayer = scene->layerAt(snapshotLayerIndex);
            snapshotFrameCount = snapshotLayer ? snapshotLayer->framesCount() : -1;
        } else if (snapshotLayerIndex != layerIndex) {
            if (error)
                *error = QStringLiteral("Tween rebase snapshot currently requires one layer");
            return QString();
        }

        TupItemTweener *tween = object->tweenById(tweenId);
        const QString xml = tweenXml(tween);
        if (!tween || xml.isEmpty()) {
            if (error)
                *error = QStringLiteral("Snapshot member is missing tween_id");
            return QString();
        }

        QDomElement member = document.createElement(QStringLiteral("member"));
        member.setAttribute(QStringLiteral("object_id"), objectId);
        member.setAttribute(QStringLiteral("layer"), layerIndex);
        member.setAttribute(QStringLiteral("frame"), frameIndex);
        member.setAttribute(QStringLiteral("position"), position);
        member.setAttribute(QStringLiteral("tween"), encode(xml));
        root.appendChild(member);
    }

    root.setAttribute(QStringLiteral("layer"), snapshotLayerIndex);
    root.setAttribute(QStringLiteral("frame_count"), snapshotFrameCount);
    document.appendChild(root);
    return document.toString(0);
}

bool parseSnapshot(const QString &snapshot, QString *tweenId, int *layerIndex,
                   int *frameCount, QList<MemberState> *members, QString *error)
{
    QDomDocument document;
    if (!document.setContent(snapshot)) {
        if (error)
            *error = QStringLiteral("Invalid tween rebase snapshot XML");
        return false;
    }

    QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("tween_rebase_snapshot")) {
        if (error)
            *error = QStringLiteral("Unexpected tween rebase snapshot root");
        return false;
    }

    const QString id = root.attribute(QStringLiteral("tween_id")).trimmed();
    const int snapshotLayerIndex = root.attribute(QStringLiteral("layer"), QStringLiteral("-1")).toInt();
    const int snapshotFrameCount = root.attribute(QStringLiteral("frame_count"), QStringLiteral("-1")).toInt();
    if (id.isEmpty() || snapshotLayerIndex < 0 || snapshotFrameCount < 1) {
        if (error)
            *error = QStringLiteral("Tween rebase snapshot is missing tween_id");
        return false;
    }

    QList<MemberState> parsedMembers;
    QDomElement memberElement = root.firstChildElement(QStringLiteral("member"));
    while (!memberElement.isNull()) {
        MemberState state;
        state.objectId = memberElement.attribute(QStringLiteral("object_id")).trimmed();
        state.layerIndex = memberElement.attribute(QStringLiteral("layer")).toInt();
        state.frameIndex = memberElement.attribute(QStringLiteral("frame")).toInt();
        state.position = memberElement.attribute(QStringLiteral("position")).toInt();
        state.tweenXml = decode(memberElement.attribute(QStringLiteral("tween")));

        if (state.objectId.isEmpty() || state.layerIndex < 0
                || state.frameIndex < 0 || state.position < 0
                || state.tweenXml.isEmpty()) {
            if (error)
                *error = QStringLiteral("Incomplete tween rebase snapshot member");
            return false;
        }

        parsedMembers.append(state);
        memberElement = memberElement.nextSiblingElement(QStringLiteral("member"));
    }

    if (parsedMembers.isEmpty()) {
        if (error)
            *error = QStringLiteral("Tween rebase snapshot has no members");
        return false;
    }

    if (tweenId)
        *tweenId = id;
    if (layerIndex)
        *layerIndex = snapshotLayerIndex;
    if (frameCount)
        *frameCount = snapshotFrameCount;
    if (members)
        *members = parsedMembers;
    return true;
}

bool parsePayload(const QString &payload, QString *tweenId, int *targetLayer,
                  int *targetFrame, QList<MemberState> *members, QString *error)
{
    QDomDocument document;
    if (!document.setContent(payload)) {
        if (error)
            *error = QStringLiteral("Invalid RebaseTween payload XML");
        return false;
    }

    QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("tween_rebase")) {
        if (error)
            *error = QStringLiteral("Unexpected RebaseTween payload root");
        return false;
    }

    const QString id = root.attribute(QStringLiteral("tween_id")).trimmed();
    const int layerIndex = root.attribute(QStringLiteral("target_layer"), QStringLiteral("-1")).toInt();
    const int frameIndex = root.attribute(QStringLiteral("target_frame"), QStringLiteral("-1")).toInt();
    if (id.isEmpty() || layerIndex < 0 || frameIndex < 0) {
        if (error)
            *error = QStringLiteral("RebaseTween payload is missing target identity");
        return false;
    }

    QList<MemberState> parsedMembers;
    QDomElement memberElement = root.firstChildElement(QStringLiteral("member"));
    while (!memberElement.isNull()) {
        MemberState state;
        state.objectId = memberElement.attribute(QStringLiteral("object_id")).trimmed();
        state.tweenXml = decode(memberElement.attribute(QStringLiteral("tween")));
        if (state.objectId.isEmpty() || state.tweenXml.isEmpty()) {
            if (error)
                *error = QStringLiteral("RebaseTween member is missing object_id or target tween");
            return false;
        }

        TupItemTweener parsedTween;
        parsedTween.fromXml(state.tweenXml);
        if (parsedTween.tweenId().trimmed() != id
                || parsedTween.getInitLayer() != layerIndex
                || parsedTween.getInitFrame() != frameIndex) {
            if (error)
                *error = QStringLiteral("RebaseTween member target snapshot conflicts with request identity");
            return false;
        }

        parsedMembers.append(state);
        memberElement = memberElement.nextSiblingElement(QStringLiteral("member"));
    }

    if (parsedMembers.isEmpty()) {
        if (error)
            *error = QStringLiteral("RebaseTween payload has no members");
        return false;
    }

    if (tweenId)
        *tweenId = id;
    if (targetLayer)
        *targetLayer = layerIndex;
    if (targetFrame)
        *targetFrame = frameIndex;
    if (members)
        *members = parsedMembers;
    return true;
}

} // namespace

TupTweenService::Result::Result() : success(false)
{
}

TupTweenService::Result TupTweenService::rebaseTween(TupScene *scene,
                                                      const QString &payload)
{
    Result result;
    if (!scene) {
        result.error = QStringLiteral("RebaseTween requires a scene");
        return result;
    }

    QString tweenId;
    int targetLayerIndex = -1;
    int targetFrameIndex = -1;
    QList<MemberState> targetMembers;
    if (!parsePayload(payload, &tweenId, &targetLayerIndex, &targetFrameIndex,
                      &targetMembers, &result.error)) {
        return result;
    }

    TupItemTweener *existingTween = scene->tweenById(tweenId);
    if (!existingTween) {
        result.error = QStringLiteral("RebaseTween tween_id was not found");
        return result;
    }

    const int existingTweenType = existingTween->getType();
    for (const MemberState &member : targetMembers) {
        TupItemTweener targetTween;
        targetTween.fromXml(member.tweenXml);
        if (targetTween.getType() != existingTweenType) {
            result.error = QStringLiteral("RebaseTween member target type does not match existing tween");
            return result;
        }
    }

    TupLayer *targetLayer = scene->layerAt(targetLayerIndex);
    if (!targetLayer) {
        result.error = QStringLiteral("RebaseTween target layer does not exist");
        return result;
    }

    QList<QString> objectIds;
    for (int memberIndex = 0; memberIndex < targetMembers.size(); ++memberIndex) {
        MemberState &member = targetMembers[memberIndex];
        if (objectIds.contains(member.objectId)) {
            result.error = QStringLiteral("RebaseTween contains duplicate object_id");
            return result;
        }

        int currentLayerIndex = -1;
        int currentPosition = -1;
        TupGraphicObject *object = findGraphicObject(scene, member.objectId,
                                                     &currentLayerIndex, nullptr,
                                                     &currentPosition);
        if (!object || !object->tweenById(tweenId)) {
            result.error = QStringLiteral("RebaseTween member object_id does not own tween_id");
            return result;
        }

        if (currentLayerIndex != targetLayerIndex || currentPosition < 0) {
            result.error = QStringLiteral("RebaseTween currently requires same-layer native members");
            return result;
        }

        member.position = currentPosition;
        objectIds.append(member.objectId);
    }

    const QList<QGraphicsItem *> indexedMembers = scene->getItemsFromTweenId(tweenId);
    if (indexedMembers.size() != objectIds.size()) {
        result.error = QStringLiteral("RebaseTween payload does not contain the complete tween membership");
        return result;
    }

    result.sourceSnapshot = createSnapshot(scene, tweenId, objectIds, &result.error);
    if (result.sourceSnapshot.isEmpty())
        return result;

    int requiredFrameCount = targetFrameIndex + 1;
    for (const MemberState &member : targetMembers) {
        TupItemTweener targetTween;
        targetTween.fromXml(member.tweenXml);
        requiredFrameCount = qMax(requiredFrameCount,
                                  targetFrameIndex + qMax(1, targetTween.getFrames()));
    }

    while (targetLayer->framesCount() < requiredFrameCount) {
        const int newFrameIndex = targetLayer->framesCount();
        if (!targetLayer->createFrame(QStringLiteral("Frame"), newFrameIndex)) {
            result.error = QStringLiteral("RebaseTween could not extend the target layer");
            QString rollbackError;
            restoreTweenSnapshot(scene, result.sourceSnapshot, &rollbackError);
            return result;
        }
    }

    TupFrame *targetFrame = targetLayer->frameAt(targetFrameIndex);
    if (!targetFrame) {
        result.error = QStringLiteral("RebaseTween target frame does not exist after extension");
        QString rollbackError;
        restoreTweenSnapshot(scene, result.sourceSnapshot, &rollbackError);
        return result;
    }

    bool applied = true;
    for (const MemberState &member : targetMembers) {
        TupGraphicObject *object = findGraphicObject(scene, member.objectId);
        if (!object) {
            result.error = QStringLiteral("RebaseTween member disappeared before relocation");
            applied = false;
            break;
        }

        TupObjectRelocationService::Result relocation =
                TupObjectRelocationService::relocate(object, targetFrame, member.position);
        if (!relocation.success) {
            result.error = relocation.error;
            applied = false;
            break;
        }

        const int targetPosition = targetFrame->graphicIndexById(member.objectId);
        if (targetPosition < 0
                || !replaceTween(object, member.tweenXml, tweenId,
                                 targetPosition, &result.error)) {
            applied = false;
            break;
        }
    }

    if (!applied) {
        QString rollbackError;
        if (!restoreTweenSnapshot(scene, result.sourceSnapshot, &rollbackError)) {
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        }
        return result;
    }

    result.targetSnapshot = createSnapshot(scene, tweenId, objectIds, &result.error);
    if (result.targetSnapshot.isEmpty()) {
        QString rollbackError;
        if (!restoreTweenSnapshot(scene, result.sourceSnapshot, &rollbackError)) {
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        }
        return result;
    }

    result.success = true;
    return result;
}

bool TupTweenService::restoreTweenSnapshot(TupScene *scene,
                                             const QString &snapshot,
                                             QString *error)
{
    if (!scene) {
        if (error)
            *error = QStringLiteral("Tween snapshot restore requires a scene");
        return false;
    }

    QString tweenId;
    int snapshotLayerIndex = -1;
    int snapshotFrameCount = -1;
    QList<MemberState> members;
    if (!parseSnapshot(snapshot, &tweenId, &snapshotLayerIndex,
                       &snapshotFrameCount, &members, error))
        return false;

    TupLayer *snapshotLayer = scene->layerAt(snapshotLayerIndex);
    if (!snapshotLayer) {
        if (error)
            *error = QStringLiteral("Tween snapshot layer cannot be resolved");
        return false;
    }

    while (snapshotLayer->framesCount() < snapshotFrameCount) {
        const int newFrameIndex = snapshotLayer->framesCount();
        if (!snapshotLayer->createFrame(QStringLiteral("Frame"), newFrameIndex)) {
            if (error)
                *error = QStringLiteral("Tween snapshot could not restore required frame count");
            return false;
        }
    }

    // Validate the entire destination state before moving any wrapper.
    for (const MemberState &member : members) {
        TupLayer *layer = scene->layerAt(member.layerIndex);
        TupFrame *frame = layer ? layer->frameAt(member.frameIndex) : nullptr;
        TupGraphicObject *object = findGraphicObject(scene, member.objectId);
        if (!layer || !frame || !object || !object->tweenById(tweenId)) {
            if (error)
                *error = QStringLiteral("Tween snapshot member cannot be resolved");
            return false;
        }

        if (object->frame()->parentLayer() != layer) {
            if (error)
                *error = QStringLiteral("Tween snapshot restore currently requires same-layer members");
            return false;
        }
    }

    for (const MemberState &member : members) {
        TupLayer *layer = scene->layerAt(member.layerIndex);
        TupFrame *targetFrame = layer->frameAt(member.frameIndex);
        TupGraphicObject *object = findGraphicObject(scene, member.objectId);

        TupObjectRelocationService::Result relocation =
                TupObjectRelocationService::relocate(object, targetFrame, member.position);
        if (!relocation.success) {
            if (error)
                *error = relocation.error;
            return false;
        }

        const int position = targetFrame->graphicIndexById(member.objectId);
        if (position < 0
                || !replaceTween(object, member.tweenXml, tweenId,
                                 position, error)) {
            return false;
        }
    }

    while (snapshotLayer->framesCount() > snapshotFrameCount) {
        if (!snapshotLayer->removeLastEmptyFrameForDomainOperation()) {
            if (error)
                *error = QStringLiteral("Tween snapshot cannot remove non-empty trailing frame");
            return false;
        }
    }

    return true;
}

TupTweenService::Result TupTweenService::rebaseMotionTween(TupScene *scene,
                                                            const QString &payload)
{
    return rebaseTween(scene, payload);
}

bool TupTweenService::restoreMotionTweenSnapshot(TupScene *scene,
                                                  const QString &snapshot,
                                                  QString *error)
{
    return restoreTweenSnapshot(scene, snapshot, error);
}

QString TupTweenService::packSnapshots(const QString &sourceSnapshot,
                                        const QString &targetSnapshot)
{
    if (sourceSnapshot.isEmpty() || targetSnapshot.isEmpty())
        return QString();

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("tween_rebase_state"));
    root.setAttribute(QStringLiteral("source"), encode(sourceSnapshot));
    root.setAttribute(QStringLiteral("target"), encode(targetSnapshot));
    document.appendChild(root);
    return document.toString(0);
}

bool TupTweenService::unpackSnapshots(const QString &state,
                                       QString *sourceSnapshot,
                                       QString *targetSnapshot)
{
    QDomDocument document;
    if (!document.setContent(state))
        return false;

    QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("tween_rebase_state"))
        return false;

    const QString source = decode(root.attribute(QStringLiteral("source")));
    const QString target = decode(root.attribute(QStringLiteral("target")));
    if (source.isEmpty() || target.isEmpty())
        return false;

    if (sourceSnapshot)
        *sourceSnapshot = source;
    if (targetSnapshot)
        *targetSnapshot = target;
    return true;
}
