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
    bool present = true;
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


QList<QString> nativeMotionMemberIds(TupScene *scene, const QString &tweenId)
{
    QList<QString> objectIds;
    if (!scene || tweenId.trimmed().isEmpty())
        return objectIds;

    for (int layerIndex = 0; layerIndex < scene->layersCount(); ++layerIndex) {
        TupLayer *layer = scene->layerAt(layerIndex);
        if (!layer)
            continue;

        for (int frameIndex = 0; frameIndex < layer->framesCount(); ++frameIndex) {
            TupFrame *frame = layer->frameAt(frameIndex);
            if (!frame)
                continue;

            for (int objectIndex = 0; objectIndex < frame->graphicsCount(); ++objectIndex) {
                TupGraphicObject *object = frame->graphicAt(objectIndex);
                if (!object || !object->tweenById(tweenId))
                    continue;

                const QString objectId = object->objectId().trimmed();
                if (!objectId.isEmpty() && !objectIds.contains(objectId))
                    objectIds.append(objectId);
            }
        }
    }

    return objectIds;
}

bool parseMotionPathPayload(const QString &payload, const QString &expectedTweenId,
                            QDomDocument *document, QString *route,
                            QString *intervals, int *frames, QString *error)
{
    if (!document || !document->setContent(payload)) {
        if (error)
            *error = QStringLiteral("Invalid UpdateTweenPath payload XML");
        return false;
    }

    const QDomElement root = document->documentElement();
    if (root.tagName() != QStringLiteral("tween_path_update")) {
        if (error)
            *error = QStringLiteral("Unexpected UpdateTweenPath payload root");
        return false;
    }

    const QString tweenId = root.attribute(QStringLiteral("tween_id")).trimmed();
    const QString coords = root.attribute(QStringLiteral("coords")).trimmed();
    const QString intervalText = root.attribute(QStringLiteral("intervals")).trimmed();
    bool framesOk = false;
    const int frameCount = root.attribute(QStringLiteral("frames")).toInt(&framesOk);
    if (tweenId.isEmpty() || tweenId != expectedTweenId.trimmed()
            || coords.isEmpty() || intervalText.isEmpty()
            || !framesOk || frameCount < 1) {
        if (error)
            *error = QStringLiteral("UpdateTweenPath payload is missing canonical Motion state");
        return false;
    }

    int intervalTotal = 0;
    int intervalCount = 0;
    const QStringList values = intervalText.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &value : values) {
        bool ok = false;
        const int interval = value.trimmed().toInt(&ok);
        if (!ok || interval < 1) {
            if (error)
                *error = QStringLiteral("UpdateTweenPath payload contains an invalid interval");
            return false;
        }
        intervalTotal += interval;
        ++intervalCount;
    }

    int stepCount = 0;
    QDomElement step = root.firstChildElement(QStringLiteral("step"));
    while (!step.isNull()) {
        ++stepCount;
        step = step.nextSiblingElement(QStringLiteral("step"));
    }

    if (intervalCount < 1 || intervalTotal != frameCount || stepCount != frameCount) {
        if (error)
            *error = QStringLiteral("UpdateTweenPath payload timing/step state is inconsistent");
        return false;
    }

    if (route)
        *route = coords;
    if (intervals)
        *intervals = intervalText;
    if (frames)
        *frames = frameCount;
    return true;
}

