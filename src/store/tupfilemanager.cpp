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

#include "tupfilemanager.h"
#include "tconfig.h"
#include "tupproject.h"
#include "tupscene.h"
#include "tuplayer.h"
#include "tuplibrary.h"
#include "tuppackagehandler.h"
#include "talgorithm.h"
#include "tbackupdialog.h"
#include "tosd.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>

TupFileManager::TupFileManager() : QObject()
{
}

TupFileManager::~TupFileManager()
{
}

bool TupFileManager::writeTextFile(const QString &fileName, const QString &content)
{
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
#ifdef TUP_DEBUG
        qWarning() << "[TupFileManager::writeTextFile()] - Error: Can't open file ->" << fileName
                   << "Error:" << file.errorString();
#endif
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
#ifdef TUP_DEBUG
        qWarning() << "[TupFileManager::writeTextFile()] - Error while writing ->" << fileName;
#endif
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
#ifdef TUP_DEBUG
        qWarning() << "[TupFileManager::writeTextFile()] - Error committing ->" << fileName
                   << "Error:" << file.errorString();
#endif
        return false;
    }

    return true;
}

QString TupFileManager::managedRecoveryRoot() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (root.isEmpty())
        root = QDir::homePath();
    return QDir(root).filePath("TupiTube Recovery");
}

bool TupFileManager::copyRecoveryTree(const QString &sourceFolder, const QString &destFolder,
                                      bool skipRecoveryManifest)
{
    QDir sourceDir(sourceFolder);
    if (!sourceDir.exists())
        return false;

    QDir destDir(destFolder);
    if (!destDir.exists() && !QDir().mkpath(destFolder))
        return false;

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries,
                                                           QDir::Name | QDir::DirsFirst);
    for (const QFileInfo &entry : entries) {
        if (skipRecoveryManifest && entry.fileName() == "recovery.ini")
            continue;

        const QString sourcePath = entry.absoluteFilePath();
        const QString destPath = QDir(destFolder).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyRecoveryTree(sourcePath, destPath, skipRecoveryManifest))
                return false;
        } else if (entry.isFile()) {
            if (QFile::exists(destPath) && !QFile::remove(destPath))
                return false;
            if (!QFile::copy(sourcePath, destPath))
                return false;
        }
    }

    return true;
}

bool TupFileManager::validateProjectDirectory(const QString &projectPath, QString *error) const
{
    const QString projectFile = QDir(projectPath).filePath("project.tpp");
    const QString libraryFile = QDir(projectPath).filePath("library.tpl");
    const QStringList scenes = QDir(projectPath).entryList(QStringList() << "scene*.tps",
                                                            QDir::Files | QDir::Readable,
                                                            QDir::Name);

    QStringList requiredFiles;
    requiredFiles << projectFile << libraryFile;
    for (const QString &scene : scenes)
        requiredFiles << QDir(projectPath).filePath(scene);

    if (scenes.isEmpty()) {
        if (error)
            *error = tr("No scene files were found in the recovery snapshot.");
        return false;
    }

    for (const QString &fileName : requiredFiles) {
        QFile file(fileName);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size() == 0) {
            if (error)
                *error = tr("Recovery file is missing or empty: %1").arg(fileName);
            return false;
        }

        QDomDocument doc;
        if (!doc.setContent(QString::fromLocal8Bit(file.readAll()))) {
            if (error)
                *error = tr("Recovery XML is invalid: %1").arg(fileName);
            return false;
        }
    }

    return true;
}

