#include <raspicvcam.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

// OPENCV
#include <iostream>
#include <fstream>
#include <sstream>
#include "time.h"

#include "opencv2/core/core.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/objdetect/objdetect.hpp"

extern "C" {
    #include "bcm_host.h"
    #include "interface/vcos/vcos.h"

    #include "interface/mmal/mmal.h"
    #include "interface/mmal/mmal_logging.h"
    #include "interface/mmal/mmal_buffer.h"
    #include "interface/mmal/util/mmal_util.h"
    #include "interface/mmal/util/mmal_util_params.h"
    #include "interface/mmal/util/mmal_default_components.h"
    #include "interface/mmal/util/mmal_connection.h"

    #include "raspicamcv/RaspiCamControl.h"
    #include "raspicamcv/RaspiPreview.h"
    #include "raspicamcv/RaspiCLI.h"
}

using namespace std;
using namespace cv;

//////////////////////////////////////////////////////////////////////////////////////////////////
// OPENCV CODE
//////////////////////////////////////////////////////////////////////////////////////////////////


// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1

// Video format information
#define VIDEO_FRAME_RATE_NUM 90
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

int mmal_status_to_int(MMAL_STATUS_T status);



//////////////////////////////////////////////////////////////////////////////////////////////////
// mmal/cv

MMAL_STATUS_T status;// = -1;
MMAL_PORT_T *camera_video_port = NULL;
MMAL_PORT_T *preview_input_port = NULL;
MMAL_PORT_T *encoder_input_port = NULL;
MMAL_PORT_T *encoder_output_port = NULL;

// variable to convert I420 frame to IplImage
IplImage *py, *pu, *pv, *pu_big, *pv_big, *mergedImage, *dstImage;

MMAL_POOL_T *video_pool; /// Pointer to the pool of buffers used by encoder output port

//////////////////////////////////////////////////////////////////////////////////////////////////
// cam state struct

int sourceHeight4;
RASPIPREVIEW_PARAMETERS preview_parameters;   /// Preview setup parameters
RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
MMAL_CONNECTION_T *preview_connection; /// Pointer to the connection from camera to preview
MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder


//////////////////////////////////////////////////////////////////////////////////////////////////
// Statics
//////////////////////////////////////////////////////////////////////////////////////////////////

int RaspiCvCam::SourceWidth = 320;
int RaspiCvCam::SourceHeight = 240;
bool RaspiCvCam::UseColor = true;
bool RaspiCvCam::Rotate180 = false;
bool RaspiCvCam::UseFlip = false;

bool RaspiCvCam::CameraOn = false;
time_t RaspiCvCam::CameraStartedAt;
uint32_t RaspiCvCam::FrameCount = 0;
Mat RaspiCvCam::ImageMat;

RaspiCvCam::FrameCallback RaspiCvCam::FrameUpdatedCallback;


//////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////////////////////////

void RaspiCvCam::Init()
{
    CameraOn = false;

    // Load settings
    //SourceWidth = 320;
    //SourceHeight = 240;
    sourceHeight4 = SourceHeight/4;

    // Init the bcm
    bcm_host_init();

    // Setup defaults
    raspipreview_set_defaults(&preview_parameters);
    raspicamcontrol_set_defaults(&camera_parameters);
    camera_parameters.hflip = true;

    // Init OpenCV Stuff
    py = cvCreateImage(cvSize(SourceWidth,SourceHeight), IPL_DEPTH_8U, 1);		// Y component of YUV I420 frame
    pu = cvCreateImage(cvSize(SourceWidth/2,SourceHeight/2), IPL_DEPTH_8U, 1);	// U component of YUV I420 frame
    pv = cvCreateImage(cvSize(SourceWidth/2,SourceHeight/2), IPL_DEPTH_8U, 1);	// V component of YUV I420 frame
    pu_big = cvCreateImage(cvSize(SourceWidth,SourceHeight), IPL_DEPTH_8U, 1);
    pv_big = cvCreateImage(cvSize(SourceWidth,SourceHeight), IPL_DEPTH_8U, 1);
    mergedImage = cvCreateImage(cvSize(SourceWidth,SourceHeight), IPL_DEPTH_8U, 3);	// final picture to display
    dstImage = cvCreateImage(cvSize(SourceWidth,SourceHeight), IPL_DEPTH_8U, 3);

    cout << "RaspiCvCam inited" << endl;
}

