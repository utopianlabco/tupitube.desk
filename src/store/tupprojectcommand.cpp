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

#include "tupprojectcommand.h"
#include "tupcommandexecutor.h"
#include "tupprojectrequest.h"
#include "tuppaintareaevent.h"
#include "tuprequestparser.h"
#include "tuprequestbuilder.h"
#include "tupprojectresponse.h"
#include "tupsvg2qt.h"

#include <QVariant>
#include <QDomDocument>
#include <QDebug>

static int undoSequence = 0;
static int redoSequence = 0;

TupProjectCommand::TupProjectCommand(TupCommandExecutor *exec, const TupProjectRequest *request)
    : QUndoCommand(),
      executor(exec),
      response(nullptr),
      executed(false),
      executionSucceeded(false)
{
#ifdef TUP_DEBUG
    qDebug() << "[TupProjectCommand()]";
#endif

    if (!executor) {
        executionErrorCode = QStringLiteral("missing_executor");
        return;
    }

    if (!request) {
        executionErrorCode = QStringLiteral("missing_request");
        return;
    }

    TupRequestParser parser;
    if (!parser.parse(request->getXml())) {
#ifdef TUP_DEBUG
        qWarning() << "[TupProjectCommand::TupProjectCommand()] - Request XML cannot be parsed";
#endif
        executionErrorCode = QStringLiteral("invalid_request_xml");
        return;
    }

    response = parser.getResponse();
    if (!response) {
#ifdef TUP_DEBUG
        qWarning() << "[TupProjectCommand::TupProjectCommand()] - Parser returned no response";
#endif
        executionErrorCode = QStringLiteral("missing_response");
        return;
    }

    response->setExternal(request->isRequestExternal());
    initText();
}

TupProjectCommand::TupProjectCommand(TupCommandExecutor *exec, TupProjectResponse *res)
    : QUndoCommand(),
      executor(exec),
      response(res),
      executed(false),
      executionSucceeded(false)
{
#ifdef TUP_DEBUG
    qDebug() << "[TupProjectCommand()]";
#endif

    if (!executor) {
        executionErrorCode = QStringLiteral("missing_executor");
        return;
    }

    if (!response) {
        executionErrorCode = QStringLiteral("missing_response");
        return;
    }

    initText();
}

void TupProjectCommand::initText()
{
    if (!response)
        return;

    switch (response->getPart()) {
        case TupProjectRequest::Frame:
        {
            setText(actionString(response->getAction()) + " frame");
        }
        break;
        case TupProjectRequest::Layer:
        {
            setText(actionString(response->getAction()) + " layer");
        }
        break;
        case TupProjectRequest::Scene:
        {
            setText(actionString(response->getAction()) + " scene");
        }
        break;
        case TupProjectRequest::Item:
        {
            setText(actionString(response->getAction()) + " item");
        }
        break;
        case TupProjectRequest::Library:
        {
            setText(actionString(response->getAction()) + " symbol");
        }
        break;
        default:
        {				  
            #ifdef TUP_DEBUG
                qDebug() << "[TupProjectCommand::initText()] - Error: can't handle ID -> " << response->getPart();
            #endif
        }
        break;
    }
}

QString TupProjectCommand::actionString(int action) const
{
    switch(action)
    {
        case TupProjectRequest::Add:
        {
            return "add";
        }
        case TupProjectRequest::Duplicate:
        {
            return "duplicate";
        }
        case TupProjectRequest::Remove:
        {
            return "remove";
        }
        case TupProjectRequest::Move:
        {
            return "move";
        }
        case TupProjectRequest::ReverseSelection:
        {
            return "reverse";
        }
        case TupProjectRequest::Lock:
        {
            return "lock";
        }
        case TupProjectRequest::Rename:
        {
            return "rename";
        }
        case TupProjectRequest::Select:
        {
            return "select";
        }
        case TupProjectRequest::EditNodes:
        {
            return "edit node";
        }
        case TupProjectRequest::Pen:
        {
            return "pen";
        }
        case TupProjectRequest::Brush:
        {
            return "brush";
        }
        case TupProjectRequest::View:
        {
            return "view";
        }
        case TupProjectRequest::Transform:
        {
            return "transform";
        }
        case TupProjectRequest::Convert:
        {
            return "convert";
        }
    }
    
    return QString("Unknown");
}

