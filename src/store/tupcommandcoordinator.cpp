/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tupcommandcoordinator.h"

#include <QDebug>

TupCommandCoordinator::TupCommandCoordinator(QObject *parent)
    : QObject(parent)
{
}

TupCommandCoordinator::~TupCommandCoordinator()
{
}

bool TupCommandCoordinator::registerDependency(
    const QString &prerequisiteCommandId,
    const TupProjectRequest &dependentRequest)
{
    const QString prerequisiteId = prerequisiteCommandId.trimmed();

    if (prerequisiteId.isEmpty()) {
        qWarning()
            << "[TupCommandCoordinator::registerDependency()]"
            << "The prerequisite command ID is empty.";
        return false;
    }

    if (!dependentRequest.isValid()) {
        qWarning()
            << "[TupCommandCoordinator::registerDependency()]"
            << "The dependent request is invalid."
            << "Prerequisite:" << prerequisiteId;
        return false;
    }

    if (dependentRequest.getCommandId().isEmpty()) {
        qWarning()
            << "[TupCommandCoordinator::registerDependency()]"
            << "The dependent request has no command ID."
            << "Prerequisite:" << prerequisiteId;
        return false;
    }

    if (dependentRequest.getCommandId() == prerequisiteId) {
        qWarning()
            << "[TupCommandCoordinator::registerDependency()]"
            << "A command cannot depend on itself:"
            << prerequisiteId;
        return false;
    }

    QList<TupProjectRequest> &commands = m_pendingCommands[prerequisiteId];

    for (const TupProjectRequest &request : commands) {
        if (request.getCommandId() == dependentRequest.getCommandId()) {
            qWarning()
                << "[TupCommandCoordinator::registerDependency()]"
                << "Dependent command is already registered:"
                << dependentRequest.getCommandId();
            return false;
        }
    }

    commands.append(dependentRequest);

#ifdef TUP_DEBUG
    qDebug()
        << "[TupCommandCoordinator::registerDependency()]"
        << "Registered dependent command:"
        << dependentRequest.getCommandId()
        << "Prerequisite:" << prerequisiteId
        << "Pending for prerequisite:" << commands.count();
#endif

    return true;
}

bool TupCommandCoordinator::hasPendingCommands(
    const QString &prerequisiteCommandId) const
{
    return m_pendingCommands.contains(prerequisiteCommandId.trimmed());
}

int TupCommandCoordinator::pendingCommandCount(
    const QString &prerequisiteCommandId) const
{
    return m_pendingCommands.value(prerequisiteCommandId.trimmed()).count();
}

int TupCommandCoordinator::totalPendingCommandCount() const
{
    int total = 0;

    for (auto it = m_pendingCommands.constBegin();
         it != m_pendingCommands.constEnd(); ++it) {
        total += it.value().count();
    }

    return total;
}

bool TupCommandCoordinator::isEmpty() const
{
    return m_pendingCommands.isEmpty();
}

void TupCommandCoordinator::handleCommandResult(
    const QString &commandId,
    const QString &status,
    const QString &errorCode,
    const QString &message)
{
    const QString prerequisiteId = commandId.trimmed();

    if (prerequisiteId.isEmpty()) {
        qWarning()
            << "[TupCommandCoordinator::handleCommandResult()]"
            << "Received a command result without a command ID.";
        return;
    }

    // Most command results have no dependent command. They are deliberately
    // ignored by the coordinator.
    if (!m_pendingCommands.contains(prerequisiteId)) {
#ifdef TUP_DEBUG
        qDebug()
            << "[TupCommandCoordinator::handleCommandResult()]"
            << "No dependent commands registered for:"
            << prerequisiteId;
#endif
        return;
    }

    switch (statusFromString(status)) {
        case Committed:
            releaseDependentCommands(prerequisiteId);
            break;

        case Rejected:
        case Failed:
            cancelDependentCommands(
                prerequisiteId,
                errorCode.isEmpty()
                    ? QStringLiteral("prerequisite_command_failed")
                    : errorCode,
                message);
            break;

        case InvalidStatus:
        default:
            qWarning()
                << "[TupCommandCoordinator::handleCommandResult()]"
                << "Unknown command-result status:"
                << status
                << "Command:" << prerequisiteId;
            break;
    }
}

void TupCommandCoordinator::clear()
{
#ifdef TUP_DEBUG
    qDebug()
        << "[TupCommandCoordinator::clear()]"
        << "Discarding pending commands:"
        << totalPendingCommandCount();
#endif

    m_pendingCommands.clear();
}

TupCommandCoordinator::ResultStatus TupCommandCoordinator::statusFromString(
    const QString &status)
{
    const QString normalized = status.trimmed().toLower();

    if (normalized == QStringLiteral("committed"))
        return Committed;

    if (normalized == QStringLiteral("rejected"))
        return Rejected;

    if (normalized == QStringLiteral("failed"))
        return Failed;

    return InvalidStatus;
}

void TupCommandCoordinator::releaseDependentCommands(
    const QString &prerequisiteCommandId)
{
    // Remove the list before emitting anything. A released command may
    // synchronously generate another result or register another dependency.
    const QList<TupProjectRequest> commands =
        m_pendingCommands.take(prerequisiteCommandId);

#ifdef TUP_DEBUG
    qDebug()
        << "[TupCommandCoordinator::releaseDependentCommands()]"
        << "Prerequisite committed:"
        << prerequisiteCommandId
        << "Releasing commands:" << commands.count();
#endif

    for (const TupProjectRequest &storedRequest : commands) {
        TupProjectRequest request(storedRequest);
        emit commandReady(&request);
    }

    emit dependencyResolved(prerequisiteCommandId, commands.count());
}

void TupCommandCoordinator::cancelDependentCommands(
    const QString &prerequisiteCommandId,
    const QString &errorCode,
    const QString &message)
{
    const QList<TupProjectRequest> commands =
        m_pendingCommands.take(prerequisiteCommandId);

#ifdef TUP_DEBUG
    qWarning()
        << "[TupCommandCoordinator::cancelDependentCommands()]"
        << "Prerequisite was not committed:"
        << prerequisiteCommandId
        << "Cancelling commands:" << commands.count()
        << "Error:" << errorCode
        << "Message:" << message;
#endif

    for (const TupProjectRequest &request : commands) {
        emit commandCancelled(
            request.getCommandId(),
            prerequisiteCommandId,
            errorCode,
            message);
    }

    emit dependencyRejected(
        prerequisiteCommandId,
        commands.count(),
        errorCode,
        message);
}
