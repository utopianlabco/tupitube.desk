/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              * 
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Naara's Development Team                                             *
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

#include "tinputfield.h"

TInputField::TInputField(const QString &input, QWidget *parent) : QLineEdit(parent)
{
    setText(input);
}

TInputField::~TInputField()
{
}

void TInputField::focusInEvent(QFocusEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupInputField::focusInEvent()]";
    #endif

    bool enabled = true;
    if (text().isEmpty())
        enabled = false;
    emit inputFilled(enabled);

    QLineEdit::focusInEvent(event);
}

void TInputField::focusOutEvent(QFocusEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupInputField::focusOutEvent()]";
    #endif

    bool enabled = true;
    if (text().isEmpty())
        enabled = false;
    emit inputFilled(enabled);

    QLineEdit::focusOutEvent(event);
}

void TInputField::keyPressEvent(QKeyEvent *event)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupInputField::keyPressEvent()]";
    #endif

    bool enabled = true;
    if (text().isEmpty())
        enabled = false;
    emit inputFilled(enabled);

    QLineEdit::keyPressEvent(event);
}
