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

#ifndef TUPPROJECTLISTPARSER_H
#define TUPPROJECTLISTPARSER_H

#include "tglobal.h"

#include <QXmlStreamReader>

class TUPITUBE_EXPORT TupProjectListParser : public QXmlStreamReader
{
    public:
        struct ProjectInfo
        {
            QString name;
            QString author;
            QString description;
            QString date;
            QString file;
        };

        TupProjectListParser();
        TupProjectListParser(const QString &xml);
        ~TupProjectListParser();

        bool parse();
        QList<ProjectInfo> works();
        QList<ProjectInfo> contributions();
        int workSize();
        int contributionSize();

    private:
        QList<TupProjectListParser::ProjectInfo> worksList;
        QList<TupProjectListParser::ProjectInfo> contribList;
        bool pivot;
};

#endif
