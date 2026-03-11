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
#include "tupprojectresponse.h"
#include "tupsvg2qt.h"

#include <QVariant>

static int undoSequence = 0;
static int redoSequence = 0;

TupProjectCommand::TupProjectCommand(TupCommandExecutor *exec, const TupProjectRequest *request) : QUndoCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand()]";
    #endif

    TupRequestParser parser;
    if (!parser.parse(request->getXml())) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupProjectCommand::TupProjectCommand()] - Fatal Error: request xml can't be parsed!";
        #endif
        return;
    }

    executor = exec;
    executed = false;
    response = parser.getResponse();
    response->setExternal(request->isRequestExternal());

    if (!response) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupProjectCommand::TupProjectCommand()] - Unparsed response!";
        #endif
    }

    initText();
}

TupProjectCommand::TupProjectCommand(TupCommandExecutor *exec, TupProjectResponse *res) : QUndoCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand()]";
    #endif
	
    executor = exec;
    response = res;
    executed = false;

    initText();
}

void TupProjectCommand::initText()
{
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

QString TupProjectCommand::actionString(int action)
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
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::redo()] - Seq:" << redoSequence
                 << "action:" << response->getAction()
                 << "part:" << response->getPart();
    #endif
	
    if (executed) {
        response->setMode(TupProjectResponse::Redo);
    } else {
        response->setMode(TupProjectResponse::Do);
        executed = true;
    }

    switch (response->getPart()) {
            case TupProjectRequest::Project:
            {
                #ifdef TUP_DEBUG
                     qWarning() << "[TupProjectCommand::redo()] - Error: Project response isn't implemented";
                #endif
            }
            break;
            case TupProjectRequest::Frame:
            {
                 frameCommand();
            }
            break;
            case TupProjectRequest::Layer:
            {
                 layerCommand();
            }
            break;
            case TupProjectRequest::Scene:
            {
                 sceneCommand();
            }
            break;
            case TupProjectRequest::Item:
            {
                 itemCommand();
            }
            break;
            case TupProjectRequest::Library:
            {
                 libraryCommand();
            }
            break;
            default:
            {
                #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::redo()] - Error: Unknown project response";
                #endif
            }
            break;
    }
}

void TupProjectCommand::undo()
{
    undoSequence++;
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::undo()] - Seq:" << undoSequence 
                 << "action:" << response->getAction()
                 << "part:" << response->getPart();
    #endif

    response->setMode(TupProjectResponse::Undo);
    switch (response->getPart()) {
            case TupProjectRequest::Project:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::undo()] - Error: Project response isn't implemented";
                 #endif
            }
            break;
            case TupProjectRequest::Frame:
            {
                 frameCommand();
            }
            break;
            case TupProjectRequest::Layer:
            {
                 layerCommand();
            }
            break;
            case TupProjectRequest::Scene:
            {
                 sceneCommand();
            }
            break;
            case TupProjectRequest::Item:
            {
                 itemCommand();
            }
            break;
            case TupProjectRequest::Library:
            {
                 libraryCommand();
            }
            break;
            default:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::undo()] - Error: Unknown project response";
                 #endif
            }
            break;
    }
}

void TupProjectCommand::frameCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::frameCommand()]";
    #endif

    TupFrameResponse *frameResponse = static_cast<TupFrameResponse *>(response);

    switch (frameResponse->getAction()) {
            case TupProjectRequest::Add:
            {
                 executor->createFrame(frameResponse);
            }
            break;
            case TupProjectRequest::Remove:
            {
                 executor->removeFrame(frameResponse);
            }
            break;
            case TupProjectRequest::RemoveSelection:
            {
                 executor->removeFrameSelection(frameResponse);
            }
            break;
            case TupProjectRequest::Reset:
            {
                 executor->resetFrame(frameResponse);
            }
            break;
            case TupProjectRequest::Exchange:
            {
                 executor->exchangeFrame(frameResponse);
            }
            break;
            case TupProjectRequest::Move:
            {
                 executor->moveFrame(frameResponse);
            }
            break;
            case TupProjectRequest::ReverseSelection:
            {
                 executor->reverseFrameSelection(frameResponse);
            }
            break;
            /*
            case TupProjectRequest::Lock:
            {
                 executor->lockFrame(res);
            }
            break;
            */
            case TupProjectRequest::Rename:
            {
                 executor->renameFrame(frameResponse);
            }
            break;
            case TupProjectRequest::Select:
            {
                 executor->selectFrame(frameResponse);
            }
            break;
            case TupProjectRequest::View:
            {
                 executor->setFrameVisibility(frameResponse);
            }
            break;
            case TupProjectRequest::Extend:
            {
                 executor->extendFrame(frameResponse);
            }
            break;
            /*
            case TupProjectRequest::Paste:
            {
                 executor->pasteFrame(res);
            }
            break;
            */
            case TupProjectRequest::CopySelection:
            {
                 executor->copyFrameSelection(frameResponse);
            }
            break;
            case TupProjectRequest::PasteSelection:
            {
                 executor->pasteFrameSelection(frameResponse);
            }
            break;
            default: 
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::frameCommand()] - Fatal Error: Unknown project request";
                 #endif
            }
            break;
    }
}

