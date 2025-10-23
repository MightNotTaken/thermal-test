#include "sample.h"


void* upgrade_process(void* callback_data, void* priv_data)
{
    printf("download percentage = %.2f %%\n", *((float*)callback_data) * 100);
    return NULL;
}

void find_text_in_brackets(const char* pn, char* text)
{
    const char* start = strrchr(pn, '[');
    const char* end = strrchr(pn, ']');

    if (start != NULL && end != NULL && (end > start + 1))
    {
        const char* text_start = start + 1;
        size_t text_size = end - text_start;
        memcpy(text, text_start, text_size);
        text[text_size] = '\0';
    }
}

int read_file(IrcmdHandle_t* ircmd_handle, int gain_mode, int file_type, void* org_table)
{
    if (ircmd_handle == NULL || org_table == NULL)
    {
        printf("ircmd_handle or nuc_table is NULL!\n");
        return -1;
    }

    if (gain_mode != HIGH_GAIN && gain_mode != LOW_GAIN)
    {
        printf("gain_mode is invalid!\n");
        return -1;
    }

    if (file_type != KT && file_type != BT && file_type != CORRECT && file_type != NUC_T)
    {
        printf("file_type is invalid!\n");
        return -1;
    }
    //File name in device provided by our company
    char file_name_in_device[256] = { 0 };
    void* priv_data = NULL;
    int ret;
    int file_length = 0;

    if (file_type == KT)
    {
        file_length = ADVANCED_KT_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device,"default_data/high_gain/tpd_kt1.bin");
        }
        else if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "default_data/low_gain/tpd_kt1.bin");
        }
    }

    if (file_type == BT)
    {
        file_length = ADVANCED_BT_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "default_data/high_gain/tpd_bt1.bin");
        }
        else if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "default_data/low_gain/tpd_bt1.bin");
        }
    }

    if (file_type == NUC_T)
    {
        file_length = ADVANCED_NUC_T_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "default_data/high_gain/tpd_nuc_t.bin");
        }
        if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "default_data/low_gain/tpd_nuc_t.bin");
        }
    }

    if (file_type == CORRECT)
    {
        file_length = CORRECT_TABLE_SIZE * 2;
        char pn_data[100] = { 0 };
        ret = basic_device_info_get(ircmd_handle, BASIC_DEV_INFO_GET_PN, pn_data);
        printf("ret = %d\n", ret);
        printf("PN is: %s\n", pn_data);

        //Find the data in [] from the last part in the PN
        char text[10] = { 0 };
        find_text_in_brackets(pn_data, text);
        if (strlen(text)==0)
        {
            printf("text in [] is empty, use F1\n");
            memcpy(text, "F1", 3);
        }

        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "system_data/tpd_dis_table/high%s.bin", text);
        }
        else if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "system_data/tpd_dis_table/low%s.bin", text);
        }
        printf("correct table file name:%s\n", file_name_in_device);
    }

    uint8_t* single_file = NULL;
    single_file = (uint8_t*)malloc(file_length * sizeof(uint8_t));
    if (single_file == NULL)
    {
        printf("there is no more space!\n");
        return -1;
    }

    ret = single_file_read(ircmd_handle, file_name_in_device, single_file, &file_length, \
        upgrade_process, priv_data);
    if (ret != IRLIB_SUCCESS)
    {
        printf("====== fail to read single file ======\n");
        free(single_file);
        return -1;
    }
    memcpy(org_table, single_file, file_length);
    printf("====== succeed to read single file ======\n");
    free(single_file);
    return 0;
}

