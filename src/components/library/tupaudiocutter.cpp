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

// The output bit rate in bit/s
#define OUTPUT_BIT_RATE 96000
// The number of output channels
#define OUTPUT_CHANNELS 2

TupAudioCutter::TupAudioCutter(const QString &inputFile, const QString &audioFile)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::TupAudioCutter()] - inputFile ->" << inputFile;
        qDebug() << "[TupAudioCutter::TupAudioCutter()] - audioFile ->" << audioFile;
    #endif

    videoFilePath = inputFile;
    mp3AudioFilePath = audioFile;
}

TupAudioCutter::~TupAudioCutter()
{
}

bool TupAudioCutter::startAACAudioExtraction()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - Extracting audio stream...";
    #endif

    QByteArray stringBytes = videoFilePath.toLocal8Bit();
    const char *inputVideoFilename = stringBytes.data();

    AVFormatContext *inputAACFormatContext = nullptr;
    if (avformat_open_input(&inputAACFormatContext, inputVideoFilename, nullptr, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Could not open video input file!";
        #endif

        return false;
    }

    if (avformat_find_stream_info(inputAACFormatContext, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Could not find stream information!";
        #endif

        return false;
    }

    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputAACFormatContext->nb_streams; i++) {
        if (inputAACFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Could not find an audio stream!";
        #endif

        return false;
    }

    aacAudioFilePath = QDir::tempPath() + "/aac_audio_" + TAlgorithm::randomString(12) + ".aac";
    stringBytes = aacAudioFilePath.toLocal8Bit();
    const char *outputAACFilename = stringBytes.data();

    AVFormatContext *outputFormatContext = nullptr;
    avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputAACFilename);
    if (!outputFormatContext) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Could not create output context!";
        #endif

        return false;
    }

    AVStream *outStream = avformat_new_stream(outputFormatContext, nullptr);
    if (!outStream) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Failed to allocate output stream!";
        #endif

        return false;
    }

    if (avcodec_parameters_copy(outStream->codecpar, inputAACFormatContext->streams[audioStreamIndex]->codecpar) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Failed to copy codec parameters!";
        #endif

        return false;
    }

    outStream->codecpar->codec_tag = 0;

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, outputAACFilename, AVIO_FLAG_WRITE) < 0) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                            "Fatal Error: Could not open output file!";
            #endif

            return false;
        }
    }

    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::startAACAudioExtraction()] - "
                        "Fatal Error: Error occurred when writing header!";
        #endif

        return false;
    }

    AVPacket packet;
    while (av_read_frame(inputAACFormatContext, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex) {
            packet.stream_index = outStream->index;
            av_interleaved_write_frame(outputFormatContext, &packet);
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(outputFormatContext);

    avformat_close_input(&inputAACFormatContext);
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outputFormatContext->pb);
    avformat_free_context(outputFormatContext);

    return true;
}

// Open an input file and the required decoder.
//   @param      filename             File to be opened
//   @param[out] input_format_context Format context of opened file
//   @param[out] input_codec_context  Codec context of opened file
//   @return Error code (0 if successful)

