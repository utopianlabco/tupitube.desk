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

#include "txyspinbox.h"

TXYSpinBox::TXYSpinBox(const QString &title, const QString &xLabel, const QString &yLabel,
                       QWidget *parent) : QGroupBox(title, parent), isChecked(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    QGridLayout *internal = new QGridLayout;
    m_textX = new QLabel(xLabel + " ");
    internal->addWidget(m_textX, 0, 0, Qt::AlignRight);

    m_x = new QSpinBox;
    m_x->setMinimumWidth(60);
    internal->addWidget(m_x, 0, 1);
    connect(m_x, SIGNAL(valueChanged(int)), this, SIGNAL(valuesHaveChanged()));

    m_textX->setBuddy(m_x);

    m_textY = new QLabel(yLabel + " ");
    internal->addWidget(m_textY, 1, 0, Qt::AlignRight);

    m_y = new QSpinBox;
    m_y->setMinimumWidth(60);
    internal->addWidget(m_y, 1, 1);
    connect(m_y, SIGNAL(valueChanged(int)), this, SIGNAL(valuesHaveChanged()));

    m_textY->setBuddy(m_y);
    layout->addLayout(internal);

    button = new QPushButton;
    button->setMaximumWidth(20);
    button->setIcon(QPixmap(ICONS_DIR + "rectangle_dimension.png"));
    button->setToolTip(tr("Rectangle Dimension"));

    layout->addWidget(button);

    connect(button, SIGNAL(clicked()), this, SLOT(toggleModify()));
    setLayout(layout);

    connect(m_x, SIGNAL(editingFinished()), this, SLOT(updateYValue()));
    connect(m_y, SIGNAL(editingFinished()), this, SLOT(updateXValue()));

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

TXYSpinBox::~TXYSpinBox()
{
}

void TXYSpinBox::updateXValue()
{
     if (isChecked) {
         int y = m_y->value();
         if (m_x->value() != y)
             m_x->setValue(y);
     }
}

void TXYSpinBox::updateYValue()
{
     if (isChecked) {
         int x = m_x->value();
         if (m_y->value() != x)
             m_y->setValue(x);
     }
}

bool TXYSpinBox::buttonIsChecked()
{
    return isChecked;
}

void TXYSpinBox::toggleModify()
{
    if (!isChecked) {
        isChecked = true;
        button->setIcon(QPixmap(ICONS_DIR + "square_dimension.png"));
        button->setToolTip(tr("Square Dimension"));

        int x = m_x->value();
        if (m_y->value() != x)
            m_y->setValue(x);

    } else {
        isChecked = false;
        button->setIcon(QPixmap(ICONS_DIR + "rectangle_dimension.png"));
        button->setToolTip(tr("Rectangle Dimension"));
    }
}

void TXYSpinBox::setSingleStep(int step)
{
    m_x->setSingleStep(step);
    m_y->setSingleStep(step);
}

void TXYSpinBox::setMinimum(int min)
{
    m_x->setMinimum(min);
    m_y->setMinimum(min);
}

void TXYSpinBox::setMaximum(int max)
{
    m_x->setMaximum(max);
    m_y->setMaximum(max);
}

void TXYSpinBox::setX(int x)
{
    m_x->setValue(x);
}

void TXYSpinBox::setY(int y)
{
    m_y->setValue(y);
}

int TXYSpinBox::x()
{
    return m_x->value();
}

int TXYSpinBox::y()
{
    return m_y->value();
}

