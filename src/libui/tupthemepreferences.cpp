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

#include "tupthemepreferences.h"
#include "tosd.h"
#include "tslider.h"
#include "tupcolorbutton.h"
#include "tradiobutton.h"
#include "tseparator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TupThemePreferences::TupThemePreferences(QWidget *parent) : QWidget(parent)
{
    setupPage();
}

TupThemePreferences::~TupThemePreferences()
{
}

void TupThemePreferences::setupPage()
{
    TCONFIG->beginGroup("Theme");
    currentColorRow = TCONFIG->value("ColorRow", 0).toInt();
    oldColorRow = currentColorRow;
    QString bgColor = TCONFIG->value("BgColor", "#a0a0a0").toString();
    currentColor = QColor(bgColor);
    oldColor = currentColor;
    colorPos = TCONFIG->value("ColorPos", 0).toInt();

    themeChanged = false;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *themeLayout = new QHBoxLayout;
    QWidget *pageWidget = new QWidget;
    QVBoxLayout *pageLayout = new QVBoxLayout;

    QLabel *generalLabel = new QLabel(tr("Theme Preferences"));
    QFont labelFont = font();
    labelFont.setBold(true);
    labelFont.setPointSize(labelFont.pointSize() + 3);
    generalLabel->setFont(labelFont);
    pageLayout->addWidget(generalLabel);
    pageLayout->addSpacing(15);
    pageWidget->setLayout(pageLayout);

    labelFont = font();
    labelFont.setBold(true);

    QLabel *lightThemeLabel = new QLabel(tr("Background Color"));
    lightThemeLabel->setFont(labelFont);
    QHBoxLayout *lightThemeLayout = new QHBoxLayout;
    lightThemeLayout->addWidget(lightThemeLabel);

    QVBoxLayout *blockLayout = new QVBoxLayout;
    blockLayout->addWidget(pageWidget);
    blockLayout->setAlignment(pageWidget, Qt::AlignLeft);
    blockLayout->addLayout(lightThemeLayout);

    QStringList labelList;
    labelList << tr("Gray") << tr("Brown") << tr("Chocolate") << tr("Blue")
              << tr("Honey") << tr("Green") << tr("Violet") << tr("Orange")
              << tr("Black");

    QList<QColor> initList;
    initList << QColor(160, 160, 160) << QColor(150, 139, 139)
             << QColor(156, 144, 129) << QColor(132, 203, 238) << QColor(255, 221, 154)
             << QColor(149, 184, 140) << QColor(238, 196, 206) << QColor(247, 205, 163)
             << QColor(0, 0, 0);

    QWidget *lightFormWidget = new QWidget;
    lightFormLayout = new QGridLayout(lightFormWidget);
    for(int i=0; i<labelList.size()-1; i++)
        addColorEntry(LIGHT_THEME, i, labelList.at(i), initList.at(i), Qt::white);

    blockLayout->addWidget(lightFormWidget);

    QWidget *darkFormWidget = new QWidget;
    darkFormLayout = new QGridLayout(darkFormWidget);
    addColorEntry(DARK_THEME, static_cast<int>(Black), tr("Black"), initList.last(), QColor(89, 89, 89));

    blockLayout->addWidget(darkFormWidget);
    blockLayout->addStretch(3);

    QPushButton *resetButton = new QPushButton(QIcon(QPixmap(THEME_DIR + "icons/reset.png")), "");
    resetButton->setToolTip(tr("Restore Default Theme"));
    resetButton->setMaximumWidth(50);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(restoreDefaultTheme()));

    QLabel *resetLabel = new QLabel(tr("Restore Default Theme"));

    QHBoxLayout *resetLayout = new QHBoxLayout;
    resetLayout->addWidget(resetButton);
    resetLayout->addWidget(resetLabel);

    themeLayout->addLayout(blockLayout);
    themeLayout->addStretch();

    TSeparator *div = new TSeparator();

    mainLayout->addLayout(themeLayout);
    mainLayout->addWidget(div);
    mainLayout->addLayout(resetLayout);
    mainLayout->addStretch();
}