void RaspiCvCam::Destroy()
{
    cvReleaseImage(&dstImage);
    cvReleaseImage(&mergedImage);
    cvReleaseImage(&pu);
    cvReleaseImage(&pv);
    cvReleaseImage(&py);
    cvReleaseImage(&pu_big);
    cvReleaseImage(&pv_big);
}

uint32_t memcpy_rev(void* dst, const void* src, uint32_t length)
{
    for (uint32_t i = 0 ; i < length ; i++) {
        (*((uint8_t*)dst + length - i - 1)) = (*((uint8_t*)src + i));
    }
    return length;
}

static void video_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
    MMAL_BUFFER_HEADER_T *new_buffer;

    if (buffer->length)
    {
        // Grab and copy the frame
        mmal_buffer_header_mem_lock(buffer);

        if (!RaspiCvCam::Rotate180) {
            memcpy(py->imageData,buffer->data,RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight);	// read Y
        } else {
            memcpy_rev(py->imageData,buffer->data,RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight);	// read Y
        }

        if (RaspiCvCam::UseColor) {
            if (!RaspiCvCam::Rotate180) {
                memcpy(pu->imageData,buffer->data+RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight,RaspiCvCam::SourceWidth*sourceHeight4); // read U
                memcpy(pv->imageData,buffer->data+RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight+RaspiCvCam::SourceWidth*sourceHeight4,RaspiCvCam::SourceWidth*sourceHeight4); // read v
            } else {
                memcpy_rev(pu->imageData,buffer->data+RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight,RaspiCvCam::SourceWidth*sourceHeight4); // read U
                memcpy_rev(pv->imageData,buffer->data+RaspiCvCam::SourceWidth*RaspiCvCam::SourceHeight+RaspiCvCam::SourceWidth*sourceHeight4,RaspiCvCam::SourceWidth*sourceHeight4); // read v
            }

            cvResize(pu, pu_big, CV_INTER_NN);
            cvResize(pv, pv_big, CV_INTER_NN);  //CV_INTER_LINEAR looks better but it's slower
            cvMerge(py, pu_big, pv_big, NULL, mergedImage);
            cvCvtColor(mergedImage,dstImage,CV_YCrCb2RGB);	// convert in RGB color space (slow)
            RaspiCvCam::ImageMat = cvarrToMat(dstImage);
        } else {
            RaspiCvCam::ImageMat = cvarrToMat(py);
        }

        if (RaspiCvCam::UseFlip) {
            flip(RaspiCvCam::ImageMat, RaspiCvCam::ImageMat, 1);
        }

        mmal_buffer_header_mem_unlock(buffer);

        RaspiCvCam::FrameCount++;

        RaspiCvCam::FrameUpdatedCallback();
    }
    else
        cout << "raspicam buffer null" << endl;

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      MMAL_STATUS_T status;
      new_buffer = mmal_queue_get(video_pool->queue);
      if (new_buffer)
         status = mmal_port_send_buffer(port, new_buffer);
      if (!new_buffer || status != MMAL_SUCCESS)
         cout << "Unable to return a buffer to the encoder port" << endl;
   }

}


/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroyCamera()
{
    // Destroy raspipreview
    raspipreview_destroy(&preview_parameters);

    if (mmal_port_disable(camera_video_port) != MMAL_SUCCESS)
       cout << "mmal_port_disable error" << endl;

    if (mmal_component_disable(camera_component) != MMAL_SUCCESS)
       cout << "camera component couldn't be disabled" << endl;

    mmal_port_pool_destroy(camera_video_port, video_pool);

    mmal_component_destroy(camera_component);
    mmal_component_release(camera_component);

    camera_component = NULL;
    camera_video_port = NULL;
}

