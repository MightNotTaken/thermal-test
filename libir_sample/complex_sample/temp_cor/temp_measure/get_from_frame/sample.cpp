#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "libircmd.h"
#include "libiruvc.h"

int main(int argc, char* argv[])
{
    int width = 384;                    // Width of the image data and temperature data
    int image_height = 288;             // Height of the image data
    int temp_height = 288;              // Height of the temperature data
    int info_height = 2;
    int dummy_height = 0;
    int image_ratio = 2;
    int info_ratio = 2;
    int temp_ratio = 2;
    int frame_ratio = 2;
    int dummy_ratio = 2;
    char output_type[10] = "YUYV";
    int output_format =  ADV_USB_FORMAT;
    int output_fps = 9;
    int output_num = DOUBLE_VIDEO;
    int output_status = BASIC_ENABLE;
    bool is_auto_output = true;
    int pid = 0x43D1;
    int vid = 0x3474;
    int same_idx = 0;

    // calculate the size of the data
    int image_byte_size = width * image_height * image_ratio;                                   // Size of the image data
    int temp_byte_size = width * temp_height * temp_ratio;                                      // Size of the temperature data
    int info_byte_size = width * info_height * info_ratio;                                      // Size of the information data
    int dummy_byte_size = width * dummy_height * dummy_ratio;                                   // Used when there are two information lines in the frame information
    int raw_byte_size = image_byte_size + temp_byte_size + info_byte_size + dummy_byte_size;    // Size of the total data

    // All handles used
    IrVideoHandle_t* ir_video_handle = NULL;
    IruvcHandle_t* iruvc_handle = NULL;
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
    uint8_t* temp_frame_data = NULL;
    uint8_t* raw_frame_data = NULL;

    int ret = IRLIB_SUCCESS;

    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iruvc_version());

    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iruvc_log_register(IRUVC_LOG_ERROR, NULL, NULL);

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
        return -1;
    }
    printf("video handle create\n");
    ir_control_handle_create(&ir_control_handle);
    ret = iruvc_usb_handle_create_with_exist_instance(ir_control_handle, iruvc_handle);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to create iruvc handle, ret is %d\n", ret);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
    }
    ircmd_handle = ircmd_create_handle(ir_control_handle);

    // Send the image output command. If no data frame is received, the temperature cannot be calculated.
    if (!is_auto_output)
    {
        output_info.video_output_format = output_format;
        // output_info.video_output_fps = output_fps;
        output_info.video_output_num = output_num;
        output_info.video_output_status = output_status;
        ret = basic_preview_start(ircmd_handle, output_info);
        if (ret != IRLIB_SUCCESS)
        {
            printf("fail to start preview, ret is %d\n", ret);
            ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
            ir_video_handle->ir_video_close(iruvc_handle);
            goto fail;
        }
        printf("start preview\n");
    }

    video_params.usr_callback.iruvc_handle = iruvc_handle;
    video_params.usr_callback.usr_func = NULL;
    video_params.usr_callback.usr_param = NULL;
    video_params.camera_param.width = width;
    video_params.camera_param.height = image_height + temp_height + info_height + dummy_height;
    video_params.camera_param.frame_size = raw_byte_size;
    video_params.camera_param.fps = output_fps;
    video_params.camera_param.timeout_ms_delay = 2000;
    video_params.camera_param.format = output_type;

    ret = ir_video_handle->ir_video_start_stream(iruvc_handle, &video_params);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to start video stream, ret is %d\n", ret);
        ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
        ir_video_handle->ir_video_close(iruvc_handle);
        goto fail;
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

    while (true)
    {
        printf("start read frame\n");
        // Get a frame
        ret = ir_video_handle->ir_video_frame_get(iruvc_handle, NULL, raw_frame_data, raw_byte_size);
        if (ret != IRLIB_SUCCESS)
        {
            continue;
        }
        printf("read frame success\n");

        // Divide the data into temperature data
        if (temp_byte_size > 0)
        {
            // Temperature data
            memcpy(temp_frame_data, raw_frame_data + image_byte_size + temp_byte_size, info_byte_size);
        }

        // Start processing the temperature data
        int x = 25, y = 25;     // Coordinates of the point to measure temperature (0 <= x < width, 0 <= y < height)

        // The temperature data is two bytes per data, so the type needs to be cast to uint16_t*, and then the value is taken
        // Temperature equals (value / 4 / 16 - 273.15) (value is the temperature data)
        // Divided by 4 is because the data is stored in the high 14 bits, and needs to be right-shifted by 2 bits before use (right-shift by 2 bits is equivalent to divided by 4)
        // Divided by 16 is because the temperature data is 16 times the actual temperature (stored as an unsigned integer)
        // Subtracting 273.15 is to convert the temperature data from Kelvin into Celsius
        printf("(%d,%d) is %f (float) \n", 0, 0, ((float)(((uint16_t*)temp_frame_data)[x * width + y])) / 4 / 16 - 273.15);
        break;
    }

    ir_video_handle->ir_video_stop_stream(iruvc_handle, &stop_params);
    ir_video_handle->ir_video_release(iruvc_handle, &dev_param);
    ir_video_handle->ir_video_close(iruvc_handle);

fail:
    if (temp_frame_data != NULL)
    {
        free(temp_frame_data);
        temp_frame_data = NULL;
    }
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
    system("pause");
    return 0;
}