bool TupFileManager::createRecoverySnapshot(const QString &sourceFolder, const QString &recoveryRoot,
                                            const QString &projectName, const QString &originalFileName,
                                            QString *recoveryPath)
{
    if (!QDir().mkpath(recoveryRoot))
        return false;

    const QString stamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss");
    QString baseName = projectName + "-recovery-" + stamp;
    QString finalPath = QDir(recoveryRoot).filePath(baseName + ".bck");
    int suffix = 1;
    while (QFileInfo::exists(finalPath)) {
        finalPath = QDir(recoveryRoot).filePath(baseName + "-" + QString::number(suffix) + ".bck");
        ++suffix;
    }

    const QString tempPath = finalPath + ".partial";
    QDir tempDir(tempPath);
    if (tempDir.exists() && !tempDir.removeRecursively())
        return false;
    if (!QDir().mkpath(tempPath))
        return false;

    if (!copyRecoveryTree(sourceFolder, tempPath)) {
        QDir(tempPath).removeRecursively();
        return false;
    }

    QSettings manifest(QDir(tempPath).filePath("recovery.ini"), QSettings::IniFormat);
    manifest.setValue("Recovery/FormatVersion", 1);
    manifest.setValue("Recovery/ProjectName", projectName);
    manifest.setValue("Recovery/OriginalFile", originalFileName);
    manifest.setValue("Recovery/CreatedUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    manifest.sync();
    if (manifest.status() != QSettings::NoError) {
        QDir(tempPath).removeRecursively();
        return false;
    }

    QString validationError;
    if (!validateProjectDirectory(tempPath, &validationError)) {
#ifdef TUP_DEBUG
        qWarning() << "[TupFileManager::createRecoverySnapshot()] - Validation failed ->" << validationError;
#endif
        QDir(tempPath).removeRecursively();
        return false;
    }

    QDir rootDir(recoveryRoot);
    if (!rootDir.rename(QFileInfo(tempPath).fileName(), QFileInfo(finalPath).fileName())) {
        QDir(tempPath).removeRecursively();
        return false;
    }

    if (recoveryPath)
        *recoveryPath = finalPath;
    return true;
}

bool TupFileManager::save(const QString &fileName, TupProject *project)
{
    QString projectName = project->getName();

    #ifdef TUP_DEBUG
        qDebug() << "---";
        qDebug() << "[TupFileManager::save()] - Saving file -> " << fileName;
        qDebug() << "[TupFileManager::save()] - Project name -> " << projectName;
        qDebug() << "---";
    #endif

    QFileInfo info(fileName);
    QString filename = info.baseName();
    QString currentDirName = CACHE_DIR + projectName;
    QDir projectDir(currentDirName);
    bool ok;

    // Project name has been changed by the user
    if ((filename.compare(projectName) != 0) && projectDir.exists(currentDirName)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupFileManager::save()] - Case I: User changed project's name...";
        #endif

        QString newPath = CACHE_DIR + filename;
        project->setProjectName(filename);
        projectDir.setPath(newPath);
        project->updateLibraryPaths(newPath);
        project->setDataDir(newPath);

        if (project->soundsListSize()) // The project has at least one sound
            emit projectPathChanged();

        if (!projectDir.exists(newPath)) { // Target dir doesn't exist
            // Update the cache path with new project's name
            #ifdef TUP_DEBUG
                qDebug() << "[TupFileManager::save()] - Case IA: "
                            "Renaming old path -> " << currentDirName << " into -> " << newPath;
            #endif

            // Try to rename old folder into new one
            if (projectDir.rename(currentDirName, newPath)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupFileManager::save()] - "
                                "Success! Directory renamed to -> " << newPath;
                #endif
            } else { // The rename action failed
                #ifdef TUP_DEBUG
                    qWarning() << "[TupFileManager::save()] - Case IA-I - Warning: Renaming action failed!";
                #endif
                // Trying to create new project's path
                if (projectDir.mkdir(newPath)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupFileManager::save()] - "
                                    "Directory was created successfully -> " << newPath;
                    #endif
                    // Copying the whole old folder into the new one
                    if (TAlgorithm::copyFolder(currentDirName, newPath)) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupFileManager::save()] - "
                                        "Success! Old path -> " << currentDirName << " copied to -> " << newPath;
                        #endif
                        // Removing old path
                        projectDir.setPath(currentDirName);
                        if (projectDir.removeRecursively()) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupFileManager::save()] - "
                                            "Success! Old path removed -> " << currentDirName;
                            #endif
                        } else {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupFileManager::save()] - "
                                            "Fatal Error: Can't remove old path -> " << currentDirName;
                            #endif
                            TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("001"));

                            return false;
                        }
                    } else { // Copy action failed
                        #ifdef TUP_DEBUG
                            qWarning() << "[TupFileManager::save()] - "
                                          "Fatal Error: Can't copy content into new path -> " << newPath;
                        #endif
                        TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("002"));

                        return false;
                    }
                } else { // New path creation failed
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupFileManager::save()] - "
                                      "Error: Can't create path -> " << newPath;
                    #endif
                    TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("003"));

                    return false;
                }
            }
        } else { // Target dir exists
            #ifdef TUP_DEBUG
                qDebug() << "[TupFileManager::save()] - Case IB: "
                            "Folder path already exists! -> " << newPath;
            #endif

            // If source dir exists
            if (projectDir.exists(currentDirName) && (newPath.compare(project->getDataDir()) != 0)) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupFileManager::save()] - "
                                "Removing existing folder -> " << currentDirName;
                #endif
                // Removing target dir
                if (projectDir.removeRecursively()) {
                    // If rename action fails, then try to create target dir
                    if (projectDir.mkdir(newPath)) {
                        #ifdef TUP_DEBUG
                            qDebug() << "[TupFileManager::save()] - "
                                        "Directory was created successfully after deletion -> " << newPath;
                            qDebug() << "[TupFileManager::save()] - Now copying files from -> " << currentDirName;
                        #endif
                        // Copying old dir into target dir
                        if (TAlgorithm::copyFolder(currentDirName, newPath)) {
                            #ifdef TUP_DEBUG
                                qDebug() << "[TupFileManager::save()] - "
                                            "Success! Project files copied into -> " << newPath;
                            #endif
                        } else { // Copy action failed
                            #ifdef TUP_DEBUG
                                qWarning() << "[TupFileManager::save()] - "
                                              "Fatal Error: Can't copy content into new path -> " << newPath;
                            #endif
                            TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("004"));

                            return false;
                        }
                    } else { // Failed while creating target dir
                        #ifdef TUP_DEBUG
                            qWarning() << "[TupFileManager::save()] - "
                                          "Error: Can't create path after removing -> " << newPath;
                        #endif
                        TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("005"));

                        return false;
                    }
                } else { // Failed removing target dir
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupFileManager::save()] - "
                                      "Error: Can't create path after removing -> " << newPath;
                    #endif
                    TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("006"));

                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    // SQA: This case is still under revision
                    qDebug() << "---";
                    qDebug() << "[TupFileManager::save()] - "
                                "User is saving the current opened project in same/other folder using same name...";
                    qDebug() << "*** fileName -> " << fileName;
                    qDebug() << "---";
                #endif
            }
        }

        // if (project->soundsListSize()) // The project has at least one sound
        if (project->hasLibrarySounds()) // The project has at least one sound
            emit soundPathsChanged();
    } else {
        // If project's path doesn't exist, create it
        if (!projectDir.exists()) {
            QString projectPath = projectDir.path();
            #ifdef TUP_DEBUG
                qDebug() << "[TupFileManager::save()] -  - Case II: Project dir doesn't exist... -> " << projectPath;
            #endif
            if (filename.compare(project->getName()) != 0) { // User renamed the source file name
                #ifdef TUP_DEBUG
                    qDebug() << "[TupFileManager::save()] - Updating project name to -> " << filename;
                #endif
                QString newPath = CACHE_DIR + filename;
                projectPath = newPath;
                projectDir.setPath(projectPath);
                project->setProjectName(filename);
            }

            if (!projectDir.exists()) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupFileManager::save()] - Creating project's directory -> " << projectPath;
                #endif
                if (projectDir.mkdir(projectPath)) {
                    #ifdef TUP_DEBUG
                        qDebug() << "[TupFileManager::save()] - Directory was created successfully -> "
                                 << projectPath;
                    #endif
                } else {
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupFileManager::save()] - Error: Can't create path -> "
                                   << projectPath;
                    #endif
                    TOsd::self()->display(TOsd::Error, tr("Can't save project! (Code %1)").arg("007"));

                    return false;
                }
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupFileManager::save()] - "
                                "Warning: Project's directory already exists! -> " << projectDir.path();
                #endif
            }
        }
    }

    // Saving project components...
    {
        #ifdef TUP_DEBUG
            qDebug() << "[TupFileManager::save()] - Saving project components...";
            qDebug() << "---";
            qDebug() << "[TupFileManager::save()] - source files path -> " << projectDir.path();
        #endif

        // Save project atomically so a failed write never truncates the last valid cache file.
        QDomDocument doc;
        project->setProjectName(filename);
        doc.appendChild(project->toXml(doc));
        const QString projectFileName = projectDir.path() + "/project.tpp";
        #ifdef TUP_DEBUG
            qDebug() << "[TupFileManager::save()] - Saving config file (TPP)";
        #endif
        if (!writeTextFile(projectFileName, doc.toString())) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupFileManager::save()] - Error: Can't safely write file -> " << projectFileName;
            #endif
            return false;
        }
    }

    // Save scenes atomically. Remove stale scene files only after all current scenes
    // have been written successfully, preventing old scene*.tps files from leaking
    // into a later package when the project scene count decreases.
    {
        int totalScenes = project->getScenes().size();
        QSet<QString> expectedSceneFiles;

        for (int i = 0; i < totalScenes; ++i) {
            QDomDocument doc;
            doc.appendChild(project->getScenes().at(i)->toXml(doc));
            const QString sceneName = "scene" + QString::number(i) + ".tps";
            const QString scenePath = projectDir.path() + "/" + sceneName;
            expectedSceneFiles.insert(sceneName);

            #ifdef TUP_DEBUG
                qDebug() << "[TupFileManager::save()] - Saving scene file " << i;
                qDebug() << "[TupFileManager::save()] - Scene file -> " << scenePath;
            #endif

            if (!writeTextFile(scenePath, doc.toString())) {
                #ifdef TUP_DEBUG
                    qWarning() << "[TupFileManager::save()] - Error: Can't safely write file -> " << scenePath;
                #endif
                return false;
            }
        }

        const QStringList existingSceneFiles = projectDir.entryList(QStringList() << "scene*.tps",
                                                                     QDir::Files, QDir::Name);
        for (const QString &sceneFile : existingSceneFiles) {
            if (!expectedSceneFiles.contains(sceneFile)) {
                const QString stalePath = projectDir.filePath(sceneFile);
                if (!QFile::remove(stalePath)) {
                    #ifdef TUP_DEBUG
                        qWarning() << "[TupFileManager::save()] - Error: Can't remove stale scene file -> " << stalePath;
                    #endif
                    return false;
                }
            }
        }
    }

    {
         // Save library atomically.
         QDomDocument doc;
         doc.appendChild(project->getLibrary()->toXml(doc));
         const QString libraryFileName = projectDir.path() + "/library.tpl";
         #ifdef TUP_DEBUG
             qDebug() << "[TupFileManager::save()] - Saving library file (TPL)";
         #endif
         if (!writeTextFile(libraryFileName, doc.toString())) {
             #ifdef TUP_DEBUG
                 qWarning() << "[TupFileManager::save()] - Error: Can't safely write file -> " << libraryFileName;
             #endif
             return false;
         }
    }

    {
        #ifdef TUP_DEBUG
            qDebug() << "[TupFileManager::save()] - Creating TUP file...";
        #endif
        TupPackageHandler packageHandler;
        ok = packageHandler.makePackage(projectDir.path(), fileName);

        if (ok) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupFileManager::save()] - Project saved at -> " << fileName;
            #endif
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupFileManager::save()] - Error: Project couldn't be saved at -> " << fileName;
            #endif

            QApplication::restoreOverrideCursor();

            // Package creation can fail under severe memory pressure even after
            // the complete unpacked project has been serialized successfully.
            // Preserve that validated directory as an independent recovery snapshot.
            QString recoveryPath;
            bool recovered = createRecoverySnapshot(projectDir.path(), managedRecoveryRoot(),
                                                      filename, fileName, &recoveryPath);

            if (!recovered) {
                TBackupDialog dialog(filename);
                if (dialog.exec() == QDialog::Accepted) {
                    recovered = createRecoverySnapshot(projectDir.path(), dialog.selectedDirectory(),
                                                       filename, fileName, &recoveryPath);
                }
            }

            if (recovered) {
                TCONFIG->beginGroup("General");
                TCONFIG->setValue("RecoveryDir", recoveryPath);
                TCONFIG->sync();
            }

            // The main window owns the user-facing save-failure message. This
            // keeps all save failures (serialization, permissions, packaging)
            // on one consistent modal path while preserving RecoveryDir when a
            // package failure produced a validated recovery snapshot.

            // A recovery snapshot is not a successful normal save. The caller
            // must keep the project dirty/open so the user can retry.
            return false;
        }
    }

    return ok;
}

