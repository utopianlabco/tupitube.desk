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
#include "quazipfileinfo.h"
#include "JlCompress.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QUuid>
#include <QDirIterator>
#include <QDomDocument>
#include <QSet>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// The transaction record is committed before the first directory rename. It
// identifies the logical destination independently of the random backup name.
// A destination that already exists is never overwritten during recovery.
static bool recoverPackageTransactions(const QString &workPath, const QString &root)
{
    QDir workDir(workPath);
    const QStringList journals = workDir.entryList(
                QStringList() << ".tup-import-txn-*", QDir::Files | QDir::Hidden);
    for (const QString &name : journals) {
        const QString journalPath = workDir.filePath(name);
        const QFileInfo journalInfo(journalPath);
        if (journalInfo.isSymLink()) {
            qCritical() << "[TupPackageHandler] Unsafe import journal:" << journalPath;
            return false;
        }
        QFile file(journalPath);
        if (!file.open(QIODevice::ReadOnly))
            return false;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            qCritical() << "[TupPackageHandler] Invalid import journal:" << journalPath;
            return false;
        }
        const QJsonObject data = document.object();
        const QString projectRoot = data.value("root").toString();
        if (data.value("version").toInt() != 1 || projectRoot.isEmpty() ||
            projectRoot == "." || projectRoot == ".." || projectRoot.contains('/') ||
            projectRoot.contains('\\') || projectRoot.startsWith('.') ||
            !data.value("hadPrevious").isBool()) {
            qCritical() << "[TupPackageHandler] Unsafe import journal metadata:" << journalPath;
            return false;
        }
        if (!root.isEmpty() && projectRoot != root)
            continue;
        const QString id = name.mid(QStringLiteral(".tup-import-txn-").size());
        if (QUuid(id).isNull()) {
            qCritical() << "[TupPackageHandler] Invalid transaction id:" << journalPath;
            return false;
        }
        const QString backup = workDir.filePath(".tup-import-backup-" + id);
        const QString staging = workDir.filePath(".tup-import-" + id);
        const QString destination = workDir.filePath(projectRoot);
        const QFileInfo backupInfo(backup), destinationInfo(destination), stagingInfo(staging);
        if (backupInfo.isSymLink() || destinationInfo.isSymLink() || stagingInfo.isSymLink() ||
            (backupInfo.exists() && !backupInfo.isDir()) ||
            (destinationInfo.exists() && !destinationInfo.isDir()) ||
            (stagingInfo.exists() && !stagingInfo.isDir())) {
            qCritical() << "[TupPackageHandler] Unsafe interrupted import path:" << journalPath;
            return false;
        }
        const bool hadPrevious = data.value("hadPrevious").toBool();
        if (backupInfo.exists()) {
            if (destinationInfo.exists()) {
                // Could be a completed promotion or a newer successful import.
                // Refuse to guess or overwrite the existing project.
                qCritical() << "[TupPackageHandler] Ambiguous interrupted import; preserving both caches:"
                            << destination << backup;
                return false;
            }
            if (!workDir.rename(QFileInfo(backup).fileName(), projectRoot)) {
                qCritical() << "[TupPackageHandler] Cannot restore interrupted cache:" << backup;
                return false;
            }
            qWarning() << "[TupPackageHandler] Recovered interrupted import:" << destination;
        } else if (hadPrevious && !destinationInfo.exists()) {
            qCritical() << "[TupPackageHandler] Previous cache missing from interrupted import:"
                        << journalPath;
            return false;
        }
        if (stagingInfo.exists() && !QDir(staging).removeRecursively()) {
            qWarning() << "[TupPackageHandler] Could not clean abandoned staging:" << staging;
            return false;
        }
        if (!QFile::remove(journalPath)) {
            qWarning() << "[TupPackageHandler] Could not remove resolved journal:" << journalPath;
            return false;
        }
    }
    return true;
}

