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

#include "tupcommandtracker.h"

#include <QDateTime>
#include <QDebug>

TupCommandTracker::TupCommandTracker(QObject *parent)
    : QObject(parent)
{
}

TupCommandTracker::~TupCommandTracker()
{
}

bool TupCommandTracker::track(const TupProjectRequest &request)
{
    const QString commandId = request.getCommandId().trimmed();
    const QString xml = request.getXml();

    if (!request.isValid() || commandId.isEmpty() || xml.isEmpty()) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupCommandTracker::track()]"
            << "Cannot track an invalid command."
            << "Command:" << commandId;
#endif
        return false;
    }

    if (m_pendingCommands.contains(commandId)) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupCommandTracker::track()]"
            << "Command is already pending:"
            << commandId;
#endif
        return false;
    }

    PendingCommand pending;
    pending.commandId = commandId;
    pending.xml = xml;
    pending.lastSentAt = QDateTime::currentMSecsSinceEpoch();
    pending.retryCount = 0;

    m_pendingCommands.insert(commandId, pending);

#ifdef TUP_DEBUG
    qDebug()
        << "[TupCommandTracker::track()]"
        << "Tracking command:" << commandId
        << "Pending commands:" << m_pendingCommands.count();
#endif

    return true;
}

bool TupCommandTracker::contains(const QString &commandId) const
{
    const QString normalized = commandId.trimmed();
    return !normalized.isEmpty() && m_pendingCommands.contains(normalized);
}

int TupCommandTracker::pendingCount() const
{
    return m_pendingCommands.count();
}

QString TupCommandTracker::commandXml(const QString &commandId) const
{
    const QString normalized = commandId.trimmed();
    return m_pendingCommands.value(normalized).xml;
}

int TupCommandTracker::retryCount(const QString &commandId) const
{
    const QString normalized = commandId.trimmed();
    return m_pendingCommands.value(normalized).retryCount;
}

qint64 TupCommandTracker::lastSentAt(const QString &commandId) const
{
    const QString normalized = commandId.trimmed();
    return m_pendingCommands.value(normalized).lastSentAt;
}

QList<QString> TupCommandTracker::expiredCommandIds(qint64 timeoutMs) const
{
    QList<QString> expired;

    if (timeoutMs <= 0)
        return expired;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_pendingCommands.constBegin();
         it != m_pendingCommands.constEnd(); ++it) {
        const PendingCommand &pending = it.value();

        if (pending.lastSentAt > 0
                && now - pending.lastSentAt >= timeoutMs) {
            expired.append(it.key());
        }
    }

    return expired;
}

bool TupCommandTracker::markRetried(const QString &commandId)
{
    const QString normalized = commandId.trimmed();

    auto it = m_pendingCommands.find(normalized);
    if (normalized.isEmpty() || it == m_pendingCommands.end())
        return false;

    it->retryCount++;
    it->lastSentAt = QDateTime::currentMSecsSinceEpoch();

#ifdef TUP_DEBUG
    qWarning()
        << "[TupCommandTracker::markRetried()]"
        << "Command marked for retry:" << normalized
        << "Retry count:" << it->retryCount;
#endif

    return true;
}

bool TupCommandTracker::complete(const QString &commandId)
{
    const QString normalized = commandId.trimmed();

    if (normalized.isEmpty())
        return false;

    const bool removed = m_pendingCommands.remove(normalized) > 0;

#ifdef TUP_DEBUG
    if (removed) {
        qDebug()
            << "[TupCommandTracker::complete()]"
            << "Command completed:" << normalized
            << "Pending commands:" << m_pendingCommands.count();
    } else {
        qDebug()
            << "[TupCommandTracker::complete()]"
            << "Command result has no tracked entry:"
            << normalized;
    }
#endif

    return removed;
}

void TupCommandTracker::clear()
{
#ifdef TUP_DEBUG
    qDebug()
        << "[TupCommandTracker::clear()]"
        << "Discarding pending commands:"
        << m_pendingCommands.count();
#endif

    m_pendingCommands.clear();
}
