#include "AudioHandler.h"

const char* AudioHandler::GetDefaultAudioInputDeviceName() {
    // FIXME::Make it dynamic not like currently hard-coded
    return "audio=Microphone (Arctis 5 Chat)";
}

const char* AudioHandler::GetDefaultAudioOutputDeviceName() {
    // FIXME::Make it dynamic not like currently hard-coded
    return "audio=Headphones (Arctis 5 Game)";
}

int AudioHandler::CaptureAudioInput() {
    avdevice_register_all(); // Get all audio devices

    // Set up the input format context for the audio device
    AVFormatContext* input_format_context = nullptr;

    // Choose the input format based on your platform
    const AVInputFormat* input_format = av_find_input_format("dshow");
    if (!input_format) {
        printf("Could not find input format\n");
        return -1;
    }

    const char* device_name = GetDefaultAudioInputDeviceName();

    // Open the input device
    AVDictionary* options = nullptr;

    if (avformat_open_input(&input_format_context, device_name, input_format, &options) != 0) {
        printf("Could not open input device\n");
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(input_format_context, nullptr) < 0) {
        printf("Could not find stream info\n");
        avformat_close_input(&input_format_context);
        return -1;
    }

    // Set up the output format context for the output file
    AVFormatContext* output_format_context = nullptr;
    const char* output_filename = "output_INPUT.wav";

    if (avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, output_filename) < 0) {
        printf("Could not create output context\n");
        avformat_close_input(&input_format_context);
        return -1;
    }

    // Find the audio stream index and copy codec parameters
    int audio_stream_index = -1;
    for (unsigned int i = 0; i < input_format_context->nb_streams; i++) {
        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        printf("Could not find audio stream index\n");
        avformat_close_input(&input_format_context);
        avformat_free_context(output_format_context);
        return -1;
    }

    AVStream* in_stream = input_format_context->streams[audio_stream_index];
    AVStream* out_stream = avformat_new_stream(output_format_context, nullptr);
    if (!out_stream) {
        printf("Failed to allocate output stream\n");
        avformat_close_input(&input_format_context);
        avformat_free_context(output_format_context);
        return -1;
    }

    // Copy the codec parameters from the input to the output
    if (avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) < 0) {
        printf("Failed to copy codec parameters\n");
        avformat_close_input(&input_format_context);
        avformat_free_context(output_format_context);
        return -1;
    }
    out_stream->codecpar->codec_tag = 0;

    // Open the output file for writing
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            printf("Could not open output file\n");
            avformat_close_input(&input_format_context);
            avformat_free_context(output_format_context);
            return -1;
        }
    }

    // Write the header to the output file
    if (avformat_write_header(output_format_context, nullptr) < 0) {
        printf("Error occurred when writing header to output file\n");
        avformat_close_input(&input_format_context);
        if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&output_format_context->pb);
        }
        avformat_free_context(output_format_context);
        return -1;
    }

    // Read packets from the input device and write them to the output file
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        printf("Failed to allocate packet\n");
        avformat_close_input(&input_format_context);
        if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&output_format_context->pb);
        }
        avformat_free_context(output_format_context);
        return -1;
    }

    printf("Start capturing audio input...\n");

    while (StaticData::RecordingButtonPressed) {
        if (av_read_frame(input_format_context, packet) < 0) {
            break; // Exit the loop if no more frames
        }

        if (packet->stream_index == audio_stream_index) {
            // Rescale packet timestamp
            packet->pts = av_rescale_q_rnd(packet->pts, in_stream->time_base, out_stream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet->dts = av_rescale_q_rnd(packet->dts, in_stream->time_base, out_stream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream->time_base);
            packet->pos = -1;
            packet->stream_index = out_stream->index;

            // Write the packet to the output file
            if (av_interleaved_write_frame(output_format_context, packet) < 0) {
                printf("Error writing packet to output file\n");
                break;
            }
        }
        av_packet_unref(packet);
    }

    printf("Ended capturing audio input...\n");

    // Write the trailer and clean up
    av_write_trailer(output_format_context);

    av_packet_free(&packet);
    avformat_close_input(&input_format_context);

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context->pb);
    }
    avformat_free_context(output_format_context);

    printf("Audio input capture completed.\n");

    return 0;
}

void WriteWavHeader(std::ofstream& file, uint32_t dataLength, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
    // Calculate sizes
    uint32_t chunkSize = 36 + dataLength;
    uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign = channels * bitsPerSample / 8;

    // Write RIFF header
    file.seekp(0, std::ios::beg);
    file.write("RIFF", 4);
    file.write((char*)&chunkSize, 4);
    file.write("WAVE", 4);

    // Write fmt chunk
    file.write("fmt ", 4);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    file.write((char*)&subchunk1Size, 4);
    file.write((char*)&audioFormat, 2);
    file.write((char*)&channels, 2);
    file.write((char*)&sampleRate, 4);
    file.write((char*)&byteRate, 4);
    file.write((char*)&blockAlign, 2);
    file.write((char*)&bitsPerSample, 2);

    // Write data chunk header
    file.write("data", 4);
    file.write((char*)&dataLength, 4);
}

