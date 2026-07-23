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

#include "tupvideoimporterdialog.h"
#include "tconfig.h"
#include "tseparator.h"
#include "tapptheme.h"
#include "tosd.h"
#include "tupprojectrequest.h"
#include "tseparator.h"
#include "talgorithm.h"
#include "tupaudiocutter.h"

#include <QPushButton>

TupVideoImporterDialog::TupVideoImporterDialog(const QString &filename, const QString &photogramsPath, const QSize &canvasSize,
                                               TupVideoCutter *cutter, QWidget *parent) : QDialog(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::TupVideoImporterDialog()]";
    #endif

    setModal(true);
    videoPath = filename;
    projectSize = canvasSize;

    framesTotal = 1;
    sizeFlag = false;

    QFileInfo fileInfo(videoPath);
    setWindowTitle(tr("Photograms Extractor") + " (" + fileInfo.fileName() + ")");
    setWindowIcon(QIcon(QPixmap(THEME_DIR + "icons/scenes.png")));
    setStyleSheet(TAppTheme::themeStyles());

    videoFramesTotal = cutter->totalFrames();
    videoCutter = cutter;
    connect(videoCutter, SIGNAL(imageExtracted(MediaType, int)), this, SLOT(updateMediaProgress(MediaType, int)));
    connect(videoCutter, SIGNAL(imageExtractionIsDone()), this, SLOT(startImageImportation()));

    assetsPath = photogramsPath;
    videoSize = videoCutter->getVideoSize();

    layout = new QVBoxLayout(this);
    fixSize = projectSize != videoSize;
    setDialogUI(fixSize);
}

TupVideoImporterDialog::~TupVideoImporterDialog()
{
    delete videoCutter;
}

void TupVideoImporterDialog::setDialogUI(bool fixSize)
{
    // Video info section (always shown)
    QGroupBox *videoInfoGroup = new QGroupBox(tr("Video Information"));
    QVBoxLayout *videoLayout = new QVBoxLayout();
    videoLayout->addWidget(new QLabel(tr("Source: %1").arg(videoPath)));
    videoLayout->addWidget(new QLabel(tr("Resolution: %1x%2").arg(videoSize.width()).arg(videoSize.height())));
    videoLayout->addWidget(new QLabel(tr("Duration: %1 frames").arg(videoFramesTotal)));
    videoInfoGroup->setLayout(videoLayout);
    layout->addWidget(videoInfoGroup);

    // Frames extraction section (always shown)
    QGroupBox *framesGroup = new QGroupBox(tr("Frames to Extract (100 max)"));
    QVBoxLayout *framesLayout = new QVBoxLayout();
    framesLayout->addWidget(new QLabel(tr("Number of frames:")));
    imagesBox = new QSpinBox();

    int maxFramesToExtract = qMin(100, videoFramesTotal);
    imagesBox->setRange(1, maxFramesToExtract);
    imagesBox->setValue(videoFramesTotal);
    framesLayout->addWidget(imagesBox);
    framesGroup->setLayout(framesLayout);
    layout->addWidget(framesGroup);

    // Resize options section (ONLY shown if dimensions differ)
    if (fixSize) {
        QGroupBox *resizeGroup = new QGroupBox(tr("Resize Options"));
        QVBoxLayout *resizeLayout = new QVBoxLayout();

        checkButton1 = new QRadioButton(tr("Keep original video size"));
        checkButton2 = new QRadioButton(tr("Adjust video size to project size"));
        checkButton3 = new QRadioButton(tr("Adjust project size to video size"));

        checkButton1->setChecked(true); // Default option

        resizeLayout->addWidget(checkButton1);
        resizeLayout->addWidget(checkButton2);
        resizeLayout->addWidget(checkButton3);
        resizeGroup->setLayout(resizeLayout);
        layout->addWidget(resizeGroup);
    }

    // Audio import section (ALWAYS shown - independent of dimensions)
    QGroupBox *audioGroup = new QGroupBox(tr("Audio Import"));
    QVBoxLayout *audioLayout = new QVBoxLayout();
    audioCheck = new QCheckBox(tr("Import audio if it's available"));
    audioCheck->setChecked(true); // Default to importing audio
    audioLayout->addWidget(audioCheck);
    audioGroup->setLayout(audioLayout);
    layout->addWidget(audioGroup);

    // Progress section (always shown)
    progressWidget = new QWidget();
    QVBoxLayout *progressLayout = new QVBoxLayout();
    progressLabel = new QLabel();
    progressBar = new QProgressBar();
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(progressBar);
    progressWidget->setLayout(progressLayout);
    progressWidget->setVisible(false);
    layout->addWidget(progressWidget);

    // Buttons section (always shown)
    buttonsWidget = new QWidget();
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *startButton = new QPushButton(tr("Start"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    buttonsLayout->addWidget(startButton);
    buttonsLayout->addWidget(cancelButton);
    buttonsWidget->setLayout(buttonsLayout);
    layout->addWidget(buttonsWidget);

    connect(startButton, SIGNAL(clicked()), this, SLOT(startMediaExtraction()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void TupVideoImporterDialog::startMediaExtraction()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::startMediaExtraction()]";
    #endif

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    framesTotal = imagesBox->value() - 1;
    framesCounter = 1;

    imagesBox->setEnabled(false);
    buttonsWidget->setVisible(false);

    if (fixSize) {
        if (checkButton2 && checkButton2->isChecked()) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupVideoImporterDialog::startMediaExtraction()] - Resizing photograms...";
            #endif
            sizeFlag = true;
        }
        if (checkButton3 && checkButton3->isChecked()) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupVideoImporterDialog::startMediaExtraction()] - Resizing project canvas...";
            #endif
            emit projectSizeHasChanged(videoSize);
        }
    }

    progressWidget->setVisible(true);
    progressLabel->setText(tr("Starting procedure..."));

    // Create assets directory if it doesn't exist
    if (!QFile::exists(assetsPath)) {
        QDir dir;
        if (!dir.mkpath(assetsPath)) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupVideoImporterDialog::startMediaExtraction()] - Fatal Error: Couldn't create images directory ->" << assetsPath;
            #endif
            TOsd::self()->display(TOsd::Error, tr("Couldn't create temporary directory!"));
            QApplication::restoreOverrideCursor();
            return;
        }
    }

    // Start video frame extraction
    videoCutter->setExtractionParams(framesTotal);
    if (!videoCutter->startExtraction()) {
        TOsd::self()->display(TOsd::Error, tr("Can't extract photograms!"));
        videoCutter->releaseResources();
        QApplication::restoreOverrideCursor();
        return;
    }
    videoCutter->releaseResources();

    if (audioCheck && audioCheck->isChecked()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupVideoImporterDialog::startMediaExtraction()] - Importing audio...";
        #endif

        progressLabel->setText(tr("Importing audio..."));
        progressBar->setValue(0);
        framesCounter = 0;

        QFileInfo file(videoPath);
        QString audioName = file.baseName();

        progressLabel->setText(tr("Importing audio track from video file..."));

        audioPath = QDir::tempPath() + "/" + audioName + ".mp3";
        if (QFile::exists(audioPath)) {
            audioPath = QDir::tempPath() + "/mp3_audio_" + TAlgorithm::randomString(12) + ".mp3";
        }

        TupAudioCutter *audioCutter = new TupAudioCutter(videoPath, audioPath);
        connect(audioCutter, SIGNAL(audioExtracted(MediaType, int)),
                this, SLOT(updateMediaProgress(MediaType, int)));
        connect(audioCutter, SIGNAL(extractionIsDone(const QString &)),
                this, SIGNAL(audioExtractionIsDone(const QString &)));

        if (!audioCutter->generateMP3Audio()) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupVideoImporterDialog::startMediaExtraction()] - Error: Couldn't extract audio from video file ->" << videoPath;
            #endif
        }
    }
}

