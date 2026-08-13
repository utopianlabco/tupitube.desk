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

#include "tuprequestbuilder.h"
#include "tupprojectrequest.h"
#include "tupprojectresponse.h"

#include <QUuid>

TupRequestBuilder::TupRequestBuilder()
{
}

TupRequestBuilder::~TupRequestBuilder()
{
}

QString TupRequestBuilder::createCommandId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString TupRequestBuilder::resolveCommandId(const QString &commandId)
{
    if (!commandId.isEmpty())
        return commandId;

    return createCommandId();
}

TupProjectRequest TupRequestBuilder::buildRequest(const QDomDocument &doc, int actionId,
                                                  const QString &commandId,
                                                  const QString &dependencyCommandId)
{
    TupProjectRequest request(doc.toString(0));
    request.setActionId(actionId);
    request.setCommandId(commandId);

    if (!dependencyCommandId.trimmed().isEmpty())
        request.setDependencyCommandId(dependencyCommandId);

    return request;
}

TupProjectRequest TupRequestBuilder::createItemRequest(int sceneIndex, int layerIndex, int frameIndex,
                                                       int itemIndex, QPointF point,
                                                       TupProject::Mode spaceMode,
                                                       TupLibraryObject::ObjectType type,
                                                       int actionId, const QVariant &arg,
                                                       const QByteArray &data,
                                                       const QString &commandId,
                                                       const QString &dependencyCommandId,
                                                       const QString &objectId)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("project_request");

    const QString resolvedCommandId = resolveCommandId(commandId);
    root.setAttribute("command_id", resolvedCommandId);
    if (!dependencyCommandId.trimmed().isEmpty())
        root.setAttribute("depends_on", dependencyCommandId.trimmed());

    QDomElement scene = doc.createElement("scene");
    scene.setAttribute("index", sceneIndex);

    QDomElement layer = doc.createElement("layer");
    layer.setAttribute("index", layerIndex);

    QDomElement frame = doc.createElement("frame");
    frame.setAttribute("index", frameIndex);

    QDomElement item = doc.createElement("item");
    item.setAttribute("index", itemIndex);
    if (!objectId.trimmed().isEmpty())
        item.setAttribute("object_id", objectId.trimmed());

    QDomElement objectType = doc.createElement("objectType");
    objectType.setAttribute("id", type);

    QDomElement position = doc.createElement("position");
    position.setAttribute("x", QString::number(point.x()));
    position.setAttribute("y", QString::number(point.y()));

    QDomElement space = doc.createElement("spaceMode");
    space.setAttribute("current", spaceMode);

    QDomElement action = doc.createElement("action");
    action.setAttribute("id", actionId);
    action.setAttribute("arg", arg.toString());
    action.setAttribute("part", TupProjectRequest::Item);

    appendData(doc, action, data);
    root.appendChild(action);
    item.appendChild(objectType);
    item.appendChild(position);
    item.appendChild(space);
    frame.appendChild(item);
    layer.appendChild(frame);
    scene.appendChild(layer);
    root.appendChild(scene);
    doc.appendChild(root);

    return buildRequest(doc, actionId, resolvedCommandId, dependencyCommandId);
}

TupProjectRequest TupRequestBuilder::createFrameRequest(int sceneIndex, int layerIndex, int frameIndex,
                                                        int actionId, const QVariant &arg,
                                                        const QByteArray &data,
                                                        const QString &commandId,
                                                        const QString &dependencyCommandId)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("project_request");

    const QString resolvedCommandId = resolveCommandId(commandId);
    root.setAttribute("command_id", resolvedCommandId);
    if (!dependencyCommandId.trimmed().isEmpty())
        root.setAttribute("depends_on", dependencyCommandId.trimmed());

    QDomElement scene = doc.createElement("scene");
    scene.setAttribute("index", sceneIndex);

    QDomElement layer = doc.createElement("layer");
    layer.setAttribute("index", layerIndex);

    QDomElement frame = doc.createElement("frame");
    frame.setAttribute("index", frameIndex);

    QDomElement action = doc.createElement("action");
    action.setAttribute("id", actionId);
    action.setAttribute("arg", arg.toString());
    action.setAttribute("part", TupProjectRequest::Frame);

    appendData(doc, action, data);
    root.appendChild(action);
    layer.appendChild(frame);
    scene.appendChild(layer);
    root.appendChild(scene);
    doc.appendChild(root);

    return buildRequest(doc, actionId, resolvedCommandId, dependencyCommandId);
}

