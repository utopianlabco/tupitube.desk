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

#include "opacity_tweener.h"
#include "tosd.h"
#include "tupinputdeviceinformation.h"
#include "tupbrushmanager.h"
#include "tupgraphicsscene.h"
#include "tupgraphicobject.h"
#include "tupsvgitem.h"
#include "tupitemtweener.h"
#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tuplibraryobject.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "talgorithm.h"

#include <QMessageBox>
#include <QPointF>
#include <QKeySequence>
#include <QGraphicsView>
#include <QDomDocument>
#include <QDir>

OpacityTweener::OpacityTweener() : TupToolPlugin()
{
    setupActions();

    configPanel = nullptr;
    initFrame = 0;
}

OpacityTweener::~OpacityTweener()
{
}

/* This method initializes the plugin */
void OpacityTweener::init(TupGraphicsScene *gScene)
{
    scene = gScene;
    objects.clear();

    mode = TupToolPlugin::View;
    editMode = TupToolPlugin::None;
    initFrame = scene->currentFrameIndex();
    initLayer = scene->currentLayerIndex();
    initScene = scene->currentSceneIndex();

    configPanel->resetUI();

    QList<QPair<QString, QString>> tweenList = scene->currentScene()->getTweenEntries(TupItemTweener::Opacity);
    if (tweenList.size() > 0) {
        configPanel->loadTweenList(tweenList);
        setCurrentTween(tweenList.at(0).second);
    } else {
        configPanel->activeButtonsPanel(false);
    }

    int total = framesCount();
    configPanel->initStartCombo(total, initFrame);
}

void OpacityTweener::updateStartPoint(int index)
{
     if (initFrame != index && index >= 0)
         initFrame = index;
}

/* This method returns the plugin name */
QList<TAction::ActionId> OpacityTweener::keys() const
{
    return QList<TAction::ActionId>() << TAction::Opacity;
}

/* This method makes an action when the mouse is pressed on the workspace 
 * depending on the active mode: Selecting an object or Creating a path  
 */
void OpacityTweener::press(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *scene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[Opacity Tweener::press()]";
    #endif

    Q_UNUSED(input)
    Q_UNUSED(brushManager)
    Q_UNUSED(scene)
}

/* This method is executed while the mouse is pressed and on movement */

void OpacityTweener::move(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    Q_UNUSED(input)
    Q_UNUSED(brushManager)
    Q_UNUSED(gScene)
}

/* This method finishes the action started on the press method depending
 * on the active mode: Selecting an object or Creating a path
*/

void OpacityTweener::release(const TupInputDeviceInformation *input, TupBrushManager *brushManager, TupGraphicsScene *gScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "[Opacity Tweener::release()]";
    #endif

    Q_UNUSED(input)
    Q_UNUSED(brushManager)

    if (gScene->currentFrameIndex() == initFrame) {
        if (editMode == TupToolPlugin::Selection) {
            #ifdef TUP_DEBUG
                qDebug() << "[Opacity Tweener::release()] - Tracing selection mode";
            #endif

            if (gScene->selectedItems().size() > 0) {
                #ifdef TUP_DEBUG
                    qDebug() << "[Opacity Tweener::release()] - selection size -> " << gScene->selectedItems().size();
                #endif
                objects = gScene->selectedItems();
                foreach (QGraphicsItem *item, objects) {
                    QString tip = item->toolTip();
                    if (tip.contains(tr("Opacity"))) {
                        QPair<int, int> dimension = TAlgorithm::screenDimension();
                        int screenWidth = dimension.first;
                        int screenHeight = dimension.second;

                        QMessageBox msgBox;
                        msgBox.setWindowTitle(tr("Warning"));
                        msgBox.setIcon(QMessageBox::Warning);
                        msgBox.setText(tr("The selected items already have this kind of tween assigned."));
                        msgBox.setInformativeText(tr("Please, edit the previous tween of these objects."));
                        msgBox.addButton(QString(tr("Accept")), QMessageBox::AcceptRole);
                        msgBox.show();
                        msgBox.move(static_cast<int>((screenWidth - msgBox.width())/2),
                                    static_cast<int>((screenHeight - msgBox.height())/2));
                        msgBox.exec();

                        objects.clear();
                        gScene->clearSelection();
                        return;
                    }
                }
                #ifdef TUP_DEBUG
                    qDebug() << "[Opacity Tweener::release()] - Notifying selection...";
                #endif
                configPanel->notifySelection(true);
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[Opacity Tweener::release()] - Selection mode: no items selected";
                #endif
            }
        }
    }
}