QString motionPathTargetXml(const QString &sourceXml, const QDomElement &payload,
                            const QString &tweenId, const QString &route,
                            const QString &intervals, int frames, QString *error)
{
    QDomDocument document;
    if (!document.setContent(sourceXml)) {
        if (error)
            *error = QStringLiteral("UpdateTweenPath member source snapshot is invalid");
        return QString();
    }

    QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("tweening")
            || root.attribute(QStringLiteral("tween_id")).trimmed() != tweenId
            || root.attribute(QStringLiteral("type")).toInt() != TupItemTweener::Motion) {
        if (error)
            *error = QStringLiteral("UpdateTweenPath member source identity is invalid");
        return QString();
    }

    root.setAttribute(QStringLiteral("coords"), route);
    root.setAttribute(QStringLiteral("intervals"), intervals);
    root.setAttribute(QStringLiteral("frames"), frames);

    while (!root.firstChild().isNull())
        root.removeChild(root.firstChild());

    QDomElement step = payload.firstChildElement(QStringLiteral("step"));
    while (!step.isNull()) {
        root.appendChild(document.importNode(step, true));
        step = step.nextSiblingElement(QStringLiteral("step"));
    }

    return document.toString(0);
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
                       const QList<QString> &objectIds, QString *error,
                       bool allowAbsent = false, bool allLayerCounts = false)
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
        if ((!tween && !allowAbsent) || (tween && xml.isEmpty())) {
            if (error)
                *error = QStringLiteral("Snapshot member is missing tween_id");
            return QString();
        }

        QDomElement member = document.createElement(QStringLiteral("member"));
        member.setAttribute(QStringLiteral("object_id"), objectId);
        member.setAttribute(QStringLiteral("layer"), layerIndex);
        member.setAttribute(QStringLiteral("frame"), frameIndex);
        member.setAttribute(QStringLiteral("position"), position);
        member.setAttribute(QStringLiteral("present"), tween ? 1 : 0);
        member.setAttribute(QStringLiteral("tween"), encode(xml));
        root.appendChild(member);
    }

    if (allLayerCounts) {
        QStringList counts;
        for (int index = 0; index < scene->layersCount(); ++index) {
            TupLayer *layer = scene->layerAt(index);
            if (!layer) return QString();
            counts.append(QString::number(layer->framesCount()));
        }
        root.setAttribute(QStringLiteral("layer_counts"), counts.join(QLatin1Char(',')));
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
        state.present = memberElement.attribute(QStringLiteral("present"), QStringLiteral("1")) != QStringLiteral("0");

        if (state.objectId.isEmpty() || state.layerIndex < 0
                || state.frameIndex < 0 || state.position < 0
                || (state.present && state.tweenXml.isEmpty())) {
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

TupTweenService::Result TupTweenService::updateMotionTweenPath(
    TupScene *scene, const QString &tweenId, const QString &objectId,
    const QString &payload)
{
    Result result;
    const QString normalizedTweenId = tweenId.trimmed();
    const QString normalizedObjectId = objectId.trimmed();

    if (!scene) {
        result.error = QStringLiteral("UpdateTweenPath requires a scene");
        return result;
    }
    if (normalizedTweenId.isEmpty()) {
        result.error = QStringLiteral("UpdateTweenPath requires tween_id");
        return result;
    }
    if (normalizedObjectId.isEmpty()) {
        result.error = QStringLiteral("UpdateTweenPath requires object_id");
        return result;
    }

    QDomDocument payloadDocument;
    QString route;
    QString intervals;
    int frames = 0;
    if (!parseMotionPathPayload(payload, normalizedTweenId, &payloadDocument,
                                &route, &intervals, &frames, &result.error)) {
        return result;
    }

    TupItemTweener *logicalTween = scene->tweenById(normalizedTweenId);
    if (!logicalTween) {
        result.error = QStringLiteral("UpdateTweenPath tween_id was not found");
        return result;
    }
    if (logicalTween->getType() != TupItemTweener::Motion) {
        result.error = QStringLiteral("UpdateTweenPath requires a Motion tween");
        return result;
    }

    int representativePosition = -1;
    TupGraphicObject *representative = findGraphicObject(
        scene, normalizedObjectId, nullptr, nullptr, &representativePosition);
    TupItemTweener *representativeTween = representative
        ? representative->tweenById(normalizedTweenId) : nullptr;
    if (!representative || representativePosition < 0 || !representativeTween
            || representativeTween->getType() != TupItemTweener::Motion) {
        result.error = QStringLiteral("UpdateTweenPath object_id is not bound to tween_id");
        return result;
    }

    const QList<QString> objectIds = nativeMotionMemberIds(scene, normalizedTweenId);
    if (objectIds.isEmpty() || !objectIds.contains(normalizedObjectId)) {
        result.error = QStringLiteral("UpdateTweenPath native membership could not be resolved");
        return result;
    }

    // This milestone is native-only. Refuse to claim one atomic logical update
    // if the tween also contains an SVG member whose durable identity is still
    // intentionally deferred.
    const QList<QGraphicsItem *> indexedMembers = scene->getItemsFromTweenId(normalizedTweenId);
    if (indexedMembers.size() != objectIds.size()) {
        result.error = QStringLiteral("UpdateTweenPath atomic native update does not support SVG members");
        return result;
    }

    result.sourceSnapshot = createSnapshot(
        scene, normalizedTweenId, objectIds, &result.error);
    if (result.sourceSnapshot.isEmpty())
        return result;

    const QDomElement payloadRoot = payloadDocument.documentElement();
    bool applied = true;
    for (const QString &memberObjectId : objectIds) {
        int layerIndex = -1;
        int position = -1;
        TupGraphicObject *object = findGraphicObject(
            scene, memberObjectId, &layerIndex, nullptr, &position);
        TupItemTweener *memberTween = object
            ? object->tweenById(normalizedTweenId) : nullptr;
        const QString sourceXml = tweenXml(memberTween);
        if (!object || position < 0 || !memberTween
                || memberTween->getType() != TupItemTweener::Motion
                || sourceXml.isEmpty()) {
            result.error = QStringLiteral("UpdateTweenPath member disappeared before mutation");
            applied = false;
            break;
        }

        const QString targetXml = motionPathTargetXml(
            sourceXml, payloadRoot, normalizedTweenId, route,
            intervals, frames, &result.error);
        if (targetXml.isEmpty()
                || !replaceTween(object, targetXml, normalizedTweenId,
                                 position, &result.error)) {
            applied = false;
            break;
        }

        scene->addTweenObject(layerIndex, object);
    }

    if (!applied) {
        QString rollbackError;
        if (!restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError))
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        return result;
    }

    result.targetSnapshot = createSnapshot(
        scene, normalizedTweenId, objectIds, &result.error);
    if (result.targetSnapshot.isEmpty()) {
        QString rollbackError;
        if (!restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError))
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        return result;
    }

    result.success = true;
    return result;
}