static bool createPackageJournal(const QString &path, const QString &root, bool hadPrevious)
{
    QJsonObject data;
    data.insert("version", 1);
    data.insert("root", root);
    data.insert("hadPrevious", hadPrevious);
    QSaveFile journal(path);
    journal.setDirectWriteFallback(false);
    if (!journal.open(QIODevice::WriteOnly))
        return false;
    const QByteArray payload = QJsonDocument(data).toJson(QJsonDocument::Compact);
    if (journal.write(payload) != payload.size()) {
        journal.cancelWriting();
        return false;
    }
    return journal.commit();
}

bool TupPackageHandler::recoverInterruptedImports()
{
    // Use the journal metadata; never infer a destination from a backup name.
    return recoverPackageTransactions(QDir::cleanPath(CACHE_DIR), QString());
}

TupPackageHandler::TupPackageHandler()
{
}

TupPackageHandler::~TupPackageHandler()
{
    // Do not destroy the preserved backup after a failed rollback.
    if (!rollbackFailed)
        finishImport();
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
    // Do not begin another transaction before the previous one is resolved.
    if (!importedCachePath.isEmpty())
        return false;

    gPath.clear();
    projectDir.clear();
    rollbackFailed = false;
    const QString workPath = QDir::cleanPath(CACHE_DIR + tempFolder);
    QDir workDir(workPath);
    if (!workDir.exists())
        return false;

    // Treat archive entry names as untrusted paths, even when using JlCompress.
    // One top-level project directory is required; no absolute, parent, link,
    // drive-qualified or mixed-separator entry may reach extraction.
    QuaZip zip(packagePath);
    if (!zip.open(QuaZip::mdUnzip))
        return false;

    QString topLevel;
    QStringList expectedFiles;
    QSet<QString> knownEntries;
    bool safe = zip.goToFirstFile();
    if (!safe) {
        zip.close();
        return false;
    }
    do {
        const QString entry = zip.getCurrentFileName();
        const QStringList components = entry.split('/', Qt::KeepEmptyParts);
        if (entry.isEmpty() || entry.startsWith('/') || entry.contains('\\') ||
            entry.contains(':') || components.size() < 2) {
            safe = false;
            break;
        }
        for (int i = 0; i < components.size(); ++i) {
            const QString &part = components.at(i);
            if (part == "." || part == ".." ||
                (part.isEmpty() && i != components.size() - 1)) {
                safe = false;
                break;
            }
        }
        if (!safe)
            break;

        const QString root = components.first();
        if (root.isEmpty() || root.startsWith('.') ||
            (!topLevel.isEmpty() && root != topLevel)) {
            safe = false;
            break;
        }
        topLevel = root;
        if (knownEntries.contains(entry)) {
            safe = false;
            break;
        }
        knownEntries.insert(entry);

        QuaZipFileInfo info;
        if (!zip.getCurrentFileInfo(&info)) {
            safe = false;
            break;
        }
        // Unix ZIP external mode bits: explicitly reject symbolic links.
        const quint32 mode = (info.externalAttr >> 16) & 0170000;
        if (mode == 0120000) {
            safe = false;
            break;
        }
        if (!entry.endsWith('/'))
            expectedFiles << entry;
    } while (zip.goToNextFile());
    zip.close();
    if (!safe || topLevel.isEmpty() || expectedFiles.isEmpty())
        return false;

    // Recover an earlier interrupted transaction before creating a new one.
    // Legacy pre-journal orphan directories are retained for manual forensic review.
    if (!recoverPackageTransactions(workPath, topLevel))
        return false;

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString journalPath = workDir.filePath(".tup-import-txn-" + id);
    const QString stagingPath = workDir.filePath(".tup-import-" + id);
    const QString backupPath = workDir.filePath(".tup-import-backup-" + id);
    const QString stagedProject = QDir(stagingPath).filePath(topLevel);
    const QString destination = workDir.filePath(topLevel);
    if (!QDir().mkpath(stagingPath))
        return false;

    const QStringList extracted = JlCompress::extractDir(packagePath, stagingPath);
    bool valid = !extracted.isEmpty();
    const QDir stagedDir(stagedProject);
    if (!stagedDir.exists())
        valid = false;
    for (const QString &entry : expectedFiles) {
        const QFileInfo info(QDir(stagingPath).filePath(entry));
        if (!info.isFile() || info.isSymLink()) {
            valid = false;
            break;
        }
    }
    // Reject links and unexpected entries left by the extraction library.
    QDirIterator it(stagingPath, QDir::AllEntries | QDir::NoDotAndDotDot |
                                  QDir::Hidden | QDir::System,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isSymLink()) {
            valid = false;
            break;
        }
    }
    if (valid) {
        QStringList required;
        required << "project.tpp" << "library.tpl";
        const QStringList scenes = stagedDir.entryList(QStringList() << "scene*.tps",
                                                        QDir::Files | QDir::Readable);
        valid = !scenes.isEmpty();
        required.append(scenes);
        for (const QString &name : required) {
            QFile file(stagedDir.filePath(name));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size() == 0) {
                valid = false;
                break;
            }
            QDomDocument xml;
            if (!xml.setContent(&file)) {
                valid = false;
                break;
            }
        }
    }
    if (!valid) {
        QDir(stagingPath).removeRecursively();
        return false;
    }

    // QDir has no atomic replace-directory API. Move the previous cache aside
    // first, then promote the validated directory, restoring on any failure.
    const QFileInfo prior(destination);
    if (prior.isSymLink() || (prior.exists() && !prior.isDir())) {
        QDir(stagingPath).removeRecursively();
        return false;
    }
    const bool hadPrevious = prior.exists();
    if (!createPackageJournal(journalPath, topLevel, hadPrevious)) {
        QDir(stagingPath).removeRecursively();
        return false;
    }
    if (hadPrevious && !workDir.rename(topLevel, QFileInfo(backupPath).fileName())) {
        QDir(stagingPath).removeRecursively();
        QFile::remove(journalPath);
        return false;
    }
    if (!QDir().rename(stagedProject, destination)) {
        const bool restored = !hadPrevious ||
                workDir.rename(QFileInfo(backupPath).fileName(), topLevel);
        if (!restored)
            qCritical() << "[TupPackageHandler::importPackage()] Cache rollback failed:" << backupPath;
        QDir(stagingPath).removeRecursively();
        if (restored)
            QFile::remove(journalPath);
        return false;
    }
    QDir(stagingPath).removeRecursively();

    importedCachePath = destination;
    transactionJournalPath = journalPath;
    if (hadPrevious)
        previousCachePath = backupPath;
    projectDir = QFileInfo(packagePath).baseName();
    gPath = destination;
    return true;
}

