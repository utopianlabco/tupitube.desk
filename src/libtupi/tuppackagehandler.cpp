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

#include "tuppackagehandler.h"
#include "quazip.h"
#include "quazipfile.h"
#include "JlCompress.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QUuid>

TupPackageHandler::TupPackageHandler()
{
}

TupPackageHandler::~TupPackageHandler()
{
}

bool TupPackageHandler::makePackage(const QString &projectPath, const QString &packagePath)
{
    #ifdef TUP_DEBUG
        qWarning() << "[TupPackageHandler::makePackage()] - projectPath -> " << projectPath;
        qWarning() << "[TupPackageHandler::makePackage()] - packagePath -> " << packagePath;
    #endif

    if (!QFile::exists(projectPath)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Project path doesn't exist -> " << projectPath;
        #endif

        return false;
    }

    // Never let the compressor write directly over the last valid package.
    // Build and validate an independent temporary archive first, then copy it
    // through QSaveFile so the destination is replaced only by commit().
    QFileInfo packageInfo(packagePath);
    const QString packageDirPath = packageInfo.absolutePath();
    QDir packageDir(packageDirPath);
    if (!packageDir.exists()) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Package directory doesn't exist -> " << packageDirPath;
        #endif

        return false;
    }

    const QString tempPackagePath = packageDir.filePath(
                packageInfo.fileName() + ".partial-" +
                QUuid::createUuid().toString(QUuid::WithoutBraces));

    #ifdef TUP_DEBUG
        qDebug() << "[TupPackageHandler::makePackage()] - Calling JlCompress library...";
        qDebug() << "[TupPackageHandler::makePackage()] - Temporary package -> " << tempPackagePath;
    #endif

    if (!JlCompress::compressDir(tempPackagePath, projectPath, true)) {
        QFile::remove(tempPackagePath);
        return false;
    }

    QFileInfo tempInfo(tempPackagePath);
    if (!tempInfo.exists() || tempInfo.size() <= 0) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Temporary package is missing or empty -> " << tempPackagePath;
        #endif

        QFile::remove(tempPackagePath);
        return false;
    }

    QuaZip zipChecker(tempPackagePath);
    if (!zipChecker.open(QuaZip::mdUnzip) || !zipChecker.goToFirstFile()) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Temporary package validation failed -> " << tempPackagePath;
        #endif

        zipChecker.close();
        QFile::remove(tempPackagePath);
        return false;
    }
    zipChecker.close();

    QFile tempPackage(tempPackagePath);
    if (!tempPackage.open(QIODevice::ReadOnly)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Can't open temporary package -> " << tempPackagePath;
        #endif

        QFile::remove(tempPackagePath);
        return false;
    }

    QSaveFile destination(packagePath);
    destination.setDirectWriteFallback(false);
    if (!destination.open(QIODevice::WriteOnly)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Can't open destination safely -> " << packagePath
                       << "Error:" << destination.errorString();
        #endif

        tempPackage.close();
        QFile::remove(tempPackagePath);
        return false;
    }

    const qint64 bufferSize = 1024 * 1024;
    while (!tempPackage.atEnd()) {
        const QByteArray chunk = tempPackage.read(bufferSize);
        if (chunk.isEmpty() && tempPackage.error() != QFile::NoError) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupPackageHandler::makePackage()] - "
                              "Error reading temporary package -> " << tempPackagePath
                           << "Error:" << tempPackage.errorString();
            #endif

            destination.cancelWriting();
            tempPackage.close();
            QFile::remove(tempPackagePath);
            return false;
        }

        if (destination.write(chunk) != chunk.size()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupPackageHandler::makePackage()] - "
                              "Error writing destination package -> " << packagePath
                           << "Error:" << destination.errorString();
            #endif

            destination.cancelWriting();
            tempPackage.close();
            QFile::remove(tempPackagePath);
            return false;
        }
    }

    tempPackage.close();

    if (!destination.commit()) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupPackageHandler::makePackage()] - "
                          "Error committing destination package -> " << packagePath
                       << "Error:" << destination.errorString();
        #endif

        QFile::remove(tempPackagePath);
        return false;
    }

    QFile::remove(tempPackagePath);
    return true;
}

bool TupPackageHandler::importPackage(const QString &packagePath, const QString &tempFolder)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPackageHandler::importPackage()] - packagePath -> " << packagePath;
        qDebug() << "[TupPackageHandler::importPackage()] - CACHE_DIR -> " << CACHE_DIR;
        if (!tempFolder.isEmpty())
            qDebug() << "[TupPackageHandler::importPackage()] - tempFolder -> " << tempFolder;
        QFile file(packagePath);
        qDebug() << "[TupPackageHandler::importPackage()] - source file size -> "
                 << (QString::number(file.size()) + " bytes");
    #endif

    QString workPath = CACHE_DIR;
    if (!tempFolder.isEmpty())
        workPath += tempFolder + "/";

    QFileInfo fileInfo(packagePath);
    projectDir = fileInfo.baseName();
    QuaZip zipChecker(packagePath);
    if (zipChecker.open(QuaZip::mdUnzip)) {
        zipChecker.goToFirstFile();
        QString firstFile = zipChecker.getCurrentFileName();
        // qDebug() << "[TupPackageHandler::importPackage()] - firstFile -> " << firstFile;
        int index = firstFile.indexOf("/");
        QString dirName = workPath + firstFile.left(index);
        QDir dir(dirName);
        if (dir.exists(dirName)) {
            if (dir.removeRecursively()) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPackageHandler::importPackage()] - "
                                "Warning: Project directory already exists. Removing successfully! -> " << dirName;
                #endif
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupPackageHandler::importPackage()] - "
                                "Fatal Error: Project directory can't be removed -> " << dirName;
                #endif
            }
        }
    }

    QStringList list = JlCompress::extractDir(packagePath, workPath);
    if (list.size() == 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupPackageHandler::importPackage()] - "
                        "Fatal Error: Project source file has NO elements! -> " << packagePath;
        #endif

        return false;
    }

    QString path = list.at(0);
    int index = path.indexOf("/", workPath.length());
    gPath = path.left(index);

    return true;
}

QString TupPackageHandler::importedProjectPath() const
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPackageHandler::importedProjectPath()] - project path -> " << gPath;
    #endif

    return gPath;
}

QString TupPackageHandler::projectDirectory() const
{
    return projectDir;
}