int TupAudioCutter::openInputAACAudioFile(const char *filename,
                           AVFormatContext **inputFormatContext,
                           AVCodecContext **inputCodecContext)
{
    AVCodecContext *avCodecContext;
    const AVCodec *inputCodec;
    const AVStream *stream;
    int error;

    // Open the input file to read from it
    if ((error = avformat_open_input(inputFormatContext, filename, nullptr,
                                     nullptr)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Could not open input file ->" << filename;
        #endif

        *inputFormatContext = nullptr;
        return error;
    }

    // Get information on the input file (number of streams, etc)
    if ((error = avformat_find_stream_info(*inputFormatContext, nullptr)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Could not open find stream info";
        #endif
        avformat_close_input(inputFormatContext);

        return error;
    }

    // Make sure that there is only one stream in the input file
    if ((*inputFormatContext)->nb_streams != 1) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Expected one audio input stream, but found ->"
                        << (*inputFormatContext)->nb_streams;
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR_EXIT;
    }

    stream = (*inputFormatContext)->streams[0];

    // Find a decoder for the audio stream
    if (!(inputCodec = avcodec_find_decoder(stream->codecpar->codec_id))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Could not find input codec";
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR_EXIT;
    }

    // Allocate a new decoding context
    avCodecContext = avcodec_alloc_context3(inputCodec);
    if (!avCodecContext) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Could not allocate a decoding context";
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR(ENOMEM);
    }

    // Initialize the stream parameters with demuxer information
    error = avcodec_parameters_to_context(avCodecContext, stream->codecpar);
    if (error < 0) {
        avformat_close_input(inputFormatContext);
        avcodec_free_context(&avCodecContext);

        return error;
    }

    // Open the decoder for the audio stream to use it later
    if ((error = avcodec_open2(avCodecContext, inputCodec, nullptr)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openInputAACAudioFile()] - "
                        "Fatal Error: Could not open input codec";
        #endif
        avcodec_free_context(&avCodecContext);
        avformat_close_input(inputFormatContext);

        return error;
    }

    // Set the packet timebase for the decoder
    avCodecContext->pkt_timebase = stream->time_base;

    // Save the decoder context for easier access later
    *inputCodecContext = avCodecContext;

    return 0;
}

// Open an output file and the required encoder
// Also set some basic encoder parameters
// Some of these parameters are based on the input file's parameters
//   @param      filename              File to be opened
//   @param      input_codec_context   Codec context of input file
//   @param[out] output_format_context Format context of output file
//   @param[out] output_codec_context  Codec context of output file
//   @return Error code (0 if successful)

int TupAudioCutter::openOutputMP3File(const char *filename,
                            AVCodecContext *inputCodecContext,
                            AVFormatContext **outputFormatContext,
                            AVCodecContext **outputCodecContext)
{
    AVCodecContext *avCodecContext = nullptr;
    AVIOContext *outputIOContext   = nullptr;
    AVStream *stream               = nullptr;
    const AVCodec *outputCodec     = nullptr;
    int error;

    // Open the output file to write to it
    if ((error = avio_open(&outputIOContext, filename,
                           AVIO_FLAG_WRITE)) < 0) {
        #ifdef TUP_DEBUG
                qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                            "Fatal Error: Could not open output file ->" << filename;
        #endif

        return error;
    }

    // Create a new format context for the output container format
    if (!(*outputFormatContext = avformat_alloc_context())) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not allocate output format context";
        #endif

        return AVERROR(ENOMEM);
    }

    // Associate the output file (pointer) with the container format context
    (*outputFormatContext)->pb = outputIOContext;

    // Guess the desired container format based on the file extension
    if (!((*outputFormatContext)->oformat = av_guess_format(nullptr, filename,
                                                              nullptr))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not find output file format";
        #endif

        goto cleanup;
    }

    if (!((*outputFormatContext)->url = av_strdup(filename))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not allocate url";
        #endif
        error = AVERROR(ENOMEM);

        goto cleanup;
    }

    // Find the encoder to be used by its name
    if (!(outputCodec = avcodec_find_encoder(AV_CODEC_ID_MP3))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not find an AAC encoder";
        #endif

        goto cleanup;
    }

    // Create a new audio stream in the output file container
    if (!(stream = avformat_new_stream(*outputFormatContext, nullptr))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not create new stream";
        #endif
        error = AVERROR(ENOMEM);

        goto cleanup;
    }

    avCodecContext = avcodec_alloc_context3(outputCodec);
    if (!avCodecContext) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not allocate an encoding context";
        #endif
        error = AVERROR(ENOMEM);

        goto cleanup;
    }

    // Set the basic encoder parameters.
    // The input file's sample rate is used to avoid a sample rate conversion
    av_channel_layout_default(&avCodecContext->ch_layout, OUTPUT_CHANNELS);
    avCodecContext->codec_id       = AV_CODEC_ID_MP3;
    avCodecContext->codec_type     = AVMEDIA_TYPE_AUDIO;
    avCodecContext->sample_rate    = inputCodecContext->sample_rate;
    avCodecContext->sample_fmt     = outputCodec->sample_fmts[0];
    avCodecContext->bit_rate       = OUTPUT_BIT_RATE;

    // Set the sample rate for the container
    stream->time_base.den = inputCodecContext->sample_rate;
    stream->time_base.num = 1;

    // Some container formats (like MP4) require global headers to be present
    // Mark the encoder so that it behaves accordingly
    if ((*outputFormatContext)->oformat->flags & AVFMT_GLOBALHEADER)
        avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Open the encoder for the audio stream to use it later
    if ((error = avcodec_open2(avCodecContext, outputCodec, nullptr)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not open output codec";
        #endif
        goto cleanup;
    }

    error = avcodec_parameters_from_context(stream->codecpar, avCodecContext);
    if (error < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::openOutputMP3File()] - "
                        "Fatal Error: Could not initialize stream parameters";
        #endif

        goto cleanup;
    }

    // Save the encoder context for easier access later
    *outputCodecContext = avCodecContext;

    return 0;

