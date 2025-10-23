#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tchar.h"

// openCV
#include "opencv2/opencv.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "libirparse.h"
#include "libircmd.h"
#include "libiruvc.h"

int gettimeofday(struct timeval* tv, struct timezone* tz);
void display_one_frame(uint8_t* frame, uint16_t width, uint16_t height, const char* title);

int main(int argc, char* argv[])
{
    // Configuration parameters
    // Assume that in a frame, the data storage format is -- image data + temperature data + information data (strict order)
    // These three types of data are not mandatory and need to be adjusted according to the actual situation
    // For example, there may be only image data / image data + information data / image data + temperature data / image data + information data + temperature data, etc.
    int width = 384;                                // The width of all data
    int image_height = 288;                         // The height of image data
    int temp_height = 288;                          // The height of temperature data
    int info_height = 2;                            // The height of information data(information line)
    int dummy_height = 0;                           // The height of dummy data (used when there are two information lines in the frame information)
    int image_ratio = 2;                            // Image data one pixel point occupies two bytes
    int temp_ratio = 2;                             // Temperature data one pixel point occupies two bytes
    int info_ratio = 2;                             // Information data one pixel point occupies two bytes
    int dummy_ratio = 2;                            // Used when there are two information lines in the frame information
    char output_type[10] = "YUYV";                  // The output format of the image data
    int output_format =  ADV_USB_FORMAT;            // The output format of the video data
    int output_fps = 30;                            // Frame rate. The temperature measurement device does not support setting the frame rate, but the imaging device does. However, this frame rate still needs to be correctly configured and will be used when configuring UVC parameters.
    int output_num = DOUBLE_VIDEO;                  // Used to indicate whether to output a single image or dual images, for details please refer to the video_output_num_e structure
    int output_status = BASIC_ENABLE;               // The status of the video output, whether to enable the video output
    int pid = 0x43D1;                               // The device's PID, provided with the device or can be viewed through the device manager
    int vid = 0x3474;                               // The device's VID, provided with the device or can be viewed through the device manager
    int same_idx = 0;                               // When only one device is being used, set to 0

    // Calculate the size of the data
    int image_byte_size = width * image_height * image_ratio;                                   // The size of the image data
    int temp_byte_size = width * temp_height * temp_ratio;                                      // The size of the temperature data
    int info_byte_size = width * info_height * info_ratio;                                      // The size of the information data
    int dummy_byte_size = width * dummy_height * dummy_ratio;                                   // Used when there are two information lines in the frame information
    int raw_byte_size = image_byte_size + temp_byte_size + info_byte_size + dummy_byte_size;    // The total size of the data

    // All handles used
    IrVideoHandle_t* ir_video_handle = NULL;
    IruvcHandle_t * iruvc_handle = NULL;
    IrControlHandle_t* ir_control_handle = NULL;
    IrcmdHandle_t* ircmd_handle = NULL;

    // All parameters used
    IruvcDevParam_t dev_param;
    memset(&dev_param, 0, sizeof(IruvcDevParam_t));
    VideoOutputInfo_t output_info;
    IruvcCamStreamParams_t video_params;
    memset(&video_params, 0, sizeof(IruvcCamStreamParams_t));
    IruvcStreamStopParams_t stop_params;
    memset(&stop_params, 0, sizeof(IruvcStreamStopParams_t));

    // All data used
    uint8_t* image_frame_data = NULL;
    uint8_t* temp_frame_data = NULL;
    uint8_t* info_frame_data = NULL;
    uint8_t* dummy_frame_data = NULL;
    uint8_t* raw_frame_data = NULL;
    uint8_t* rgb_image_frame = NULL;
    uint8_t* bgr_image_frame = NULL;

    int ret = IRLIB_SUCCESS;

    // Print the version information of the libraries
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iruvc_version());

    // Register library logs and set log level to ERROR
    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iruvc_log_register(IRUVC_LOG_ERROR, NULL, NULL);

    // create video handle
    ir_video_handle_create(&ir_video_handle);
    iruvc_handle = iruvc_camera_handle_create(ir_video_handle);

    dev_param.pid = pid;
    dev_param.vid = vid;
    dev_param.same_idx = same_idx;
    ret = ir_video_handle->ir_video_open(iruvc_handle, &dev_param);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to open video handle, ret is %d\n", ret);
        goto fail;
    }
    ret = ir_video_handle->ir_video_init(iruvc_handle, &dev_param);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to init video handle, ret is %d\n", ret);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    printf("video handle create\n");
    // create video handle over

    // create control handle
    ir_control_handle_create(&ir_control_handle);
    ret = iruvc_usb_handle_create_with_exist_instance(ir_control_handle, iruvc_handle);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to create iruvc handle, ret is %d\n", ret);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    // Call ircmd_create_handle to create ircmd_handle
    ircmd_handle = ircmd_create_handle(ir_control_handle);
    // create control handle over

    // Send the image output command (some devices are not in automatic image output mode, and the image output command needs to be sent to obtain the video stream normally
    // Or the automatic image output format may not match the desired format, so the image output command needs to be sent)
    output_info.video_output_format = output_format;
    // output_info.video_output_fps = output_fps;              // Only the imaging device needs to be configured, the temperature measurement device does not need to be configured (configuration has no effect)
    output_info.video_output_num = output_num;
    output_info.video_output_status = output_status;
    ret = basic_preview_start(ircmd_handle, output_info);   // Image output command of the temperature measurement device，the imaging device use adv_digital_video_output_set() instead
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to start preview, ret is %d\n", ret);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }

    // Set the parameters of the uvc video stream
    video_params.usr_callback.iruvc_handle = iruvc_handle;
    video_params.usr_callback.usr_func = NULL;
    video_params.usr_callback.usr_param = NULL;
    video_params.camera_param.width = width;                                                    // Width
    video_params.camera_param.height = image_height + temp_height + info_height + dummy_height; // Calculated based on the case of dual image output, so two heights are added
    video_params.camera_param.frame_size = raw_byte_size;                                       // The size of the frame data
    video_params.camera_param.fps = output_fps;                                                 // fps
    video_params.camera_param.timeout_ms_delay = 2000;
    video_params.camera_param.format = output_type;                                             // The format of the image data
    stop_params.stop_mode = CLOSE_CAM_SIDE_PREVIEW;

    ret = ir_video_handle->ir_video_start_stream(iruvc_handle, &video_params);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to start video stream, ret is %d\n", ret);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    printf("image is %dx%d, info is %dx%d, temp is %dx%d\n", width,
        image_height, width, info_height, width, temp_height);
    printf("image size is %d, info size is %d, temp size is %d\n",
        image_byte_size, info_byte_size, temp_byte_size);
    printf("raw size is %d\n", raw_byte_size);
    printf("dummy size is %d\n", dummy_byte_size);
    printf("start display\n");

    // Malloc the space needed for the data
    if (image_byte_size > 0)
    {
        image_frame_data = (uint8_t*) malloc(image_byte_size);
    }
    if (temp_byte_size > 0)
    {
        temp_frame_data = (uint8_t*) malloc(temp_byte_size);
    }
    if (info_byte_size > 0)
    {
        info_frame_data = (uint8_t*) malloc(info_byte_size);
    }
    if (dummy_byte_size > 0)
    {
        dummy_frame_data = (uint8_t*) malloc(dummy_byte_size);
    }
    if (raw_byte_size <= 0)
    {
        printf("raw_byte_size error. size is %d", raw_byte_size);
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    raw_frame_data = (uint8_t*) malloc(raw_byte_size);

    // Mallo the space needed for the image display
    rgb_image_frame = (uint8_t*)malloc(width * image_height * 3);   // // The height of the temperature data is the same as the height of the image data, so the image height is used to allocate space here
    if (rgb_image_frame == NULL)
    {
        printf("there is no more space!\n");
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }

    bgr_image_frame = (uint8_t*)malloc(width * image_height * 3);
    if (bgr_image_frame == NULL)
    {
        printf("there is no more space!\n");
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }

    while (true)
    {
        printf("start read frame\n");
        // Get a frame
        ir_video_handle->ir_video_frame_get(iruvc_handle, NULL, raw_frame_data, raw_byte_size);
        printf("read frame success\n");
        // Divide the data into image data, temperature data, and information data
        if (image_byte_size > 0)
        {
            memcpy(image_frame_data, raw_frame_data, image_byte_size);
        }
        if (temp_byte_size > 0)
        {
            memcpy(temp_frame_data, raw_frame_data + image_byte_size, temp_byte_size);
        }
        if (info_byte_size > 0)
        {
            memcpy(info_frame_data, raw_frame_data + image_byte_size + temp_byte_size, info_byte_size);
        }

        // Display the image data and temperature data
        if (image_byte_size > 0)
        {
            yuv422_to_rgb(image_frame_data, width * image_height, rgb_image_frame);
            rgb_to_bgr(rgb_image_frame, width * image_height, bgr_image_frame);
            display_one_frame(bgr_image_frame, width, image_height, "image");
        }
        if (temp_byte_size > 0)
        {
            y16_to_rgb((uint16_t*)temp_frame_data, width, temp_height, rgb_image_frame);
            rgb_to_bgr(rgb_image_frame, width * temp_height, bgr_image_frame);
            display_one_frame(bgr_image_frame, width, temp_height, "temp");
        }

        printf("display success\n");
    }

    ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
    ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
    ir_video_handle->ir_video_close(iruvc_handle);

    // Cleanup process after execution ends to avoid memory leaks
fail:
    if (image_frame_data != NULL)
    {
        free(image_frame_data);
        image_frame_data = NULL;
    }
    if (info_frame_data != NULL)
    {
        free(info_frame_data);
        info_frame_data = NULL;
    }
    if (temp_frame_data != NULL)
    {
        free(temp_frame_data);
        temp_frame_data = NULL;
    }
    if (dummy_frame_data != NULL)
    {
        free(dummy_frame_data);
        dummy_frame_data = NULL;
    }
    if (raw_frame_data != NULL)
    {
        free(raw_frame_data);
        raw_frame_data = NULL;
    }
    if (rgb_image_frame != NULL)
    {
        free(rgb_image_frame);
        rgb_image_frame = NULL;
    }
    if (bgr_image_frame != NULL)
    {
        free(bgr_image_frame);
        bgr_image_frame = NULL;
    }
    if (ircmd_handle != NULL)
    {
        ircmd_delete_handle(ircmd_handle);
        ircmd_handle = NULL;
    }
    if (ir_control_handle != NULL)
    {
        ir_control_handle_delete(&ir_control_handle);
    }
    if (iruvc_handle != NULL)
    {
        iruvc_camera_handle_delete(iruvc_handle);
        iruvc_handle = NULL;
    }
    if (ir_video_handle != NULL)
    {
        ir_video_handle_delete(&ir_video_handle);
    }

    printf("Process run successful!\n");
    system("pause");
    return 0;
}

