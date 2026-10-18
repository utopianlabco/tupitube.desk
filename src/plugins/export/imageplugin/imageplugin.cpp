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

#include "imageplugin.h"

ImagePlugin::ImagePlugin()
{
}

ImagePlugin::~ImagePlugin()
{
}

QString ImagePlugin::formatName() const
{
    return tr("Image Sequence");
}

TupExportInterface::Plugin ImagePlugin::key()
{
    return TupExportInterface::ImageSequence;
}

TupExportInterface::Formats ImagePlugin::availableFormats()
{
    return TupExportInterface::PNG | TupExportInterface::JPEG | TupExportInterface::SVG;
}

bool ImagePlugin::exportToFormat(int colorAlpha, const QString &filePath, const QList<TupScene *> &scenes,
                                 TupExportInterface::Format format, const QSize &size, const QSize &newSize, int fps,
                                 TupProject *project, bool waterMark)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ImagePlugin::exportToFormat()] - color alpha ->" << colorAlpha;
        qDebug() << "[ImagePlugin::exportToFormat()] - size ->" << size;
        qDebug() << "[ImagePlugin::exportToFormat()] - scenes.size() ->" << scenes.size();
        qDebug() << "[ImagePlugin::exportToFormat()] - format ->" << format;
    #endif

    Q_UNUSED(fps)

    errorMsg.clear();
    if (!project || scenes.isEmpty() || size.isEmpty() || newSize.isEmpty() || filePath.isEmpty()) {
        errorMsg = tr("Invalid image sequence export parameters.");
        qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
        return false;
    }

    int frames = 0;
    foreach (TupScene *scene, scenes)
        frames += scene->framesCount();

    QFileInfo fileInfo(filePath);

    QDir dir = fileInfo.dir();
    if (!dir.exists() && !dir.mkpath(".")) {
        errorMsg = tr("Could not create image export directory: %1.").arg(dir.absolutePath());
        qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
        return false;
    }

    m_baseName = fileInfo.baseName();
    const char *extension = "";
    QImage::Format imageFormat = QImage::Format_RGB32;
    switch (format) {
            case TupExportInterface::JPEG:
                 extension = "JPEG";
                 imageFormat = QImage::Format_RGB32;
                 break;
            case TupExportInterface::PNG:
                 extension = "PNG";
                 imageFormat = QImage::Format_ARGB32;
                 break;
            case TupExportInterface::XPM:
                 extension = "XPM";
                 imageFormat = QImage::Format_RGB32;
                 break;
            case TupExportInterface::SVG:
                 extension = "SVG";
                 break;
            default:
                 #ifdef TUP_DEBUG
                     qDebug() << "[ImagePlugin::exportToFormat()] - Warning: Image format is unknown ->" << format;
                 #endif
                 errorMsg = tr("Unsupported image sequence format.");
                 qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                 return false;
    }

    TupAnimationRenderer renderer(project->getLibrary(), waterMark);
    int exportedFrames = 0;
    foreach (TupScene *scene, scenes) {
        if (colorAlpha == 0)
            renderer.setScene(scene, size, Qt::transparent);
        else
            renderer.setScene(scene, size, scene->getBgColor());

        int photogram = 0;
        while (renderer.nextPhotogram()) {
            QString index = "";
            if (exportedFrames < 10) {
                index = "000";
            } else if (exportedFrames < 100) {
                index = "00";
            } else {
                index = "0";
            }

            index += QString("%1").arg(exportedFrames);

            if (QString(extension).compare("SVG") == 0) {
                QString path = fileInfo.absolutePath() + "/"
                               + QString(m_baseName + "%1.%2").arg(index, QString(extension).toLower());

                QFile output(path);
                if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    errorMsg = tr("Could not open image file %1: %2.").arg(path, output.errorString());
                    qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                    return false;
                }

                QSvgGenerator generator;
                generator.setOutputDevice(&output);
                generator.setSize(size);
                generator.setViewBox(QRect(0, 0, size.width(), size.height()));
                QFileInfo info(path);
                generator.setTitle(info.fileName());
                generator.setDescription(scene->getSceneName());

                QPainter painter;
                if (!painter.begin(&generator)) {
                    output.close();
                    errorMsg = tr("Could not start SVG export to %1.").arg(path);
                    qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                    return false;
                }
                painter.setRenderHint(QPainter::Antialiasing, true);
                renderer.render(&painter); // Frame render is created here
                const bool painterFinished = painter.end();
                output.close();
                if (!painterFinished || output.error() != QFile::NoError) {
                    errorMsg = tr("Could not finish SVG export to %1: %2.").arg(path, output.errorString());
                    qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                    return false;
                }
            } else { // Exporting JPG/PNG
                QImage image(size, imageFormat);
                if (image.isNull()) {
                    errorMsg = tr("Could not allocate image for export.");
                    qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                    return false;
                }
                if (QString(extension).compare("PNG") == 0) {
                    if (colorAlpha == 0)
                        image.fill(Qt::transparent);
                    else
                        image.fill(scene->getBgColor());
                } else {
                    image.fill(scene->getBgColor());
                }

                {
                    QPainter painter;
                    if (!painter.begin(&image)) {
                        errorMsg = tr("Could not start image sequence rendering.");
                        qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                        return false;
                    }
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    renderer.render(&painter);
                    if (!painter.end()) {
                        errorMsg = tr("Could not finish image sequence rendering.");
                        qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                        return false;
                    }
                }

                if (size != newSize)
                    image = image.QImage::scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

                QString imgPath = fileInfo.absolutePath() + "/"
                                  + QString(m_baseName + "%1.%2").arg(index, QString(extension).toLower());
                #ifdef TUP_DEBUG
                    qDebug() << "[ImagePlugin::exportToFormat()] - Saving image at ->" << imgPath;
                #endif
                if (!image.save(imgPath, extension, 100)) {
                    errorMsg = tr("Could not save image sequence frame to %1.").arg(imgPath);
                    qWarning() << "[ImagePlugin::exportToFormat()] -" << errorMsg;
                    return false;
                }
            }
            #ifdef TUP_DEBUG
                qDebug() << "[ImagePlugin::exportToFormat()] - Rendering frame ->" << photogram;
            #endif
            photogram++;
            exportedFrames++;

            if (frames > 0)
                emit progressChanged((exportedFrames * 100) / frames);
        }
    }

    return true;
}