void UpdateWavHeader(std::ofstream& file) {
    uint32_t fileSize = (uint32_t)file.tellp();
    uint32_t dataLength = fileSize - 44; // Subtract header size

    // Update chunk sizes
    file.seekp(4, std::ios::beg);
    uint32_t chunkSize = fileSize - 8;
    file.write((char*)&chunkSize, 4);

    file.seekp(40, std::ios::beg);
    file.write((char*)&dataLength, 4);
}

struct PaUserData {
    std::ofstream outputFile;
};

static int RecordCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {

    PaUserData* data = (PaUserData*)userData;
    const int16_t* in = (const int16_t*)inputBuffer;

    // Calculate the number of bytes to write
    size_t bytesToWrite = framesPerBuffer * sizeof(int16_t) * 2; // 2 channels

    // Write the audio data to the file
    if (inputBuffer != nullptr) {
        data->outputFile.write((const char*)in, bytesToWrite);
    }
    else {
        // Input buffer is null; write zeros (silence)
        std::vector<char> silence(bytesToWrite, 0);
        data->outputFile.write(silence.data(), bytesToWrite);
    }

    return paContinue;
}


int AudioHandler::CaptureAudioOutput() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio Init Error: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Find the WASAPI host API
    PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
    if (hostApiIndex < 0) {
        printf("WASAPI not available\n");
        Pa_Terminate();
        return -1;
    }

    // Get the default output device (for capturing output)
    PaDeviceIndex deviceIndex = Pa_GetHostApiInfo(hostApiIndex)->defaultOutputDevice;
    if (deviceIndex == paNoDevice) {
        printf("No default output device\n");
        Pa_Terminate();
        return -1;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (deviceInfo == nullptr) {
        printf("Could not get device info\n");
        Pa_Terminate();
        return -1;
    }

    const int maxDeviceInfoChannels = deviceInfo->maxInputChannels;

    // Set up the input parameters for loopback capture
    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(PaStreamParameters));
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = maxDeviceInfoChannels; // Stereo
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency; // Note: Output latency
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Set up the WASAPI stream info for loopback mode
    PaWasapiStreamInfo wasapiInfo;
    memset(&wasapiInfo, 0, sizeof(PaWasapiStreamInfo));
    wasapiInfo.size = sizeof(PaWasapiStreamInfo);
    wasapiInfo.hostApiType = paWASAPI;
    wasapiInfo.version = 1;
    wasapiInfo.flags = paWinWasapiExclusive; // Enable loopback
    wasapiInfo.channelMask = 0;
    wasapiInfo.hostProcessorOutput = nullptr;

    inputParameters.hostApiSpecificStreamInfo = &wasapiInfo;

    // Prepare user data for the callback
    PaUserData data;
    data.outputFile.open("output_OUTPUT.wav", std::ios::binary);
    if (!data.outputFile.is_open()) {
        printf("Failed to open output file\n");
        Pa_Terminate();
        return -1;
    }

    // Write a placeholder WAV header (we will update it after recording)
    WriteWavHeader(data.outputFile, 0, (uint32_t)deviceInfo->defaultSampleRate, 32, maxDeviceInfoChannels);// Sample rate: 44100 Hz, Bits per sample: 16, Channels: 2

    // Open the stream
    PaStream* stream;
    err = Pa_OpenStream(&stream,
        &inputParameters,
        nullptr, // No output
        44100, // Sample rate
        paFramesPerBufferUnspecified, // Frames per buffer
        paClipOff, // Stream flags
        RecordCallback,
        &data);
    if (err != paNoError) {
        printf("Error opening audio stream: %s\n", Pa_GetErrorText(err));
        data.outputFile.close();
        Pa_Terminate();
        return -1;
    }

    // Start the stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Error starting audio stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        data.outputFile.close();
        Pa_Terminate();
        return -1;
    }

    printf("Start capturing audio output...\n");

    // Capture loop
    while (StaticData::RecordingButtonPressed) {
        Pa_Sleep(100); // Sleep for 100 milliseconds
    }

    printf("Ended capturing audio output...\n");

    // Stop the stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        printf("Error stopping audio stream: %s\n", Pa_GetErrorText(err));
    }

    Pa_CloseStream(stream);
    Pa_Terminate();

    // Update the WAV header with the correct data length
    UpdateWavHeader(data.outputFile);

    data.outputFile.close();

    printf("Audio output capture completed.\n");

    return 0;
}