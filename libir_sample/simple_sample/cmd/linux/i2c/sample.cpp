#include <stdio.h>
#include <stdlib.h>

#include "libircmd.h"
#include "libiri2c.h"

int main(int argc, char* argv[])
{
    // Print the version information of the libraries
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iri2c_version());

    // Register library logs and set log level to ERROR
    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iri2c_log_register(IRI2C_LOG_ERROR, NULL, NULL);

    // All handles used
    IrControlHandle_t* ir_control_handle = NULL;
    Iri2cHandle_t* iri2c_handle = NULL;
    IrcmdHandle_t* ircmd_handle = NULL;

    int ret = IRLIB_SUCCESS;
    char* dev_node = "/dev/i2c-1";              // i2c device node
    char data[100] = {0};                       // Used to store queried information

    // Start to create handle
    // First call ir_control_handle_create to create ir_control_handle
    ir_control_handle_create(&ir_control_handle);
    // Secondly call iri2c_handle_create to create iri2c_handle (currently using i2c protocol, if using other protocols, please call other functions)
    iri2c_handle = iri2c_handle_create(ir_control_handle);
    // Then call the handle's open function to open the device node
    ret = ir_control_handle->ir_control_open(iri2c_handle, dev_node);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to open node\n");
        goto fail;
    }
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
    if (ir_control_handle != NULL)
    {
        iri2c_handle_delete(ir_control_handle);
        iri2c_handle = NULL;
        ir_control_handle_delete(&ir_control_handle);
    }

    printf("Process run over!\n");
    return 0;
}