cleanup:
    avcodec_free_context(&avCodecContext);
    avio_closep(&(*outputFormatContext)->pb);
    avformat_free_context(*outputFormatContext);
    *outputFormatContext = nullptr;
    return error < 0 ? error : AVERROR_EXIT;
}

// Initialize one data packet for reading or writing.
//   @param[out] packet Packet to be initialized
//   @return Error code (0 if successful)

int TupAudioCutter::initPacket(AVPacket **packet)
{
    if (!(*packet = av_packet_alloc())) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initPacket()] - "
                        "Fatal Error: Could not allocate packet";
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Initialize one audio frame for reading from the input file.
//   @param[out] frame Frame to be initialized
//   @return Error code (0 if successful)

int TupAudioCutter::initInputAACFrame(AVFrame **frame)
{
    if (!(*frame = av_frame_alloc())) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initInputAACFrame()] - "
                        "Fatal Error: Could not allocate input frame";
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Initialize the audio resampler based on the input and output codec settings.
// If the input and output sample formats differ, a conversion is required
// libswresample takes care of this, but requires initialization
//   @param      input_codec_context  Codec context of the input file
//   @param      output_codec_context Codec context of the output file
//   @param[out] resample_context     Resample context for the required conversion
//   @return Error code (0 if successful)

int TupAudioCutter::initAudioResampler(AVCodecContext *inputCodecContext,
                          AVCodecContext *outputCodecContext,
                          SwrContext **resampleContext)
{
    int error;

    // Create a resampler context for the conversion
    // Set the conversion parameters
    error = swr_alloc_set_opts2(resampleContext,
                                &outputCodecContext->ch_layout,
                                outputCodecContext->sample_fmt,
                                outputCodecContext->sample_rate,
                                &inputCodecContext->ch_layout,
                                inputCodecContext->sample_fmt,
                                inputCodecContext->sample_rate,
                                0, nullptr);
    if (error < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initAudioResampler()] - "
                        "Fatal Error: Could not allocate resample context";
        #endif

        return error;
    }

    // Perform a sanity check so that the number of converted samples is
    // not greater than the number of samples to be converted
    // If the sample rates differ, this case has to be handled differently
    av_assert0(outputCodecContext->sample_rate == inputCodecContext->sample_rate);

    // Open the resampler with the specified parameters
    if ((error = swr_init(*resampleContext)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initAudioResampler()] - "
                        "Fatal Error: Could not open resample context";
        #endif
        swr_free(resampleContext);

        return error;
    }

    return 0;
}

// Initialize a FIFO buffer for the audio samples to be encoded.
//   @param[out] fifo                 Sample buffer
//   @param      output_codec_context Codec context of the output file
//   @return Error code (0 if successful)

