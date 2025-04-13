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

#include "tupaudiotranscoder.h"

// The output bit rate in bit/s
#define OUTPUT_BIT_RATE 96000
// The number of output channels
#define OUTPUT_CHANNELS 2

TupAudioTranscoder::TupAudioTranscoder(const QString &input, const QString &output)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::TupAudioTranscoder()]";
    #endif

    audioInputFile = input;
    audioOutputFile = output;
}

TupAudioTranscoder::~TupAudioTranscoder()
{
}

// Open an input file and the required decoder.
// @param      filename             File to be opened
// @param[out] inputFormatContext   Format context of opened file
// @param[out] inputCodecContext    Codec context of opened file
// @return     Error code (0 if successful)
int TupAudioTranscoder::openInputFile(AVFormatContext **inputFormatContext, AVCodecContext **inputCodecContext)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::openInputFile()]";
    #endif

    AVCodecContext *codecContext;
    const AVCodec *inputCodec;
    const AVStream *stream;
    int errorCode;

    QByteArray byteArray = audioInputFile.toLocal8Bit();
    const char *filename = byteArray.data();

    // Open the input file to read from it
    if ((errorCode = avformat_open_input(inputFormatContext, filename, nullptr, nullptr)) < 0) {
        errorMsg = "Fatal Error: Could not open input file " + audioInputFile;
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        *inputFormatContext = nullptr;

        return errorCode;
    }

    // Get information on the input file (number of streams etc.)
    if ((errorCode = avformat_find_stream_info(*inputFormatContext, nullptr)) < 0) {
        errorMsg = "Fatal Error: Could not open find stream info.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        avformat_close_input(inputFormatContext);

        return errorCode;
    }

    // Make sure that there is only one stream in the input file
    if ((*inputFormatContext)->nb_streams != 1) {
        errorMsg = "Fatal Error: Expected one audio input stream, but found "
                   + QString::number((*inputFormatContext)->nb_streams);
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR_EXIT;
    }

    stream = (*inputFormatContext)->streams[0];

    // Find a decoder for the audio stream
    if (!(inputCodec = avcodec_find_decoder(stream->codecpar->codec_id))) {
        errorMsg = "Fatal Error: Could not find input codec.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR_EXIT;
    }

    // Allocate a new decoding context
    codecContext = avcodec_alloc_context3(inputCodec);
    if (!codecContext) {
        errorMsg = "Fatal Error: Could not allocate a decoding context.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        avformat_close_input(inputFormatContext);

        return AVERROR(ENOMEM);
    }

    // Initialize the stream parameters with demuxer information
    errorCode = avcodec_parameters_to_context(codecContext, stream->codecpar);
    if (errorCode < 0) {
        avformat_close_input(inputFormatContext);
        avcodec_free_context(&codecContext);

        return errorCode;
    }

    // Open the decoder for the audio stream to use it later
    if ((errorCode = avcodec_open2(codecContext, inputCodec, nullptr)) < 0) {
        errorMsg = "Fatal Error: Could not open input codec.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openInputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        avcodec_free_context(&codecContext);
        avformat_close_input(inputFormatContext);

        return errorCode;
    }

    // Set the packet timebase for the decoder
    codecContext->pkt_timebase = stream->time_base;

    // Save the decoder context for easier access later
    *inputCodecContext = codecContext;

    return 0;
}

