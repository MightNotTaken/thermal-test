#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libircmd.h"
#include "libiruvc.h"
#include "libircmd_temp.h"

int main(int argc, char* argv[])
{
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iruvc_version());

    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iruvc_log_register(IRUVC_LOG_ERROR, NULL, NULL);

    // All handles used
    IrControlHandle_t* ir_control_handle = NULL;
    IruvcHandle_t* iruvc_handle = NULL;
    IrcmdHandle_t* ircmd_handle = NULL;

    // All parameters used
    IruvcDevParam_t control_dev_param;
    memset(&control_dev_param, 0, sizeof(IruvcDevParam_t));

    int ret = IRLIB_SUCCESS;
    IrcmdPoint_t point_pos;             // The structure of the point
    float point_temp_value = 0;         // Result

    ir_control_handle_create(&ir_control_handle);
    iruvc_handle = iruvc_usb_handle_create(ir_control_handle);
    control_dev_param.pid = 0x43D1;
    control_dev_param.vid = 0x3474;
    control_dev_param.same_idx = 0;
    ret = ir_control_handle->ir_control_open(iruvc_handle, &control_dev_param);
    if (ret != IRLIB_SUCCESS)
    {
        printf("uvc open control node failed\n");
        goto fail;
    }
    ircmd_handle = ircmd_create_handle(ir_control_handle);

// Only devices that support temperature measurement can use the temperature measurement function
// If the module does not automatically output images, the image output command needs to be called first, otherwise temperature measurement will not work properly
    point_pos.x = 25;                   // X coordinate
    point_pos.y = 25;                   // Y coordinate
    ret = basic_point_temp_info_get(ircmd_handle, point_pos, &point_temp_value);    // Send the command
    if (ret != IRLIB_SUCCESS)
    {
        printf("get temp info failed\n");
    }
    else
    {
        printf("(%d,%d) is %f (float) \n", point_pos.x, point_pos.y, point_temp_value);
    }

    ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
fail:
    if (ircmd_handle != NULL)
    {
        ircmd_delete_handle(ircmd_handle);
        ircmd_handle = NULL;
    }
    if (iruvc_handle != NULL)
    {
        iruvc_usb_handle_delete(iruvc_handle);
        iruvc_handle = NULL;
    }
    if (ir_control_handle != NULL)
    {
        ir_control_handle_delete(&ir_control_handle);
    }

    printf("Process run successful!\n");
    system("pause");
    return 0;
}