TupProjectCommand::~TupProjectCommand()
{
    delete response;
}

void TupProjectCommand::redo()
{
    redoSequence++;
    resetExecutionResult();
#ifdef TUP_DEBUG
    qDebug() << "[TupProjectCommand::redo()] - Seq:" << redoSequence
             << "command:" << commandId()
             << "action:" << (response ? response->getAction() : -1)
             << "part:" << (response ? response->getPart() : -1);
#endif

    if (!response) {
        fail(QStringLiteral("missing_response"));
        return;
    }
    if (!executor) {
        fail(QStringLiteral("missing_executor"));
        return;
    }

    if (executed)
        response->setMode(TupProjectResponse::Redo);
    else {
        response->setMode(TupProjectResponse::Do);
        executed = true;
    }

    executionSucceeded = executeResponse();
    if (!executionSucceeded && executionErrorCode.isEmpty())
        executionErrorCode = QStringLiteral("execution_failed");
}

void TupProjectCommand::undo()
{
    undoSequence++;
    resetExecutionResult();
#ifdef TUP_DEBUG
    qDebug() << "[TupProjectCommand::undo()] - Seq:" << undoSequence
             << "command:" << commandId()
             << "action:" << (response ? response->getAction() : -1)
             << "part:" << (response ? response->getPart() : -1);
#endif

    if (!response) {
        fail(QStringLiteral("missing_response"));
        return;
    }
    if (!executor) {
        fail(QStringLiteral("missing_executor"));
        return;
    }

    response->setMode(TupProjectResponse::Undo);
    executionSucceeded = executeResponse();
    if (!executionSucceeded && executionErrorCode.isEmpty())
        executionErrorCode = QStringLiteral("execution_failed");
}

void TupProjectCommand::resetExecutionResult()
{
    executionSucceeded = false;
    executionErrorCode.clear();
}

bool TupProjectCommand::fail(const QString &code)
{
    executionSucceeded = false;
    executionErrorCode = code;
    return false;
}

bool TupProjectCommand::succeeded() const
{
    return executionSucceeded;
}

QString TupProjectCommand::errorCode() const
{
    return executionErrorCode;
}

QString TupProjectCommand::commandId() const
{
    return response ? response->getCommandId() : QString();
}

bool TupProjectCommand::hasAuthoritativeEventPayload() const
{
    return !authoritativeEventPayload().isEmpty();
}

QString TupProjectCommand::authoritativeEventPayload() const
{
    if (!response || !executionSucceeded)
        return QString();

    if (response->getPart() != TupProjectRequest::Item)
        return QString();

    TupItemResponse *itemResponse = static_cast<TupItemResponse *>(response);

    if (response->originalAction() == TupProjectRequest::Add) {
        if (itemResponse->getItemType() == TupLibraryObject::Svg
                || itemResponse->getObjectId().trimmed().isEmpty()) {
            return QString();
        }

        const TupProjectRequest request = TupRequestBuilder::createItemRequest(
            itemResponse->getSceneIndex(),
            itemResponse->getLayerIndex(),
            itemResponse->getFrameIndex(),
            itemResponse->getItemIndex(),
            itemResponse->position(),
            itemResponse->spaceMode(),
            itemResponse->getItemType(),
            TupProjectRequest::Add,
            itemResponse->getArg().toString(),
            itemResponse->getData(),
            response->getCommandId(),
            QString(),
            itemResponse->getObjectId());

        return request.getXml();
    }

    if (response->originalAction() != TupProjectRequest::Convert)
        return QString();

    if (!itemResponse->hasConversionSnapshots())
        return QString();

    const QString targetSnapshot = itemResponse->conversionTargetSnapshot();
    if (targetSnapshot.isEmpty())
        return QString();

    const TupProjectRequest request = TupRequestBuilder::createItemRequest(
        itemResponse->getSceneIndex(),
        itemResponse->getLayerIndex(),
        itemResponse->getFrameIndex(),
        itemResponse->getItemIndex(),
        itemResponse->position(),
        itemResponse->spaceMode(),
        itemResponse->getItemType(),
        TupProjectRequest::Convert,
        QStringLiteral("path"),
        targetSnapshot.toUtf8(),
        response->getCommandId(),
        QString(),
        itemResponse->getObjectId());

    QDomDocument document;
    if (!document.setContent(request.getXml()))
        return QString();

    QDomElement root = document.documentElement();
    QDomElement sourceElement = document.createElement(
        QStringLiteral("conversion_source"));
    sourceElement.setAttribute(QStringLiteral("encoding"), QStringLiteral("base64"));
    sourceElement.appendChild(document.createTextNode(
        QString::fromLatin1(
            itemResponse->conversionSourceSnapshot().toUtf8().toBase64())));
    root.appendChild(sourceElement);

    return document.toString();
}