/* This method returns the list of actions defined in this plugin */
QMap<TAction::ActionId, TAction *> OpacityTweener::actions() const
{
    return opacityActions;
}

TAction * OpacityTweener::getAction(TAction::ActionId toolId)
{
    return opacityActions[toolId];
}

/* This method returns the list of actions defined in this plugin */
int OpacityTweener::toolType() const
{
    return TupToolInterface::Tweener;
}

/* This method returns the tool panel associated to this plugin */
QWidget *OpacityTweener::configurator()
{
    if (!configPanel) {
        mode = TupToolPlugin::View;

        configPanel = new OpacityConfigurator;
        connect(configPanel, SIGNAL(startingPointChanged(int)), this, SLOT(updateStartPoint(int)));
        connect(configPanel, SIGNAL(clickedApplyTween()), this, SLOT(applyTween()));
        connect(configPanel, SIGNAL(clickedSelect()), this, SLOT(setSelection()));
        connect(configPanel, SIGNAL(clickedDefineProperties()), this, SLOT(setPropertiesMode()));
        connect(configPanel, SIGNAL(clickedResetInterface()), this, SLOT(applyReset()));
        connect(configPanel, SIGNAL(setMode(TupToolPlugin::Mode)), this, SLOT(updateMode(TupToolPlugin::Mode)));
        connect(configPanel, SIGNAL(getTweenData(const QString &)), this, SLOT(setCurrentTween(const QString &)));
        connect(configPanel, SIGNAL(clickedRemoveTween(const QString &)), this, SLOT(removeTween(const QString &)));
    } 

    return configPanel;
}

/* This method is called when there's a change on/of scene */
void OpacityTweener::aboutToChangeScene(TupGraphicsScene *)
{
}

/* This method is called when this plugin is off */
void OpacityTweener::aboutToChangeTool()
{
}

/* This method defines the actions contained in this plugin */
void OpacityTweener::setupActions()
{
    QString name = tr("Opacity Tween");
    QString shortcut = tr("Shift+O");

    TAction *action = new TAction(QPixmap(ICONS_DIR + "opacity_tween.png"), name, this);
    action->setCursor(QCursor(kAppProp->themeDir() + "cursors/tweener.png",0 ,0));
    action->setShortcut(QKeySequence(shortcut));
    action->setToolTip(name + " - " + shortcut);
    action->setActionId(TAction::Opacity);

    opacityActions.insert(TAction::Opacity, action);
}

/* This method saves the settings of this plugin */
void OpacityTweener::saveConfig()
{
}

/* This method updates the workspace when the plugin changes the scene */
void OpacityTweener::updateScene(TupGraphicsScene *gScene)
{
    mode = configPanel->mode();

    if (mode == TupToolPlugin::Add) {
        int total = framesCount();

        if (editMode == TupToolPlugin::Properties) {
            if (total > configPanel->startComboSize()) {
                configPanel->activateMode(TupToolPlugin::Selection);
                clearSelection();
                setSelection();
            }
        } else if (editMode == TupToolPlugin::Selection) {
                   if (gScene->currentFrameIndex() != initFrame)
                       clearSelection();
                   initFrame = gScene->currentFrameIndex();
                   setSelection();
        }

        if (configPanel->startComboSize() < total) {
            configPanel->initStartCombo(total, initFrame);
        } else {
            if (gScene->currentFrameIndex() != initFrame)
                configPanel->setStartFrame(gScene->currentFrameIndex());
        }
    } else {
        if (gScene->currentFrameIndex() != initFrame)
            configPanel->setStartFrame(gScene->currentFrameIndex());
    }
}

