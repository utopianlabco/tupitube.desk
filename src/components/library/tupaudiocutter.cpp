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

#include "tupaudiocutter.h"

TupAudioCutter::TupAudioCutter(const QString &inputFile, const QString &audioFile)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::TupAudioCutter()] - inputFile ->" << inputFile;
        qDebug() << "[TupAudioCutter::TupAudioCutter()] - audioFile ->" << audioFile;
    #endif

    videoFilePath = inputFile;
    audioFilePath = audioFile;
}

TupAudioCutter::~TupAudioCutter()
{
}

bool TupAudioCutter::startExtraction()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::startExtraction()] - Extracting audio stream...";
    #endif

    QByteArray bytes = videoFilePath.toLocal8Bit();
    const char *inputFilename = bytes.data();

    bytes = audioFilePath.toLocal8Bit();
    const char *outputFilename = bytes.data();

    AVFormatContext *inputFormatContext = nullptr;
    if (avformat_open_input(&inputFormatContext, inputFilename, nullptr, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not open video input file!";
        #endif
        return false;
    }

    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not find stream information!";
        #endif
        return false;
    }

    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not find an audio stream!";
        #endif
        return false;
    }

    AVFormatContext *outputFormatContext = nullptr;
    avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilename);
    if (!outputFormatContext) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not create output context!";
        #endif
        return false;
    }

    AVStream *outStream = avformat_new_stream(outputFormatContext, nullptr);
    if (!outStream) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Failed to allocate output stream!";
        #endif
        return false;
    }

    if (avcodec_parameters_copy(outStream->codecpar, inputFormatContext->streams[audioStreamIndex]->codecpar) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Failed to copy codec parameters!";
        #endif
        return false;
    }

    outStream->codecpar->codec_tag = 0;

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, outputFilename, AVIO_FLAG_WRITE) < 0) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not open output file!";
            #endif
            return false;
        }
    }

    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Error occurred when writing header!";
        #endif
        return false;
    }

    AVPacket packet;
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex) {
            packet.stream_index = outStream->index;
            av_interleaved_write_frame(outputFormatContext, &packet);
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(outputFormatContext);

    avformat_close_input(&inputFormatContext);
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outputFormatContext->pb);
    avformat_free_context(outputFormatContext);

    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::startExtraction()] - Process is done. Audio extracted!";
    #endif
    emit extractionIsDone(audioFilePath);

    return true;
}