int TupAudioCutter::initFifo(AVAudioFifo **fifo, AVCodecContext *outputCodecContext)
{
    // Create the FIFO buffer based on the specified output sample format
    if (!(*fifo = av_audio_fifo_alloc(outputCodecContext->sample_fmt,
                                      outputCodecContext->ch_layout.nb_channels, 1))) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initFifo()] - "
                        "Fatal Error: Could not allocate FIFO";
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Write the header of the output file container.
//   @param output_format_context Format context of the output file
//   @return Error code (0 if successful)

int TupAudioCutter::writeOutputMP3FileHeader(AVFormatContext *outputFormatContext)
{
    int error;
    if ((error = avformat_write_header(outputFormatContext, nullptr)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::writeOutputMP3FileHeader()] - "
                        "Fatal Error: Could not write output file header";
        #endif

        return error;
    }

    return 0;
}

// Decode one audio frame from the input file.
//   @param      frame                Audio frame to be decoded
//   @param      input_format_context Format context of the input file
//   @param      input_codec_context  Codec context of the input file
//   @param[out] data_present         Indicates whether data has been decoded
//   @param[out] finished             Indicates whether the end of file has
//                                  been reached and all data has been
//                                  decoded. If this flag is false, there
//                                  is more data to be decoded, i.e., this
//                                  function has to be called again
//   @return Error code (0 if successful)

int TupAudioCutter::decodeAACAudioFrame(AVFrame *frame,
                              AVFormatContext *inputFormatContext,
                              AVCodecContext *inputCodecContext,
                              int *dataPresent, int *finished)
{
    // Packet used for temporary storage
    AVPacket *inputPacket;
    int error;

    error = initPacket(&inputPacket);
    if (error < 0)
        return error;

    *dataPresent = 0;
    *finished = 0;
    // Read one audio frame from the input file into a temporary packet
    if ((error = av_read_frame(inputFormatContext, inputPacket)) < 0) {
        // If we are at the end of the file, flush the decoder below
        if (error == AVERROR_EOF)
            *finished = 1;
        else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupAudioCutter::decodeAACAudioFrame()] - "
                            "Fatal Error: Could not read frame";
            #endif

            goto cleanup;
        }
    }

    // Send the audio frame stored in the temporary packet to the decoder.
    // The input audio stream decoder is used to do this.
    if ((error = avcodec_send_packet(inputCodecContext, inputPacket)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::decodeAACAudioFrame()] - "
                        "Fatal Error: Could not send packet for decoding";
        #endif

        goto cleanup;
    }

    // Receive one frame from the decoder
    error = avcodec_receive_frame(inputCodecContext, frame);
    // If the decoder asks for more data to be able to decode a frame,
    // return indicating that no data is present
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
        // If the end of the input file is reached, stop decoding
    } else if (error == AVERROR_EOF) {
        *finished = 1;
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::decodeAACAudioFrame()] - "
                        "Fatal Error: Could not decode frame";
        #endif
        goto cleanup;
        // Default case: Return decoded data
    } else {
        *dataPresent = 1;
        goto cleanup;
    }

cleanup:
    av_packet_free(&inputPacket);
    return error;
}


// Initialize a temporary storage for the specified number of audio samples.
// The conversion requires temporary storage due to the different format.
// The number of audio samples to be allocated is specified in frame_size.
//   @param[out] converted_input_samples Array of converted samples. The
//                                       dimensions are reference, channel
//                                       (for multi-channel audio), sample.
//   @param      output_codec_context    Codec context of the output file
//   @param      frame_size              Number of samples to be converted in
//                                       each round
//   @return Error code (0 if successful)