void OpacityTweener::setCurrentTween(const QString &tweenId)
{
    TupScene *sceneData = scene->currentScene();
    currentTween = sceneData->tweenById(tweenId);
    if (currentTween && currentTween->getType() == TupItemTweener::Opacity)
        configPanel->setCurrentTween(currentTween);
    else
        currentTween = nullptr;
}

int OpacityTweener::framesCount()
{
    int total = 1;
    TupLayer *layer = scene->currentScene()->layerAt(scene->currentLayerIndex());
    if (layer)
        total = layer->framesCount();

    return total;
}

/* This method clears selection */
void OpacityTweener::clearSelection()
{
    if (objects.size() > 0) {
        foreach (QGraphicsItem *item, objects) {
            if (item->isSelected())
                item->setSelected(false);
        }
        objects.clear();
        configPanel->notifySelection(false);
    }
}

/* This method disables object selection */
void OpacityTweener::disableSelection()
{
    foreach (QGraphicsView *view, scene->views()) {
         view->setDragMode (QGraphicsView::NoDrag);
         foreach (QGraphicsItem *item, view->scene()->items()) {
             item->setFlag(QGraphicsItem::ItemIsSelectable, false);
             item->setFlag(QGraphicsItem::ItemIsMovable, false);
         }
    }
}

void OpacityTweener::setSelection()
{
    editMode = TupToolPlugin::Selection;

    scene->enableItemsForSelection();
    foreach (QGraphicsView *view, scene->views())
        view->setDragMode(QGraphicsView::RubberBandDrag);
    // When Object selection is enabled, previous selection is set
    if (objects.size() > 0) {
        foreach (QGraphicsItem *item, objects) {
            item->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
            item->setSelected(true);
        }
        configPanel->notifySelection(true);
    }
}

void OpacityTweener::setPropertiesMode()
{
    editMode = TupToolPlugin::Properties;
    disableSelection();
}

/* This method resets this plugin */
void OpacityTweener::applyReset()
{
    disableSelection();
    clearSelection();

    mode = TupToolPlugin::View;
    editMode = TupToolPlugin::None;

    initFrame = scene->currentFrameIndex();
    initLayer = scene->currentLayerIndex();
    initScene = scene->currentSceneIndex();
}