bool TupPackageHandler::rollbackImport()
{
    if (importedCachePath.isEmpty())
        return true;
    const QString path = importedCachePath;
    const QString backup = previousCachePath;
    if (!QDir(path).removeRecursively()) {
        rollbackFailed = true;
        qCritical() << "[TupPackageHandler::rollbackImport()] Cannot remove failed import:" << path;
        return false;
    }
    if (!backup.isEmpty() && !QDir().rename(backup, path)) {
        rollbackFailed = true;
        qCritical() << "[TupPackageHandler::rollbackImport()] Previous cache preserved at:" << backup;
        return false;
    }
    if (!transactionJournalPath.isEmpty() && !QFile::remove(transactionJournalPath)) {
        qWarning() << "[TupPackageHandler::rollbackImport()] Cannot remove journal:"
                   << transactionJournalPath;
    }
    transactionJournalPath.clear();
    importedCachePath.clear();
    previousCachePath.clear();
    rollbackFailed = false;
    gPath.clear();
    return true;
}

void TupPackageHandler::finishImport()
{
    if (!previousCachePath.isEmpty() && !QDir(previousCachePath).removeRecursively()) {
        qWarning() << "[TupPackageHandler::finishImport()] Cannot remove old cache:" << previousCachePath;
        // Keep the journal so a later run detects the ambiguous state.
        return;
    }
    if (!transactionJournalPath.isEmpty() && !QFile::remove(transactionJournalPath))
        qWarning() << "[TupPackageHandler::finishImport()] Cannot remove journal:" << transactionJournalPath;
    transactionJournalPath.clear();
    previousCachePath.clear();
    importedCachePath.clear();
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