bool TupTweenService::restoreMotionTweenMemberSnapshot(
    TupScene *scene, const QString &objectId, const QString &snapshot, QString *error)
{
    if (!scene) {
        if (error)
            *error = QStringLiteral("Motion tween restore requires a scene");
        return false;
    }

    const QString normalizedObjectId = objectId.trimmed();
    if (normalizedObjectId.isEmpty() || snapshot.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Motion tween restore requires object_id and snapshot");
        return false;
    }

    TupItemTweener parsedTween;
    parsedTween.fromXml(snapshot);
    const QString tweenId = parsedTween.tweenId().trimmed();
    if (tweenId.isEmpty() || parsedTween.getType() != TupItemTweener::Motion) {
        if (error)
            *error = QStringLiteral("Motion tween restore snapshot has invalid identity or type");
        return false;
    }

    int layerIndex = -1;
    int frameIndex = -1;
    int position = -1;
    TupGraphicObject *object = findGraphicObject(
        scene, normalizedObjectId, &layerIndex, &frameIndex, &position);
    TupItemTweener *memberTween = object ? object->tweenById(tweenId) : nullptr;
    if (!object || position < 0 || !memberTween
            || memberTween->getType() != TupItemTweener::Motion) {
        if (error)
            *error = QStringLiteral("Motion tween restore target cannot be resolved");
        return false;
    }

    // Restore the exact serialized tween state on the existing logical tween
    // instance so tools holding the current tween pointer do not observe an
    // identity-breaking object replacement.
    memberTween->fromXml(snapshot);
    memberTween->setZLevel(position);
    scene->addTweenObject(layerIndex, object);
    return true;
}

QString TupTweenService::currentMotionTweenMemberSnapshot(
    TupScene *scene, const QString &objectId, const QString &referenceSnapshot,
    QString *error)
{
    if (!scene) {
        if (error)
            *error = QStringLiteral("Motion tween member snapshot capture requires a scene");
        return QString();
    }

    const QString normalizedObjectId = objectId.trimmed();
    if (normalizedObjectId.isEmpty() || referenceSnapshot.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral(
                "Motion tween member snapshot capture requires object_id and reference snapshot");
        return QString();
    }

    TupItemTweener referenceTween;
    referenceTween.fromXml(referenceSnapshot);
    const QString tweenId = referenceTween.tweenId().trimmed();
    if (tweenId.isEmpty() || referenceTween.getType() != TupItemTweener::Motion) {
        if (error)
            *error = QStringLiteral(
                "Motion tween member reference snapshot has invalid identity or type");
        return QString();
    }

    int position = -1;
    TupGraphicObject *object = findGraphicObject(
        scene, normalizedObjectId, nullptr, nullptr, &position);
    TupItemTweener *memberTween = object ? object->tweenById(tweenId) : nullptr;
    if (!object || position < 0 || !memberTween
            || memberTween->getType() != TupItemTweener::Motion) {
        if (error)
            *error = QStringLiteral(
                "Motion tween member snapshot target cannot be resolved");
        return QString();
    }

    const QString snapshot = tweenXml(memberTween);
    if (snapshot.isEmpty() && error)
        *error = QStringLiteral("Motion tween member snapshot could not be captured");

    return snapshot;
}