/* This method applies to the project, the Tween created from this plugin */
void OpacityTweener::applyTween()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QString name = configPanel->currentTweenName();
    if (name.length() == 0) {
        TOsd::self()->display(TOsd::Warning, tr("Tween name is missing!"));
        QApplication::restoreOverrideCursor();
        return;
    }

    TupItemTweener *identityTween = nullptr;
    if (mode == TupToolPlugin::Edit && currentTween)
        identityTween = currentTween;

    const bool tweenAlreadyExists = identityTween != nullptr;
    const QString sourceTweenName = tweenAlreadyExists ? identityTween->getTweenName() : name;
    QString tweenId;
    if (tweenAlreadyExists) {
        currentTween = identityTween;
        tweenId = currentTween->tweenId().trimmed();
        if (tweenId.isEmpty()) {
            #ifdef TUP_DEBUG
                qWarning() << "[Opacity Tweener::applyTween()] - Existing tween is missing tween_id ->" << sourceTweenName;
            #endif
            QApplication::restoreOverrideCursor();
            return;
        }
    } else {
        tweenId = TupItemTweener::createTweenId();
    }

    bool semanticRebase = false;

    // Tween is new
    if (!tweenAlreadyExists) {
        initFrame = scene->currentFrameIndex();
        initLayer = scene->currentLayerIndex();
        initScene = scene->currentSceneIndex();

        foreach (QGraphicsItem *item, objects) {
            TupLibraryObject::ObjectType type = TupLibraryObject::Item;
            int objectIndex = -1;

            if (TupSvgItem *svg = qgraphicsitem_cast<TupSvgItem *>(item)) {
                type = TupLibraryObject::Svg;
                objectIndex = scene->currentFrame()->indexOf(svg);
            } else {
                objectIndex = scene->currentFrame()->indexOf(item);
            }

            QString objectId;
            if (type == TupLibraryObject::Item) {
                TupGraphicObject *graphicObject = scene->currentFrame()->graphicAt(objectIndex);
                if (graphicObject)
                    objectId = graphicObject->objectId();
            }

            TupProjectRequest request = TupRequestBuilder::createItemRequest(
                        initScene, initLayer, initFrame,
                        objectIndex, QPointF(), scene->getSpaceContext(),
                        type, TupProjectRequest::SetTween,
                        configPanel->tweenToXml(initScene, initLayer, initFrame, tweenId),
                        QByteArray(), QString(), QString(), objectId);
            emit requested(&request);
        }
    } else { // Tween already exists
        const int previousInitFrame = currentTween->getInitFrame();
        const int previousInitLayer = currentTween->getInitLayer();

        initFrame = configPanel->startFrame();
        initLayer = previousInitLayer;
        initScene = scene->currentSceneIndex();

        if (initFrame != previousInitFrame) {
            semanticRebase = true;

            TupScene *sceneData = scene->currentScene();
            TupLayer *layer = sceneData->layerAt(previousInitLayer);
            TupFrame *sourceFrame = layer ? layer->frameAt(previousInitFrame) : nullptr;
            if (!sourceFrame) {
                TOsd::self()->display(TOsd::Error, tr("Opacity tween source frame is unavailable!"));
                QApplication::restoreOverrideCursor();
                return;
            }

            QDomDocument rebaseDocument;
            QDomElement root = rebaseDocument.createElement(QStringLiteral("tween_rebase"));
            root.setAttribute(QStringLiteral("tween_id"), tweenId);
            root.setAttribute(QStringLiteral("target_layer"), initLayer);
            root.setAttribute(QStringLiteral("target_frame"), initFrame);

            QString representativeObjectId;
            int representativeObjectIndex = -1;

            foreach (QGraphicsItem *item, objects) {
                if (qgraphicsitem_cast<TupSvgItem *>(item)) {
                    TOsd::self()->display(TOsd::Error, tr("Opacity tween rebase currently requires native objects."));
                    QApplication::restoreOverrideCursor();
                    return;
                }

                const int sourceIndex = sourceFrame->indexOf(item);
                TupGraphicObject *graphicObject = sourceIndex >= 0 ? sourceFrame->graphicAt(sourceIndex) : nullptr;
                if (!graphicObject) {
                    TOsd::self()->display(TOsd::Error, tr("Opacity tween member could not be resolved."));
                    QApplication::restoreOverrideCursor();
                    return;
                }

                const QString objectId = graphicObject->objectId().trimmed();
                if (objectId.isEmpty()) {
                    TOsd::self()->display(TOsd::Error, tr("Opacity tween member is missing object identity."));
                    QApplication::restoreOverrideCursor();
                    return;
                }

                const QString targetTweenXml = configPanel->tweenToXml(
                            initScene, initLayer, initFrame, tweenId);

                QDomElement member = rebaseDocument.createElement(QStringLiteral("member"));
                member.setAttribute(QStringLiteral("object_id"), objectId);
                member.setAttribute(QStringLiteral("tween"),
                                    QString::fromLatin1(targetTweenXml.toUtf8().toBase64()));
                root.appendChild(member);

                if (representativeObjectId.isEmpty()) {
                    representativeObjectId = objectId;
                    representativeObjectIndex = sourceIndex;
                }
            }

            if (root.firstChildElement(QStringLiteral("member")).isNull()) {
                TOsd::self()->display(TOsd::Error, tr("Opacity tween has no native members to rebase."));
                QApplication::restoreOverrideCursor();
                return;
            }

            rebaseDocument.appendChild(root);
            TupProjectRequest request = TupRequestBuilder::createItemRequest(
                        initScene, previousInitLayer, previousInitFrame,
                        representativeObjectIndex, QPointF(), scene->getSpaceContext(),
                        TupLibraryObject::Item, TupProjectRequest::RebaseTween,
                        rebaseDocument.toString(), QByteArray(), QString(), QString(),
                        representativeObjectId);
            emit requested(&request);
        } else {
            TupScene *sceneData = scene->currentScene();
            TupLayer *layer = sceneData->layerAt(initLayer);
            TupFrame *frame = layer ? layer->frameAt(previousInitFrame) : nullptr;
            if (!frame) {
                QApplication::restoreOverrideCursor();
                return;
            }

            foreach (QGraphicsItem *item, objects) {
                TupLibraryObject::ObjectType type = TupLibraryObject::Item;
                int objectIndex = frame->indexOf(item);

                if (TupSvgItem *svg = qgraphicsitem_cast<TupSvgItem *>(item)) {
                    type = TupLibraryObject::Svg;
                    objectIndex = frame->indexOf(svg);
                }

                QString objectId;
                if (type == TupLibraryObject::Item) {
                    TupGraphicObject *graphicObject = frame->graphicAt(objectIndex);
                    if (graphicObject)
                        objectId = graphicObject->objectId();
                }

                TupProjectRequest request = TupRequestBuilder::createItemRequest(
                            initScene, initLayer, initFrame,
                            objectIndex, QPointF(), scene->getSpaceContext(),
                            type, TupProjectRequest::SetTween,
                            configPanel->tweenToXml(initScene, initLayer, initFrame, tweenId),
                            QByteArray(), QString(), QString(), objectId);
                emit requested(&request);
            }
        }
    }

    if (!semanticRebase) {
        int total = initFrame + configPanel->totalSteps();
        int framesNumber = framesCount();
        int layersCount = scene->currentScene()->layersCount();
        TupProjectRequest request;

        if (total >= framesNumber) {
            for (int i = framesNumber; i < total; i++) {
                for (int j = 0; j < layersCount; j++) {
                    request = TupRequestBuilder::createFrameRequest(initScene, j, i,
                                                                    TupProjectRequest::Add,
                                                                    tr("Frame"));
                    emit requested(&request);
                }
            }
        }

        QString selection = QString::number(initLayer) + "," + QString::number(initLayer) + ","
                            + QString::number(initFrame) + "," + QString::number(initFrame);

        request = TupRequestBuilder::createFrameRequest(initScene, initLayer, initFrame,
                                                        TupProjectRequest::Select, selection);
        emit requested(&request);
    }

    refreshTweenList();
    setCurrentTween(tweenId);
    TOsd::self()->display(TOsd::Info, tr("Tween %1 applied!").arg(name));

    QApplication::restoreOverrideCursor();
}