bool TupFileManager::loadProjectDirectory(const QString &projectPath, TupProject *project)
{
    QDir projectDir(projectPath);
    QString validationError;
    if (!validateProjectDirectory(projectPath, &validationError)) {
#ifdef TUP_DEBUG
        qWarning() << "[TupFileManager::loadProjectDirectory()] - Invalid project directory ->" << validationError;
#endif
        return false;
    }

    const QString projectConfigPath = projectDir.filePath("project.tpp");
    QFile pfile(projectConfigPath);
    if (!pfile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    project->fromXml(QString::fromLocal8Bit(pfile.readAll()));
    pfile.close();

    project->setDataDir(projectPath);

    const QString libraryPath = projectDir.filePath("library.tpl");
    if (!project->loadLibrary(libraryPath)) {
        TOsd::self()->display(TOsd::Error, tr("Library file is corrupted!"));
        return false;
    }

    scenesLabels.clear();
    const QStringList scenes = projectDir.entryList(QStringList() << "scene*.tps",
                                                     QDir::Readable | QDir::Files, QDir::Name);
    int index = 0;
    for (const QString &sceneFileName : scenes) {
        const QString scenePath = projectDir.filePath(sceneFileName);
        QFile file(scenePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;

        const QString xml = QString::fromLocal8Bit(file.readAll());
        QDomDocument doc;
        if (!doc.setContent(xml))
            return false;

        const QDomElement root = doc.documentElement();
        const QString sceneName = root.attribute("name");
        scenesLabels << sceneName;
        project->createScene(sceneName, index, true)->fromXml(xml);
        ++index;
    }

    if (project->scenesCount() > 0) {
        const QColor sceneBgColor = project->sceneAt(0)->getBgColor();
        project->setCurrentBgColor(sceneBgColor);
    }

    project->setOpen(true);
    return true;
}

bool TupFileManager::load(const QString &fileName, TupProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupFileManager::load()] - fileName ->" << fileName;
    #endif

    TupPackageHandler packageHandler;
    if (!packageHandler.importPackage(fileName)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupFileManager::load()] - Error: Can't import package ->" << fileName;
        #endif
        return false;
    }

    return loadProjectDirectory(packageHandler.importedProjectPath(), project);
}