#define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64
int gettimeofday(struct timeval* tv, struct timezone* tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;
    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
        tmpres /= 10;
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    return 0;
}

void display_one_frame(uint8_t* frame, uint16_t width, uint16_t height, const char* title)
{
    struct timeval image_now_time, temp_now_time;
    static struct timeval image_last_time, temp_last_time;
    float fps = 0;
    if (!strcmp("image", title))
    {
        gettimeofday(&image_now_time, NULL);
        fps = 1000000 / (float)((image_now_time.tv_sec - image_last_time.tv_sec) * 1000000 + \
            (image_now_time.tv_usec - image_last_time.tv_usec));
        image_last_time = image_now_time;
    }
    if (!strcmp("temp", title))
    {
        temp_now_time = image_last_time;
        fps = 1000000 / (float)((temp_now_time.tv_sec - temp_last_time.tv_sec) * 1000000 + \
            (temp_now_time.tv_usec - temp_last_time.tv_usec));
        temp_last_time = temp_now_time;
    }

    char frameText[10] = { " " };
    sprintf_s(frameText, sizeof(frameText), "%.2f", fps);

    cv::Mat image = cv::Mat(height, width, CV_8UC3, frame);
    putText(image, frameText, cv::Point(11, 11), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar::all(0), 1, 8);
    putText(image, frameText, cv::Point(10, 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar::all(255), 1, 8);
    cv::imshow(title, image);         // Show the image
    cvWaitKey(5);
}