void OpacityTweener::removeTweenFromProject(const QString &tweenId)
{
    TupScene *sceneData = scene->currentScene();
    TupItemTweener *tween = sceneData->tweenById(tweenId);
    if (!tween || tween->getType() != TupItemTweener::Opacity) {
        #ifdef TUP_DEBUG
            qDebug() << "[Opacity Tweener::removeTweenFromProject()] - Opacity tween couldn't be found -> " << tweenId;
        #endif
        return;
    }

    const int tweenScene = tween->getInitScene();
    const int tweenLayer = tween->getInitLayer();
    const int tweenFrame = tween->getInitFrame();
    QList<QGraphicsItem *> tweenItems = sceneData->getItemsFromTweenId(tweenId);

    TupLayer *layer = sceneData->layerAt(tweenLayer);
    TupFrame *frame = layer ? layer->frameAt(tweenFrame) : nullptr;
    if (!frame) {
        #ifdef TUP_DEBUG
            qDebug() << "[Opacity Tweener::removeTweenFromProject()] - Invalid tween frame -> "
                     << tweenScene << tweenLayer << tweenFrame;
        #endif
        return;
    }

    bool requestSent = false;
    foreach (QGraphicsItem *item, tweenItems) {
        TupLibraryObject::ObjectType type = TupLibraryObject::Item;
        int objectIndex = -1;
        QString objectId;

        if (TupSvgItem *svg = qgraphicsitem_cast<TupSvgItem *>(item)) {
            type = TupLibraryObject::Svg;
            objectIndex = frame->indexOf(svg);
        } else {
            objectIndex = frame->indexOf(item);
            TupGraphicObject *graphicObject = frame->graphicAt(objectIndex);
            if (graphicObject)
                objectId = graphicObject->objectId().trimmed();

            if (objectId.isEmpty()) {
                #ifdef TUP_DEBUG
                    qWarning() << "[Opacity Tweener::removeTweenFromProject()] - "
                                  "Native tween removal requires object_id at index ->"
                               << objectIndex;
                #endif
                continue;
            }
        }

        if (objectIndex < 0)
            continue;

        TupProjectRequest request = TupRequestBuilder::createItemRequest(
                                    tweenScene, tweenLayer, tweenFrame,
                                    objectIndex, QPointF(), scene->getSpaceContext(),
                                    type, TupProjectRequest::RemoveTween, tweenId,
                                    QByteArray::number(static_cast<int>(TupItemTweener::Opacity)),
                                    QString(), QString(), objectId);
        emit requested(&request);
        requestSent = true;
    }

    if (requestSent) {
        foreach (QGraphicsView * view, scene->views()) {
            foreach (QGraphicsItem *item, view->scene()->items()) {
                QString tip = item->toolTip();
                if (tip.compare("Tweens: " + tr("Opacity")) == 0) {
                    item->setToolTip("");
                    item->setRotation(0);
                } else {
                    if (tip.contains(tr("Opacity"))) {
                        tip = tip.replace(tr("Opacity") + ",", "");
                        tip = tip.replace(tr("Opacity"), "");
                        if (tip.endsWith(","))
                            tip.chop(1);
                        item->setToolTip(tip);
                        item->setRotation(0);
                    }
                }
            }
        }
        emit tweenRemoved();
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[Opacity Tweener::removeTweenFromProject()] - No tween removal request was sent -> " << tweenId;
        #endif
    }
}