TupProjectRequest TupRequestBuilder::createLayerRequest(int sceneIndex, int layerIndex, int actionId,
                                                        const QVariant &arg, const QByteArray &data,
                                                        const QString &commandId,
                                                        const QString &dependencyCommandId)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("project_request");

    const QString resolvedCommandId = resolveCommandId(commandId);
    root.setAttribute("command_id", resolvedCommandId);
    if (!dependencyCommandId.trimmed().isEmpty())
        root.setAttribute("depends_on", dependencyCommandId.trimmed());

    QDomElement scene = doc.createElement("scene");
    scene.setAttribute("index", sceneIndex);

    QDomElement layer = doc.createElement("layer");
    layer.setAttribute("index", layerIndex);

    QDomElement action = doc.createElement("action");
    action.setAttribute("id", actionId);
    action.setAttribute("arg", arg.toString());
    action.setAttribute("part", TupProjectRequest::Layer);

    appendData(doc, action, data);
    root.appendChild(action);
    scene.appendChild(layer);
    root.appendChild(scene);
    doc.appendChild(root);

    return buildRequest(doc, actionId, resolvedCommandId, dependencyCommandId);
}

TupProjectRequest TupRequestBuilder::createSceneRequest(int sceneIndex, int actionId,
                                                        const QVariant &arg, const QByteArray &data,
                                                        const QString &commandId,
                                                        const QString &dependencyCommandId)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("project_request");

    const QString resolvedCommandId = resolveCommandId(commandId);
    root.setAttribute("command_id", resolvedCommandId);
    if (!dependencyCommandId.trimmed().isEmpty())
        root.setAttribute("depends_on", dependencyCommandId.trimmed());

    QDomElement scene = doc.createElement("scene");
    scene.setAttribute("index", sceneIndex);

    QDomElement action = doc.createElement("action");
    action.setAttribute("id", actionId);
    action.setAttribute("arg", arg.toString());
    action.setAttribute("part", TupProjectRequest::Scene);

    appendData(doc, action, data);
    root.appendChild(action);
    root.appendChild(scene);
    doc.appendChild(root);

    return buildRequest(doc, actionId, resolvedCommandId, dependencyCommandId);
}

TupProjectRequest TupRequestBuilder::createLibraryRequest(int actionId, const QVariant &arg,
                                                          TupLibraryObject::ObjectType type,
                                                          TupProject::Mode spaceMode,
                                                          const QByteArray &data,
                                                          const QString &folder,
                                                          int sceneIndex, int layerIndex,
                                                          int frameIndex,
                                                          const QString &commandId,
                                                          const QString &dependencyCommandId)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("project_request");

    const QString resolvedCommandId = resolveCommandId(commandId);
    root.setAttribute("command_id", resolvedCommandId);
    if (!dependencyCommandId.trimmed().isEmpty())
        root.setAttribute("depends_on", dependencyCommandId.trimmed());

    QDomElement scene = doc.createElement("scene");
    scene.setAttribute("index", sceneIndex);

    QDomElement layer = doc.createElement("layer");
    layer.setAttribute("index", layerIndex);

    QDomElement frame = doc.createElement("frame");
    frame.setAttribute("index", frameIndex);

    QDomElement library = doc.createElement("library");

    QDomElement symbol = doc.createElement("symbol");
    symbol.setAttribute("folder", folder);
    symbol.setAttribute("type", type);
    symbol.setAttribute("spaceMode", spaceMode);

    QDomElement action = doc.createElement("action");
    action.setAttribute("id", actionId);
    action.setAttribute("arg", arg.toString());
    action.setAttribute("part", TupProjectRequest::Library);

    appendData(doc, action, data);
    root.appendChild(action);
    library.appendChild(symbol);
    root.appendChild(library);
    root.appendChild(scene);
    scene.appendChild(layer);
    layer.appendChild(frame);
    doc.appendChild(root);

    return buildRequest(doc, actionId, resolvedCommandId, dependencyCommandId);
}

