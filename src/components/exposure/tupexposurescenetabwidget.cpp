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

#include "tupexposurescenetabwidget.h"
#include "timagebutton.h"
#include "tresponsiveui.h"
#include "tseparator.h"

TupExposureSceneTabWidget::TupExposureSceneTabWidget(QWidget *parent) : QFrame(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget()]";
    #endif

    audioScrubbingEnabled = false;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(1);

    scenesTabber = new QTabWidget;
    scenesTabber->setMovable(true);

    connect(scenesTabber->tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SIGNAL(sceneRenameRequested(int)));
    connect(scenesTabber->tabBar(), SIGNAL(tabMoved(int,int)), this, SIGNAL(sceneMoved(int,int)));
    connect(scenesTabber, SIGNAL(currentChanged(int)), this, SIGNAL(currentChanged(int)));

    layout->addWidget(scenesTabber);

    setLayout(layout);
}

TupExposureSceneTabWidget::~TupExposureSceneTabWidget()
{
    sceneContainers.clear();
    undoContainers.clear();

    sceneTables.clear();
    undoTables.clear();

    opacityControl.clear();
    undoOpacities.clear();

    audioScrubbingButtons.clear();

    delete scenesTabber;
}

void TupExposureSceneTabWidget::removeAllTabs()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::removeAllTabs()]";
    #endif

    int count = scenesTabber->count();
    for (int i = 0; i < count; i++)
         delete scenesTabber->currentWidget();

    sceneContainers.clear();
    undoContainers.clear();

    sceneTables.clear();
    undoTables.clear();

    opacityControl.clear();
    undoOpacities.clear();

    audioScrubbingButtons.clear();
}

void TupExposureSceneTabWidget::addScene(int index, const QString &sceneName,
                                         TupExposureTable *exposureTable, bool preserveSelection)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::addScene()] - index ->" << index;
        qDebug() << "[TupExposureSceneTabWidget::addScene()] - name ->" << sceneName;
    #endif

    int previousIndex = -1;
    if (preserveSelection)
        previousIndex = scenesTabber->currentIndex();

    QFrame *tableContainer = new QFrame;
    QVBoxLayout *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setMargin(1);

    QHBoxLayout *toolsLayout = new QHBoxLayout;
    toolsLayout->setAlignment(Qt::AlignHCenter);

    QLabel *containerHeader = new QLabel();
    QPixmap pix(ICONS_DIR + "layer_opacity.png");
    containerHeader->setToolTip(tr("Current Layer Opacity"));
    containerHeader->setPixmap(pix.scaledToWidth(TResponsiveUI::fitSmallIconSize()));

    QDoubleSpinBox *opacitySpinBox = new QDoubleSpinBox(this);
    opacitySpinBox->setRange(0.1, 1.0);
    opacitySpinBox->setSingleStep(0.1);
    opacitySpinBox->setValue(1.0);
    opacitySpinBox->setToolTip(tr("Current Layer Opacity"));
    connect(opacitySpinBox, SIGNAL(valueChanged(double)),
            this, SIGNAL(layerOpacityChanged(double)));

    opacityControl << opacitySpinBox;

    QPushButton *audioScrubbingButton = new QPushButton(this);
    QPixmap speakerPix(ICONS_DIR + "speaker.png");
    audioScrubbingButton->setIcon(speakerPix.scaledToWidth(TResponsiveUI::fitSmallIconSize()));
    audioScrubbingButton->setCheckable(true);
    audioScrubbingButton->setChecked(false);
    audioScrubbingButton->setEnabled(false);
    audioScrubbingButton->setToolTip(tr("Audio Scrubbing"));
    connect(audioScrubbingButton, SIGNAL(toggled(bool)), this, SLOT(toggleAudioScrubbing(bool)));
    audioScrubbingButtons << audioScrubbingButton;

    toolsLayout->addWidget(containerHeader);
    toolsLayout->addWidget(opacitySpinBox);
    toolsLayout->addWidget(new TSeparator(Qt::Vertical));
    toolsLayout->addWidget(audioScrubbingButton);

    tableLayout->addLayout(toolsLayout);
    tableLayout->addWidget(exposureTable);
    tableContainer->setLayout(tableLayout);

    sceneTables.insert(index, exposureTable);
    sceneContainers.insert(index, tableContainer);

    scenesTabber->blockSignals(true);
    scenesTabber->insertTab(index, tableContainer, sceneName);
    if (preserveSelection && previousIndex >= 0) {
        // Account for index shift: if previous tab was at or after insertion point, it shifted by +1
        int restoredIndex = (previousIndex >= index) ? previousIndex + 1 : previousIndex;
        scenesTabber->setCurrentIndex(restoredIndex);
    }
    scenesTabber->blockSignals(false);
}

