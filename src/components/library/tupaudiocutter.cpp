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

    AVFormatContext *inputFormatContext = NULL;
    if (avformat_open_input(&inputFormatContext, inputFilename, NULL, NULL) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not open video input file!";
        #endif
        return false;
    }

    if (avformat_find_stream_info(inputFormatContext, NULL) < 0) {
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

    AVFormatContext *output_format_context = NULL;
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, outputFilename);
    if (!output_format_context) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not create output context!";
        #endif
        return false;
    }

    AVStream *out_stream = avformat_new_stream(output_format_context, NULL);
    if (!out_stream) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Failed to allocate output stream!";
        #endif
        return false;
    }

    if (avcodec_parameters_copy(out_stream->codecpar, inputFormatContext->streams[audioStreamIndex]->codecpar) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Failed to copy codec parameters!";
        #endif
        return false;
    }

    out_stream->codecpar->codec_tag = 0;

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, outputFilename, AVIO_FLAG_WRITE) < 0) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Could not open output file!";
            #endif
            return false;
        }
    }

    if (avformat_write_header(output_format_context, NULL) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startExtraction()] - Fatal Error: Error occurred when writing header!";
        #endif
        return false;
    }

    AVPacket packet;
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex) {
            packet.stream_index = out_stream->index;
            av_interleaved_write_frame(output_format_context, &packet);
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(output_format_context);

    avformat_close_input(&inputFormatContext);
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);

    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::startExtraction()] - Process is done. Audio extracted!";
    #endif
    emit extractionIsDone(audioFilePath);

    return true;
}