int download_file(IrcmdHandle_t* ircmd_handle, int gain_mode, int file_type,void* target_table)
{
    if (ircmd_handle == NULL || target_table == NULL)
    {
        printf("ircmd_handle or nuc_table is NULL!\n");
        return -1;
    }

    if (gain_mode != HIGH_GAIN && gain_mode != LOW_GAIN)
    {
        printf("gain_mode is invalid!\n");
        return -1;
    }

    if (file_type != KT && file_type != BT && file_type != CORRECT && file_type != NUC_T)
    {
        printf("file_type is invalid!\n");
        return -1;
    }

    int ret = 0;
    int file_length = 0;
    char file_name_in_device[256] = { 0 };
    void* priv_data = NULL;

    if (file_type == KT)
    {
        file_length = ADVANCED_KT_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/high_gain/tpd_kt1.bin");
        }
        if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/low_gain/tpd_kt1.bin");
        }
    }

    if (file_type == BT)
    {
        file_length = ADVANCED_BT_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/high_gain/tpd_bt1.bin");
        }
        if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/low_gain/tpd_bt1.bin");
        }
    }

    if (file_type == NUC_T)
    {
        file_length = ADVANCED_NUC_T_SIZE * 2;
        if (gain_mode == HIGH_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/high_gain/tpd_nuc_t.bin");
        }
        if (gain_mode == LOW_GAIN)
        {
            sprintf(file_name_in_device, "secd_cali_data/low_gain/tpd_nuc_t.bin");
        }
    }

    uint8_t* single_file = NULL;
    single_file = (uint8_t*)malloc(file_length * sizeof(uint8_t));
    if (single_file == NULL)
    {
        printf("there is no more space!\n");
        return -1;
    }
    memcpy(single_file, (uint8_t*)target_table, file_length);

    ret = single_file_write(ircmd_handle, file_name_in_device, single_file, \
        file_length, upgrade_process, priv_data);
    if (ret != IRLIB_SUCCESS)
    {
        printf("====== fail to write normal file ======\n");
        free(single_file);
        return false;
    }
    free(single_file);
    printf("====== succeed to write normal file ======\n");
    return 0;
}

int main(void)
{
    printf("multi point calibration sample start\n");
    printf("version:%s\n", iruart_version());
    printf("version:%s\n", ircmd_version());
    printf("version:%s\n", ircam_version());

    iruart_log_register(IRUART_LOG_ERROR, NULL, NULL);
    ircmd_log_register(IRCMD_LOG_ERROR, NULL, NULL);
    ircam_log_register(IRCAM_LOG_ERROR, NULL, NULL);
    irtemp_log_register(IRTEMP_LOG_ERROR);

    //Intermediate handle used to handle the actual handle
    IrControlHandle_t* ir_control_handle = NULL;
    ir_control_handle_create(&ir_control_handle);

    //Create actually control handle
    //There are three control methods: I2C, UART and USB
    IruartHandle_t* iruart_handle = NULL;
    iruart_handle = iruart_handle_create(ir_control_handle);
    if (iruart_handle == NULL)
    {
        printf("fail to create iruart_handle\n");
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }

    /*
    #if defined (unix) || (linux)
    Iri2cHandle_t* i2c_handle = NULL;
    i2c_handle = iri2c_handle_create(ir_control_handle);
    if (i2c_handle == NULL)
    {
        printf("fail to create iri2c_handle\n");
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }
    #endif
    */

    /*
    irdfu_handle_create(ir_control_handle);
    IruvcHandle_t* uvc_handle = NULL;
    uvc_handle = iruvc_usb_handle_create(ir_control_handle);
    if (uvc_handle == NULL)
    {
        printf("fail to create iruvc_handle\n");
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }
    */

    //Create command handle
    IrcmdHandle_t* ircmd_handle = NULL;
    ircmd_handle = ircmd_create_handle(ir_control_handle);
    if(ircmd_handle == NULL)
    {
        printf("fail to create ircmd_handle\n");

        ir_control_handle_delete(&ir_control_handle);
        iruart_handle_delete(ir_control_handle);

        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */

        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        return -1;
    }
    //for windows
    char dev_info[] = "ONLY_I2C:/dev/i2c-1;UVC_USB:pid=0x0001,vid=0x3474,sameidx=0;UART:\\\\.\\COM18;";
    //for linux
    //char dev_info[] = "ONLY_I2C:/dev/i2c-1;UVC_USB:pid=0x0001,vid=0x3474,sameidx=0;UART:/dev/ttyUSB0;";

    int ret;
    int command_channel_type;

    //Get command channel type in use
    ret = adv_command_channel_type_get(ircmd_handle, &command_channel_type);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to get current command channel\n");
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }

    void* dev_param = NULL;

    //Parse dev_info according to the command channel type
    ret = parse_device_information((command_channel_e)command_channel_type, dev_info, &dev_param);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to parse device information\n");
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }

    //Open device
    ret = adv_device_open(ircmd_handle, dev_param);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to open device\n");
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        return -1;
    }

    /*
    int device_params[64] = { 0 };
    device_params[0] = 115200;
    ret = adv_device_init(ircmd_handle, device_params);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to init device\n");
    }
    */

    /***********init for multi points calibration***********/
    int gain_mode = -1;
    printf("-------------------please choose the number of gain mode-------------------\n");
    printf("0:low gain\n");
    printf("1:high gain\n");
    printf("--------------------------------------------------------------------------\n");
    scanf("%d", &gain_mode);

    ret = basic_gain_set(ircmd_handle, gain_mode);
    if (ret != 0)
    {
        printf("fail to set gain\n");
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        adv_device_close(ircmd_handle);
        return ret;
    }

    //The blackbody temperature needs to be backcalculated with environmental variables
    //So environmental variable parameters are required
    //The array length is the same as the number of calibration points
    float ems[4] = { 1, 1, 0.95, 1 };
    float hum[4] = { 0.45, 0.45, 0.45, 0.45 };
    float dist[4] = { 0.25, 3, 3, 4 };
    float ta[4] = { 25, 25, 25, 25 };
    float tu[4] = { 25, 25, 25, 25 };
    EnvCorrectParam env_cor_param[] = {
    { dist[0], ems[0], hum[0], ta[0], tu[0] }, \
    { dist[1], ems[1], hum[1], ta[1], tu[1] }, \
    { dist[2], ems[2], hum[2], ta[2], tu[2] }, \
    { dist[3], ems[3], hum[3], ta[3], tu[3] } };

    //Read from the module used for environment correction
    uint16_t correct_table[CORRECT_TABLE_SIZE] = { 0 };