void TupRequestBuilder::appendData(QDomDocument &doc, QDomElement &element,
                                   const QByteArray &data)
{
    if (!data.isNull() && !data.isEmpty()) {
        QDomElement dataElement = doc.createElement("data");
        QDomCDATASection cdata = doc.createCDATASection(QString(data.toBase64()));
        dataElement.appendChild(cdata);
        element.appendChild(dataElement);
    }
}

TupProjectRequest TupRequestBuilder::fromResponse(TupProjectResponse *response,
                                                         bool preserveCommandId)
{
    if (!response)
        return TupProjectRequest();

    const QString commandId = preserveCommandId
        ? response->getCommandId()
        : QString();

    switch (response->getPart()) {
        case TupProjectRequest::Item: {
            TupItemResponse *itemResponse = static_cast<TupItemResponse *>(response);
            return createItemRequest(itemResponse->getSceneIndex(),
                                     itemResponse->getLayerIndex(),
                                     itemResponse->getFrameIndex(),
                                     itemResponse->getItemIndex(),
                                     itemResponse->position(),
                                     itemResponse->spaceMode(),
                                     itemResponse->getItemType(),
                                     response->getAction(),
                                     response->getArg().toString(),
                                     response->getData(),
                                     commandId,
                                     QString(),
                                     itemResponse->getObjectId());
        }
        case TupProjectRequest::Frame: {
            TupFrameResponse *frameResponse = static_cast<TupFrameResponse *>(response);
            return createFrameRequest(frameResponse->getSceneIndex(),
                                      frameResponse->getLayerIndex(),
                                      frameResponse->getFrameIndex(),
                                      response->getAction(),
                                      response->getArg().toString(),
                                      response->getData(),
                                      commandId);
        }
        case TupProjectRequest::Layer: {
            TupLayerResponse *layerResponse = static_cast<TupLayerResponse *>(response);
            return createLayerRequest(layerResponse->getSceneIndex(),
                                      layerResponse->getLayerIndex(),
                                      response->getAction(),
                                      response->getArg().toString(),
                                      response->getData(),
                                      commandId);
        }
        case TupProjectRequest::Scene: {
            TupSceneResponse *sceneResponse = static_cast<TupSceneResponse *>(response);
            return createSceneRequest(sceneResponse->getSceneIndex(),
                                      response->getAction(),
                                      response->getArg().toString(),
                                      response->getData(),
                                      commandId);
        }
        case TupProjectRequest::Library: {
            TupLibraryResponse *libraryResponse = static_cast<TupLibraryResponse *>(response);
            return createLibraryRequest(response->getAction(),
                                        response->getArg().toString(),
                                        libraryResponse->symbolType(),
                                        libraryResponse->getSpaceMode(),
                                        response->getData(),
                                        libraryResponse->getParent(),
                                        libraryResponse->getSceneIndex(),
                                        libraryResponse->getLayerIndex(),
                                        libraryResponse->getFrameIndex(),
                                        commandId);
        }
        default:
#ifdef TUP_DEBUG
            qDebug() << "[TupRequestBuilder::fromResponse()] - Error: Unknown response part ->"
                     << response->getPart();
#endif
            break;
    }

    return TupProjectRequest();
}
