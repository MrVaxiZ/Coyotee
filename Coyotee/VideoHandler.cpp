#include "VideoHandler.h";

#pragma warning disable C6001

int FFMPEG::CaptureAudio() {

    avdevice_register_all();

    // Set up the input format context for the audio device
    AVFormatContext* input_format_context = nullptr;

    // Choose the input format based on your platform
    const AVInputFormat* input_format = av_find_input_format("dshow");
    if (!input_format) {
        printf("Could not find input format\n");
        return -1;
    }

    // FIXME::Make it dynamic not like currently hard-coded
    const char* device_name = "audio=Microphone (Arctis 5 Chat)";

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
    const char* output_filename = "output.wav";

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

    printf("Start capturing audio...\n");

    while (staticData.getRecordingButtonStatus() == true) {
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

    printf("Ended capturing audio...\n");

    // Write the trailer and clean up
    av_write_trailer(output_format_context);

    av_packet_free(&packet);
    avformat_close_input(&input_format_context);

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context->pb);
    }
    avformat_free_context(output_format_context);

    printf("Audio capture completed.\n");

    return 0;
}