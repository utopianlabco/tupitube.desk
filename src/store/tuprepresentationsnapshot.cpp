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

#include "tuprepresentationsnapshot.h"

#include <QDomDocument>

TupRepresentationSnapshot::TupRepresentationSnapshot()
    : snapshotVersion(FormatVersion), representationZValue(0.0),
      representationOpacity(1.0), representationVisible(true)
{
}

TupRepresentationSnapshot::TupRepresentationSnapshot(const QString &type, const QString &payload,
                                                       qreal zValue, qreal opacity, bool visible)
    : snapshotVersion(FormatVersion), representationType(type.trimmed()),
      representationPayload(payload.trimmed()), representationZValue(zValue),
      representationOpacity(opacity), representationVisible(visible)
{
}

int TupRepresentationSnapshot::version() const
{
    return snapshotVersion;
}

QString TupRepresentationSnapshot::type() const
{
    return representationType;
}

QString TupRepresentationSnapshot::payload() const
{
    return representationPayload;
}

qreal TupRepresentationSnapshot::zValue() const
{
    return representationZValue;
}

qreal TupRepresentationSnapshot::opacity() const
{
    return representationOpacity;
}

bool TupRepresentationSnapshot::isVisible() const
{
    return representationVisible;
}

bool TupRepresentationSnapshot::isValid() const
{
    if (snapshotVersion != FormatVersion || representationType.isEmpty() || representationPayload.isEmpty())
        return false;

    QDomDocument document;
    if (!document.setContent(representationPayload))
        return false;

    return document.documentElement().tagName() == representationType;
}

QString TupRepresentationSnapshot::toXml() const
{
    if (!isValid())
        return QString();

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("representation"));
    root.setAttribute(QStringLiteral("version"), snapshotVersion);
    root.setAttribute(QStringLiteral("type"), representationType);
    root.setAttribute(QStringLiteral("z_value"), QString::number(representationZValue, 'g', 17));
    root.setAttribute(QStringLiteral("opacity"), QString::number(representationOpacity, 'g', 17));
    root.setAttribute(QStringLiteral("visible"), representationVisible ? 1 : 0);
    document.appendChild(root);

    QDomDocument payloadDocument;
    if (!payloadDocument.setContent(representationPayload))
        return QString();

    root.appendChild(document.importNode(payloadDocument.documentElement(), true));
    return document.toString(-1);
}

bool TupRepresentationSnapshot::fromXml(const QString &xml)
{
    QDomDocument document;
    if (!document.setContent(xml))
        return false;

    const QDomElement root = document.documentElement();
    if (root.tagName() != QStringLiteral("representation"))
        return false;

    bool versionOk = false;
    const int parsedVersion = root.attribute(QStringLiteral("version")).toInt(&versionOk);
    if (!versionOk || parsedVersion != FormatVersion)
        return false;

    const QString parsedType = root.attribute(QStringLiteral("type")).trimmed();
    const QDomElement payloadElement = root.firstChildElement();
    if (parsedType.isEmpty() || payloadElement.isNull() || payloadElement.tagName() != parsedType)
        return false;

    QDomDocument payloadDocument;
    payloadDocument.appendChild(payloadDocument.importNode(payloadElement, true));

    bool zOk = false;
    bool opacityOk = false;
    const qreal parsedZValue = root.attribute(QStringLiteral("z_value")).toDouble(&zOk);
    const qreal parsedOpacity = root.attribute(QStringLiteral("opacity")).toDouble(&opacityOk);
    if (!zOk || !opacityOk)
        return false;

    snapshotVersion = parsedVersion;
    representationType = parsedType;
    representationPayload = payloadDocument.toString(-1).trimmed();
    representationZValue = parsedZValue;
    representationOpacity = parsedOpacity;
    representationVisible = root.attribute(QStringLiteral("visible"), QStringLiteral("1")).toInt() != 0;

    return isValid();
}
