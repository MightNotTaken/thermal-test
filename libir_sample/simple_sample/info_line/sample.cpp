#include <stdio.h>

#include "libir_infoparse.h"

int main()
{
    // Read data from info_data.bin file, the data only contains information line data, no image or temperature data
    FILE *fp = fopen("info_data.bin", "rb");
    if (fp == NULL)
    {
        printf("open file failed\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *info_data = (uint8_t *)malloc(file_size);
    fread(info_data, 1, file_size, fp);
    fclose(fp);
    // Read data from info_data.bin file over

    // Parse the data
    IrinfoStatusInfo_t status_info;
    irinfoparse_get_irinfo_status_info(info_data, &status_info);
	IrinfoFuncInfo_t function_info;
    irinfoparse_get_irinfo_function_info(info_data, &function_info);
    // Parse the data over

    printf("frame_status.frame_fps: %d\n", status_info.frame_status.frame_fps);
    printf("frame_status.frame_count: %d\n", status_info.frame_status.frame_count);
    printf("frame_status.frame_type: %d\n", status_info.frame_status.frame_type);
	printf("frame_status.flip_status = %d\n",status_info.image_status.flip_status);
	printf("frame_status.temp_type = %d\n",status_info.tpd_status.temp_type);
    printf("width = %d, height = %d\n", function_info.device_info.width, function_info.device_info.height);
	printf("dev_pn = %c%c%c%c\n", function_info.device_info.dev_pn[0], function_info.device_info.dev_pn[1],
        function_info.device_info.dev_pn[2], function_info.device_info.dev_pn[3]);

    free(info_data);

    return 0;
}