int TupAudioCutter::initConvertedSamples(uint8_t ***convertedInputSamples,
                                  AVCodecContext *outputCodecContext,
                                  int frameSize)
{
    int error;

    // Allocate as many pointers as there are audio channels
    // Each pointer will point to the audio samples of the corresponding
    // channels (although it may be nullptr for interleaved formats)
    // Allocate memory for the samples of all channels in one consecutive
    // block for convenience
    if ((error = av_samples_alloc_array_and_samples(convertedInputSamples, nullptr,
                                                    outputCodecContext->ch_layout.nb_channels,
                                                    frameSize,
                                                    outputCodecContext->sample_fmt, 0)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initConvertedSamples()] - "
                        "Fatal Error: Could not allocate converted input samples";
        #endif

        return error;
    }
    return 0;
}

// Convert the input audio samples into the output sample format.
// The conversion happens on a per-frame basis, the size of which is
// specified by frame_size.
//   @param      input_data       Samples to be decoded. The dimensions are
//                                channel (for multi-channel audio), sample.
//   @param[out] converted_data   Converted samples. The dimensions are channel
//                                (for multi-channel audio), sample.
//   @param      frame_size       Number of samples to be converted
//   @param      resample_context Resample context for the conversion
//   @return Error code (0 if successful)

int TupAudioCutter::convertSamples(const uint8_t **inputData,
                           uint8_t **convertedData, const int frameSize,
                           SwrContext *resampleContext)
{
    int error;

    // Convert the samples using the resampler
    if ((error = swr_convert(resampleContext,
                             convertedData, frameSize,
                             inputData    , frameSize)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::convertSamples()] - "
                        "Fatal Error: Could not convert input samples";
        #endif

        return error;
    }

    return 0;
}

// Add converted input audio samples to the FIFO buffer for later processing.
//   @param fifo                    Buffer to add the samples to
//   @param converted_input_samples Samples to be added. The dimensions are channel
//                                  (for multi-channel audio), sample
//   @param frame_size              Number of samples to be converted
//   @return Error code (0 if successful)

int TupAudioCutter::addSamplesToFifo(AVAudioFifo *fifo,
                               uint8_t **convertedInputSamples,
                               const int frameSize)
{
    int error;

    // Make the FIFO as large as it needs to be to hold both,
    // the old and the new samples
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frameSize)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::addSamplesToFifo()] - "
                        "Fatal Error: Could not reallocate FIFO";
        #endif

        return error;
    }

    // Store the new samples in the FIFO buffer
    if (av_audio_fifo_write(fifo, (void **)convertedInputSamples,
                            frameSize) < frameSize) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::addSamplesToFifo()] - "
                        "Fatal Error: Could not write data to FIFO";
        #endif

        return AVERROR_EXIT;
    }

    return 0;
}

// Read one audio frame from the input file, decode, convert and store
// it in the FIFO buffer.
//   @param      fifo                 Buffer used for temporary storage
//   @param      input_format_context Format context of the input file
//   @param      input_codec_context  Codec context of the input file
//   @param      output_codec_context Codec context of the output file
//   @param      resampler_context    Resample context for the conversion
//   @param[out] finished             Indicates whether the end of file has
//                                    been reached and all data has been
//                                    decoded. If this flag is false,
//                                    there is more data to be decoded,
//                                    i.e., this function has to be called
//                                    again.
//   @return Error code (0 if successful)