#if 1    //use correct table file read from camera
    ret = read_file(ircmd_handle, gain_mode, CORRECT, correct_table);
    if (ret != 0)
    {
        printf("fail to read single correct file\n");
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        adv_device_close(ircmd_handle);
        return -1;
    }
#else    //use correct table from exist file
    //read CORRECT table
    FILE* fp = NULL;
    fp = fopen("RS300_9.1mm_GermaniumWindows__Ver259_H.bin", "rb");
    if (fp == NULL)
    {
        printf("fail to open correct table\n");
        fclose(fp);
        ircmd_delete_handle(ircmd_handle);
        iruart_handle_delete(ir_control_handle);
        /*
        #if defined (unix) || (linux)
        iri2c_handle_create(ir_control_handle);
        #endif
        */
        /*
        iruvc_usb_handle_delete(ir_control_handle);
        irdfu_handle_delete(ir_control_handle);
        */
        ir_control_handle_delete(&ir_control_handle);
        adv_device_close(ircmd_handle);
        return -1;
    }
    fread(correct_table, 1, sizeof(correct_table), fp);
    fclose(fp);
#endif

#if defined KT_BT_NUCT_CALIBRATION

    //output_temp is temperature measured by device
    //setting_temp is blackbody temperature
    MultiPointCalibTemp_t multi_point_temp[] = { { 21.975, 20  },\
    { 76.975, 80  }, { 109.475, 120 }, { 133.1, 140 } };

    //The original table is read from the default area of the device
    uint16_t org_kt_table[ADVANCED_KT_SIZE] = { 0 };
    int16_t org_bt_table[ADVANCED_BT_SIZE] = { 0 };
    uint16_t org_nuc_table[ADVANCED_NUC_T_SIZE] = { 0 };
    //The new table is calculated from the original table and stored in the second_calibration area
    uint16_t new_kt_table[ADVANCED_KT_SIZE] = { 0 };
    int16_t new_bt_table[ADVANCED_BT_SIZE] = { 0 };
    uint16_t new_nuc_table[ADVANCED_NUC_T_SIZE] = { 0 };

    //KT BT calibration by two points
    //The high and low temperature you use is up to you
    //Ktemp,Btemp and AddressCA provided by our company
    //The following KT,BT,NUC-T size is used by default
    second_calibration_param_t second_cal_param = { 16585, -3130, 2330, \
        ADVANCED_KT_SIZE, ADVANCED_BT_SIZE, ADVANCED_NUC_T_SIZE };
    //High blackbody temperature
    second_cal_param.setting_temp_high = 120;
    //Low blackbody temperature
    second_cal_param.setting_temp_low = 20;
    //Vtemp collected by the device when facing high temperature blackbody
    second_cal_param.high_vtemp = 7528;
    //Vtemp collected by the device when facing low temperature blackbody
    second_cal_param.low_vtemp = 7533;
    //Temperature collected by the device when facing high temperature blackbody
    second_cal_param.object_temp_high = 109.475;
    //Temperature collected by the device when facing low temperature blackbody
    second_cal_param.object_temp_low = 21.975;
    //The Vtemp is collected by the device when facing blackbody
    uint16_t vtemp[4] = { 7533, 7531, 7528, 7500 };

    //Store original data for calibration
    MultiPointCalibArray_t multi_point_calib_array = { org_kt_table, org_bt_table, org_nuc_table, correct_table };
    //Store parameters for KT BT NUCT calibration
    AdvancedMultiPointCalibParam_t multi_point_calib_param = { env_cor_param[2], env_cor_param[0],\
        second_cal_param };
    MultiPointCalibInputParam_version_t multi_point_calib_input_param;
    multi_point_calib_input_param.multi_point_param = &multi_point_calib_param;
    multi_point_calib_input_param.multi_point_calib_array = &multi_point_calib_array;

    //Store KT BT calibration results
    TwoPointCalibResult_t two_point_calib_result = { new_kt_table, new_bt_table };

    MultiPointCalibNuc_t multi_point_nuc[] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    int i = 0;

    //read original KT table
    ret = read_file(ircmd_handle, gain_mode, KT, org_kt_table);
    if (ret != 0)
    {
        printf("fail to read single kt file\n");
        goto delete1;
    }
    //read original BT table
    ret = read_file(ircmd_handle, gain_mode, BT, org_bt_table);
    if (ret != 0)
    {
        printf("fail to read single bt file\n");
        goto delete1;
    }
    //read original NUC_T table
    ret = read_file(ircmd_handle, gain_mode, NUC_T, org_nuc_table);
    if (ret != 0)
    {
        printf("fail to read single nuc file\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_KT, ADV_SECOND_CALI_DISABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_BT, ADV_SECOND_CALI_DISABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_NUC_T, ADV_SECOND_CALI_DISABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

    //Calculate new KT BT table
    //MULTI_POINT_CALIB_V2 is default version
    ret = new_ktbt_recal_double_point_calculate_with_version(&multi_point_calib_input_param, MULTI_POINT_CALIB_V2, &two_point_calib_result);
    if (ret != IRLIB_SUCCESS)
    {
        printf("New KT BT calibration failed\n");
        goto delete1;
    }

    //The input number cant be greater than the actual temperature measurement number
    printf("please input the number of muliti points:");
    scanf("%d", (int*)&multi_point_calib_input_param.multi_point_num);
    if (multi_point_calib_input_param.multi_point_num > (sizeof(multi_point_temp) / sizeof(MultiPointCalibTemp_t)))
    {
        printf("The input number is greater than the actual temperature measurement number\n");
        goto delete1;
    }

    for (i = 0; i < multi_point_calib_input_param.multi_point_num; i++)
    {
        multi_point_calib_param.high_temp_env_cor_param = env_cor_param[i];
        multi_point_calib_param.low_temp_env_cor_param = env_cor_param[i];
        multi_point_calib_param.second_calib_param = second_cal_param;
        multi_point_calib_param.cur_vtemp = vtemp[i];
        multi_point_calib_input_param.multi_point_temp = &multi_point_temp[i];
        multi_point_calib_input_param.multi_point_nuc = &multi_point_nuc[i];

        multi_point_calc_user_defined_nuc_with_version(&multi_point_calib_input_param, MULTI_POINT_CALIB_V2, \
            & two_point_calib_result);
        printf("setting_nuc=%d output_nuc=%d\n", multi_point_nuc[i].setting_nuc, multi_point_nuc[i].output_nuc);
    }

    //Calculate new NUC-T table
    multi_point_calib_input_param.multi_point_nuc = multi_point_nuc;
    multi_point_calc_new_nuc_table_with_version(&multi_point_calib_input_param, MULTI_POINT_CALIB_V2, new_nuc_table);

    //download new KT table
    ret = download_file(ircmd_handle, gain_mode, KT, new_kt_table);
    if (ret != 0)
    {
        printf("fail to write normal file\n");
        goto delete1;
    }

    //download new BT table
    ret = download_file(ircmd_handle, gain_mode, BT, new_bt_table);
    if (ret != 0)
    {
        printf("fail to write normal file\n");
        goto delete1;
    }

    //download new NUC_T table
    ret = download_file(ircmd_handle, gain_mode, NUC_T, new_nuc_table);
    if (ret != 0)
    {
        printf("fail to write normal file\n");
        goto delete1;
    }
    
    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_KT, ADV_SECOND_CALI_ENABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_BT, ADV_SECOND_CALI_ENABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_NUC_T, ADV_SECOND_CALI_ENABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete1;
    }

#endif

#if defined ONLY_NUC_T_CALIBRATION

    //output_temp is temperature measured by device
    //setting_temp is blackbody temperature
    MultiPointCalibTemp_t multi_point_temp[] = { { 21.975, 20  },\
    { 76.975, 80  }, { 109.475, 120 }, { 133.1, 140 } };

    //The original table is read from the default area of the device
    uint16_t org_nuc_table[ADVANCED_NUC_T_SIZE] = { 0 };
    //The new table is calculated from the original table and stored in the second_calibration area
    uint16_t new_nuc_table[ADVANCED_NUC_T_SIZE] = { 0 };

    MultiPointCalibInputParam_version3_t multi_point_calib_input_param;
    //Store the NUC of the calculation
    MultiPointCalibNuc_t multi_point_nuc[] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    int i = 0;

    //read original NUC_T table
    ret = read_file(ircmd_handle, gain_mode, NUC_T, org_nuc_table);
    if (ret != 0)
    {
        printf("fail to read single nuc file\n");
        goto delete1;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_NUC_T, ADV_SECOND_CALI_DISABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set nuc_t to default data\n");
        goto delete1;
    }

    multi_point_calib_input_param.multi_point_param = (AdvancedMultiPointCalibParam_t*)malloc(sizeof(AdvancedMultiPointCalibParam_t));
    if (multi_point_calib_input_param.multi_point_param == NULL)
    {
        printf("fail to malloc for multi_point_calib_input_param.multi_point_param\n");
        goto delete1;
    }
    multi_point_calib_input_param.multi_point_calib_array = (MultiPointCalibArray_version3_t*)malloc(sizeof(MultiPointCalibArray_version3_t));
    if (multi_point_calib_input_param.multi_point_calib_array == NULL)
    {
        printf("fail to malloc for multi_point_calib_input_param.multi_point_calib_array\n");
        goto delete2;
    }

    printf("please input the number of muliti points:");
    scanf("%d", (int*)&multi_point_calib_input_param.multi_point_num);

    multi_point_calib_input_param.multi_point_calib_array->default_nuc_table = org_nuc_table;


    //reverse calc NUC
    for (i = 0; i < multi_point_calib_input_param.multi_point_num; i++)
    {
        multi_point_calib_input_param.multi_point_param->high_temp_env_cor_param = env_cor_param[i];
        multi_point_calib_input_param.multi_point_param->low_temp_env_cor_param = env_cor_param[i];
        multi_point_calib_input_param.multi_point_temp = &multi_point_temp[i];
        multi_point_calib_input_param.multi_point_nuc = &multi_point_nuc[i];

        ret = reverse_calc_NUC_with_nuc_t_table_len(org_nuc_table, multi_point_calib_input_param.multi_point_temp->output_temp, \
            ADVANCED_NUC_T_SIZE, &multi_point_calib_input_param.multi_point_nuc->output_nuc);
        if (ret != IRLIB_SUCCESS)
        {
            printf("reverse_calc_nuc_with_nuc_t failed\n");
            goto delete3;
        }

        ret = reverse_enhance_distance_temp_correct(&(multi_point_calib_input_param.multi_point_param->high_temp_env_cor_param), \
            correct_table, CORRECT_TABLE_SIZE, multi_point_calib_input_param.multi_point_temp->setting_temp, \
            &multi_point_calib_input_param.multi_point_temp->setting_temp);
        if (ret != IRLIB_SUCCESS)
        {
            printf("reverse setting temp failed\n");
            goto delete3;
        }

        ret = reverse_calc_NUC_with_nuc_t_table_len(org_nuc_table, multi_point_calib_input_param.multi_point_temp->setting_temp, \
            ADVANCED_NUC_T_SIZE, &multi_point_calib_input_param.multi_point_nuc->setting_nuc);
        if (ret != IRLIB_SUCCESS)
        {
            printf("reverse calc setting_temp nuc failed\n");
            goto delete3;
        }
        printf("setting_nuc=%d output_nuc=%d\n", multi_point_nuc[i].setting_nuc, multi_point_nuc[i].output_nuc);
    }

    //calc new NUC_T table
    multi_point_calib_input_param.multi_point_nuc = multi_point_nuc;
    multi_point_calc_new_nuc_table_with_version(&multi_point_calib_input_param, MULTI_POINT_CALIB_V3, new_nuc_table);

    //download new NUC_T table
    ret = download_file(ircmd_handle, gain_mode, NUC_T, new_nuc_table);
    if (ret != 0)
    {
        printf("fail to write normal file\n");
        goto delete3;
    }

    ret = adv_second_cali_flag_set(ircmd_handle, ADV_SECOND_CALI_FLAG_NUC_T, ADV_SECOND_CALI_ENABLE);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to set second cali flag\n");
        goto delete3;
    }
    printf("multi point calibration sample finish\n");

    delete3:
    free(multi_point_calib_input_param.multi_point_calib_array);
    delete2:
    free(multi_point_calib_input_param.multi_point_param);
