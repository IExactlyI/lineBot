#include <iostream>
#include <string>
#include "raspicvcam.h"

#include <unistd.h>
#include <stdint.h>

#include <stdio.h>
#include <signal.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "serialib/serialib.h"
#include "UserProtocol.h"

#define TYPE_POWER              0
#define TYPE_DIRECTION          1
#define TYPE_BREAK              2
#define TYPE_LEFT_BREAK         3
#define TYPE_RIGHT_BREAK        4


using namespace std;
using namespace cv;

serialib serial;
uint8_t serial_buf[20];
uint32_t last_frmCount = 0;

uint16_t fps = 0;

Mat output_img;

void MPower(int8_t power)
{
    uint8_t data[5];

    data[0] = TYPE_POWER;
    data[1] = power;
    uint16_t serial_size = UserProtocol::createDataPack(data, serial_buf, 2);

    if (serial.Write((const void*)serial_buf, serial_size) != 1 )
    {
        cout << "Cannot serial.Write()." << endl;
    }
}

void MDirection(int8_t direction)
{
    uint8_t data[5];

    data[0] = TYPE_DIRECTION;
    data[1] = direction;
    uint16_t serial_size = UserProtocol::createDataPack(data, serial_buf, 2);

    if (serial.Write((const void*)serial_buf, serial_size) != 1 )
    {
        cout << "Cannot serial.Write()." << endl;
    }
}

void MBreak()
{
    uint8_t data[5];

    data[0] = TYPE_BREAK;
    data[1] = 0;
    uint16_t serial_size = UserProtocol::createDataPack(data, serial_buf, 2);

    if (serial.Write((const void*)serial_buf, serial_size) != 1 )
    {
        cout << "Cannot serial.Write()." << endl;
    }
}

uint32_t linePos(Mat _img, uint16_t *_max_pos_out, uint16_t *_min_pos_out)
{
    static uint16_t last_pos = 156;
    uint32_t start_index;
    uint32_t end_index;
    int16_t line_dif[RaspiCvCam::SourceWidth];
    int16_t _max = 0;
    uint16_t _max_pos = 0;
    int16_t _min = 0;
    uint16_t _min_pos = 0;

    uint16_t px_count = 0;

    start_index = ((RaspiCvCam::SourceWidth * RaspiCvCam::SourceHeight) / 2);
    end_index = (((RaspiCvCam::SourceWidth * RaspiCvCam::SourceHeight) / 2)) + RaspiCvCam::SourceWidth;

    px_count = 0;
    for (uint16_t i = start_index + 2 ; i < end_index - 2 ; i++) {
        line_dif[px_count] = (int16_t)_img.at<uint8_t>(i + 2) - (int16_t)_img.at<uint8_t>(i - 2);
        px_count++;
    }

    _max = line_dif[0];
    _min = line_dif[0];

    for (uint16_t i = 0 ; i < (RaspiCvCam::SourceWidth - 4) ; i++) {
        if (line_dif[i] > _max) {
            _max = line_dif[i];
            if (_max > 10) {
                _max_pos = i;
            }
        }

        if (line_dif[i] < _min) {
            _min = line_dif[i];
            if (_min < -10) {
                _min_pos = i;
            }
        }
    }

    if ((_max_pos != 0) && (_min_pos != 0) && (_max_pos > _min_pos)) {
        (*_max_pos_out) = _max_pos + 2;
        (*_min_pos_out) = _min_pos + 2;
        last_pos = (_max_pos + _min_pos) / 2;
        return last_pos;
    } else {
        return last_pos;
    }
}

void exit_int(int s)
{
    printf("Exiting detected\r\n");
    RaspiCvCam::StopCamera();
    MBreak();
    imwrite("output.jpg", RaspiCvCam::ImageMat);
    exit(1);
}

void FrameReady()
{
    static uint16_t _max_pos = 0;
    static uint16_t _min_pos = 0;
    char output_name[50];
    int16_t pos = (int16_t)linePos(RaspiCvCam::ImageMat, &_max_pos, &_min_pos);

    pos = pos - 156;

    pos = pos / 2;

    //printf("FPS : %d ; pos : %d\r", fps, pos);

    //printf("pos : %d\r", pos);

    cout << "FPS : " << fps << " ; POS : " << pos << endl;

    if (pos > 100) {
        pos = 100;
    }

    if (pos < -100) {
        pos = -100;
    }

    MDirection((int8_t)pos);

    /*
    cvtColor(RaspiCvCam::ImageMat, output_img, CV_GRAY2RGB);

    line(output_img, Point(0, 120), Point(319, 120), Scalar(0, 0, 255), 2);

    circle(output_img, Point(_min_pos, 120), 5, Scalar(0, 255, 0), 2);
    circle(output_img, Point(_max_pos, 120), 5, Scalar(0, 255, 0), 2);
    */
}

int main()
{
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = exit_int;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    if (serial.Open("/dev/ttyAMA0", 115200) != 1) {
        cout << "Cannot open port /dev/ttyAMA0." << endl;
        exit(1);
    }

    RaspiCvCam::Init();
    RaspiCvCam::UseColor = false;
    RaspiCvCam::Rotate180 = true;
    RaspiCvCam::UseFlip = true;
    RaspiCvCam::FrameUpdatedCallback = &FrameReady;
    cout << "Start Camera" << endl;
    RaspiCvCam::StartCamera();

    MPower(80);


    while (1)
    {
        fps = RaspiCvCam::FrameCount - last_frmCount;
        last_frmCount = RaspiCvCam::FrameCount;
        sleep(1);
    }

    return 0;
}

