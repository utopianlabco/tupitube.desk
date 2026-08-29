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

#include "motion_configurator.h"
#include "tapplicationproperties.h"
#include "tseparator.h"
#include "stepsviewer.h"
#include "tuptweenerstep.h"
#include "tosd.h"
#include "tradiobuttongroup.h"
#include "tresponsiveui.h"

MotionConfigurator::MotionConfigurator(QWidget *parent) : QFrame(parent)
{
    framesCount = 1;
    currentFrame = 0;

    currentMode = TupToolPlugin::View;
    selectionDone = false;
    currentTween = nullptr;
    state = Manager;

    layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel *toolTitle = new QLabel;
    toolTitle->setAlignment(Qt::AlignHCenter);
    QPixmap pic(THEME_DIR + "icons/motion_tween.png");
    toolTitle->setPixmap(pic.scaledToWidth(TResponsiveUI::fitTitleIconSize(), Qt::SmoothTransformation));
    toolTitle->setToolTip(tr("Motion Tween Properties"));
    layout->addWidget(toolTitle);
    layout->addWidget(new TSeparator(Qt::Horizontal));

    settingsLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    settingsLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    settingsLayout->setMargin(0);
    settingsLayout->setSpacing(0);

    setTweenManagerPanel();
    setButtonsPanel();
    setPropertiesPanel();

    layout->addLayout(settingsLayout);
    layout->addStretch(2);
}

MotionConfigurator::~MotionConfigurator()
{
}

void MotionConfigurator::loadTweenList(QList<QString> tweenList)
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::loadTweenList()]";
    #endif

    // The tween manager is a view of the project model, not a second source
    // of truth. Always replace its contents when the model changes.
    tweenManager->resetUI();
    tweenManager->loadTweenList(tweenList);
    currentTween = nullptr;

    activeButtonsPanel(state == Manager && !tweenList.isEmpty());
}

void MotionConfigurator::setPropertiesPanel()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::setPropertiesPanel()]";
    #endif

    settingsPanel = new MotionSettings(this);

    connect(settingsPanel, SIGNAL(startingFrameChanged(int)), this, SIGNAL(startingFrameChanged(int)));

    connect(settingsPanel, SIGNAL(clickedSelect()), this, SIGNAL(clickedSelect()));
    connect(settingsPanel, SIGNAL(clickedCreatePath()), this, SIGNAL(clickedCreatePath()));

    connect(settingsPanel, SIGNAL(clickedApplyTween()), this, SLOT(applyItem()));
    connect(settingsPanel, SIGNAL(clickedResetTween()), this, SLOT(closeTweenProperties()));

    connect(settingsPanel, SIGNAL(framesTotalChanged()), this, SIGNAL(framesTotalChanged()));
    connect(settingsPanel, SIGNAL(pathThicknessChanged(int)), this, SIGNAL(pathThicknessChanged(int)));
    connect(settingsPanel, SIGNAL(pathColorUpdated(QColor)), this, SIGNAL(pathColorUpdated(QColor)));

    settingsLayout->addWidget(settingsPanel);

    activePropertiesPanel(false);
}

void MotionConfigurator::activePropertiesPanel(bool enable)
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::activePropertiesPanel()] - enable flag ->" << enable;
    #endif

    settingsPanel->enableInitCombo(enable);

    if (enable) {
        settingsPanel->show();
    } else {
        settingsPanel->clearData();
        settingsPanel->hide();
    }
}

void MotionConfigurator::setTweenManagerPanel()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::setTweenManagerPanel()]";
    #endif

    tweenManager = new TweenManager(this);

    connect(tweenManager, SIGNAL(addNewTween(const QString &)), this, SLOT(addTween(const QString &)));
    connect(tweenManager, SIGNAL(editCurrentTween(const QString &)), this, SLOT(editTween()));
    connect(tweenManager, SIGNAL(removeCurrentTween(const QString &)), this, SLOT(removeTween(const QString &)));
    connect(tweenManager, SIGNAL(getTweenData(const QString &)), this, SLOT(updateTweenData(const QString &)));

    settingsLayout->addWidget(tweenManager);

    state = Manager;
}

void MotionConfigurator::activeTweenManagerPanel(bool enable)
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::activeTweenManagerPanel()] - enable flag ->" << enable;
    #endif

    if (enable)
        tweenManager->show();
    else 
        tweenManager->hide();

    if (tweenManager->listSize() > 0)
        activeButtonsPanel(enable);
}

void MotionConfigurator::setButtonsPanel()
{
    controlPanel = new ButtonsPanel(this);
    connect(controlPanel, SIGNAL(clickedEditTween()), this, SLOT(editTween()));
    connect(controlPanel, SIGNAL(clickedRemoveTween()), this, SLOT(removeTween()));

    settingsLayout->addWidget(controlPanel);

    activeButtonsPanel(false);
}

void MotionConfigurator::activeButtonsPanel(bool enable)
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::activeButtonsPanel()] - enable flag ->" << enable;
    #endif

    if (enable)
        controlPanel->show();
    else
        controlPanel->hide();
}