#endif

#if defined ENHANCE_DISTANCE_TEMP_CORRECT
    IrcmdPoint_t point_pos;
    float org_temp = 0;
    float new_temp = 0;
    uint32_t table_len = 0;

    printf("Please enter point coordinate\n");
    scanf("%hd %hd", &(point_pos.x), &(point_pos.y));

    ret = read_file(ircmd_handle, gain_mode, CORRECT, correct_table);
    if (ret != 0)
    {
        printf("fail to read single correct file\n");
        goto delete1;
    }

    ret = basic_point_temp_info_get(ircmd_handle, point_pos, &org_temp);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to get point temp\n");
        goto delete1;
    }

    ret = enhance_distance_temp_correct(&env_cor_param[0], correct_table, table_len, org_temp, &new_temp);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to enhance_distance_temp_correct\n");
        goto delete1;
    }

    printf("point_temp_value is %f\n", new_temp);
#endif
delete1:
    ret = adv_device_close(ircmd_handle);
    if (ret != IRLIB_SUCCESS)
    {
        printf("fail to close device\n");
    }
    ircmd_delete_handle(ircmd_handle);

    iruart_handle_delete(ir_control_handle);

    /*
    #if defined (unix) || (linux)
    iri2c_handle_create(ir_control_handle);
    #endif
    */

    /*
    iruvc_usb_handle_delete(ir_control_handle);
    irdfu_handle_delete(ir_control_handle);
    */

    ir_control_handle_delete(&ir_control_handle);

    return 0;
}