int TupAudioCutter::readDecodeConvertAndStore(AVAudioFifo *fifo,
                                         AVFormatContext *inputFormatContext,
                                         AVCodecContext *inputCodecContext,
                                         AVCodecContext *outputCodecContext,
                                         SwrContext *resamplerContext,
                                         int *finished)
{
    // Temporary storage of the input samples of the frame read from the file
    AVFrame *inputFrame = nullptr;
    // Temporary storage for the converted input samples
    uint8_t **convertedInputSamples = nullptr;
    int dataPresent;
    int ret = AVERROR_EXIT;

    // Initialize temporary storage for one input frame
    if (initInputAACFrame(&inputFrame))
        goto cleanup;
    // Decode one frame worth of audio samples
    if (decodeAACAudioFrame(inputFrame, inputFormatContext,
                           inputCodecContext, &dataPresent, finished))
        goto cleanup;
    // If we are at the end of the file and there are no more samples
    // in the decoder which are delayed, we are actually finished
    // This must not be treated as an error
    if (*finished) {
        ret = 0;
        goto cleanup;
    }
    // If there is decoded data, convert and store it
    if (dataPresent) {
        // Initialize the temporary storage for the converted input samples
        if (initConvertedSamples(&convertedInputSamples, outputCodecContext,
                                   inputFrame->nb_samples))
            goto cleanup;

        // Convert the input samples to the desired output sample format
        // This requires a temporary storage provided by converted_input_samples
        if (convertSamples((const uint8_t**)inputFrame->extended_data, convertedInputSamples,
                            inputFrame->nb_samples, resamplerContext))
            goto cleanup;

        // Add the converted input samples to the FIFO buffer for later processing
        if (addSamplesToFifo(fifo, convertedInputSamples,
                                inputFrame->nb_samples))
            goto cleanup;
    }
    ret = 0;

cleanup:
    if (convertedInputSamples)
        av_freep(&convertedInputSamples[0]);
    av_freep(&convertedInputSamples);
    av_frame_free(&inputFrame);

    return ret;
}

// Initialize one input frame for writing to the output file.
// The frame will be exactly frame_size samples large.
//   @param[out] frame                Frame to be initialized
//   @param      output_codec_context Codec context of the output file
//   @param      frame_size           Size of the frame
//   @return Error code (0 if successful)

int TupAudioCutter::initOutputFrame(AVFrame **frame,
                             AVCodecContext *outputCodecContext,
                             int frameSize)
{
    int error;

    // Create a new frame to store the audio samples
    if (!(*frame = av_frame_alloc())) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initOutputFrame()] - "
                        "Fatal Error: Could not allocate output frame";
        #endif

        return AVERROR_EXIT;
    }

    // Set the frame's parameters, especially its size and format
    // av_frame_get_buffer needs this to allocate memory for the
    // audio samples of the frame
    // Default channel layouts based on the number of channels
    // are assumed for simplicity
    (*frame)->nb_samples     = frameSize;
    av_channel_layout_copy(&(*frame)->ch_layout, &outputCodecContext->ch_layout);
    (*frame)->format         = outputCodecContext->sample_fmt;
    (*frame)->sample_rate    = outputCodecContext->sample_rate;

    // Allocate the samples of the created frame. This call will make
    // sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::initOutputFrame()] - "
                        "Fatal Error: Could not allocate frame buffer";
        #endif
        av_frame_free(frame);

        return error;
    }

    return 0;
}

// Global timestamp for the audio frames
int64_t pts = 0;

// Encode one frame worth of audio to the output file.
//   @param      frame                 Samples to be encoded
//   @param      output_format_context Format context of the output file
//   @param      output_codec_context  Codec context of the output file
//   @param[out] data_present          Indicates whether data has been
//                                     encoded
//   @return Error code (0 if successful)

int TupAudioCutter::encodeAudioFrame(AVFrame *frame,
                              AVFormatContext *outputFormatContext,
                              AVCodecContext *outputCodecContext,
                              int *dataPresent)
{
    // Packet used for temporary storage
    AVPacket *outputPacket;
    int error;

    error = initPacket(&outputPacket);
    if (error < 0)
        return error;

    // Set a timestamp based on the sample rate for the container
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    *dataPresent = 0;
    // Send the audio frame stored in the temporary packet to the encoder
    // The output audio stream encoder is used to do this
    error = avcodec_send_frame(outputCodecContext, frame);
    // Check for errors, but proceed with fetching encoded samples if the
    // encoder signals that it has nothing more to encode
    if (error < 0 && error != AVERROR_EOF) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::encodeAudioFrame()] - "
                        "Fatal Error: Could not send packet for encoding";
        #endif

        goto cleanup;
    }

    // Receive one encoded frame from the encoder
    error = avcodec_receive_packet(outputCodecContext, outputPacket);
    // If the encoder asks for more data to be able to provide an
    // encoded frame, return indicating that no data is present
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
        // If the last frame has been encoded, stop encoding
    } else if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::encodeAudioFrame()] - "
                        "Fatal Error: Could not encode frame";
        #endif
        goto cleanup;
        // Default case: Return encoded data
    } else {
        *dataPresent = 1;
    }

    // Write one audio frame from the temporary packet to the output file
    if (*dataPresent &&
        (error = av_write_frame(outputFormatContext, outputPacket)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::encodeAudioFrame()] - "
                        "Fatal Error: Could not write frame";
        #endif

        goto cleanup;
    }