// Open an output file and the required encoder.
// Also set some basic encoder parameters.
// Some of these parameters are based on the input file's parameters.
// @param      filename              File to be opened
// @param      inputCodecContext     Codec context of input file
// @param[out] outputFormatContext   Format context of output file
// @param[out] outputCodecContext    Codec context of output file
// @return     Error code (0 if successful)
int TupAudioTranscoder::openOutputFile(AVCodecContext *inputCodecContext,
                                       AVFormatContext **outputFormatContext,
                                       AVCodecContext **outputCodecContext)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::openOutputFile()]";
    #endif

    AVCodecContext *codecContext   = nullptr;
    AVIOContext *outputIOContext   = nullptr;
    AVStream *stream               = nullptr;
    const AVCodec *outputCodec     = nullptr;
    int errorCode;

    QByteArray byteArray = audioOutputFile.toLocal8Bit();
    const char *filename = byteArray.data();

    // Open the output file to write to it
    if ((errorCode = avio_open(&outputIOContext, filename,
                           AVIO_FLAG_WRITE)) < 0) {
        errorMsg = "Fatal Error: Could not open output file " + audioOutputFile;
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    // Create a new format context for the output container format
    if (!(*outputFormatContext = avformat_alloc_context())) {
        errorMsg = "Fatal Error: Could not allocate output format context.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return AVERROR(ENOMEM);
    }

    // Associate the output file (pointer) with the container format context
    (*outputFormatContext)->pb = outputIOContext;

    // Guess the desired container format based on the file extension
    if (!((*outputFormatContext)->oformat = av_guess_format(nullptr, filename, nullptr))) {
        errorMsg = "Fatal Error: Could not find output file format.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    if (!((*outputFormatContext)->url = av_strdup(filename))) {
        errorMsg = "Fatal Error: Could not allocate url.";
        errorCode = AVERROR(ENOMEM);
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    // Find the encoder to be used by its name
    if (!(outputCodec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
        errorMsg = "Fatal Error: Could not find an AAC encoder.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
        #endif
        goto cleanup;
    }

    // Create a new audio stream in the output file container
    if (!(stream = avformat_new_stream(*outputFormatContext, nullptr))) {
        errorMsg = "Fatal Error: Could not create new stream.";
        errorCode = AVERROR(ENOMEM);
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
        #endif
        goto cleanup;
    }

    codecContext = avcodec_alloc_context3(outputCodec);
    if (!codecContext) {
        errorMsg = "Fatal Error: Could not allocate an encoding context.";
        errorCode = AVERROR(ENOMEM);
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    //  Set the basic encoder parameters
    //  The input file's sample rate is used to avoid a sample rate conversion
    av_channel_layout_default(&codecContext->ch_layout, OUTPUT_CHANNELS);
    codecContext->sample_rate    = inputCodecContext->sample_rate;
    codecContext->sample_fmt     = outputCodec->sample_fmts[0];
    codecContext->bit_rate       = OUTPUT_BIT_RATE;

    //  Set the sample rate for the container
    stream->time_base.den = inputCodecContext->sample_rate;
    stream->time_base.num = 1;

    //  Some container formats (like MP4) require global headers to be present
    //  Mark the encoder so that it behaves accordingly
    if ((*outputFormatContext)->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //  Open the encoder for the audio stream to use it later
    if ((errorCode = avcodec_open2(codecContext, outputCodec, nullptr)) < 0) {
        errorMsg = "Fatal Error: Could not open output codec.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    errorCode = avcodec_parameters_from_context(stream->codecpar, codecContext);
    if (errorCode < 0) {
        errorMsg = "Fatal Error: Could not initialize stream parameters.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::openOutputFile()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    //  Save the encoder context for easier access later
    *outputCodecContext = codecContext;

    return 0;

    cleanup:
        avcodec_free_context(&codecContext);
        avio_closep(&(*outputFormatContext)->pb);
        avformat_free_context(*outputFormatContext);
        *outputFormatContext = nullptr;

    return errorCode < 0 ? errorCode : AVERROR_EXIT;
}

// Initialize one data packet for reading or writing.
// @param[out] packet Packet to be initialized
// @return Error code (0 if successful)
int TupAudioTranscoder::initPacket(AVPacket **packet)
{
    if (!(*packet = av_packet_alloc())) {
        errorMsg = "Fatal Error: Could not allocate packet.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initPacket()] - " << errorMsg;
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Initialize one audio frame for reading from the input file.
// @param[out] frame Frame to be initialized
// @return Error code (0 if successful)
int TupAudioTranscoder::initInputFrame(AVFrame **frame)
{
    if (!(*frame = av_frame_alloc())) {
        errorMsg = "Fatal Error: Could not allocate input frame.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initInputFrame()] - " << errorMsg;
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Initialize the audio resampler based on the input and output codec settings.
// If the input and output sample formats differ, a conversion is required
// libswresample takes care of this, but requires initialization.
// @param        inputCodecContext    Codec context of the input file
// @param        outputCodecContext   Codec context of the output file
// @param[out]   resampleContext      Resample context for the required conversion
// @return       Error code (0 if successful)
int TupAudioTranscoder::initResampler(AVCodecContext *inputCodecContext, AVCodecContext *outputCodecContext,
                                      SwrContext **resampleContext)
{
    int errorCode;

    //  Create a resampler context for the conversion.
    //  Set the conversion parameters.
    errorCode = swr_alloc_set_opts2(resampleContext,
                                         &outputCodecContext->ch_layout,
                                          outputCodecContext->sample_fmt,
                                          outputCodecContext->sample_rate,
                                         &inputCodecContext->ch_layout,
                                          inputCodecContext->sample_fmt,
                                          inputCodecContext->sample_rate,
                                          0, nullptr);
    if (errorCode < 0) {
        errorMsg = "Fatal Error: Could not allocate resample context.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initResampler()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    //  Perform a sanity check so that the number of converted samples is
    //  not greater than the number of samples to be converted.
    //  If the sample rates differ, this case has to be handled differently
    av_assert0(outputCodecContext->sample_rate == inputCodecContext->sample_rate);

    //  Open the resampler with the specified parameters.
    if ((errorCode = swr_init(*resampleContext)) < 0) {
        errorMsg = "Fatal Error: Could not open resample context.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initResampler()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        swr_free(resampleContext);

        return errorCode;
    }

    return 0;
}

// Initialize a FIFO buffer for the audio samples to be encoded.
// @param[out] fifo                 Sample buffer
// @param      outputCodecContext Codec context of the output file
// @return Error code (0 if successful)
int TupAudioTranscoder::initFifo(AVAudioFifo **fifo, AVCodecContext *outputCodecContext)
{
    // Create the FIFO buffer based on the specified output sample format
    if (!(*fifo = av_audio_fifo_alloc(outputCodecContext->sample_fmt,
                                      outputCodecContext->ch_layout.nb_channels, 1))) {
        errorMsg = "Fatal Error: Could not allocate FIFO.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initFifo()] - " << errorMsg;
        #endif

        return AVERROR(ENOMEM);
    }

    return 0;
}

// Write the header of the output file container.
// @param   outputFormatContext   Format context of the output file
// @return  Error code (0 if successful)
int TupAudioTranscoder::writeOutputFileHeader(AVFormatContext *outputFormatContext)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::writeOutputFileHeader()]";
    #endif

    int errorCode;
    if ((errorCode = avformat_write_header(outputFormatContext, nullptr)) < 0) {
        errorMsg = "Fatal Error: Could not write output file header.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::writeOutputFileHeader()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    return 0;
}

// Decode one audio frame from the input file.
// @param      frame                Audio frame to be decoded
// @param      inputFormatContext   Format context of the input file
// @param      inputCodecContext    Codec context of the input file
// @param[out] dataPresent          Indicates whether data has been decoded
// @param[out] finished             Indicates whether the end of file has
//                                  been reached and all data has been
//                                  decoded. If this flag is false, there
//                                  is more data to be decoded, i.e., this
//                                  function has to be called again.
// @return Error code (0 if successful)
int TupAudioTranscoder::decodeAudioFrame(AVFrame *frame,
                              AVFormatContext *inputFormatContext,
                              AVCodecContext *inputCodecContext,
                              int *dataPresent, int *finished)
{
    //  Packet used for temporary storage.
    AVPacket *inputPacket;
    int errorCode;

    errorCode = initPacket(&inputPacket);
    if (errorCode < 0)
        return errorCode;

    *dataPresent = 0;
    *finished = 0;
    // Read one audio frame from the input file into a temporary packet
    if ((errorCode = av_read_frame(inputFormatContext, inputPacket)) < 0) {
        // If we are at the end of the file, flush the decoder below
        if (errorCode == AVERROR_EOF)
            *finished = 1;
        else {
            errorMsg = "Fatal Error: Could not read frame.";
            #ifdef TUP_DEBUG
                qCritical() << "[TupAudioTranscoder::decodeAudioFrame()] - " << errorMsg;
                qCritical() << "ERROR CODE -> " << errorCode;
            #endif
            goto cleanup;
        }
    }

    // Send the audio frame stored in the temporary packet to the decoder
    // The input audio stream decoder is used to do this
    if ((errorCode = avcodec_send_packet(inputCodecContext, inputPacket)) < 0) {
        errorMsg = "Fatal Error: Could not send packet for decoding.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::decodeAudioFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    // Receive one frame from the decoder
    errorCode = avcodec_receive_frame(inputCodecContext, frame);
    // If the decoder asks for more data to be able to decode a frame
    // return indicating that no data is present
    if (errorCode == AVERROR(EAGAIN)) {
        errorCode = 0;
        goto cleanup;
    // If the end of the input file is reached, stop decoding
    } else if (errorCode == AVERROR_EOF) {
        *finished = 1;
        errorCode = 0;
        goto cleanup;
    } else if (errorCode < 0) {
        errorMsg = "Fatal Error: Could not decode frame.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::decodeAudioFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    // Default case: Return decoded data
    } else {
        *dataPresent = 1;
        goto cleanup;
    }

    cleanup:
        av_packet_free(&inputPacket);

    return errorCode;
}

// Initialize a temporary storage for the specified number of audio samples.
// The conversion requires temporary storage due to the different format.
// The number of audio samples to be allocated is specified in frameSize.
// @param[out] convertedInputSamples   Array of converted samples. The
//                                     dimensions are reference, channel
//                                     (for multi-channel audio), sample.
// @param      outputCodecContext      Codec context of the output file
// @param      frameSize               Number of samples to be converted in
//                                     each round
// @return Error code (0 if successful)
int TupAudioTranscoder::initConvertedSamples(uint8_t ***convertedInputSamples,
                                  AVCodecContext *ouputCodecContext,
                                  int frameSize)
{
    int errorCode;

    // Allocate as many pointers as there are audio channels
    // Each pointer will point to the audio samples of the corresponding
    // channels (although it may be NULL for interleaved formats)
    // Allocate memory for the samples of all channels in one consecutive
    // block for convenience
    if ((errorCode = av_samples_alloc_array_and_samples(convertedInputSamples, nullptr,
                                  ouputCodecContext->ch_layout.nb_channels,
                                  frameSize,
                                  ouputCodecContext->sample_fmt, 0)) < 0) {
        errorMsg = "Fatal Error: Could not allocate converted input samples.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initConvertedSamples()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    return 0;
}

// Convert the input audio samples into the output sample format.
// The conversion happens on a per-frame basis, the size of which is
// specified by frameSize.
// @param      inputData        Samples to be decoded. The dimensions are
//                              channel (for multi-channel audio), sample.
// @param[out] convertedData    Converted samples. The dimensions are channel
//                              (for multi-channel audio), sample.
// @param      frameSize        Number of samples to be converted
// @param      resampleContext  Resample context for the conversion
// @return Error code (0 if successful)
int TupAudioTranscoder::convertSamples(const uint8_t **inputData,
                           uint8_t **convertedData, const int frameSize,
                           SwrContext *resampleContext)
{
    int errorCode;

    // Convert the samples using the resampler
    if ((errorCode = swr_convert(resampleContext,
                             convertedData, frameSize,
                             inputData, frameSize)) < 0) {
        errorMsg = "Fatal Error: Could not convert input samples.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::convertSamples()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    return 0;
}

// Add converted input audio samples to the FIFO buffer for later processing.
// @param  fifo                    Buffer to add the samples to
// @param  convertedInputSamples   Samples to be added. The dimensions are channel
//                                 (for multi-channel audio), sample.
// @param  frameSize               Number of samples to be converted
// @return Error code (0 if successful)
int TupAudioTranscoder::addSamplesToFifo(AVAudioFifo *fifo,
                               uint8_t **convertedInputSamples,
                               const int frameSize)
{
    int errorCode;

    // Make the FIFO as large as it needs to be to hold both,
    // the old and the new samples. */
    if ((errorCode = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frameSize)) < 0) {
        errorMsg = "Fatal Error: Could not reallocate FIFO.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::addSamplesToFifo()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    // Store the new samples in the FIFO buffer
    if (av_audio_fifo_write(fifo, (void **)convertedInputSamples,
                            frameSize) < frameSize) {
        errorMsg = "Fatal Error: Could not write data to FIFO.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::addSamplesToFifo()] - " << errorMsg;
        #endif

        return AVERROR_EXIT;
    }

    return 0;
}

// Read one audio frame from the input file, decode, convert and store
// it in the FIFO buffer.
// @param      fifo                 Buffer used for temporary storage
// @param      inputFormatContext   Format context of the input file
// @param      inputCodecContext    Codec context of the input file
// @param      outputCodecContext   Codec context of the output file
// @param      resamplerContext     Resample context for the conversion
// @param[out] finished             Indicates whether the end of file has
//                                  been reached and all data has been
//                                  decoded. If this flag is false,
//                                  there is more data to be decoded,
//                                  i.e., this function has to be called
//                                  again.
// @return Error code (0 if successful)
int TupAudioTranscoder::readDecodeConvertAndStore(AVAudioFifo *fifo,
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
    int response = AVERROR_EXIT;

    // Initialize temporary storage for one input frame
    if (initInputFrame(&inputFrame))
        goto cleanup;
    // Decode one frame worth of audio samples
    if (decodeAudioFrame(inputFrame, inputFormatContext, inputCodecContext, &dataPresent, finished))
        goto cleanup;
    // If we are at the end of the file and there are no more samples
    // in the decoder which are delayed, we are actually finished
    // This must not be treated as an error
    if (*finished) {
        response = 0;
        goto cleanup;
    }
    // If there is decoded data, convert and store it
    if (dataPresent) {
        // Initialize the temporary storage for the converted input samples
        if (initConvertedSamples(&convertedInputSamples, outputCodecContext,
                                   inputFrame->nb_samples))
            goto cleanup;

        //  Convert the input samples to the desired output sample format
        //  This requires a temporary storage provided by converted_input_samples
        if (convertSamples((const uint8_t**)inputFrame->extended_data, convertedInputSamples,
                            inputFrame->nb_samples, resamplerContext))
            goto cleanup;

        //  Add the converted input samples to the FIFO buffer for later processing
        if (addSamplesToFifo(fifo, convertedInputSamples, inputFrame->nb_samples))
            goto cleanup;
    }

    response = 0;

    cleanup:
        if (convertedInputSamples)
            av_freep(&convertedInputSamples[0]);
        av_freep(&convertedInputSamples);
        av_frame_free(&inputFrame);

    return response;
}

// Initialize one input frame for writing to the output file
// The frame will be exactly frameSize samples large
// @param[out] frame                Frame to be initialized
// @param      outputCodecContext   Codec context of the output file
// @param      frameSize            Size of the frame
// @return Error code (0 if successful)
int TupAudioTranscoder::initOutputFrame(AVFrame **frame,
                             AVCodecContext *outputCodecContext,
                             int frameSize)
{
    int errorCode;

    //  Create a new frame to store the audio samples
    if (!(*frame = av_frame_alloc())) {        
        errorMsg = "Fatal Error: Could not allocate output frame.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initOutputFrame()] - " << errorMsg;
        #endif

        return AVERROR_EXIT;
    }

    //  Set the frame's parameters, especially its size and format
    //  av_frame_get_buffer needs this to allocate memory for the
    //  audio samples of the frame
    //  Default channel layouts based on the number of channels
    //  are assumed for simplicity
    (*frame)->nb_samples     = frameSize;
    av_channel_layout_copy(&(*frame)->ch_layout, &outputCodecContext->ch_layout);
    (*frame)->format         = outputCodecContext->sample_fmt;
    (*frame)->sample_rate    = outputCodecContext->sample_rate;

    //  Allocate the samples of the created frame. This call will make
    //  sure that the audio frame can hold as many samples as specified
    if ((errorCode = av_frame_get_buffer(*frame, 0)) < 0) {
        errorMsg = "Fatal Error: Could not allocate output frame samples";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::initOutputFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        av_frame_free(frame);

        return errorCode;
    }

    return 0;
}

// Global timestamp for the audio frames
int64_t pts = 0;

// Encode one frame worth of audio to the output file.
// @param      frame                 Samples to be encoded
// @param      outputFormatContext   Format context of the output file
// @param      outputCodecContext    Codec context of the output file
// @param[out] dataPresent           Indicates whether data has been
//                                   encoded
// @return Error code (0 if successful)
int TupAudioTranscoder::encodeAudioFrame(AVFrame *frame,
                              AVFormatContext *outputFormatContext,
                              AVCodecContext *outputCodecContext,
                              int *dataPresent)
{
    // Packet used for temporary storage
    AVPacket *outputPacket;
    int errorCode;

    errorCode = initPacket(&outputPacket);
    if (errorCode < 0)
        return errorCode;

    // Set a timestamp based on the sample rate for the container
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    *dataPresent = 0;
    // Send the audio frame stored in the temporary packet to the encoder
    // The output audio stream encoder is used to do this
    errorCode = avcodec_send_frame(outputCodecContext, frame);
    // Check for errors, but proceed with fetching encoded samples if the
    // encoder signals that it has nothing more to encode
    if (errorCode < 0 && errorCode != AVERROR_EOF) {
        errorMsg = "Fatal Error: Could not send packet for encoding.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::encodeAudioFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    // Receive one encoded frame from the encoder
    errorCode = avcodec_receive_packet(outputCodecContext, outputPacket);
    // If the encoder asks for more data to be able to provide an
    // encoded frame, return indicating that no data is present
    if (errorCode == AVERROR(EAGAIN)) {
        errorCode = 0;
        goto cleanup;
    // If the last frame has been encoded, stop encoding
    } else if (errorCode == AVERROR_EOF) {
        errorCode = 0;
        goto cleanup;
    } else if (errorCode < 0) {
        errorMsg = "Fatal Error: Could not encode frame.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::encodeAudioFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    // Default case: Return encoded data
    } else {
        *dataPresent = 1;
    }

    // Write one audio frame from the temporary packet to the output file
    if (*dataPresent && (errorCode = av_write_frame(outputFormatContext, outputPacket)) < 0) {
        errorMsg = "Fatal Error: Could not write frame.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::encodeAudioFrame()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif
        goto cleanup;
    }

    cleanup:
        av_packet_free(&outputPacket);

    return errorCode;
}

// Load one audio frame from the FIFO buffer, encode and write it to the
// output file.
// @param fifo                  Buffer used for temporary storage
// @param outputFormatContext   Format context of the output file
// @param outputCodecContext    Codec context of the output file
// @return Error code (0 if successful)
int TupAudioTranscoder::loadEncodeAndWrite(AVAudioFifo *fifo,
                                           AVFormatContext *outputFormatContext,
                                           AVCodecContext *outputCodecContext)
{
    //  Temporary storage of the output samples of the frame written to the file
    AVFrame *outputFrame;
    //  Use the maximum number of possible samples per frame
    //  If there is less than the maximum possible frame size in the FIFO
    //  buffer use this number. Otherwise, use the maximum possible frame size
    const int frameSize = FFMIN(av_audio_fifo_size(fifo),
                                outputCodecContext->frame_size);
    int dataWritten;

    //  Initialize temporary storage for one output frame
    if (initOutputFrame(&outputFrame, outputCodecContext, frameSize))
        return AVERROR_EXIT;

    //  Read as many samples from the FIFO buffer as required to fill the frame
    //  The samples are stored in the frame temporarily
    if (av_audio_fifo_read(fifo, (void **)outputFrame->data, frameSize) < frameSize) {
        errorMsg = "Fatal Error: Could not read data from FIFO.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::loadEncodeAndWrite()] - " << errorMsg;
        #endif
        av_frame_free(&outputFrame);

        return AVERROR_EXIT;
    }

    //  Encode one frame worth of audio samples
    if (encodeAudioFrame(outputFrame, outputFormatContext,
                           outputCodecContext, &dataWritten)) {
        av_frame_free(&outputFrame);

        return AVERROR_EXIT;
    }
    av_frame_free(&outputFrame);

    return 0;
}

// Write the trailer of the output file container
// @param    outputFormatContext    Format context of the output file
// @return   Error code (0 if successful)
int TupAudioTranscoder::writeOutputFileTrailer(AVFormatContext *outputFormatContext)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::writeOutputFileTrailer()]";
    #endif

    int errorCode;
    if ((errorCode = av_write_trailer(outputFormatContext)) < 0) {
        errorMsg = "Fatal Error: Could not write output file trailer.";
        #ifdef TUP_DEBUG
            qCritical() << "[TupAudioTranscoder::writeOutputFileTrailer()] - " << errorMsg;
            qCritical() << "ERROR CODE -> " << errorCode;
        #endif

        return errorCode;
    }

    return 0;
}

int TupAudioTranscoder::processAudio()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupAudioTranscoder::processAudio()]";
    #endif

    AVFormatContext *inputFormatContext = nullptr, *outputFormatContext = nullptr;
    AVCodecContext *inputCodecContext = nullptr, *outputCodecContext = nullptr;
    SwrContext *resampleContext = nullptr;
    AVAudioFifo *fifo = nullptr;
    int response = AVERROR_EXIT;
    int percent = 0;

    // Open the input file for reading
    if (openInputFile(&inputFormatContext, &inputCodecContext))
        goto cleanup;
    // Open the output file for writing
    if (openOutputFile(inputCodecContext, &outputFormatContext, &outputCodecContext))
        goto cleanup;
    // Initialize the resampler to be able to convert audio sample formats
    if (initResampler(inputCodecContext, outputCodecContext, &resampleContext))
        goto cleanup;
    // Initialize the FIFO buffer to store audio samples to be encoded
    if (initFifo(&fifo, outputCodecContext))
        goto cleanup;
    //  Write the header of the output file container
    if (writeOutputFileHeader(outputFormatContext))
        goto cleanup;

    //  Loop as long as we have input samples to read or output samples
    //  to write; abort as soon as we have neither
    while (1) {
        #ifdef TUP_DEBUG
            qDebug() << "[TupAudioTranscoder::processAudio()] - Progress -> " << percent << "%";
        #endif
        emit progressChanged(percent);
        percent++;

        //  Use the encoder's desired frame size for processing
        const int outputFrameSize = outputCodecContext->frame_size;
        int finished              = 0;

        //  Make sure that there is one frame worth of samples in the FIFO
        //  buffer so that the encoder can do its work
        //  Since the decoder's and the encoder's frame size may differ, we
        //  need to FIFO buffer to store as many frames worth of input samples
        //  that they make up at least one frame worth of output samples
        while (av_audio_fifo_size(fifo) < outputFrameSize) {
            //  Decode one frame worth of audio samples, convert it to the
            //  output sample format and put it into the FIFO buffer
            if (readDecodeConvertAndStore(fifo, inputFormatContext,
                                              inputCodecContext,
                                              outputCodecContext,
                                              resampleContext, &finished))
                goto cleanup;

            //  If we are at the end of the input file, we continue
            //  encoding the remaining audio samples to the output file
            if (finished)
                break;
        }

        //  If we have enough samples for the encoder, we encode them
        //  At the end of the file, we pass the remaining samples to
        //  the encoder
        while (av_audio_fifo_size(fifo) >= outputFrameSize ||
               (finished && av_audio_fifo_size(fifo) > 0))
            // Take one frame worth of audio samples from the FIFO buffer
            // encode it and write it to the output file
            if (loadEncodeAndWrite(fifo, outputFormatContext,
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

    //  Write the trailer of the output file container
    if (writeOutputFileTrailer(outputFormatContext))
        goto cleanup;

    response = 0;

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

    return response;
}

QString TupAudioTranscoder::getErrorMsg() const
{
    QString errorDetail = "It seems there was an internal error while exporting your animation.<br/>"
                          "Please, contact our technical support team.<br/>"
                          "More info: <b>https://tupitube.com</b>";
    return errorDetail;
}
