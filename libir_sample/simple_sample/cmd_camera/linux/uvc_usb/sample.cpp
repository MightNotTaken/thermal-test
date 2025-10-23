#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "libirparse.h"
#include "libircmd.h"
#include "libiruvc.h"

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
    IruvcHandle_t *iruvc_handle = NULL;
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
    uint8_t* raw_frame_data = NULL;

    int ret = IRLIB_SUCCESS;
    FILE* file = NULL;
    size_t written = 0;

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
    printf("raw size is %d\n", raw_byte_size);

    if (raw_byte_size <= 0)
    {
        printf("raw_byte_size error. size is %d", raw_byte_size);
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    raw_frame_data = (uint8_t*) malloc(raw_byte_size);

    ret = IRUVC_GET_FRAME_TIMEOUT;
    while (ret != IRLIB_SUCCESS)
    {
        ret = ir_video_handle->ir_video_frame_get(iruvc_handle, NULL, raw_frame_data, raw_byte_size);  // If the program does not complete for a long time, consider that the image output was not successful, please manually close the program
        usleep(1);
    }
    // Write the data to a file
    file = fopen("frame_data.bin", "wb");
    if (!file)
    {
        printf("can not create frame_data.bin\n");
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }

    written = fwrite(raw_frame_data, sizeof(uint8_t), raw_byte_size, file);
    fclose(file);
    if (written != raw_byte_size)
    {
        printf("write data failed\n");
        ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    printf("write success!\n");

    // Cleanup process after execution ends to avoid memory leaks
    ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
    ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
    ir_video_handle->ir_video_close(iruvc_handle);

fail:
    if (raw_frame_data != NULL)
    {
        free(raw_frame_data);
        raw_frame_data = NULL;
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
    return 0;
}