void OpacityTweener::removeTween(const QString &tweenId)
{
    removeTweenFromProject(tweenId);
    applyReset();

    const QString nextTweenId = configPanel->getTweenIdFromList();
    if (!nextTweenId.isEmpty())
        setCurrentTween(nextTweenId);
}

void OpacityTweener::updateMode(TupToolPlugin::Mode currentMode)
{
    mode = currentMode;

    if (mode == TupToolPlugin::Edit) {
        if (currentTween) {
            initScene = currentTween->getInitScene();
            initLayer = currentTween->getInitLayer();
            initFrame = currentTween->getInitFrame();

            if (initFrame != scene->currentFrameIndex() || initLayer != scene->currentLayerIndex()) {
                QString selection = QString::number(initLayer) + "," + QString::number(initLayer) + ","
                                    + QString::number(initFrame) + "," + QString::number(initFrame);

                TupProjectRequest request = TupRequestBuilder::createFrameRequest(initScene, initLayer, initFrame,
                                                                                  TupProjectRequest::Select, selection);
                emit requested(&request);
            }

            if (objects.isEmpty())
                objects = scene->currentScene()->getItemsFromTweenId(currentTween->tweenId());
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[Opacity Tweener::updateMode()] - Current tween pointer is NULL!";
            #endif
        }
    }
}

void OpacityTweener::refreshTweenList()
{
    QList<QPair<QString, QString>> tweenList = scene->currentScene()->getTweenEntries(TupItemTweener::Opacity);
    QString tweenId = configPanel->getTweenIdFromList();

    configPanel->loadTweenList(tweenList);

    if (tweenList.isEmpty()) {
        currentTween = nullptr;
        return;
    }

    TupItemTweener *selectedTween = scene->currentScene()->tweenById(tweenId);
    if (!selectedTween || selectedTween->getType() != TupItemTweener::Opacity)
        tweenId = tweenList.at(0).second;

    setCurrentTween(tweenId);
}