void TupProjectCommand::layerCommand()
{
    TupLayerResponse *res = static_cast<TupLayerResponse *>(response);

    switch (res->getAction()) {
            case TupProjectRequest::Add:
            {
                 executor->createLayer(res);
            }
            break;
            case TupProjectRequest::Duplicate:
            {
                 executor->duplicateLayer(res);
            }
            break;
            case TupProjectRequest::AddLipSync:
            {
                 executor->addLipSync(res);
            }
            break;
            case TupProjectRequest::UpdateLipSync:
            {
                 executor->updateLipSync(res);
            }
            break;
            case TupProjectRequest::Remove:
            {
                 executor->removeLayer(res);
            }
            break;
            case TupProjectRequest::RemoveLipSync:
            {
                 executor->removeLipSync(res);
            }
            break;
            case TupProjectRequest::Move:
            {
                 executor->moveLayer(res);
            }
            break;
            case TupProjectRequest::Lock:
            {
                 executor->lockLayer(res);
            }
            break;
            case TupProjectRequest::Rename:
            {
                 executor->renameLayer(res);
            }
            break;
            case TupProjectRequest::Select:
            {
                 executor->selectLayer(res);
            }
            break;
            case TupProjectRequest::View:
            {
                 executor->setLayerVisibility(res);
            }
            break;
            case TupProjectRequest::UpdateOpacity:
            {
                 executor->setLayerOpacity(res);
            }
            break;
            default: 
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::layerCommand()] - Error: Unknown project response";
                 #endif
            }
            break;
    }
}

void TupProjectCommand::sceneCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::sceneCommand()]";
    #endif

    TupSceneResponse *sceneResponse = static_cast<TupSceneResponse *>(response);

    switch (sceneResponse->getAction()) {
	    // SQA: Check if this case is valid 
        case TupProjectRequest::GetInfo:
        {
             executor->getScenes(sceneResponse);
        }
        break;
        case TupProjectRequest::Add:
        {
             executor->createScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Duplicate:
        {
             executor->duplicateScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Remove:
        {
             executor->removeScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Reset:
        {
             executor->resetScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Move:
        {
             executor->moveScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Lock:
        {
             executor->lockScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Rename:
        {
             executor->renameScene(sceneResponse);
        }
        break;
        case TupProjectRequest::Select:
        {
             executor->selectScene(sceneResponse);
        }
        break;
        case TupProjectRequest::View:
        {
             executor->setSceneVisibility(sceneResponse);
        }
        break;
        case TupProjectRequest::BgColor:
        {
             executor->setBgColor(sceneResponse);
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
}

void TupProjectCommand::itemCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::itemCommand()] - action:" << response->originalAction() 
                 << "mode:" << response->getMode();
    #endif

    TupItemResponse *res = static_cast<TupItemResponse *>(response);
    
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::itemCommand()] - itemIndex:" << res->getItemIndex();
    #endif

    // Use originalAction() to route to the correct executor method
    // The executor methods handle Undo/Redo mode internally
    switch (res->originalAction()) {
            case TupProjectRequest::Add:
            {
                 executor->createItem(res);
            }
            break;
            case TupProjectRequest::Remove:
            {
                 executor->removeItem(res);
            }
            break;
            case TupProjectRequest::Move:
            {
                 executor->moveItem(res);
            }
            break;
            case TupProjectRequest::Lock:
            {
            }
            break;
            case TupProjectRequest::Rename:
            {
            }
            break;
            case TupProjectRequest::Convert:
            {
                 executor->convertItem(res);
            }
            break;
            case TupProjectRequest::EditNodes:
            {
                 executor->setPathItem(res);
            }
            break;
            case TupProjectRequest::Pen:
            {
                 executor->setPen(res);
            }
            break;
            case TupProjectRequest::Brush:
            {
                 executor->setBrush(res);
            }
            break;
            case TupProjectRequest::TextColor:
            {
                 executor->setTextColor(res);
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
                 executor->transformItem(res);
            }
            break;
            case TupProjectRequest::Group:
            {
                 executor->groupItems(res);
            }
            break;
            case TupProjectRequest::Ungroup:
            {
                 executor->ungroupItems(res);
            }
            break;
            case TupProjectRequest::SetTween:
            {
                 executor->setTween(res);
            }
            break;
            case TupProjectRequest::UpdateTweenPath:
            {
                 executor->updateTweenPath(res);
            }
            break;
            case TupProjectRequest::AddRasterItem:
            {
                 executor->createRasterPath(res);
            }
            break;
            case TupProjectRequest::ClearRasterCanvas:
            {
                 executor->clearRasterCanvas(res);
            }
            break;
            default:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::itemCommand()] - Error: Unknown project response";
                 #endif
            }
            break;
    }
}

void TupProjectCommand::libraryCommand()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupProjectCommand::libraryCommand()]";
    #endif
    
    TupLibraryResponse *res = static_cast<TupLibraryResponse *>(response);

    switch (res->getAction()) {
            case TupProjectRequest::Add:
            {
                 executor->createSymbol(res);
            }
            break;

            case TupProjectRequest::Remove:
            {
                 executor->removeSymbol(res);
            }
            break;

            case TupProjectRequest::InsertSymbolIntoFrame:
            {
                 executor->insertSymbolIntoFrame(res);
            }
            break;

            case TupProjectRequest::RemoveSymbolFromFrame:
            {
                 executor->removeSymbolFromFrame(res);
            }
            break;

            default:
            {
                 #ifdef TUP_DEBUG
                     qDebug() << "[TupProjectCommand::libraryCommand()] - Error: Unknown project response";
                 #endif
            }
            break;
    }
}

void TupProjectCommand::paintAreaCommand()
{
    // SQA: FIX ME in tupprojectcommand.cpp
	
    /*
     if (redo)
         executor->reemitEvent(response);
     else
         executor->reemitEvent(response);
    */
}