cleanup:
    av_packet_free(&outputPacket);
    return error;
}

// Load one audio frame from the FIFO buffer, encode and write it to the
// output file.
//   @param fifo                  Buffer used for temporary storage
//   @param output_format_context Format context of the output file
//   @param output_codec_context  Codec context of the output file
//   @return Error code (0 if successful)

int TupAudioCutter::loadEncodeAndWriteFrame(AVAudioFifo *fifo,
                                 AVFormatContext *outputFormatContext,
                                 AVCodecContext *outputCodecContext)
{
    // Temporary storage of the output samples of the frame written to the file
    AVFrame *outputFrame;
    // Use the maximum number of possible samples per frame
    // If there is less than the maximum possible frame size in the FIFO
    // buffer use this number. Otherwise, use the maximum possible frame size
    const int frameSize = FFMIN(av_audio_fifo_size(fifo),
                                 outputCodecContext->frame_size);
    int dataWritten;

    // Initialize temporary storage for one output frame
    if (initOutputFrame(&outputFrame, outputCodecContext, frameSize))
        return AVERROR_EXIT;

    // Read as many samples from the FIFO buffer as required to fill the frame
    // The samples are stored in the frame temporarily
    if (av_audio_fifo_read(fifo, (void **)outputFrame->data, frameSize) < frameSize) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::loadEncodeAndWriteFrame()] - "
                        "Fatal Error: Could not read data from FIFO";
        #endif
        av_frame_free(&outputFrame);

        return AVERROR_EXIT;
    }

    // Encode one frame worth of audio samples
    if (encodeAudioFrame(outputFrame, outputFormatContext,
                           outputCodecContext, &dataWritten)) {
        av_frame_free(&outputFrame);
        return AVERROR_EXIT;
    }
    av_frame_free(&outputFrame);

    return 0;
}

// Write the trailer of the output file container.
//   @param output_format_context Format context of the output file
//   @return Error code (0 if successful)

int TupAudioCutter::writeOutputMP3FileTrailer(AVFormatContext *outputFormatContext)
{
    int error;
    if ((error = av_write_trailer(outputFormatContext)) < 0) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::writeOutputMP3FileTrailer()] - "
                        "Fatal Error: Could not write output file trailer";
        #endif

        return error;
    }

    return 0;
}

