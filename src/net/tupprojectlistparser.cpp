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

#include "tupprojectlistparser.h"

TupProjectListParser::TupProjectListParser() : QXmlStreamReader()
{
    pivot = false;
}

TupProjectListParser::TupProjectListParser(const QString &xml) : QXmlStreamReader(xml)
{
    pivot = false;
}

TupProjectListParser::~TupProjectListParser()
{
}

bool TupProjectListParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            QString tag = name().toString();
            if (tag == "works") {
                pivot = false;
            } else if (tag == "contributions") {
                pivot = true;
            } else if (tag == "project") {
                ProjectInfo info;
                info.file = attributes().value("filename").toString();
                info.name = attributes().value("name").toString();
                info.description = attributes().value("description").toString();
                info.date = attributes().value("date").toString();

                if (pivot) {
                    info.author = attributes().value("author").toString();
                    contribList << info;
                } else {
                    worksList << info;
                }
            }
        }
    }

    if (hasError()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupProjectListParser::parse()] - Fatal Error!";
        #endif
        return false;
    }

    return true;
}

QList<TupProjectListParser::ProjectInfo> TupProjectListParser::works()
{
    return worksList;
}

QList<TupProjectListParser::ProjectInfo> TupProjectListParser::contributions()
{
    return contribList;
}

int TupProjectListParser::workSize()
{
    return worksList.count();
}

int TupProjectListParser::contributionSize()
{
    return contribList.count();
}