bool TupFileManager::loadRecovery(const QString &recoveryPath, TupProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupFileManager::loadRecovery()] - recoveryPath ->" << recoveryPath;
    #endif

    QString validationError;
    if (!validateProjectDirectory(recoveryPath, &validationError)) {
        #ifdef TUP_DEBUG
            qWarning() << "[TupFileManager::loadRecovery()] - Invalid recovery snapshot ->" << validationError;
        #endif
        return false;
    }

    QSettings manifest(QDir(recoveryPath).filePath("recovery.ini"), QSettings::IniFormat);
    QString projectName = manifest.value("Recovery/ProjectName").toString().trimmed();
    if (projectName.isEmpty()) {
        QFile projectFile(QDir(recoveryPath).filePath("project.tpp"));
        if (!projectFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QDomDocument doc;
        if (!doc.setContent(QString::fromLocal8Bit(projectFile.readAll())))
            return false;
        projectName = doc.documentElement().attribute("name").trimmed();
    }
    if (projectName.isEmpty())
        return false;

    // Restore into the normal cache location so all existing local-mode asset
    // and Save As code continues to operate on its expected data directory.
    const QString cachePath = CACHE_DIR + projectName;
    QDir cacheDir(cachePath);
    if (cacheDir.exists() && !cacheDir.removeRecursively())
        return false;
    if (!QDir().mkpath(cachePath))
        return false;
    if (!copyRecoveryTree(recoveryPath, cachePath, true)) {
        QDir(cachePath).removeRecursively();
        return false;
    }

    return loadProjectDirectory(cachePath, project);
}

