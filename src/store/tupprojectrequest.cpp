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

#include "tupprojectrequest.h"
#include "tupprojectresponse.h"
#include "tuprequestbuilder.h"

TupProjectRequestArgument::TupProjectRequestArgument()
{
}

TupProjectRequestArgument::TupProjectRequestArgument(const QString &value) : m_value(value)
{
}

TupProjectRequestArgument::~TupProjectRequestArgument()
{
}

void TupProjectRequestArgument::operator = (const QString &value)
{
    setValue(value);
}

void TupProjectRequestArgument::setValue(const QString &value)
{
    m_value = value;
}

bool TupProjectRequestArgument::toBool()
{
    if (m_value == "false" || m_value == "0")
        return false;

    return true;
}

int TupProjectRequestArgument::toInt()
{
    return m_value.toInt();
}

double TupProjectRequestArgument::toReal()
{
    return m_value.toDouble();
}

QString TupProjectRequestArgument::toString()
{
    return m_value;
}

TupProjectRequest::TupProjectRequest(const QString &data)
    : xml(data),
      actionId(TupProjectRequest::None),
      isExternal(false)
{
}

TupProjectRequest::TupProjectRequest(const TupProjectRequest &request)
{
    *this = request;
}

TupProjectRequest::~TupProjectRequest()
{
}

void TupProjectRequest::setActionId(int value)
{
    actionId = value;
}

int TupProjectRequest::getActionId() const
{
    return actionId;
}

bool TupProjectRequest::isValid() const
{
     return !xml.isEmpty(); // TODO: Variable must contain a XML string
}

QString TupProjectRequest::getXml() const
{
     return xml;
}

void TupProjectRequest::setExternal(bool b)
{
     isExternal = b;
}

bool TupProjectRequest::isRequestExternal() const
{
     return isExternal;
}

void TupProjectRequest::setCommandId(const QString &value)
{
     commandId = value;
}

QString TupProjectRequest::getCommandId() const
{
     return commandId;
}

TupProjectRequest &TupProjectRequest::operator=(const TupProjectRequest &other)
{
    if (this == &other)
        return *this;

    xml = other.xml;
    actionId = other.actionId;
    isExternal = other.isExternal;
    commandId = other.commandId;

    return *this;
}
