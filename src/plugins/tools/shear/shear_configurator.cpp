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

#include "shear_configurator.h"
#include "tapplicationproperties.h"
#include "tseparator.h"
#include "tosd.h"

ShearConfigurator::ShearConfigurator(QWidget *parent) : QFrame(parent)
{
    framesCount = 1;
    currentFrame = 0;

    currentMode = TupToolPlugin::View;
    state = ShearConfigurator::Manager;

    layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel *toolTitle = new QLabel;
    toolTitle->setAlignment(Qt::AlignHCenter);
    QPixmap pic(THEME_DIR + "icons/shear_tween.png");
    toolTitle->setPixmap(pic.scaledToWidth(20, Qt::SmoothTransformation));
    toolTitle->setToolTip(tr("Shear Tween Properties"));
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

ShearConfigurator::~ShearConfigurator()
{
}

void ShearConfigurator::loadTweenList(QList<QString> tweenList)
{
    tweenManager->loadTweenList(tweenList);
    if (tweenList.count() > 0)
        activeButtonsPanel(true);
}

void ShearConfigurator::setPropertiesPanel()
{
    settingsPanel = new ShearSettings(this);

    connect(settingsPanel, SIGNAL(initFrameChanged(int)), this, SIGNAL(initFrameChanged(int)));
    connect(settingsPanel, SIGNAL(clickedSelect()), this, SIGNAL(clickedSelect()));
    connect(settingsPanel, SIGNAL(clickedDefineProperties()), this, SIGNAL(clickedDefineProperties()));
    connect(settingsPanel, SIGNAL(clickedApplyTween()), this, SLOT(applyItem()));
    connect(settingsPanel, SIGNAL(clickedResetTween()), this, SLOT(closeTweenProperties()));

    settingsLayout->addWidget(settingsPanel);

    activePropertiesPanel(false);
}

void ShearConfigurator::activePropertiesPanel(bool enable)
{
    if (enable)
        settingsPanel->show();
    else
        settingsPanel->hide();
}

void ShearConfigurator::setCurrentTween(TupItemTweener *tween)
{
    currentTween = tween;
}

void ShearConfigurator::setTweenManagerPanel()
{
    tweenManager = new TweenManager(this);
    connect(tweenManager, SIGNAL(addNewTween(const QString &)), this, SLOT(addTween(const QString &)));
    connect(tweenManager, SIGNAL(editCurrentTween(const QString &)), this, SLOT(editTween()));
    connect(tweenManager, SIGNAL(removeCurrentTween(const QString &)), this, SLOT(removeTween(const QString &)));
    connect(tweenManager, SIGNAL(getTweenData(const QString &)), this, SLOT(updateTweenData(const QString &)));

    settingsLayout->addWidget(tweenManager);
    state = ShearConfigurator::Manager;
}

void ShearConfigurator::activeTweenManagerPanel(bool enable)
{
    if (enable)
        tweenManager->show();
    else
        tweenManager->hide();

    if (tweenManager->listSize() > 0)
        activeButtonsPanel(enable);
}

void ShearConfigurator::setButtonsPanel()
{
    controlPanel = new ButtonsPanel(this);
    connect(controlPanel, SIGNAL(clickedEditTween()), this, SLOT(editTween()));
    connect(controlPanel, SIGNAL(clickedRemoveTween()), this, SLOT(removeTween()));

    settingsLayout->addWidget(controlPanel);

    activeButtonsPanel(false);
}

void ShearConfigurator::activeButtonsPanel(bool enable)
{
    if (enable)
        controlPanel->show();
    else
        controlPanel->hide();
}

void ShearConfigurator::initStartCombo(int frames, int frameIndex)
{
    framesCount = frames;
    currentFrame = frameIndex;
    settingsPanel->initStartCombo(framesCount, currentFrame);
}

void ShearConfigurator::setStartFrame(int currentIndex)
{
    currentFrame = currentIndex;
    settingsPanel->setStartFrame(currentIndex);
}

int ShearConfigurator::startFrame()
{
    return settingsPanel->startFrame();
}

int ShearConfigurator::startComboSize()
{
    return settingsPanel->startComboSize();
}

QString ShearConfigurator::tweenToXml(int currentScene, int currentLayer, int currentFrame, QPointF point)
{
    return settingsPanel->tweenToXml(currentScene, currentLayer, currentFrame, point);
}

int ShearConfigurator::totalSteps()
{
    return settingsPanel->totalSteps();
}

void ShearConfigurator::activateMode(TupToolPlugin::EditMode mode)
{
    settingsPanel->activateMode(mode);
}

void ShearConfigurator::addTween(const QString &name)
{
    currentMode = TupToolPlugin::Add;
    emit setMode(currentMode);

    settingsPanel->setParameters(name, framesCount, currentFrame);
    
    activeTweenManagerPanel(false);
    activePropertiesPanel(true);

    state = Properties;

    // emit setMode(currentMode);
}

void ShearConfigurator::editTween()
{
    currentMode = TupToolPlugin::Edit;
    emit setMode(currentMode);

    activeTweenManagerPanel(false);

    // currentMode = TupToolPlugin::Edit;
    state = Properties;

    settingsPanel->notifySelection(true);
    settingsPanel->setParameters(currentTween);
    activePropertiesPanel(true);

    // emit setMode(currentMode);
}

void ShearConfigurator::removeTween()
{
    QString name = tweenManager->currentTweenName();
    tweenManager->removeItemFromList();

    removeTween(name);
}

void ShearConfigurator::removeTween(const QString &name)
{
    if (tweenManager->listSize() == 0)
        activeButtonsPanel(false);

    emit clickedRemoveTween(name);
}

QString ShearConfigurator::currentTweenName() const
{
    QString oldName = tweenManager->currentTweenName();
    QString newName = settingsPanel->currentTweenName();

    if (oldName.compare(newName) != 0)
        tweenManager->updateTweenName(newName);

    return newName;
}

QString ShearConfigurator::getTweenNameFromList() const
{
    return tweenManager->currentTweenName();
}

void ShearConfigurator::notifySelection(bool flag)
{
    settingsPanel->notifySelection(flag);
}

void ShearConfigurator::closeTweenProperties()
{
    if (currentMode == TupToolPlugin::Add)
        tweenManager->removeItemFromList();

    emit clickedResetInterface();
    closeSettingsPanel();
}

void ShearConfigurator::closeSettingsPanel()
{
    if (state == ShearConfigurator::Properties) {
        activeTweenManagerPanel(true);
        activePropertiesPanel(false);
        currentMode = TupToolPlugin::View;
        state = ShearConfigurator::Manager;
    } // else {
        // settingsPanel->activateMode(TupToolPlugin::Properties);
    // }
}

TupToolPlugin::Mode ShearConfigurator::mode()
{
    return currentMode;
}

void ShearConfigurator::applyItem()
{
    currentMode = TupToolPlugin::Edit;
    emit clickedApplyTween();
}

void ShearConfigurator::resetUI()
{
    tweenManager->resetUI();
    closeSettingsPanel();
    settingsPanel->notifySelection(false);
}

void ShearConfigurator::updateTweenData(const QString &name)
{
    emit getTweenData(name);
}