bool TupAudioCutter::generateMP3Audio()
{
    if (startAACAudioExtraction()) {
        AVFormatContext *inputFormatContext = nullptr, *outputFormatContext = nullptr;
        AVCodecContext *inputCodecContext = nullptr, *outputCodecContext = nullptr;
        SwrContext *resampleContext = nullptr;
        AVAudioFifo *fifo = nullptr;
        int ret = AVERROR_EXIT;
        int frameCounter = 1;

        QByteArray stringBytes = aacAudioFilePath.toLocal8Bit();
        const char *aacFilePath = stringBytes.data();
        stringBytes = mp3AudioFilePath.toLocal8Bit();
        const char *mp3FilePath = stringBytes.data();

        // Open the AAC audio input file for reading
        if (openInputAACAudioFile(aacFilePath, &inputFormatContext,
                                  &inputCodecContext))
            goto cleanup;
        // Open the MP3 audio output file for writing
        if (openOutputMP3File(mp3FilePath, inputCodecContext,
                             &outputFormatContext, &outputCodecContext))
            goto cleanup;
        // Initialize the resampler to be able to convert audio sample formats
        if (initAudioResampler(inputCodecContext, outputCodecContext,
                               &resampleContext))
            goto cleanup;
        // Initialize the FIFO buffer to store audio samples to be encoded
        if (initFifo(&fifo, outputCodecContext))
            goto cleanup;
        // Write the header of the output file container
        if (writeOutputMP3FileHeader(outputFormatContext))
            goto cleanup;

        // Loop as long as we have input samples to read or output samples
        // to write; abort as soon as we have neither

        while (1) {
            emit audioExtracted(Audio, frameCounter);
            frameCounter++;

            // Use the encoder's desired frame size for processing
            const int output_frame_size = outputCodecContext->frame_size;
            int finished                = 0;

            // Make sure that there is one frame worth of samples in the FIFO
            // buffer so that the encoder can do its work.
            // Since the decoder's and the encoder's frame size may differ, we
            // need to FIFO buffer to store as many frames worth of input samples
            // that they make up at least one frame worth of output samples
            while (av_audio_fifo_size(fifo) < output_frame_size) {
            // Decode one frame worth of audio samples, convert it to the
            // output sample format and put it into the FIFO buffer
                if (readDecodeConvertAndStore(fifo, inputFormatContext,
                                              inputCodecContext,
                                              outputCodecContext,
                                              resampleContext, &finished))
                    goto cleanup;

                // If we are at the end of the input file, we continue
                // encoding the remaining audio samples to the output file
                if (finished)
                    break;
            }

            // If we have enough samples for the encoder, we encode them.
            // At the end of the file, we pass the remaining samples to
            // the encoder
            while (av_audio_fifo_size(fifo) >= output_frame_size ||
                   (finished && av_audio_fifo_size(fifo) > 0))
                // Take one frame worth of audio samples from the FIFO buffer,
                // encode it and write it to the output file
                if (loadEncodeAndWriteFrame(fifo, outputFormatContext,
                                            outputCodecContext))
                    goto cleanup;

            // If we are at the end of the input file and have encoded
            // all remaining samples, we can exit this loop and finish
            if (finished) {
                int data_written;
                // Flush the encoder as it may have delayed frames
                do {
                    if (encodeAudioFrame(nullptr, outputFormatContext,
                                         outputCodecContext, &data_written))
                        goto cleanup;
                } while (data_written);
                break;
            }
        }

        // Write the trailer of the output file container
        if (writeOutputMP3FileTrailer(outputFormatContext))
            goto cleanup;
        ret = 0;

    cleanup:
        if (fifo)
            av_audio_fifo_free(fifo);
        swr_free(&resampleContext);
        if (outputCodecContext)
            avcodec_free_context(&outputCodecContext);
        if (outputFormatContext) {
            avio_closep(&outputFormatContext->pb);
            avformat_free_context(outputFormatContext);
        }
        if (inputCodecContext)
            avcodec_free_context(&inputCodecContext);
        if (inputFormatContext)
            avformat_close_input(&inputFormatContext);

        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioCutter::generateMP3Audio()] - Process is done. Audio extracted!";
        #endif
        emit extractionIsDone(mp3AudioFilePath);

        QFile *audioFile = new QFile(aacAudioFilePath);
        if (audioFile->exists()) {
            if (!audioFile->remove()) {
                #ifdef TUP_DEBUG
                    qCritical() << "[TupAudioCutter::generateMP3Audio()] - "
                                   "Fatal Error: Can't remove temporary AAC file! ->" << aacAudioFilePath;
                #endif
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupAudioCutter::generateMP3Audio()] - "
                                "Temporary AAC file has been removed successfully! ->" << aacAudioFilePath;
                #endif
            }
        } else {
            #ifdef TUP_DEBUG
                qWarning() << "[TupAudioCutter::generateMP3Audio()] - "
                              "Warning: Temporary AAC file doesn't exist! ->" << aacAudioFilePath;
            #endif
        }
        audioFile->close();

        if (ret < 0)
            return false;

        return true;
    }

    return false;
}
