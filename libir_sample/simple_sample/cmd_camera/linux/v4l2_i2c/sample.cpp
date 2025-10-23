#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "libircmd.h"
#include "libiri2c.h"
#include "libirv4l2.h"

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
    int output_format =  ADV_MIPI_FORMAT;           // The output format of the video data
    int output_fps = 30;                            // Frame rate. The temperature measurement device does not support setting the frame rate, but the imaging device does. However, this frame rate still needs to be correctly configured and will be used when configuring UVC parameters.
    int output_num = DOUBLE_VIDEO;                  // Used to indicate whether to output a single image or dual images, for details please refer to the video_output_num_e structure
    int output_status = BASIC_ENABLE;               // The status of the video output, whether to enable the video output
    char* control_device_name = "/dev/i2c-1";       // i2c device node
    char* video_image_device_name = "/dev/video5";  // video node. mipi is 5 dvp is 0

    // Calculate the size of the data
    int image_byte_size = width * image_height * image_ratio;                                   // 图像数据大小
    int temp_byte_size = width * temp_height * temp_ratio;                                      // 温度数据大小
    int info_byte_size = width * info_height * info_ratio;                                      // 信息数据大小
    int dummy_byte_size = width * dummy_height * dummy_ratio;                                   // 帧信息中有两个信息行时使用
    int raw_byte_size = image_byte_size + temp_byte_size + info_byte_size + dummy_byte_size;    // 数据总大小

    // All handles used
    IrVideoHandle_t* ir_video_handle = NULL;
    Irv4l2VideoHandle_t* irv4l2_handle = NULL;
    IrControlHandle_t* ir_control_handle = NULL;
    Iri2cHandle_t* iri2c_handle = NULL;
    IrcmdHandle_t* ircmd_handle = NULL;

    // All parameters used
    VideoOutputInfo_t output_info;
    CamDevParams_t dev_params;
    memset(&dev_params, 0, sizeof(dev_params));
    CamStreamParams_t stream_params;
    memset(&stream_params, 0, sizeof(stream_params));

    // All data used
    uint8_t* raw_frame_data = NULL;

    int ret = IRLIB_SUCCESS;
    FILE* file = NULL;
    size_t written = 0;

    // Print the version information of the libraries
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iri2c_version());
    printf("version:%s\n", libv4l2_version());

    // Register library logs and set log level to ERROR
    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iri2c_log_register(IRI2C_LOG_ERROR, NULL, NULL);
    irv4l2_log_register(IRV4L2_LOG_ERROR, NULL, NULL);

    // create video handle
    ir_video_handle_create(&ir_video_handle);
    irv4l2_handle = irv4l2_handle_create(ir_video_handle);
    printf("video handle create\n");
    // create video handle over

    // create control handle
    ir_control_handle_create(&ir_control_handle);
    iri2c_handle = iri2c_handle_create(ir_control_handle);
    ret = ir_control_handle->ir_control_open(ir_control_handle->ir_control_handle, control_device_name);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to open node\n");
        goto fail;
    }
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
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }
    printf("start preview\n");

    ret = ir_video_handle->ir_video_open(irv4l2_handle, video_image_device_name);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to open video handle, ret is %d\n", ret);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }

    // Set the parameters of V4L2 video stream
    dev_params.width = width;
    dev_params.format = V4L2_PIX_FMT_YUYV;  //we transfer the data by composite format, only use V4L2_PIX_FMT_YUYV for 2 bytes transfer.
    dev_params.fps = output_fps;
    dev_params.height = image_height + temp_height + info_height;
    stream_params.width = dev_params.width;
    stream_params.height = dev_params.height;
    ret = ir_video_handle->ir_video_init(irv4l2_handle, &dev_params);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to init video handle, ret is %d\n", ret);
        ir_video_handle->ir_video_close(irv4l2_handle);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }

    ret = ir_video_handle->ir_video_start_stream(irv4l2_handle, &stream_params);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to start video stream, ret is %d\n", ret);
        ir_video_handle->ir_video_release(irv4l2_handle, &dev_params);
        ir_video_handle->ir_video_close(irv4l2_handle);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }
    printf("irv4l2_camera_start_stream done\n");

    if (raw_byte_size <= 0)
    {
        printf("raw_byte_size error. size is %d", raw_byte_size);
        ir_video_handle->ir_video_stop_stream(irv4l2_handle, &stream_params);
        ir_video_handle->ir_video_release(irv4l2_handle, &dev_params);
        ir_video_handle->ir_video_close(irv4l2_handle);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }
    raw_frame_data = (uint8_t*) malloc(raw_byte_size);

    ret = IRV4L2_SELECT_FRAME_TIMEOUT;
    while (ret != IRLIB_SUCCESS)
    {
        ret = ir_video_handle->ir_video_frame_get(irv4l2_handle, NULL, raw_frame_data, raw_byte_size);  // 如果较长时间程序为运行完成，考虑未成功出图，请手动关闭程序
        usleep(1);
    }
    // Write the data to a file
    file = fopen("frame_data.bin", "wb");
    if (!file)
    {
        printf("can not create frame_data.bin\n");
        ir_video_handle->ir_video_stop_stream(irv4l2_handle, &stream_params);
        ir_video_handle->ir_video_release(irv4l2_handle, &dev_params);
        ir_video_handle->ir_video_close(irv4l2_handle);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }

    written = fwrite(raw_frame_data, sizeof(uint8_t), raw_byte_size, file);
    fclose(file);
    if (written != raw_byte_size)
    {
        printf("write data failed\n");
        ir_video_handle->ir_video_stop_stream(irv4l2_handle, &stream_params);
        ir_video_handle->ir_video_release(irv4l2_handle, &dev_params);
        ir_video_handle->ir_video_close(irv4l2_handle);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }
    printf("write success!\n");

    // Cleanup process after execution ends to avoid memory leaks
    ir_video_handle->ir_video_stop_stream(irv4l2_handle, &stream_params);
    ir_video_handle->ir_video_release(irv4l2_handle, &dev_params);
    ir_video_handle->ir_video_close(irv4l2_handle);
    ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
fail:
    if (raw_frame_data)
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
        iri2c_handle_delete(ir_control_handle);
        iri2c_handle = NULL;
        ir_control_handle_delete(&ir_control_handle);
    }
    if (irv4l2_handle != NULL)
    {
        irv4l2_handle_delete(irv4l2_handle);
        irv4l2_handle = NULL;
    }
    if (ir_video_handle != NULL)
    {
        ir_video_handle_delete(&ir_video_handle);
    }

    printf("Process run successful!\n");
    return 0;
}
