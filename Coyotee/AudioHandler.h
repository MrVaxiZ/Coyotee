#ifndef AUDIOHANDLER_H
#define AUDIOHANDLER

#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <atomic>
#include <cstring>

#include "DefineAliases.h"
#include "IOHandler.h"
#include "StaticData.h"

EXTERN{
    /* FFMPEG */
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/time.h>

    /* PortAudio */
    #include "portaudio.h"
    #include <portaudio.h>
    #include <pa_win_wasapi.h>
}

struct AudioHandler
{
    const char* GetDefaultAudioInputDeviceName();
    const char* GetDefaultAudioOutputDeviceName();
	int CaptureAudioInput();
	int CaptureAudioOutput();
};

#endif // !AUDIOHANDLER