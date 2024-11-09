#ifndef VIDEOHANDLER_H
#define VIDEOHANDLER_H

#include "DefineAliases.h"
#include "IOHandler.h"
#include "StaticData.h"

EXTERN {
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/time.h>
}

struct FFMPEG {
    StaticData staticData;
    int CaptureAudio();
};

#endif // VIDEOHANDLER_H
