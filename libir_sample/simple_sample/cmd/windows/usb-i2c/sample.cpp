#include <stdio.h>
#include <stdlib.h>

#include "tchar.h"

#include "libircmd.h"
#include "libiruvc.h"
int main(int argc, char* argv[])
{
    // Print the version information of the libraries
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iruvc_version());

    // Register library logs and set log level to ERROR
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
    char data[100] = {0};                       // Used to store queried information

    // Start to create handle
    // First call ir_control_handle_create to create ir_control_handle
    ir_control_handle_create(&ir_control_handle);
    // Secondly call iruvc_usb_handle_create to create iruvc_handle (currently using usb protocol, if using other protocols, please call other functions)
    iruvc_handle = iruvc_i2c_usb_handle_create(ir_control_handle);
    // Set device parameters
    control_dev_param.pid = 0x43D1;     // Set device's pid, which can be viewed through device manager
    control_dev_param.vid = 0x3474;     // Set device's vid, which can be viewed through device manager
    control_dev_param.same_idx = 0;     // When only one device is being used, set to 0
    ret = ir_control_handle->ir_control_open(iruvc_handle, &control_dev_param);
    if (ret != IRLIB_SUCCESS) {
        printf("open failed. ret is %d\n", ret);
        goto fail;
    }
    printf("uvc open control node success\n");
    // Finally, call ircmd_create_handle to create ircmd_handle
    ircmd_handle = ircmd_create_handle(ir_control_handle);
    // Create handle over

    // Send a command to query device information, the second parameter of this command specifies the type of information to be obtained as the device name, other device information can also be obtained
    printf("get device name start\n");
    ret = basic_device_info_get(ircmd_handle, BASIC_DEV_NAME, data);
    printf("get device name ret = %d\n", ret);
    printf("get device name: %s\n", data);
    printf("get device name end\n");

    // Execute the shutter command, under normal operation you can hear the sound of the device shutter
    printf("ffc update start\n");
    ret = basic_ffc_update(ircmd_handle);
    printf("ffc update ret = %d\n", ret);
    printf("ffc update end\n");

    // Cleanup process after execution ends to avoid memory leaks
    // The cleanup order is the reverse of the creation order
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

    printf("Process run over!\n");
    system("pause");
    return 0;
}