bool ImagePlugin::exportFrame(int frameIndex, const QColor color, const QString &filePath, TupScene *scene,
                              const QSize &size, TupProject *project, bool waterMark, bool showForegroundView)
{
    #ifdef TUP_DEBUG
        qDebug() << "[ImagePlugin::exportToFrame()] - size ->" << size;
    #endif

    errorMsg.clear();
    if (!project || !scene || size.isEmpty() || frameIndex < 0 || filePath.isEmpty()) {
        errorMsg = tr("Invalid image export parameters.");
        qWarning() << "[ImagePlugin::exportFrame()] -" << errorMsg;
        return false;
    }

    QString path = filePath;
    const char *extension;
    QImage::Format imageFormat;
    const QColor bgColor = color.isValid() ? color : scene->getBgColor();

    TupAnimationRenderer renderer(project->getLibrary(), waterMark);
    renderer.setScene(scene, size, bgColor);
    renderer.renderPhotogram(frameIndex, showForegroundView);

    bool result = false;
    if (filePath.endsWith(".SVG", Qt::CaseInsensitive)) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(size);
        generator.setViewBox(QRect(0, 0, size.width(), size.height()));
        QFileInfo info(path);
        generator.setTitle(info.fileName());
        generator.setDescription(scene->getSceneName());

        QPainter painter;
        if (painter.begin(&generator)) {
            painter.setRenderHint(QPainter::Antialiasing, true);
            renderer.render(&painter);
            result = painter.end();
        }
    } else {
        if (filePath.endsWith(".PNG", Qt::CaseInsensitive)) {
            extension = "PNG";
            imageFormat = QImage::Format_ARGB32;
        } else if (filePath.endsWith(".JPG", Qt::CaseInsensitive) || filePath.endsWith(".JPEG", Qt::CaseInsensitive)) {
            extension = "JPEG";
            imageFormat = QImage::Format_RGB32;
        } else {
            extension = "PNG";
            path += ".png";
            imageFormat = QImage::Format_ARGB32;
        }

        QImage image(size, imageFormat);
        if (!image.isNull()) {
            image.fill(bgColor);
            QPainter painter;
            if (painter.begin(&image)) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                renderer.render(&painter);
                result = painter.end() && image.save(path, extension, 100);
            }
        }
    }

    if (!result) {
        errorMsg = tr("Could not export image to %1.").arg(path);
        qWarning() << "[ImagePlugin::exportFrame()] -" << errorMsg;
    }
    return result;
}

QString ImagePlugin::getExceptionMsg() const
{
    return errorMsg;
}

bool ImagePlugin::exportToAnimatic(const QString &filePath, const QList<QImage> images, const QList<int> indexes,
                              TupExportInterface::Format format, const QSize &size, int fps)
{
    Q_UNUSED(filePath)
    Q_UNUSED(images)
    Q_UNUSED(indexes)
    Q_UNUSED(format)
    Q_UNUSED(size)
    Q_UNUSED(fps)

    return true;
}
