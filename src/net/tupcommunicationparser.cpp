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

#include "tupcommunicationparser.h"

TupCommunicationParser::TupCommunicationParser() : QXmlStreamReader()
{
    stateValue = 0;
}

TupCommunicationParser::TupCommunicationParser(const QString &xml) : QXmlStreamReader(xml)
{
    stateValue = 0;
}

TupCommunicationParser::~TupCommunicationParser()
{
}

bool TupCommunicationParser::parse()
{
    QString rootTag;
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = name().toString();
            if (rootTag.isEmpty())
                rootTag = tag;

            if (rootTag == "communication_chat" || rootTag == "communication_wall") {
                if (tag == "message") {
                    messageStr = attributes().value("text").toString();
                    loginStr = attributes().value("from").toString();
                }
            } else if (rootTag == "communication_notice") {
                if (tag == "notice") {
                    loginStr = attributes().value("login").toString();
                    stateValue = attributes().value("state").toInt();
                }
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupCommunicationParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

QString TupCommunicationParser::message() const
{
    return messageStr;
}

QString TupCommunicationParser::login() const
{
    return loginStr;
}

int TupCommunicationParser::state()
{
    return stateValue;
}
