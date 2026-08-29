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

#include "scale_configurator.h"
#include "tapplicationproperties.h"
#include "tseparator.h"
#include "tosd.h"

#include <QFrame>
#include <QLabel>
#include <QGraphicsPathItem>
#include <QListWidgetItem>

ScaleConfigurator::ScaleConfigurator(QWidget *parent) : QFrame(parent)
{
    framesCount = 1;
    currentFrame = 0;

    currentMode = TupToolPlugin::View;
    state = ScaleConfigurator::Manager;

    layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel *toolTitle = new QLabel;
    toolTitle->setAlignment(Qt::AlignHCenter);
    QPixmap pic(THEME_DIR + "icons/scale_tween.png");
    toolTitle->setPixmap(pic.scaledToWidth(20, Qt::SmoothTransformation));
    toolTitle->setToolTip(tr("Scale Tween Properties"));
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

ScaleConfigurator::~ScaleConfigurator()
{
}

void ScaleConfigurator::loadTweenList(QList<QString> tweenList)
{
    tweenManager->loadTweenList(tweenList);
    if (tweenList.count() > 0)
        activeButtonsPanel(true);
}

void ScaleConfigurator::setPropertiesPanel()
{
    settingsPanel = new ScaleSettings(this);

    connect(settingsPanel, SIGNAL(startingPointChanged(int)), this, SIGNAL(startingPointChanged(int)));
    connect(settingsPanel, SIGNAL(clickedSelect()), this, SIGNAL(clickedSelect()));
    connect(settingsPanel, SIGNAL(clickedDefineProperties()), this, SIGNAL(clickedDefineProperties()));
    connect(settingsPanel, SIGNAL(clickedApplyTween()), this, SLOT(applyItem()));
    connect(settingsPanel, SIGNAL(clickedResetTween()), this, SLOT(closeTweenProperties()));

    settingsLayout->addWidget(settingsPanel);

    activePropertiesPanel(false);
}

void ScaleConfigurator::activePropertiesPanel(bool enable)
{
    if (enable)
        settingsPanel->show();
    else
        settingsPanel->hide();
}

void ScaleConfigurator::setCurrentTween(TupItemTweener *tween)
{
    currentTween = tween;
}

void ScaleConfigurator::setTweenManagerPanel()
{
    tweenManager = new TweenManager(this);
    connect(tweenManager, SIGNAL(addNewTween(const QString &)), this, SLOT(addTween(const QString &)));
    connect(tweenManager, SIGNAL(editCurrentTween(const QString &)), this, SLOT(editTween()));
    connect(tweenManager, SIGNAL(removeCurrentTween(const QString &)), this, SLOT(removeTween(const QString &)));
    connect(tweenManager, SIGNAL(getTweenData(const QString &)), this, SLOT(updateTweenData(const QString &)));

    settingsLayout->addWidget(tweenManager);
    state = ScaleConfigurator::Manager;
}

void ScaleConfigurator::activeTweenManagerPanel(bool enable)
{
    if (enable)
       tweenManager->show();
    else
       tweenManager->hide();

    if (tweenManager->listSize() > 0)
        activeButtonsPanel(enable);
}

void ScaleConfigurator::setButtonsPanel()
{
    controlPanel = new ButtonsPanel(this);
    connect(controlPanel, SIGNAL(clickedEditTween()), this, SLOT(editTween()));
    connect(controlPanel, SIGNAL(clickedRemoveTween()), this, SLOT(removeTween()));

    settingsLayout->addWidget(controlPanel);

    activeButtonsPanel(false);
}

void ScaleConfigurator::activeButtonsPanel(bool enable)
{
    if (enable)
       controlPanel->show();
    else
       controlPanel->hide();
}

void ScaleConfigurator::initStartCombo(int frames, int frameIndex)
{
    framesCount = frames;
    currentFrame = frameIndex;
    settingsPanel->initStartCombo(framesCount, currentFrame);
}

void ScaleConfigurator::setStartFrame(int currentIndex)
{
    currentFrame = currentIndex;
    settingsPanel->setStartFrame(currentIndex);
}

int ScaleConfigurator::startFrame()
{
    return settingsPanel->startFrame();
}

int ScaleConfigurator::startComboSize()
{
    return settingsPanel->startComboSize();
}

QString ScaleConfigurator::tweenToXml(int currentScene, int currentLayer, int currentFrame,
                                 QPointF point, double initialXScaleFactor, double initialYScaleFactor)
{
    return settingsPanel->tweenToXml(currentScene, currentLayer, currentFrame, point,
                                     initialXScaleFactor, initialYScaleFactor);
}

int ScaleConfigurator::totalSteps()
{
    return settingsPanel->totalSteps();
}

void ScaleConfigurator::activateMode(TupToolPlugin::EditMode currentMode)
{
    settingsPanel->activateMode(currentMode);
}

void ScaleConfigurator::addTween(const QString &name)
{
    activeTweenManagerPanel(false);

    currentMode = TupToolPlugin::Add;
    state = ScaleConfigurator::Properties;

    settingsPanel->setParameters(name, framesCount, currentFrame);
    activePropertiesPanel(true);

    emit setMode(currentMode);
}

void ScaleConfigurator::editTween()
{
    currentMode = TupToolPlugin::Edit;
    emit setMode(currentMode);

    activeTweenManagerPanel(false);

    state = ScaleConfigurator::Properties;

    settingsPanel->notifySelection(true);
    settingsPanel->setParameters(currentTween);
    activePropertiesPanel(true);
}

void ScaleConfigurator::removeTween()
{
    QString name =tweenManager->currentTweenName();
    tweenManager->removeItemFromList();

    removeTween(name);
}

void ScaleConfigurator::removeTween(const QString &name)
{
    if (tweenManager->listSize() == 0)
        activeButtonsPanel(false);

    emit clickedRemoveTween(name);
}

QString ScaleConfigurator::currentTweenName() const
{
    QString oldName = tweenManager->currentTweenName();
    QString newName = settingsPanel->currentTweenName();

    if (oldName.compare(newName) != 0)
        tweenManager->updateTweenName(newName);

    return newName;
}

QString ScaleConfigurator::getTweenNameFromList() const
{
    return tweenManager->currentTweenName();
}

void ScaleConfigurator::notifySelection(bool flag)
{
    settingsPanel->notifySelection(flag);
}

void ScaleConfigurator::closeTweenProperties()
{
    if (currentMode == TupToolPlugin::Add)
       tweenManager->removeItemFromList();

    emit clickedResetInterface();

    closeSettingsPanel();
}

void ScaleConfigurator::closeSettingsPanel()
{
    if (state == ScaleConfigurator::Properties) {
        activeTweenManagerPanel(true);
        activePropertiesPanel(false);
        currentMode = TupToolPlugin::View;
        state = ScaleConfigurator::Manager;
    }
}

TupToolPlugin::Mode ScaleConfigurator::mode()
{
    return currentMode;
}

void ScaleConfigurator::applyItem()
{
    currentMode = TupToolPlugin::Edit;
    emit clickedApplyTween();
}

void ScaleConfigurator::resetUI()
{
    tweenManager->resetUI();
    closeSettingsPanel();
    settingsPanel->notifySelection(false);
}

void ScaleConfigurator::updateTweenData(const QString &name)
{
    emit getTweenData(name);
}