TupTweenService::Result TupTweenService::applyMotionTween(TupScene *scene,
                                                              const QString &payload)
{
    Result result;
    if (!scene) {
        result.error = QStringLiteral("ApplyMotionTween requires a scene");
        return result;
    }

    QString tweenId;
    int layerIndex = -1;
    int frameIndex = -1;
    QList<MemberState> members;
    // The native Apply payload deliberately has the same identity fields and
    // member encoding as RebaseTween, but has a distinct protocol root.
    QString normalized = payload;
    QDomDocument requestDocument;
    if (!requestDocument.setContent(payload)
            || requestDocument.documentElement().tagName()
                != QStringLiteral("tween_apply")) {
        result.error = QStringLiteral("Invalid ApplyMotionTween payload");
        return result;
    }
    requestDocument.documentElement().setTagName(QStringLiteral("tween_rebase"));
    normalized = requestDocument.toString(0);
    if (!parsePayload(normalized, &tweenId, &layerIndex, &frameIndex,
                      &members, &result.error))
        return result;

    TupLayer *layer = scene->layerAt(layerIndex);
    if (!layer || frameIndex >= layer->framesCount()) {
        result.error = QStringLiteral("ApplyMotionTween target frame does not exist");
        return result;
    }

    QList<QString> ids;
    int requiredFrameCount = layer->framesCount();
    for (const MemberState &member : members) {
        int objectLayer = -1;
        int objectFrame = -1;
        TupGraphicObject *object = findGraphicObject(
            scene, member.objectId, &objectLayer, &objectFrame);
        if (!object || objectLayer != layerIndex || objectFrame != frameIndex
                || ids.contains(member.objectId)) {
            result.error = QStringLiteral("ApplyMotionTween member identity/location conflict");
            return result;
        }
        TupItemTweener targetTween;
        targetTween.fromXml(member.tweenXml);
        if (targetTween.getType() != TupItemTweener::Motion) {
            result.error = QStringLiteral("ApplyMotionTween requires native Motion members");
            return result;
        }
        for (TupItemTweener *other : object->tweensList()) {
            if (other && other->getType() == TupItemTweener::Motion
                    && other->tweenId() != tweenId) {
                result.error = QStringLiteral("ApplyMotionTween Motion type collision");
                return result;
            }
        }
        ids.append(member.objectId);
        requiredFrameCount = qMax(requiredFrameCount,
                                  frameIndex + qMax(1, targetTween.getFrames()));
    }
    {
        const QList<QString> actualIds = nativeMotionMemberIds(scene, tweenId);
        if (!actualIds.isEmpty() && actualIds.size() != ids.size()) {
            result.error = QStringLiteral("ApplyMotionTween requires complete membership");
            return result;
        }
        for (const QString &id : actualIds) {
            if (!ids.contains(id)) {
                result.error = QStringLiteral("ApplyMotionTween incomplete membership");
                return result;
            }
        }
    }

    if (scene->getItemsFromTweenId(tweenId).size()
            != nativeMotionMemberIds(scene, tweenId).size()) {
        result.error = QStringLiteral("ApplyMotionTween does not support SVG membership");
        return result;
    }

    result.sourceSnapshot = createSnapshot(scene, tweenId, ids, &result.error, true, true);
    if (result.sourceSnapshot.isEmpty())
        return result;

    bool applied = true;
    for (int index = 0; applied && index < scene->layersCount(); ++index) {
        TupLayer *currentLayer = scene->layerAt(index);
        if (!currentLayer) { applied = false; break; }
        while (currentLayer->framesCount() < requiredFrameCount) {
            if (!currentLayer->createFrame(QStringLiteral("Frame"), currentLayer->framesCount())) {
                result.error = QStringLiteral("ApplyMotionTween failed extending scene layers");
                applied = false;
                break;
            }
        }
    }
    if (applied) {
        for (const MemberState &member : members) {
            TupGraphicObject *object = findGraphicObject(scene, member.objectId);
            const int position = object && object->frame()
                ? object->frame()->graphicIndexById(member.objectId) : -1;
            if (position < 0 || !replaceTween(object, member.tweenXml,
                                             tweenId, position, &result.error)) {
                applied = false;
                break;
            }
            scene->addTweenObject(layerIndex, object);
        }
    }
    if (applied) {
        result.targetSnapshot = createSnapshot(scene, tweenId, ids, &result.error, false, true);
        applied = !result.targetSnapshot.isEmpty();
    }
    if (!applied) {
        QString rollbackError;
        if (!restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError))
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        return result;
    }
    result.success = true;
    return result;
}

