#ifndef RASPICVCAM_H
#define RASPICVCAM_H

#include <stdint.h>

#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"

using namespace std;
using namespace cv;

class RaspiCvCam
{
    typedef void (*FrameCallback) ();
public:
    static void Init();
    static void Destroy();
    static bool CameraOn;
    static int SourceHeight, SourceWidth;
    static uint32_t FrameCount;
    static time_t CameraStartedAt;
    static Mat ImageMat;
    static bool UseColor;
    static bool Rotate180;
    static bool UseFlip;

    static FrameCallback FrameUpdatedCallback;

    static void StartCamera();
    static void StopCamera();

private:
    static void enableCamera();

};

#endif // RASPICVCAM_H