QString TupProjectCommand::eventType() const
{
    if (!response)
        return QStringLiteral("project.command-committed");

    switch (response->getPart()) {
        case TupProjectRequest::Frame:
            switch (response->getAction()) {
                case TupProjectRequest::Add: return QStringLiteral("frame.created");
                case TupProjectRequest::Remove: return QStringLiteral("frame.removed");
                case TupProjectRequest::RemoveSelection: return QStringLiteral("frame.selection-removed");
                case TupProjectRequest::Reset: return QStringLiteral("frame.reset");
                case TupProjectRequest::Exchange: return QStringLiteral("frame.exchanged");
                case TupProjectRequest::Move: return QStringLiteral("frame.moved");
                case TupProjectRequest::ReverseSelection: return QStringLiteral("frame.selection-reversed");
                case TupProjectRequest::Rename: return QStringLiteral("frame.renamed");
                case TupProjectRequest::Select: return QStringLiteral("frame.selected");
                case TupProjectRequest::View: return QStringLiteral("frame.visibility-changed");
                case TupProjectRequest::Extend: return QStringLiteral("frame.extended");
                case TupProjectRequest::CopySelection: return QStringLiteral("frame.selection-copied");
                case TupProjectRequest::PasteSelection: return QStringLiteral("frame.selection-pasted");
                default: break;
            }
            break;

        case TupProjectRequest::Layer:
            switch (response->getAction()) {
                case TupProjectRequest::Add: return QStringLiteral("layer.created");
                case TupProjectRequest::Duplicate: return QStringLiteral("layer.duplicated");
                case TupProjectRequest::AddLipSync: return QStringLiteral("layer.lipsync-added");
                case TupProjectRequest::UpdateLipSync: return QStringLiteral("layer.lipsync-updated");
                case TupProjectRequest::Remove: return QStringLiteral("layer.removed");
                case TupProjectRequest::RemoveLipSync: return QStringLiteral("layer.lipsync-removed");
                case TupProjectRequest::Move: return QStringLiteral("layer.moved");
                case TupProjectRequest::Lock: return QStringLiteral("layer.locked");
                case TupProjectRequest::Rename: return QStringLiteral("layer.renamed");
                case TupProjectRequest::Select: return QStringLiteral("layer.selected");
                case TupProjectRequest::View: return QStringLiteral("layer.visibility-changed");
                case TupProjectRequest::UpdateOpacity: return QStringLiteral("layer.opacity-updated");
                default: break;
            }
            break;

        case TupProjectRequest::Scene:
            switch (response->getAction()) {
                case TupProjectRequest::GetInfo: return QStringLiteral("scene.info-read");
                case TupProjectRequest::Add: return QStringLiteral("scene.created");
                case TupProjectRequest::Duplicate: return QStringLiteral("scene.duplicated");
                case TupProjectRequest::Remove: return QStringLiteral("scene.removed");
                case TupProjectRequest::Reset: return QStringLiteral("scene.reset");
                case TupProjectRequest::Move: return QStringLiteral("scene.moved");
                case TupProjectRequest::Lock: return QStringLiteral("scene.locked");
                case TupProjectRequest::Rename: return QStringLiteral("scene.renamed");
                case TupProjectRequest::Select: return QStringLiteral("scene.selected");
                case TupProjectRequest::View: return QStringLiteral("scene.visibility-changed");
                case TupProjectRequest::BgColor: return QStringLiteral("scene.background-color-updated");
                case TupProjectRequest::SetFps: return QStringLiteral("scene.fps-updated");
                default: break;
            }
            break;

        case TupProjectRequest::Item:
            switch (response->originalAction()) {
                case TupProjectRequest::Add: return QStringLiteral("item.created");
                case TupProjectRequest::Remove: return QStringLiteral("item.removed");
                case TupProjectRequest::Move: return QStringLiteral("item.moved");
                case TupProjectRequest::Convert: return QStringLiteral("item.converted");
                case TupProjectRequest::EditNodes: return QStringLiteral("item.nodes-edited");
                case TupProjectRequest::Pen: return QStringLiteral("item.pen-updated");
                case TupProjectRequest::Brush: return QStringLiteral("item.brush-updated");
                case TupProjectRequest::TextColor: return QStringLiteral("item.text-color-updated");
                case TupProjectRequest::Transform: return QStringLiteral("item.transformed");
                case TupProjectRequest::Group: return QStringLiteral("item.grouped");
                case TupProjectRequest::Ungroup: return QStringLiteral("item.ungrouped");
                case TupProjectRequest::SetTween: return QStringLiteral("item.tween-updated");
                case TupProjectRequest::UpdateTweenPath: return QStringLiteral("item.tween-path-updated");
                case TupProjectRequest::AddRasterItem: return QStringLiteral("item.raster-item-added");
                case TupProjectRequest::ClearRasterCanvas: return QStringLiteral("item.raster-canvas-cleared");
                default: break;
            }
            break;

        case TupProjectRequest::Library:
            switch (response->getAction()) {
                case TupProjectRequest::Add: return QStringLiteral("library.object-imported");
                case TupProjectRequest::Remove: return QStringLiteral("library.object-removed");
                case TupProjectRequest::InsertSymbolIntoFrame: return QStringLiteral("frame.symbol-inserted");
                case TupProjectRequest::RemoveSymbolFromFrame: return QStringLiteral("frame.symbol-removed");
                default: break;
            }
            break;

        default:
            break;
    }

    return QStringLiteral("project.command-committed");
}