void TupExposureSceneTabWidget::restoreScene(int sceneIndex, const QString &sceneName)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::restoreScene()] - sceneIndex ->" << sceneIndex;
        qDebug() << "[TupExposureSceneTabWidget::restoreScene()] - sceneName ->" << sceneName;
    #endif

    if (!undoTables.isEmpty()) {
        TupExposureTable *exposureTable = undoTables.takeLast();
        if (exposureTable) {
            QDoubleSpinBox *opacitySpinBox = undoOpacities.takeLast();
            opacityControl << opacitySpinBox;
            sceneTables.insert(sceneIndex, exposureTable);

            QFrame *tableContainer = undoContainers.takeLast();
            sceneContainers.insert(sceneIndex, tableContainer);
            scenesTabber->insertTab(sceneIndex, tableContainer, sceneName);
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupExposureSceneTabWidget::restoreScene()] - "
                            "Fatal Error: table from undoTables stack is NULL!";
            #endif
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupExposureSceneTabWidget::restoreScene()] - "
                        "Fatal Error: The undoTables stack is empty!";
        #endif
    }
}

void TupExposureSceneTabWidget::removeScene(int sceneIndex, bool withBackup)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::removeScene()] - sceneIndex ->" << sceneIndex;
    #endif

    if (withBackup) {
        undoContainers << sceneContainers.takeAt(sceneIndex);
        undoTables << sceneTables.takeAt(sceneIndex);
        undoOpacities << opacityControl.takeAt(sceneIndex);
    } else {
        sceneContainers.takeAt(sceneIndex);
        sceneTables.takeAt(sceneIndex);
        opacityControl.takeAt(sceneIndex);
    }

    blockSignals(true);
        scenesTabber->removeTab(sceneIndex);
    blockSignals(false);
}

void TupExposureSceneTabWidget::renameScene(int sceneIndex, const QString &sceneName)
{
    scenesTabber->setTabText(sceneIndex, sceneName);
}

void TupExposureSceneTabWidget::moveScene(int index, int newIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::moveScene()] - index ->" << index;
        qDebug() << "[TupExposureSceneTabWidget::moveScene()] - newIndex ->" << newIndex;
    #endif

    QList<QString> tabLabels;
    for(int i = 0; i < scenesTabber->count(); i++)
        tabLabels.append(scenesTabber->tabText(i));
    tabLabels.swapItemsAt(index, newIndex);
    swapExposureTables(index, newIndex);

    scenesTabber->blockSignals(true);
        // Removing all tabs
        while (scenesTabber->count() > 0)
               scenesTabber->removeTab(0);
        // Restoring all tabs (new order)
        for (int i=0; i<tabLabels.count(); i++)
            scenesTabber->addTab(sceneContainers.at(i), tabLabels.at(i));
        // Keep focus on the moved scene
        scenesTabber->setCurrentIndex(newIndex);
    scenesTabber->blockSignals(false);
}

void TupExposureSceneTabWidget::swapExposureTables(int index, int newIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::swapExposureTables()] - index ->" << index;
        qDebug() << "[TupExposureSceneTabWidget::swapExposureTables()] - newIndex ->" << newIndex;
    #endif

    sceneContainers.swapItemsAt(index, newIndex);
    sceneTables.swapItemsAt(index, newIndex);
    opacityControl.swapItemsAt(index, newIndex);
}

TupExposureTable* TupExposureSceneTabWidget::getCurrentExposureTable()
{
    int index = currentSceneIndex();
    return getExposureTable(index);
}

