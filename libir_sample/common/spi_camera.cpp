#include "spi_camera.h"
#include <atomic>

std::atomic_bool isRUNNING(true);
IrVideoHandle_t* ir_image_video_handle = NULL;
IrVideoHandle_t* ir_temp_video_handle = NULL;

void wait_sem_for_streaming()
{
    sem_wait(&image_done_sem);
    return;
}

void release_sem_after_streaming()
{
    sem_post(&image_sem);
    return;
}


void init_spi_video_stream(StreamFrameInfo_t* stream_frame_info)
{
    int ret = 0;
    ret = ir_image_video_handle->ir_video_open(stream_frame_info->image_driver_handle, \
    (char*)stream_frame_info->image_name);
    printf("open video node: %d\n", ret);

    SpiParams_t spi_params;
    spi_params.mode = 3;
    spi_params.speed_hz = 49000000;
    spi_params.bits_per_word = 8;
    spi_params.delay_usecs = 0;
    spi_params.buf_len = 4096;
    spi_params.width = stream_frame_info->width;
    spi_params.height = stream_frame_info->height;

    ret = ir_image_video_handle->ir_video_init(stream_frame_info->image_driver_handle, &spi_params);
    printf("irspi_init done\n");
    return;
}

void destroy_spi_video_stream(StreamFrameInfo_t* stream_frame_info)
{
    ir_image_video_handle->ir_video_release(stream_frame_info->image_driver_handle, NULL);
    ir_image_video_handle->ir_video_close(stream_frame_info->image_driver_handle);
    ir_video_handle_delete(&ir_image_video_handle);
    return;
}

void* spi_stream_function(void* threadarg)
{
    int ret = 0;
    StreamFrameInfo_t* stream_frame_info = (StreamFrameInfo_t*)threadarg;

    init_spi_video_stream(stream_frame_info);

    while(isRUNNING)
    {
        wait_sem_for_streaming();
        //get frame
        ret = ir_image_video_handle->ir_video_frame_get(stream_frame_info->image_driver_handle, NULL, stream_frame_info->raw_frame, stream_frame_info->raw_byte_size);
        if(ret != IRLIB_SUCCESS)
        {
            release_sem_after_streaming();
            destroy_spi_video_stream(stream_frame_info);
            printf("spi_frame_get failed\n ");
            return NULL;
        }

        memcpy(stream_frame_info->image_info.data, stream_frame_info->raw_frame, \
            stream_frame_info->image_info.byte_size); //image data
        memcpy(stream_frame_info->information_line.data, stream_frame_info->raw_frame + \
            stream_frame_info->image_info.byte_size, stream_frame_info->information_line.byte_size); //information line
        memcpy(stream_frame_info->temp_info.data, stream_frame_info->raw_frame + \
            stream_frame_info->image_info.byte_size + stream_frame_info->information_line.byte_size, \
            stream_frame_info->temp_info.byte_size); //temp data

        release_sem_after_streaming();
    }

    destroy_spi_video_stream(stream_frame_info);
    return NULL;
}