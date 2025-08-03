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

#ifndef TUPAUDIOCUTTER_H
#define TUPAUDIOCUTTER_H

#include "tglobal.h"
#include "talgorithm.h"

#include <QObject>
#include <QDir>
#include <QDebug>

#ifdef __cplusplus
extern "C" {
    // Libav libraries
    #include "libavformat/avformat.h"
    #include "libavformat/avio.h"

    #include "libavcodec/avcodec.h"

    #include "libavutil/audio_fifo.h"
    #include "libavutil/avassert.h"
    #include "libavutil/avstring.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/frame.h"
    #include "libavutil/opt.h"

    #include "libswresample/swresample.h"
}
#endif

class TUPITUBE_EXPORT TupAudioCutter : public QObject
{
    Q_OBJECT

    public:
        TupAudioCutter(const QString &inputFile, const QString &audioFile);
        ~TupAudioCutter();

        bool generateMP3Audio();

    signals:
        void audioExtracted(MediaType media, int index);
        void extractionIsDone(const QString &filePath);

    private:
        bool startAACAudioExtraction();

        int openInputAACAudioFile(const char *filename, AVFormatContext **inputFormatContext,
                                  AVCodecContext **inputCodecContext);
        int openOutputMP3File(const char *filename,
                              AVCodecContext *inputCodecContext,
                              AVFormatContext **outputFormatContext,
                              AVCodecContext **outputCodecContext);
        int initPacket(AVPacket **packet);
        int initInputAACFrame(AVFrame **frame);
        int initAudioResampler(AVCodecContext *inputCodecContext,
                               AVCodecContext *outputCodecContext,
                               SwrContext **resampleContext);
        int initFifo(AVAudioFifo **fifo, AVCodecContext *outputCodecContext);
        int writeOutputMP3FileHeader(AVFormatContext *outputFormatContext);
        int decodeAACAudioFrame(AVFrame *frame,
                                AVFormatContext *inputFormatContext,
                                AVCodecContext *inputCodecContext,
                                int *dataPresent, int *finished);
        int initConvertedSamples(uint8_t ***convertedInputSamples,
                                 AVCodecContext *outputCodecContext,
                                 int frameSize);
        int convertSamples(const uint8_t **inputData,
                           uint8_t **convertedData, const int frameSize,
                           SwrContext *resampleContext);
        int addSamplesToFifo(AVAudioFifo *fifo,
                             uint8_t **convertedInputSamples,
                             const int frameSize);
        int readDecodeConvertAndStore(AVAudioFifo *fifo,
                                      AVFormatContext *inputFormatContext,
                                      AVCodecContext *inputCodecContext,
                                      AVCodecContext *outputCodecContext,
                                      SwrContext *resamplerContext,
                                      int *finished);
        int initOutputFrame(AVFrame **frame, AVCodecContext *outputCodecContext,
                            int frameSize);
        int encodeAudioFrame(AVFrame *frame,
                             AVFormatContext *outputFormatContext,
                             AVCodecContext *outputCodecContext,
                             int *dataPresent);
        int loadEncodeAndWriteFrame(AVAudioFifo *fifo,
                                    AVFormatContext *outputFormatContext,
                                    AVCodecContext *outputCodecContext);
        int writeOutputMP3FileTrailer(AVFormatContext *outputFormatContext);

        QString videoFilePath;
        QString mp3AudioFilePath;
        QString aacAudioFilePath;
};

#endif