static bool createCamera()
{
    MMAL_COMPONENT_T *camera = 0;
    MMAL_ES_FORMAT_T *format;
    MMAL_PORT_T *video_port = NULL;
    MMAL_STATUS_T status;

    // Create the component
    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
    if (status != MMAL_SUCCESS)
    {
       cout << "Failed to create camera component" << endl;
       goto error;
    }

    if (!camera->output_num)
    {
       cout << "Camera doesn't have output ports" << endl;
       goto error;
    }

    video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];

    {
        //  set up the camera configuration
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
        {
          { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
          cam_config.max_stills_w = RaspiCvCam::SourceWidth,
          cam_config.max_stills_h = RaspiCvCam::SourceHeight,
          cam_config.stills_yuv422 = 0,
          cam_config.one_shot_stills = 0,
          cam_config.max_preview_video_w = RaspiCvCam::SourceWidth,
          cam_config.max_preview_video_h = RaspiCvCam::SourceHeight,
          cam_config.num_preview_video_frames = 3,
          cam_config.stills_capture_circular_buffer_height = 0,
          cam_config.fast_preview_resume = 0,
          cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
        };
        mmal_port_parameter_set(camera->control, &cam_config.hdr);
    }

    // Set the encode format on the video  port
    format = video_port->format;
    format->encoding_variant = MMAL_ENCODING_I420;
    format->encoding = MMAL_ENCODING_I420;
    format->es->video.width = VCOS_ALIGN_UP(RaspiCvCam::SourceWidth, 32);
    format->es->video.height = VCOS_ALIGN_UP(RaspiCvCam::SourceHeight, 16);
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = RaspiCvCam::SourceWidth;
    format->es->video.crop.height = RaspiCvCam::SourceHeight;
    format->es->video.frame_rate.num = VIDEO_FRAME_RATE_NUM;
    format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

    status = mmal_port_format_commit(video_port);
    if (status)
    {
       cout << "camera video format couldn't be set" << endl;
       goto error;
    }

    // PR : plug the callback to the video port
    status = mmal_port_enable(video_port, video_buffer_callback);
    if (status)
    {
       cout << "mmal_port_enable camera video callback2 error" << endl;
       goto error;
    }

   // Ensure there are enough buffers to avoid dropping frames
   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

    //PR : create pool of message on video port
    MMAL_POOL_T *pool;
    video_port->buffer_size = video_port->buffer_size_recommended;
    video_port->buffer_num = video_port->buffer_num_recommended;
    pool = mmal_port_pool_create(video_port, video_port->buffer_num, video_port->buffer_size);
    if (!pool)
       cout << "Failed to create buffer header pool for video output port" << endl;
    video_pool = pool;

    // Enable component
    status = mmal_component_enable(camera);
    if (status)
    {
       cout << "Camera component couldn't be enabled" << endl;
       goto error;
    }
    raspicamcontrol_set_all_parameters(camera, &camera_parameters);
    camera_component = camera;
    camera_video_port   = camera_component->output[MMAL_CAMERA_VIDEO_PORT];

    // Create raspipreview
    if (raspipreview_create(&preview_parameters) != MMAL_SUCCESS)
    {
       cout << __func__ << ": Failed to create preview component" << endl;
       goto error;
    }
    return true;

error:

   destroyCamera();

   return false;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */

void RaspiCvCam::StartCamera()
{
    if (CameraOn)
        return;

    FrameCount = 0;

    // init timer
    time(&CameraStartedAt);

    //qDebug("Camera starting!");

    // create camera
    if (!createCamera())
    {
       cout << __func__ << ": Failed to create camera component" << endl;
       return;
    }

   // start capture
    if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
    {
        cout << "Capture failed!";
        StopCamera();
        return;
    }

    // Send all the buffers to the video port
    int num = mmal_queue_length(video_pool->queue);
    for (int q=0;q<num;q++)
    {
       MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(video_pool->queue);
       if (!buffer)
           cout << "Unable to get a required buffer " << q << " from pool queue" << endl;
        else if (mmal_port_send_buffer(camera_video_port, buffer)!= MMAL_SUCCESS)
           cout << "Unable to send a buffer to encoder output port (" << q << ")" << endl;
    }

    CameraOn = true;
}

void RaspiCvCam::StopCamera()
{
    if (CameraOn == false)
        return;

    //qDebug("Camera stopping!");
    CameraOn = false;

    mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 0);

    destroyCamera();

    //if (status != 0)
    //    raspicamcontrol_check_configuration(128);

}