TupTweenService::Result TupTweenService::rebaseMotionTween(TupScene *scene,
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

        TupItemTweener targetTween;
        targetTween.fromXml(member.tweenXml);
        if (targetTween.getType() != existingTween->getType()) {
            result.error = QStringLiteral("RebaseTween target snapshot type does not match tween_id");
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
            restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError);
            return result;
        }
    }

    TupFrame *targetFrame = targetLayer->frameAt(targetFrameIndex);
    if (!targetFrame) {
        result.error = QStringLiteral("RebaseTween target frame does not exist after extension");
        QString rollbackError;
        restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError);
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
        if (!restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError)) {
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        }
        return result;
    }

    result.targetSnapshot = createSnapshot(scene, tweenId, objectIds, &result.error);
    if (result.targetSnapshot.isEmpty()) {
        QString rollbackError;
        if (!restoreMotionTweenSnapshot(scene, result.sourceSnapshot, &rollbackError)) {
            result.error += QStringLiteral("; rollback failed: ") + rollbackError;
        }
        return result;
    }

    result.success = true;
    return result;
}

bool TupTweenService::restoreMotionTweenSnapshot(TupScene *scene,
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

    QList<int> intendedFrameCounts;
    QDomDocument snapshotDocument;
    if (!snapshotDocument.setContent(snapshot)) return false;
    const QString countsValue = snapshotDocument.documentElement()
        .attribute(QStringLiteral("layer_counts"));
    if (!countsValue.isEmpty()) {
        const QStringList countStrings = countsValue.split(QLatin1Char(','));
        if (countStrings.size() != scene->layersCount()) {
            if (error) *error = QStringLiteral("Tween snapshot layer count mismatch");
            return false;
        }
        for (const QString &value : countStrings) {
            bool ok = false;
            const int count = value.toInt(&ok);
            if (!ok || count < 1) return false;
            intendedFrameCounts.append(count);
        }
    } else {
        for (int index = 0; index < scene->layersCount(); ++index)
            intendedFrameCounts.append(index == snapshotLayerIndex
                ? snapshotFrameCount : scene->layerAt(index)->framesCount());
    }
    for (int index = 0; index < intendedFrameCounts.size(); ++index) {
        TupLayer *layer = scene->layerAt(index);
        if (!layer) return false;
        while (layer->framesCount() < intendedFrameCounts.at(index)) {
            if (!layer->createFrame(QStringLiteral("Frame"), layer->framesCount())) {
                if (error) *error = QStringLiteral("Tween snapshot layer extension failed");
                return false;
            }
        }
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
        if (!layer || !frame || !object
                ) {
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
        if (position < 0) {
            if (error)
                *error = QStringLiteral("Tween snapshot restored object position is invalid");
            return false;
        }

        if (!member.present) {
            const QList<TupItemTweener *> currentTweens = object->tweensList();
            for (int i = 0; i < currentTweens.size(); ++i) {
                if (currentTweens.at(i) && currentTweens.at(i)->tweenId() == tweenId) {
                    object->removeTween(i);
                    break;
                }
            }
            if (!object->hasTweens())
                scene->removeTweenObject(member.layerIndex, object);
        } else if (!replaceTween(object, member.tweenXml, tweenId,
                                 position, error)) {
            return false;
        } else {
            scene->addTweenObject(member.layerIndex, object);
        }
    }

    for (int index = 0; index < intendedFrameCounts.size(); ++index) {
        TupLayer *layer = scene->layerAt(index);
        while (layer->framesCount() > intendedFrameCounts.at(index)) {
            if (!layer->removeLastEmptyFrameForDomainOperation()) {
                if (error) *error = QStringLiteral("Tween snapshot cannot remove non-empty trailing frame");
                return false;
            }
        }
    }

    return true;
}

QString TupTweenService::currentMotionTweenSnapshot(
    TupScene *scene, const QString &referenceSnapshot, QString *error)
{
    if (!scene) {
        if (error)
            *error = QStringLiteral("Tween snapshot capture requires a scene");
        return QString();
    }

    QString tweenId;
    int layerIndex = -1;
    int frameCount = -1;
    QList<MemberState> members;
    if (!parseSnapshot(referenceSnapshot, &tweenId, &layerIndex,
                       &frameCount, &members, error)) {
        return QString();
    }

    QList<QString> objectIds;
    for (const MemberState &member : members)
        objectIds.append(member.objectId);

    return createSnapshot(scene, tweenId, objectIds, error, true,
        referenceSnapshot.contains(QStringLiteral("layer_counts=")));
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
