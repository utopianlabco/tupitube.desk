/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 ***************************************************************************/

#include "tupcommandresultparser.h"

#include <QXmlStreamReader>

TupCommandResultParser::TupCommandResultParser()
    : m_version(0),
      m_status(Invalid),
      m_committedRevision(-1),
      m_eventIndex(-1)
{
}

TupCommandResultParser::TupCommandResultParser(const QString &xml)
    : m_version(0),
      m_status(Invalid),
      m_committedRevision(-1),
      m_eventIndex(-1)
{
    parse(xml);
}

TupCommandResultParser::~TupCommandResultParser()
{
}

bool TupCommandResultParser::parse(const QString &xml)
{
    reset();

    if (xml.trimmed().isEmpty()) {
        m_errorString = QStringLiteral("The command result XML is empty.");
        return false;
    }

    QXmlStreamReader reader(xml);
    bool rootFound = false;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement())
            continue;

        const QString elementName = reader.name().toString();

        if (!rootFound) {
            if (elementName != QStringLiteral("command_result")) {
                reader.raiseError(
                    QStringLiteral("Expected command_result root element."));
                continue;
            }

            rootFound = true;

            const QXmlStreamAttributes attributes = reader.attributes();

            bool versionOk = false;
            m_version = attributes
                .value(QStringLiteral("version"))
                .toString()
                .toInt(&versionOk);

            if (!versionOk || m_version != 1) {
                reader.raiseError(
                    QStringLiteral("Unsupported command result version."));
                continue;
            }

            m_commandId = attributes
                .value(QStringLiteral("command_id"))
                .toString()
                .trimmed();

            if (m_commandId.isEmpty()) {
                reader.raiseError(
                    QStringLiteral("Missing command_id attribute."));
                continue;
            }

            m_status = statusFromString(
                attributes.value(QStringLiteral("status"))
                    .toString()
                    .trimmed()
                    .toLower());

            if (m_status == Invalid) {
                reader.raiseError(
                    QStringLiteral("Invalid command result status."));
                continue;
            }

            m_errorCode = attributes
                .value(QStringLiteral("error_code"))
                .toString()
                .trimmed();

            if (attributes.hasAttribute(QStringLiteral("committed_revision"))) {
                bool revisionOk = false;
                const qint64 revision = attributes
                    .value(QStringLiteral("committed_revision"))
                    .toString()
                    .toLongLong(&revisionOk);
                if (!revisionOk || revision <= 0) {
                    reader.raiseError(
                        QStringLiteral("Invalid committed_revision attribute."));
                    continue;
                }
                m_committedRevision = revision;
            }

            if (attributes.hasAttribute(QStringLiteral("event_index"))) {
                bool eventIndexOk = false;
                const int eventIndex = attributes
                    .value(QStringLiteral("event_index"))
                    .toString()
                    .toInt(&eventIndexOk);
                if (!eventIndexOk || eventIndex < 0) {
                    reader.raiseError(
                        QStringLiteral("Invalid event_index attribute."));
                    continue;
                }
                m_eventIndex = eventIndex;
            }

            m_eventType = attributes
                .value(QStringLiteral("event_type"))
                .toString()
                .trimmed();

            continue;
        }

        if (elementName == QStringLiteral("message")) {
            m_message = reader.readElementText().trimmed();
        } else if (elementName == QStringLiteral("authoritative_payload")) {
            m_authoritativePayload = reader.readElementText().trimmed();
        } else {
            reader.skipCurrentElement();
        }
    }

    if (reader.hasError()) {
        const QString parserError =
            QStringLiteral("%1 (line %2, column %3)")
                .arg(reader.errorString())
                .arg(reader.lineNumber())
                .arg(reader.columnNumber());

        reset();
        m_errorString = parserError;
        return false;
    }

    if (!rootFound) {
        m_errorString =
            QStringLiteral("The command_result root element was not found.");
        return false;
    }

    return true;
}

int TupCommandResultParser::version() const
{
    return m_version;
}

QString TupCommandResultParser::commandId() const
{
    return m_commandId;
}

TupCommandResultParser::Status TupCommandResultParser::status() const
{
    return m_status;
}

QString TupCommandResultParser::errorCode() const
{
    return m_errorCode;
}

QString TupCommandResultParser::message() const
{
    return m_message;
}

qint64 TupCommandResultParser::committedRevision() const
{
    return m_committedRevision;
}

int TupCommandResultParser::eventIndex() const
{
    return m_eventIndex;
}

QString TupCommandResultParser::eventType() const
{
    return m_eventType;
}

QString TupCommandResultParser::authoritativePayload() const
{
    return m_authoritativePayload;
}

QString TupCommandResultParser::errorString() const
{
    return m_errorString;
}

void TupCommandResultParser::reset()
{
    m_version = 0;
    m_commandId.clear();
    m_status = Invalid;
    m_errorCode.clear();
    m_message.clear();
    m_committedRevision = -1;
    m_eventIndex = -1;
    m_eventType.clear();
    m_authoritativePayload.clear();
    m_errorString.clear();
}

TupCommandResultParser::Status TupCommandResultParser::statusFromString(
    const QString &value)
{
    if (value == QStringLiteral("committed"))
        return Committed;

    if (value == QStringLiteral("rejected"))
        return Rejected;

    if (value == QStringLiteral("failed"))
        return Failed;

    return Invalid;
}