bool TupProjectCommand::executeResponse()
{
    switch (response->getPart()) {
        case TupProjectRequest::Project:
            return fail(QStringLiteral("project_command_not_implemented"));
        case TupProjectRequest::Frame:
            return frameCommand();
        case TupProjectRequest::Layer:
            return layerCommand();
        case TupProjectRequest::Scene:
            return sceneCommand();
        case TupProjectRequest::Item:
            return itemCommand();
        case TupProjectRequest::Library:
            return libraryCommand();
        default:
            return fail(QStringLiteral("unknown_response_part"));
    }
}

bool TupProjectCommand::frameCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::frameCommand()]";
    #endif

    TupFrameResponse *frameResponse = static_cast<TupFrameResponse *>(response);

    switch (frameResponse->getAction()) {
            case TupProjectRequest::Add:
            {
                 return executeOperation([&]() { return executor->createFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::Remove:
            {
                 return executeOperation([&]() { return executor->removeFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::RemoveSelection:
            {
                 return executeOperation([&]() { return executor->removeFrameSelection(frameResponse); });
            }
            break;
            case TupProjectRequest::Reset:
            {
                 return executeOperation([&]() { return executor->resetFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::Exchange:
            {
                 return executeOperation([&]() { return executor->exchangeFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::Move:
            {
                 return executeOperation([&]() { return executor->moveFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::ReverseSelection:
            {
                 return executeOperation([&]() { return executor->reverseFrameSelection(frameResponse); });
            }
            break;
            /*
            case TupProjectRequest::Lock:
            {
                 return executeOperation([&]() { return executor->lockFrame(res); });
            }
            break;
            */
            case TupProjectRequest::Rename:
            {
                 return executeOperation([&]() { return executor->renameFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::Select:
            {
                 return executeOperation([&]() { return executor->selectFrame(frameResponse); });
            }
            break;
            case TupProjectRequest::View:
            {
                 return executeOperation([&]() { return executor->setFrameVisibility(frameResponse); });
            }
            break;
            case TupProjectRequest::Extend:
            {
                 return executeOperation([&]() { return executor->extendFrame(frameResponse); });
            }
            break;
            /*
            case TupProjectRequest::Paste:
            {
                 return executeOperation([&]() { return executor->pasteFrame(res); });
            }
            break;
            */
            case TupProjectRequest::CopySelection:
            {
                 return executeOperation([&]() { return executor->copyFrameSelection(frameResponse); });
            }
            break;
            case TupProjectRequest::PasteSelection:
            {
                 return executeOperation([&]() { return executor->pasteFrameSelection(frameResponse); });
            }
            break;
            default: 
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::frameCommand()] - Fatal Error: Unknown project request";
                 #endif
            }
            return fail(QStringLiteral("unsupported_frame_action"));
    }


    return fail(QStringLiteral("unsupported_frame_action"));
}

bool TupProjectCommand::layerCommand()
{
    TupLayerResponse *res = static_cast<TupLayerResponse *>(response);

    switch (res->getAction()) {
            case TupProjectRequest::Add:
            {
                 return executeOperation([&]() { return executor->createLayer(res); });
            }
            break;
            case TupProjectRequest::Duplicate:
            {
                 return executeOperation([&]() { return executor->duplicateLayer(res); });
            }
            break;
            case TupProjectRequest::AddLipSync:
            {
                 return executeOperation([&]() { return executor->addLipSync(res); });
            }
            break;
            case TupProjectRequest::UpdateLipSync:
            {
                 return executeOperation([&]() { return executor->updateLipSync(res); });
            }
            break;
            case TupProjectRequest::Remove:
            {
                 return executeOperation([&]() { return executor->removeLayer(res); });
            }
            break;
            case TupProjectRequest::RemoveLipSync:
            {
                 return executeOperation([&]() { return executor->removeLipSync(res); });
            }
            break;
            case TupProjectRequest::Move:
            {
                 return executeOperation([&]() { return executor->moveLayer(res); });
            }
            break;
            case TupProjectRequest::Lock:
            {
                 return executeOperation([&]() { return executor->lockLayer(res); });
            }
            break;
            case TupProjectRequest::Rename:
            {
                 return executeOperation([&]() { return executor->renameLayer(res); });
            }
            break;
            case TupProjectRequest::Select:
            {
                 return executeOperation([&]() { return executor->selectLayer(res); });
            }
            break;
            case TupProjectRequest::View:
            {
                 return executeOperation([&]() { return executor->setLayerVisibility(res); });
            }
            break;
            case TupProjectRequest::UpdateOpacity:
            {
                 return executeOperation([&]() { return executor->setLayerOpacity(res); });
            }
            break;
            default: 
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::layerCommand()] - Error: Unknown project response";
                 #endif
            }
            return fail(QStringLiteral("unsupported_layer_action"));
    }


    return fail(QStringLiteral("unsupported_layer_action"));
}

bool TupProjectCommand::sceneCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::sceneCommand()]";
    #endif

    TupSceneResponse *sceneResponse = static_cast<TupSceneResponse *>(response);

    switch (sceneResponse->getAction()) {
	    // SQA: Check if this case is valid 
        case TupProjectRequest::GetInfo:
        {
             return executeOperation([&]() { return executor->getScenes(sceneResponse); });
        }
        break;
        case TupProjectRequest::Add:
        {
             return executeOperation([&]() { return executor->createScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Duplicate:
        {
             return executeOperation([&]() { return executor->duplicateScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Remove:
        {
             return executeOperation([&]() { return executor->removeScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Reset:
        {
             return executeOperation([&]() { return executor->resetScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Move:
        {
             return executeOperation([&]() { return executor->moveScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Lock:
        {
             return executeOperation([&]() { return executor->lockScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Rename:
        {
             return executeOperation([&]() { return executor->renameScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::Select:
        {
             return executeOperation([&]() { return executor->selectScene(sceneResponse); });
        }
        break;
        case TupProjectRequest::View:
        {
             return executeOperation([&]() { return executor->setSceneVisibility(sceneResponse); });
        }
        break;
        case TupProjectRequest::BgColor:
        {
             return executeOperation([&]() { return executor->setBgColor(sceneResponse); });
        }
        break;
        case TupProjectRequest::SetFps:
        {
             return executeOperation([&]() { return executor->setFps(sceneResponse); });
        }
        break;

        default:
        {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupProjectCommand::sceneCommand()] - Error: Unknown project response";
             #endif
        }
        break;
    }


    return fail(QStringLiteral("unsupported_scene_action"));
}

bool TupProjectCommand::itemCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::itemCommand()] - action:" << response->originalAction() 
                 << "mode:" << response->getMode();
    #endif

    TupItemResponse *res = static_cast<TupItemResponse *>(response);
    
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::itemCommand()] - itemIndex:" << res->getItemIndex()
                 << "objectId:" << res->getObjectId();
    #endif

    // Use originalAction() to route to the correct executor method
    // The executor methods handle Undo/Redo mode internally
    switch (res->originalAction()) {
            case TupProjectRequest::Add:
            {
                 return executeOperation([&]() { return executor->createItem(res); });
            }
            break;
            case TupProjectRequest::Remove:
            {
                 return executeOperation([&]() { return executor->removeItem(res); });
            }
            break;
            case TupProjectRequest::Move:
            {
                 return executeOperation([&]() { return executor->moveItem(res); });
            }
            break;
            case TupProjectRequest::Lock:
                return fail(QStringLiteral("item_lock_not_implemented"));
            case TupProjectRequest::Rename:
                return fail(QStringLiteral("item_rename_not_implemented"));
            case TupProjectRequest::Convert:
            {
                 return executeOperation([&]() { return executor->convertItem(res); });
            }
            break;
            case TupProjectRequest::EditNodes:
            {
                 return executeOperation([&]() { return executor->setPathItem(res); });
            }
            break;
            case TupProjectRequest::Pen:
            {
                 return executeOperation([&]() { return executor->setPen(res); });
            }
            break;
            case TupProjectRequest::Brush:
            {
                 return executeOperation([&]() { return executor->setBrush(res); });
            }
            break;
            case TupProjectRequest::TextColor:
            {
                 return executeOperation([&]() { return executor->setTextColor(res); });
            }
            break;
            /*
            case TupProjectRequest::Select:
            {
            }
            break;
            case TupProjectRequest::View:
            {
            }
            break;
            */
            case TupProjectRequest::Transform:
            {
                 return executeOperation([&]() { return executor->transformItem(res); });
            }
            break;
            case TupProjectRequest::Group:
            {
                 return executeOperation([&]() { return executor->groupItems(res); });
            }
            break;
            case TupProjectRequest::Ungroup:
            {
                 return executeOperation([&]() { return executor->ungroupItems(res); });
            }
            break;
            case TupProjectRequest::SetTween:
            {
                 return executeOperation([&]() { return executor->setTween(res); });
            }
            break;
            case TupProjectRequest::UpdateTweenPath:
            {
                 return executeOperation([&]() { return executor->updateTweenPath(res); });
            }
            break;
            case TupProjectRequest::AddRasterItem:
            {
                 return executeOperation([&]() { return executor->createRasterPath(res); });
            }
            break;
            case TupProjectRequest::ClearRasterCanvas:
            {
                 return executeOperation([&]() { return executor->clearRasterCanvas(res); });
            }
            break;
            default:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::itemCommand()] - Error: Unknown project response";
                 #endif
            }
            return fail(QStringLiteral("unsupported_item_action"));
    }


    return fail(QStringLiteral("unsupported_item_action"));
}

bool TupProjectCommand::libraryCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::libraryCommand()]";
    #endif
    
    TupLibraryResponse *res = static_cast<TupLibraryResponse *>(response);

    switch (res->getAction()) {
            case TupProjectRequest::Add:
            {
                 return executeOperation([&]() { return executor->createSymbol(res); });
            }
            break;

            case TupProjectRequest::Remove:
            {
                 return executeOperation([&]() { return executor->removeSymbol(res); });
            }
            break;

            case TupProjectRequest::InsertSymbolIntoFrame:
            {
                 return executeOperation([&]() { return executor->insertSymbolIntoFrame(res); });
            }
            break;

            case TupProjectRequest::RemoveSymbolFromFrame:
            {
                 return executeOperation([&]() { return executor->removeSymbolFromFrame(res); });
            }
            break;

            default:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::libraryCommand()] - Error: Unknown project response";
                 #endif
            }
            return fail(QStringLiteral("unsupported_library_action"));
    }


    return fail(QStringLiteral("unsupported_library_action"));
}

bool TupProjectCommand::paintAreaCommand()
{
    return fail(QStringLiteral("paint_area_command_not_implemented"));
}
