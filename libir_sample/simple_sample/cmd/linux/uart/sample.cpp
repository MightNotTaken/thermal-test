#include <stdio.h>
#include <stdlib.h>

#include "libircmd.h"
#include "libiruart.h"

// Declare a function to search for the serial port, the specific implementation is at the end
// This function is only applicable to Linux platform, for Windows platform please refer to the uart sample in the Windows directory
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

    // All parameters used
    UartConDevParams_t param = {0};             // Serial port parameters

    int ret = IRLIB_SUCCESS;
    char uart_data[20] = "/dev/ttyUSB0";        // You can use "ls /dev | grep ttyUSB" to check the file name corresponding to the serial port, and modify the string according to the result
    char data[100] = {0};                       // Used to store queried information

    // Start to create handle
    // First call ir_control_handle_create to create ir_control_handle
    ir_control_handle_create(&ir_control_handle);
    // Secondly call iruart_handle_create to create iruart_handle (currently using uart protocol, if using other protocols, please call other functions)
    iruart_handle = iruart_handle_create(ir_control_handle);

    // Open and initialize the device by searching for the serial port function
    // ret = search_com(ir_control_handle, &ircmd_handle);
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
    param.baudrate = 115200;                    // Set the baud rate of the serial port
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
    return 0;
}

int search_com(IrControlHandle_t* ir_control_handle, IrcmdHandle_t** cmd_handle)
{
    if (ir_control_handle == NULL)
    {
        printf("handle is NULL\n");
        return -1;
    }

    FILE* fp;
    char data[100];
    char buffer[100] = { 0 };
    char dev_node[30] = { 0 };
    char prefix[20] = "/dev/";
    char temp[10] = { 0 };
    char find_str[10] = "ttyUSB";
    char* tmp = NULL;
    char* current_addr = NULL;

    int ret;

    printf("Searching available serial port......\n");
    fp = popen("ls /dev | grep \"ttyUSB*\" ", "r");
    fread(buffer, 1, sizeof(buffer), fp);
    pclose(fp);
    fp = NULL;
    printf("content of ls /dev | grep \"ttyUSB*\" is %s\n", buffer);

    int i = 0;
    current_addr = buffer;

    while ((tmp = strstr(current_addr, find_str)) != NULL)
    {
        memset(dev_node, 0, 30 * sizeof(char));
        strcat(dev_node, prefix);

        for (i = 0; i <= (int)strlen(tmp); i++)
        {
            if (tmp[i] == '\n' || tmp[i] == '\0' || tmp[i] == ' ')
            {
                break;
            }
        }
        strncpy(dev_node + strlen(dev_node), tmp, i);
        current_addr += i;

        ret = ir_control_handle->ir_control_open(ir_control_handle->ir_control_handle, (void*)(dev_node));
        if (ret == 0)
        {
            ret = ir_control_handle->ir_control_init(ir_control_handle->ir_control_handle, NULL);
            if (ret != 0)
            {
                printf("ret is %d\n", ret);
                printf("ttyUSB%d set baudrate failed\n", i);
            }
            *cmd_handle = ircmd_create_handle(ir_control_handle);
            ret = basic_device_info_get(*cmd_handle, 1, data);
            if (ret == 0)
            {
                printf("success to open %s\n", dev_node);
                return 0;
            }
            else
            {
                printf("%s exist, but not uart0\n", dev_node);
                ir_control_handle->ir_control_close(ir_control_handle->ir_control_handle);
            }
        }
    }

    return 0;
}