void OpacityTweener::refreshAuthoritativeTween(const QString &tweenId)
{
    TupScene *sceneData = scene ? scene->currentScene() : nullptr;
    if (!sceneData || tweenId.trimmed().isEmpty())
        return;

    TupItemTweener *tween = sceneData->tweenById(tweenId);
    if (!tween || tween->getType() != TupItemTweener::Opacity)
        return;

    currentTween = tween;
    initScene = tween->getInitScene();
    initLayer = tween->getInitLayer();
    initFrame = tween->getInitFrame();
    objects = sceneData->getItemsFromTweenId(tweenId);

    // Keep authoritative zero-based indexes from being changed by
    // programmatic one-based Properties values during refresh.
    const bool signalsBlocked = configPanel->blockSignals(true);
    configPanel->setCurrentTween(currentTween);
    configPanel->notifySelection(!objects.isEmpty());
    configPanel->refreshCurrentTweenProperties(framesCount());
    configPanel->blockSignals(signalsBlocked);

    mode = TupToolPlugin::Edit;
    editMode = TupToolPlugin::Properties;
}

void OpacityTweener::sceneResponse(const TupSceneResponse *event)
{
    if ((event->getAction() == TupProjectRequest::Remove || event->getAction() == TupProjectRequest::Reset)
        && (scene->currentSceneIndex() == event->getSceneIndex())) {
        init(scene);
    }

    if (event->getAction() == TupProjectRequest::Select)
        init(scene);
}

void OpacityTweener::layerResponse(const TupLayerResponse *event)
{
    if (event->getAction() == TupProjectRequest::Remove)
        init(scene);
}

void OpacityTweener::frameResponse(const TupFrameResponse *event)
{
    if (event->getAction() == TupProjectRequest::Remove && scene->currentLayerIndex() == event->getLayerIndex())
        init(scene);

    if (event->getAction() == TupProjectRequest::Select) {
        if (initLayer != event->getLayerIndex() || initScene != event->getSceneIndex())
            init(scene);
    }
}

void OpacityTweener::itemResponse(const TupItemResponse *event)
{
    if (event->getAction() == TupProjectRequest::RemoveTween
            && event->getMode() != TupProjectResponse::Do) {
        init(scene);
        return;
    }

    if (event->getAction() == TupProjectRequest::SetTween
            || event->getAction() == TupProjectRequest::RemoveTween) {
        // Local Set/Remove UI state is already maintained by the initiating
        // configurator path. Remote peers reconcile their visible manager or
        // the Properties panel if it is currently showing the affected tween.
        if (!event->external())
            return;

        QString affectedTweenId;
        if (event->getAction() == TupProjectRequest::RemoveTween) {
            affectedTweenId = event->getArg().toString().trimmed();
        } else {
            QDomDocument document;
            if (document.setContent(event->getArg().toString()))
                affectedTweenId = document.documentElement()
                        .attribute(QStringLiteral("tween_id")).trimmed();
        }

        const QString selectedTweenId = configPanel->getTweenIdFromList().trimmed();
        const bool editingAffectedTween = configPanel->mode() == TupToolPlugin::Edit
                && !affectedTweenId.isEmpty()
                && selectedTweenId == affectedTweenId;

        if (editingAffectedTween) {
            if (event->getAction() == TupProjectRequest::SetTween) {
                refreshAuthoritativeTween(affectedTweenId);
            } else {
                currentTween = nullptr;
                init(scene);
            }
        } else {
            refreshTweenList();
        }
        return;
    }

    if (event->getAction() == TupProjectRequest::RebaseTween) {
        QDomDocument document;
        if (!document.setContent(event->getArg().toString()))
            return;

        const QString tweenId = document.documentElement()
                .attribute(QStringLiteral("tween_id")).trimmed();
        const QString selectedTweenId = configPanel->getTweenIdFromList().trimmed();
        const bool editingAffectedTween = configPanel->mode() == TupToolPlugin::Edit
                && !tweenId.isEmpty()
                && selectedTweenId == tweenId;

        if (editingAffectedTween) {
            refreshAuthoritativeTween(tweenId);
            if (event->external())
                emit rebasedFrameFollowRequested(initFrame, initLayer, initScene);
        } else if (event->external()) {
            refreshTweenList();
        }
        return;
    }
}
