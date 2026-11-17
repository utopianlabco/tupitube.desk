/*
 * Shared post-RemoveTween presentation reconciliation.
 * The project executor has already applied the model mutation when an
 * itemResponse reaches a tweener. No domain command is emitted here.
 */
#ifndef TWEENITEMRELEASE_H
#define TWEENITEMRELEASE_H

#include "tupgraphicsscene.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tupframe.h"
#include "tupgraphicobject.h"
#include "tupsvgitem.h"
#include "tupitemtweener.h"
#include "tupprojectresponse.h"
#include "tuplibraryobject.h"

#include <QGraphicsItem>
#include <QStringList>

namespace TweenItemRelease {

inline void reconcile(TupGraphicsScene *graphicsScene,
                      const TupItemResponse *response,
                      TupItemTweener::Type expectedType,
                      const QString &typeLabel)
{
    if (!graphicsScene || !response)
        return;

    // Undo restores the tween; only Do and Redo release a removed member.
    if (response->getAction() != TupProjectRequest::RemoveTween
            || response->getMode() == TupProjectResponse::Undo)
        return;

    bool validType = false;
    const int responseType = QString::fromUtf8(response->getData()).toInt(&validType);
    if (!validType || responseType != static_cast<int>(expectedType))
        return;

    // The graphics scene represents the current project scene. Never use a
    // frame index from a different scene to modify an unrelated graphic item.
    if (graphicsScene->currentSceneIndex() != response->getSceneIndex())
        return;

    TupScene *scene = graphicsScene->currentScene();
    TupLayer *layer = scene ? scene->layerAt(response->getLayerIndex()) : nullptr;
    TupFrame *frame = layer ? layer->frameAt(response->getFrameIndex()) : nullptr;
    if (!frame)
        return;

    QGraphicsItem *item = nullptr;
    bool hasRemainingTweens = false;
    if (response->getItemType() == TupLibraryObject::Svg) {
        TupSvgItem *svg = frame->svgAt(response->getItemIndex());
        if (!svg)
            return;
        item = svg;
        hasRemainingTweens = !svg->tweensList().isEmpty();
    } else if (response->getItemType() == TupLibraryObject::Item) {
        // Native identity is never inferred from a positional itemIndex.
        const QString objectId = response->getObjectId().trimmed();
        TupGraphicObject *object = objectId.isEmpty() ? nullptr : frame->graphicById(objectId);
        if (!object)
            return;
        item = object->item();
        hasRemainingTweens = !object->tweensList().isEmpty();
    }

    if (!item)
        return;

    const QString prefix = QStringLiteral("Tweens: ");
    const QString tooltip = item->toolTip();
    if (tooltip.startsWith(prefix)) {
        QStringList labels;
        const QStringList existing = tooltip.mid(prefix.size()).split(QLatin1Char(','));
        for (const QString &entry : existing) {
            const QString label = entry.trimmed();
            if (!label.isEmpty() && label != typeLabel)
                labels.append(label);
        }
        item->setToolTip(labels.isEmpty() ? QString() : prefix + labels.join(QStringLiteral(", ")));
    }

    // Do not release an object that still belongs to another tween.
    if (!hasRemainingTweens && item->toolTip().isEmpty()) {
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
    }
}

} // namespace TweenItemRelease

#endif // TWEENITEMRELEASE_H
