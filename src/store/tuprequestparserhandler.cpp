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

#include "tuprequestparserhandler.h"

#include <QDebug>

TupRequestParserHandler::TupRequestParserHandler()
    : QXmlStreamReader(),
      response(nullptr)
{
}

TupRequestParserHandler::TupRequestParserHandler(const QString &xml)
    : QXmlStreamReader(xml),
      response(nullptr)
{
}

TupRequestParserHandler::~TupRequestParserHandler()
{
}

bool TupRequestParserHandler::ensureResponse(const QString &elementName)
{
    if (response)
        return true;

    raiseError(
        QStringLiteral("Element \"%1\" found before the action element.")
            .arg(elementName));

    return false;
}

bool TupRequestParserHandler::parse()
{
    while (!atEnd()) {
        readNext();

        if (!isStartElement())
            continue;

        const QString tag = name().toString();

        if (tag == QStringLiteral("project_request")) {
            sign = attributes().value(QStringLiteral("sign")).toString();
            commandId =
                attributes().value(QStringLiteral("command_id")).toString();

        } else if (tag == QStringLiteral("action")) {
            response = TupProjectResponseFactory::create(
                attributes().value(QStringLiteral("part")).toInt(),
                attributes().value(QStringLiteral("id")).toInt());

            if (!response) {
                raiseError(
                    QStringLiteral("Unable to create a project response."));
                continue;
            }

            response->setCommandId(commandId);
            response->setArg(
                attributes().value(QStringLiteral("arg")).toString());

        } else if (tag == QStringLiteral("item")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupItemResponse *>(response)->setItemIndex(
                attributes().value(QStringLiteral("index")).toInt());

        } else if (tag == QStringLiteral("objectType")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupItemResponse *>(response)->setItemType(
                TupLibraryObject::ObjectType(
                    attributes().value(QStringLiteral("id")).toInt()));

        } else if (tag == QStringLiteral("position")) {
            if (!ensureResponse(tag))
                continue;

            TupItemResponse *itemResponse =
                static_cast<TupItemResponse *>(response);

            itemResponse->setPosX(
                attributes().value(QStringLiteral("x")).toDouble());
            itemResponse->setPosY(
                attributes().value(QStringLiteral("y")).toDouble());

        } else if (tag == QStringLiteral("spaceMode")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupItemResponse *>(response)->setSpaceMode(
                TupProject::Mode(
                    attributes().value(QStringLiteral("current")).toInt()));

        } else if (tag == QStringLiteral("frame")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupFrameResponse *>(response)->setFrameIndex(
                attributes().value(QStringLiteral("index")).toInt());

        } else if (tag == QStringLiteral("data")) {
            if (!ensureResponse(tag))
                continue;

            const QByteArray encodedData = readElementText().toLatin1();
            response->setData(QByteArray::fromBase64(encodedData));

        } else if (tag == QStringLiteral("layer")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupLayerResponse *>(response)->setLayerIndex(
                attributes().value(QStringLiteral("index")).toInt());

        } else if (tag == QStringLiteral("scene")) {
            if (!ensureResponse(tag))
                continue;

            static_cast<TupSceneResponse *>(response)->setSceneIndex(
                attributes().value(QStringLiteral("index")).toInt());

        } else if (tag == QStringLiteral("symbol")) {
            if (!ensureResponse(tag))
                continue;

            TupLibraryResponse *libraryResponse =
                static_cast<TupLibraryResponse *>(response);

            libraryResponse->setSymbolType(
                TupLibraryObject::ObjectType(
                    attributes().value(QStringLiteral("type")).toInt()));

            libraryResponse->setParent(
                attributes().value(QStringLiteral("folder")).toString());

            libraryResponse->setSpaceMode(
                TupProject::Mode(
                    attributes()
                        .value(QStringLiteral("spaceMode"))
                        .toInt()));
        }
    }

    if (hasError()) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupRequestParserHandler::parse()]"
            << "XML error:" << errorString()
            << "line:" << lineNumber()
            << "column:" << columnNumber();
#endif
        return false;
    }

    if (!response) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupRequestParserHandler::parse()]"
            << "Missing action element.";
#endif
        return false;
    }

    return true;
}

TupProjectResponse *TupRequestParserHandler::getResponse() const
{
    return response;
}

QString TupRequestParserHandler::getSign() const
{
    return sign;
}

QString TupRequestParserHandler::getCommandId() const
{
    return commandId;
}