void TupVideoImporterDialog::updateMediaProgress(MediaType media, int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::updateMediaProgress()] - index ->" << index;
    #endif

    QString msg = tr("Extracting photogram %1 of %2").arg(index).arg(framesTotal);
    if (media == Audio) {
        msg = tr("Extracting audio frame %1 of %2").arg(index).arg(framesTotal);
    }

    progressLabel->setText(msg);
    progressBar->setValue(framesCounter);
    framesCounter += 1;

    // ---------------------------------------------------------------
    // NEW: Convert extracted video frame to QByteArray and send to library
    // ---------------------------------------------------------------
    if (media == Video) {
        QString frameName = "frame" + QString::number(index);
        QString filePath = assetsPath + frameName + ".png";

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            // Emit the signal to trigger the standard library import workflow
            emit requestAddImageToLibrary(frameName, "png", data, "video_frames/");
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[TupVideoImporterDialog::updateMediaProgress()] - Failed to open frame file for byte array conversion:"
                           << filePath;
            #endif
        }
    }
}

/*
void TupVideoImporterDialog::updateMediaProgress(MediaType media, int index)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::updateMediaProgress()] - index ->" << index;
    #endif

    QString msg = tr("Extracting photogram %1 of %2").arg(index).arg(framesTotal);
    if (media == Audio)
        msg = tr("Extracting audio frame %1 of %2").arg(index).arg(framesTotal);

    progressLabel->setText(msg);
    progressBar->setValue(framesCounter);
    framesCounter += 1;
}
*/

void TupVideoImporterDialog::startImageImportation()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::startImageImportation()] - Extraction is complete!";
        qDebug() << "[TupVideoImporterDialog::startImageImportation()] - Starting image importation...";
    #endif

    progressLabel->setText(tr("Importing images..."));
    progressBar->setValue(0);
    framesCounter = 0;
    emit imageExtractionIsDone(VideoAction, assetsPath, sizeFlag);
}

void TupVideoImporterDialog::updateStatusFromLibraryWidget(const QString &msg)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::updateStatusFromLibraryWidget()] - Starting image importation...";
    #endif

    progressLabel->setText(msg);
    progressBar->setValue(framesCounter);
    framesCounter += 1;
}

void TupVideoImporterDialog::endProcedure()
{
    QDir imgDir(assetsPath);
    #ifdef TUP_DEBUG
        qDebug() << "[TupVideoImporterDialog::removeTempFolder()] - Removing temporary (images) folder ->" << assetsPath;
    #endif
    if (imgDir.exists()) {
        if (!imgDir.removeRecursively()) {
            #ifdef TUP_DEBUG
                qWarning() << "[TupVideoImporterDialog::removeTempFolder()] - Error: Can't remove temporary (images) folder ->"
                           << assetsPath;
            #endif
        }
    }

    QApplication::restoreOverrideCursor();
    TOsd::self()->display(TOsd::Info, tr("Video imported successfully!"));
    close();
}
