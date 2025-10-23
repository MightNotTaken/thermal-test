#include <stdio.h>
#include <stdlib.h>

#include "tchar.h"

#include "libircmd.h"
#include "libiruart.h"

// Declare a function to search for the serial port, the specific implementation is at the end
// This function is only applicable to Windows platform, for Linux platform please refer to the uart sample in the Linux directory
// The function includes automatically searching for the serial port, automatically adapting to 115200 or 921600 baud rate, opening and initializing the device, and creating ircmd_handle
int search_com(IrControlHandle_t* ir_control_handle, IrcmdHandle_t** cmd_handle);

int main(int argc, char* argv[])
{
    // Print the version information of the libraries
    printf("Depends lib version:\n");
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());
    printf("version:%s\n", iruart_version());

    // Register library logs and set log level to ERROR
    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    iruart_log_register(IRUART_LOG_ERROR, NULL, NULL);

    // All handles used
    IrControlHandle_t* ir_control_handle = NULL;
    IruartHandle_t* iruart_handle = NULL;
    IrcmdHandle_t* ircmd_handle = NULL;

    int ret = IRLIB_SUCCESS;
    char data[100] = {0};                       // Serial port parameters

    // Start to create handle
    // First call ir_control_handle_create to create ir_control_handle
    ir_control_handle_create(&ir_control_handle);
    // Secondly call iruart_handle_create to create iruart_handle (currently using uart protocol, if using other protocols, please call other functions)
    iruart_handle = iruart_handle_create(ir_control_handle);

    // Open and initialize the device by searching for the serial port function
    // unsigned int com_index = 0;
    // ret = search_com(ir_control_handle, &ircmd_handle, com_index);
    // if (ret != 0)
    // {
    //     printf("fail to search com\n");
    //     iruart_handle_delete(ir_control_handle);
    //     iruart_handle = NULL;
    //     ir_control_handle_delete(&ir_control_handle);
    //     return -1;
    // }
    // process over

    // Open and initialize the device by specifying the serial port number
    int num = 8;                                                // Set the serial port number
    UartConDevParams_t param = {0};                             // Serial port parameters
    param.baudrate = 115200;                                    // Set the baud rate of the serial port
    TCHAR uart_data[30] = { 0 };
    TCHAR prefix[20] = _TEXT("\\\\.\\COM");
    TCHAR temp[10] ;
    _tcscat(uart_data, prefix);
    _tcscat(uart_data, _itow(num, temp, 10));
    ret = ir_control_handle->ir_control_open(ir_control_handle->ir_control_handle, (void*)(uart_data));
    if (ret != IRLIB_SUCCESS)
    {
        printf("open failed. ret is %d\n", ret);
        goto fail;
    }
    printf("open success\n");
    ret = ir_control_handle->ir_control_init(ir_control_handle->ir_control_handle, (void*)(&param));
    if (ret != IRLIB_SUCCESS)
    {
        printf("init failed. ret is %d\n", ret);
        ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
        goto fail;
    }
    printf("init success\n");
    ircmd_handle = ircmd_create_handle(ir_control_handle);
    // process over
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
    ir_control_handle->ir_control_release(ir_control_handle->ir_control_handle, (void*)(&param));
    ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
fail:
    if (ircmd_handle != NULL)
    {
        ircmd_delete_handle(ircmd_handle);
        ircmd_handle = NULL;
    }
    if (ir_control_handle != NULL)
    {
        iruart_handle_delete(ir_control_handle);
        iruart_handle = NULL;
        ir_control_handle_delete(&ir_control_handle);
    }

    printf("Process run over!\n");
    system("pause");
    return 0;
}

int search_com(IrControlHandle_t* ir_control_handle, IrcmdHandle_t** cmd_handle)
{
    if (ir_control_handle == NULL)
    {
        printf("handle is NULL\n");
        return -1;
    }

    TCHAR dev_node[30];
    TCHAR prefix[20] = _TEXT("\\\\.\\COM");
    TCHAR temp[10];
    char data[100];
    int ret;
    printf("Searching available serial port......\n");

    int i = 0;
    for (i = 0; i <= 100; i++)
    {
        memset(dev_node, 0, 30 * sizeof(TCHAR));
        _tcscat(dev_node, prefix);
        _tcscat(dev_node, _itow(i, temp, 10));
        ret = ir_control_handle->ir_control_open(ir_control_handle->ir_control_handle, (void*)(dev_node));
        if (ret == 0)
        {
            ret = ir_control_handle->ir_control_init(ir_control_handle->ir_control_handle, NULL);
            if (ret != 0)
            {
                printf("COM%d set baudrate failed\n", i);
            }
            *cmd_handle = ircmd_create_handle(ir_control_handle);
            ret = basic_device_info_get(*cmd_handle, 1, data);
            if (ret == 0)
            {
                printf("success to open COM%d\n", i);
                return 0;
            }
            else
            {
                printf("COM%d exist, but not uart0\n", i);
                ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
            }
        }
    }
    if (i > 31)
    {
        printf("fail to search available com\n");
        return -1;
    }

    return 0;
}