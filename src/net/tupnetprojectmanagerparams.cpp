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

#include "tupnetprojectmanagerparams.h"

TupNetProjectManagerParams::TupNetProjectManagerParams() : loginStr(""), passwdStr(""), serverStr("localhost"), portValue(8080)
{
}

TupNetProjectManagerParams::~TupNetProjectManagerParams()
{
}

void TupNetProjectManagerParams::setLogin(const QString &login)
{
    loginStr = login;
}

QString TupNetProjectManagerParams::login() const
{
    return loginStr;
}

void  TupNetProjectManagerParams::setPassword(const QString &secret) 
{
    passwdStr = secret;
}

QString TupNetProjectManagerParams::password() const
{
    return passwdStr;
}

void TupNetProjectManagerParams::setWindowRecordID(const QString &windowRecordID)
{
    cacheStr = windowRecordID;
}

QString TupNetProjectManagerParams::windowRecordID() const
{
    return cacheStr;
}

void TupNetProjectManagerParams::setServer(const QString &server)
{
    serverStr = server;
}

QString TupNetProjectManagerParams::server() const
{
    return serverStr;
}

void TupNetProjectManagerParams::setPort(int port)
{
    portValue = port;
}

int TupNetProjectManagerParams::port() const
{
    return portValue;
}