void MotionConfigurator::initStartCombo(int frames, int frameIndex)
{
    framesCount = frames;
    currentFrame = frameIndex;
    settingsPanel->initStartCombo(framesCount, currentFrame);
}

void MotionConfigurator::setStartFrame(int currentIndex)
{
    currentFrame = currentIndex;
    settingsPanel->setStartFrame(currentFrame);
}

int MotionConfigurator::startFrame()
{
    return settingsPanel->startFrame();
}

int MotionConfigurator::startComboSize()
{
    return settingsPanel->startComboSize();
}

void MotionConfigurator::updateSteps(const QGraphicsPathItem *path)
{
    settingsPanel->updateSteps(path);
}

QString MotionConfigurator::tweenToXml(int currentScene, int currentLayer, int currentFrame, QPointF point, QString &path)
{
    return settingsPanel->tweenToXml(currentScene, currentLayer, currentFrame, point, path);
}

int MotionConfigurator::totalSteps()
{
    return settingsPanel->totalSteps();
}

QList<QPointF> MotionConfigurator::tweenPoints()
{
    return settingsPanel->tweenPoints();
}

void MotionConfigurator::activateMode(TupToolPlugin::EditMode mode)
{
    settingsPanel->activateMode(mode);
}

void MotionConfigurator::clearData()
{
    settingsPanel->clearData();
}

void MotionConfigurator::addTween(const QString &name)
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::addTween()] - Adding tween ->" << name;
    #endif

    emit clickedResetInterface();

    currentMode = TupToolPlugin::Add;
    settingsPanel->setParameters(name, framesCount, currentFrame);

    activeTweenManagerPanel(false);
    activePropertiesPanel(true);

    state = Properties;

    emit setMode(currentMode);
}

void MotionConfigurator::editTween()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::editTween()]";
    #endif

    activeTweenManagerPanel(false);

    currentMode = TupToolPlugin::Edit;
    state = Properties;

    settingsPanel->notifySelection(true);
    settingsPanel->setParameters(currentTween);
    activePropertiesPanel(true);

    emit setMode(currentMode);
}

void MotionConfigurator::closeTweenProperties()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::closeTweenProperties()]";
    #endif

    if (currentMode == TupToolPlugin::Add)
        tweenManager->removeItemFromList();

    emit clickedResetInterface();

    closeSettingsPanel();
}

void MotionConfigurator::removeTween()
{
    QString name = tweenManager->currentTweenName();
    tweenManager->removeItemFromList();

    currentTween = nullptr;

    removeTween(name);
}

void MotionConfigurator::removeTween(const QString &name)
{
    if (tweenManager->listSize() == 0)
        activeButtonsPanel(false);

    emit clickedRemoveTween(name);
}

QString MotionConfigurator::currentTweenName() const
{
    return settingsPanel->currentTweenName();
}

QString MotionConfigurator::getTweenNameFromList() const
{
    return tweenManager->currentTweenName();
}

void MotionConfigurator::notifySelection(bool flag)
{
    settingsPanel->notifySelection(flag);
}

void MotionConfigurator::closeSettingsPanel()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::closeSettingsPanel()]";
    #endif

    // settingsPanel->enableInitCombo(false);

    if (state == Properties) {
        activeTweenManagerPanel(true);
        activePropertiesPanel(false);
        currentMode = TupToolPlugin::View;
        state = Manager;
    }
}

TupToolPlugin::Mode MotionConfigurator::mode()
{
    return currentMode;
}

void MotionConfigurator::applyItem()
{
     currentMode = TupToolPlugin::Edit;
     emit clickedApplyTween();
}

void MotionConfigurator::resetUI()
{
    #ifdef TUP_DEBUG
        qDebug() << "[MotionConfigurator::resetUI()]";
    #endif

    tweenManager->resetUI();
    closeSettingsPanel();
    settingsPanel->notifySelection(false);
}

void MotionConfigurator::updateTweenData(const QString &name)
{
    emit tweenDataRequested(name);
}

void MotionConfigurator::setCurrentTween(TupItemTweener *tween)
{
    currentTween = tween;
}

void MotionConfigurator::undoSegment(const QPainterPath path)
{
    settingsPanel->undoSegment(path);
}

void MotionConfigurator::redoSegment(const QPainterPath path)
{
    settingsPanel->redoSegment(path);
}

void MotionConfigurator::enableSaveOption(bool flag)
{
    settingsPanel->enableSaveOption(flag);
}

int MotionConfigurator::stepsTotal()
{
    return settingsPanel->stepsTotal();
}

void MotionConfigurator::updateSegments(const QPainterPath path)
{
    settingsPanel->updateSegments(path);
}

int MotionConfigurator::getPathThickness()
{
    return settingsPanel->getPathThickness();
}

QColor MotionConfigurator::getPathColor() const
{
    return settingsPanel->getPathColor();
}