TupExposureTable* TupExposureSceneTabWidget::getExposureTable(int sceneIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::getExposureTable()] - sceneIndex ->"
                 << sceneIndex;
    #endif

    if (isExposureTableIndexValid(sceneIndex)) {
        TupExposureTable *exposureTable = sceneTables.at(sceneIndex);
        if (exposureTable) {
            return exposureTable;
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupExposureSceneTabWidget::getExposureTable()] - "
                            "Fatal Error: Table pointer is NULL!";
            #endif
        }
    }

    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::getExposureTable()] - "
                    "Fatal Error: Invalid table index ->"
                 << sceneIndex;
    #endif

    return 0;
}

void TupExposureSceneTabWidget::setCurrentSceneIndex(int sceneIndex)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::setCurrentSceneIndex()] - sceneIndex ->" << sceneIndex;
    #endif

    scenesTabber->setCurrentIndex(sceneIndex);
}

int TupExposureSceneTabWidget::currentSceneIndex()
{
    int index = scenesTabber->currentIndex();

    return index;
}

QString TupExposureSceneTabWidget::currentSceneName() const
{
    int sceneIndex = scenesTabber->currentIndex();
    if (sceneIndex > -1)
        return scenesTabber->tabText(sceneIndex);

    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::currentSceneName()] - Fatal Error: No scene selected!";
    #endif

    return "";
}

bool TupExposureSceneTabWidget::isExposureTableIndexValid(int index)
{
    if (index > -1 && index < sceneTables.count())
        return true;

    return false;
}

int TupExposureSceneTabWidget::count()
{
    return sceneTables.count();
}

void TupExposureSceneTabWidget::setLayerOpacity(int sceneIndex, double opacity)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::setLayerOpacity()] - sceneIndex/opacity ->"
                 << sceneIndex << "," << opacity;
    #endif

    if (opacityControl.at(sceneIndex)) {
        QDoubleSpinBox *spinBox = opacityControl.at(sceneIndex);
        spinBox->blockSignals(true);
            spinBox->setValue(opacity);
        spinBox->blockSignals(false);
    }
}

void TupExposureSceneTabWidget::setLayerVisibility(int sceneIndex, int layerIndex, bool visibility)
{
    if (isExposureTableIndexValid(sceneIndex)) {
        TupExposureTable *table = sceneTables.at(sceneIndex);
        table->setLayerVisibility(layerIndex, visibility);
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TupExposureSceneTabWidget::setLayerVisibility()] - "
                          "Fatal Error: Invalid scene index ->"
                       << sceneIndex;
        #endif
    }
}

void TupExposureSceneTabWidget::toggleAudioScrubbing(bool enabled)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupExposureSceneTabWidget::toggleAudioScrubbing()] - enabled ->" << enabled;
    #endif

    audioScrubbingEnabled = enabled;

    // Update icon based on state
    QString iconPath = enabled ? ICONS_DIR + "speaker_on.png" : ICONS_DIR + "speaker.png";
    QPixmap speakerPix(iconPath);
    QIcon speakerIcon(speakerPix.scaledToWidth(TResponsiveUI::fitSmallIconSize()));

    // Sync all buttons state and icon
    for (QPushButton *button : audioScrubbingButtons) {
        button->blockSignals(true);
        button->setChecked(enabled);
        button->setIcon(speakerIcon);
        button->blockSignals(false);
    }

    emit audioScrubbingToggled(enabled);
}

bool TupExposureSceneTabWidget::isAudioScrubbingEnabled() const
{
    return audioScrubbingEnabled;
}

void TupExposureSceneTabWidget::setSceneAudioEnabled(int sceneIndex, bool enabled)
{
    if (sceneIndex >= 0 && sceneIndex < audioScrubbingButtons.size()) {
        QPushButton *button = audioScrubbingButtons.at(sceneIndex);
        button->setEnabled(enabled);
        if (!enabled) {
            button->blockSignals(true);
            button->setChecked(false);
            QPixmap speakerPix(ICONS_DIR + "speaker.png");
            button->setIcon(speakerPix.scaledToWidth(TResponsiveUI::fitSmallIconSize()));
            button->blockSignals(false);
        }
    }
}