void TupThemePreferences::addColorEntry(int theme, int id, const QString &label,
                                        const QColor &initColor, const QColor &endColor)
{
    TRadioButton *colorRadio = new TRadioButton(id, label, this);
    radioList << colorRadio;
    connect(colorRadio, SIGNAL(clicked(int)), this, SLOT(updateCurrentRow(int)));

    QSize cellSize(30, 30);
    QBrush cellBrush(initColor);
    TupColorButton *colorCell = new TupColorButton(1, "", cellBrush, cellSize, "6,4,10");
    colorCell->setEditable(false);
    cellList << colorCell;

    TSlider *colorSlider = new TSlider(Qt::Horizontal, TSlider::Color, initColor, endColor);
    colorSlider->setRange(0, 100);
    sliderList << colorSlider;
    connect(colorSlider, SIGNAL(colorChanged(const QColor&)),
            this, SLOT(updateCurrentColor(const QColor&)));

    lightFormLayout->addWidget(colorRadio, id, 0, Qt::AlignLeft);
    lightFormLayout->addWidget(colorCell, id, 1, Qt::AlignCenter);
    lightFormLayout->addWidget(colorSlider, id, 2, Qt::AlignCenter);

    bool flag = false;
    if (id == currentColorRow) {
        flag = true;
        colorCell->setBrush(QBrush(currentColor));
        colorSlider->setValue(colorPos);
    }

    colorRadio->setChecked(flag);
    colorSlider->setEnabled(flag);
}

void TupThemePreferences::saveValues()
{
    if ((oldColorRow != currentColorRow) || (oldColor != currentColor)) {
        TCONFIG->beginGroup("Theme");
        TCONFIG->setValue("ColorRow", currentColorRow);
        TCONFIG->setValue("BgColor", currentColor.name());
        TCONFIG->setValue("ColorPos", colorPos);
        TCONFIG->setValue("UITheme", appTheme);
        TCONFIG->sync();

        themeChanged = true;
    }
}

bool TupThemePreferences::showWarning()
{
    return themeChanged;
}

void TupThemePreferences::updateCurrentRow(int colorRow)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupGeneralPreferences::updateCurrentRow()] - colorRow ->" << colorRow;
    #endif

    updateUITheme(colorRow);
    currentColorRow = colorRow;
    for(int i=0; i<sliderList.size(); i++) {
        bool flag = true;
        if (i != currentColorRow)
            flag = false;

        sliderList.at(i)->setEnabled(flag);
    }

    currentColor = cellList.at(currentColorRow)->color();
    colorPos = sliderList.at(currentColorRow)->currentValue();

    emit colorPicked(appTheme, currentColor);
}

void TupThemePreferences::updateCurrentColor(const QColor &color)
{
    #ifdef TUP_DEBUG
        int r = color.red();
        int g = color.green();
        int b = color.blue();
        qDebug() << "[TupThemePreferences::updateCurrentColor()] - color -> " << r << "," << g << "," << b;
        qDebug() << "[TupThemePreferences::updateCurrentColor()] - slider value -> " << sliderList.at(currentColorRow)->currentValue();
    #endif

    currentColor = color;
    colorPos = sliderList.at(currentColorRow)->currentValue();
    cellList.at(currentColorRow)->setBrush(QBrush(color));

    emit colorPicked(appTheme, currentColor);
}

void TupThemePreferences::restoreDefaultTheme()
{
    radioList.first()->setChecked(true);
    updateCurrentRow(0);
}

void TupThemePreferences::updateUITheme(int row)
{
    int blackRow = static_cast<int>(Black);
    if (row != blackRow) {
        appTheme = LIGHT_THEME;
    } else {
        appTheme = DARK_THEME;
    }
}