bool TupFileManager::createImageProject(const QString &projectCode, const QString &imgPath, TupProject *project)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupFileManager::createImageProject()] - projectCode -> " << projectCode;
        qDebug() << "[TupFileManager::createImageProject()] - imgPath -> " << imgPath;
    #endif

    QString filename = CACHE_DIR + projectCode + ".tup";

    TupProject *newProject = new TupProject();
    newProject->setProjectName(projectCode);
    newProject->setAuthor(project->getAuthor());
    newProject->setCurrentBgColor(project->getCurrentBgColor());
    newProject->setDescription(project->getDescription());
    newProject->setDimension(project->getDimension());
    newProject->setFPS(project->getFPS(), 0);
    newProject->setDataDir(CACHE_DIR + projectCode);

    TupLibrary *library = new TupLibrary("library", newProject);
    newProject->setLibrary(library);

    TupScene * newScene = newProject->createScene(tr("Scene %1").arg(QString::number(1)), 0);
    TupLayer *newLayer = newScene->createLayer(tr("Layer %1").arg(QString::number(1)), 0);
    newLayer->createFrame(tr("Frame %1").arg(QString::number(1)), 0);
    TupFrame *frame = newLayer->frameAt(0);

    QFile file(imgPath);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            if (library->createSymbol(TupLibraryObject::Image, "image.png", data, "") == nullptr) {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupProject::createImageProject()] - Fatal Error: Image object can't be created. Data is NULL!";
                #endif

                return false;
            }

            TupLibraryObject *object = library->getObject("image.png");
            if (object) {
                TupGraphicLibraryItem *libraryItem = new TupGraphicLibraryItem(object);
                int imageW = static_cast<int>(libraryItem->boundingRect().width());
                int imageH = static_cast<int> (libraryItem->boundingRect().height());

                qreal xPos = 0;
                qreal yPos = 0;
                QSize dimension = newProject->getDimension();
                if (dimension.width() > imageW)
                    xPos = (dimension.width() - imageW) / 2;
                if (dimension.height() > imageH)
                    yPos = (dimension.height() - imageH) / 2;

                libraryItem->moveBy(xPos, yPos);

                int zLevel = frame->getTopZLevel();
                libraryItem->setZValue(zLevel);
                frame->addItem("image.png", libraryItem);
            }
        }
    }

    return save(filename, newProject);
}

QList<QString> TupFileManager::scenesList()
{
    return scenesLabels;
}
