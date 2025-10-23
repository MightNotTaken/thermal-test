// SPDX-License-Identifier: GPL-2.0
/*
 * ac020 CMOS Image Sensor driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 * V0.0X01.0X01 add enum_frame_interval function.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <linux/rk-camera-module.h>
#include <media/media-entity.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <linux/pinctrl/consumer.h>
#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x1)
#define DRIVER_NAME "ac020"
#define ac020_PIXEL_RATE		(96 * 1000 * 1000)

/**
 * RV1126 DVP/BT656/BT1120 sensor subdev 驱动,设备树驱动可以共用，使用命令
 * echo X > /sys/module/ac020/parameters/mode 进行模式切换
 * 0 -- dvp 
 * 1 -- bt656 PAL
 * 2 -- bt656 NTSC
 * 3 -- bt1120
 * 可以添加其它分辨率支持
 * **/
static int mode = 0; //0--dvp 1--bt656;3--bt1120
static int fps = 30;
static int width = 0;
static int height = 0;
static int type = 16;
module_param(mode, int, 0644);
module_param(fps, int, 0644);
module_param(width, int, 0644);
module_param(height, int, 0644);
module_param(type, int, 0644);
/*
 * ac020 register definitions
 */
//get命令不止是读还有写，所有要_IOWR
//_IOWR/_IOR 会自动做一层浅拷贝到用户空间，如果有参数类型包含指针需要自己再调用copy_to_user。
//_IOW 会自动把用户空间参数拷贝到内核并且指针也会拷贝，不需要调用copy_from_user。

#define CMD_MAGIC 0xEF //定义幻数
#define CMD_MAX_NR 3 //定义命令的最大序数
#define CMD_GET _IOWR(CMD_MAGIC, 1,struct ioctl_data)
#define CMD_SET _IOW(CMD_MAGIC, 2,struct ioctl_data)
#define CMD_KBUF _IO(CMD_MAGIC, 3)
//这个是v4l2标准推荐的私有命令配置，这里直接使用自定义的命令也是可以的
//#define CMD_GET _IOWR('V', BASE_VIDIOC_PRIVATE + 11,struct ioctl_data)
//#define CMD_SET _IOW('V', BASE_VIDIOC_PRIVATE + 12,struct ioctl_data)

//结构体与usb-i2c保持一致，有效位为 寄存器地址：wIndex  数据指针:data  数据长度:wLength
struct ioctl_data{
	unsigned char bRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
	unsigned char* data;
	unsigned int timeout;		///< unit:ms
};
#define REG_NULL			0xFFFF	/* Array end token */

#define I2C_VD_BUFFER_RW			0x1D00
#define I2C_VD_BUFFER_HLD			0x9D00
#define I2C_VD_CHECK_ACCESS			0x8000
#define I2C_VD_BUFFER_DATA_LEN		256
#define I2C_OUT_BUFFER_MAX			64 // IN buffer set equal to I2C_VD_BUFFER_DATA_LEN(256)
#define I2C_TRANSFER_WAIT_TIME_2S	2000

#define I2C_VD_BUFFER_STATUS			0x0200
#define VCMD_BUSY_STS_BIT				0x01
#define VCMD_RST_STS_BIT				0x02
#define VCMD_ERR_STS_BIT				0xFC

#define VCMD_BUSY_STS_IDLE				0x00
#define VCMD_BUSY_STS_BUSY				0x01
#define VCMD_RST_STS_PASS				0x00
#define VCMD_RST_STS_FAIL				0x01

#define VCMD_ERR_STS_SUCCESS				0x00
#define VCMD_ERR_STS_LEN_ERR				0x04
#define VCMD_ERR_STS_UNKNOWN_CMD_ERR		0x08
#define VCMD_ERR_STS_HW_ERR					0x0C
#define VCMD_ERR_STS_UNKNOWN_SUBCMD_ERR		0x10
#define VCMD_ERR_STS_PARAM_ERR				0x14


static unsigned short do_crc(unsigned char *ptr, int len)
{
    unsigned int i;
    unsigned short crc = 0x0000;
    
    while(len--)
    {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    
    return crc;
}

static  u8 start_regs[] = {
	0x01,  
    0x30,
    0xc1,
    0x00,
    
    0x00,
    0x00,
    0x00,
    0x00,
      
    0x00,
    0x00,
    0x00,
    0x00,  
    
    0x0a,
    0x00,
    
    0x00,//crc [16]
    0x00,
    
    0x2F,//crc [18]
    0x0D,
    
    
    0x00,//path
    0x80,//src
    0x00,//dst
    0x32,//fps
    
    0xD0,//width&0xff;
    0x02, //width>>8; 
    0x40, //height&0xff;
    0x02,//height>>8;
    
    0x00,
    0x28
};

static  u8 stop_regs[]={
		0x01,
    0x30,
    0xc2,
    0x00,
    
    0x00,
    0x00,
    0x00,
    0x00,
      
    0x00,
    0x00,
    0x00,
    0x00,  
    
    0x0a,
    0x00,
    
    0x00,//crc [16]
    0x00,
    
    0x2F,//crc [18]
    0x0D,
    
    
    0x01,//path
    0x16,//src
    0x00,//dst
    0x19,//fps
    
    0xD0,//width&0xff;
    0x02, //width>>8; 
    0x40, //height&0xff;
    0x02,//height>>8;
    
    0x00,
    0x19
    
};

static int read_regs(struct i2c_client *client,  u16 reg, u8 *val ,int len )
{
	struct i2c_msg msg[2];
	unsigned char data[4] = { 0, 0, 0, 0 };

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = len;
	msg[1].buf = val;
	/* High byte goes out first */
	data[0] = reg>>8;
	data[1] = reg&0xff;
	i2c_transfer(client->adapter, msg, 2);
	return 0;
}

/*
static int check_access_done(struct i2c_client *client,int timeout){
	u8 ret=-1;
	do{
		read_regs(client,I2C_VD_BUFFER_STATUS,&ret,1);
		if ((ret & (VCMD_RST_STS_BIT | VCMD_BUSY_STS_BIT)) == \
					(VCMD_BUSY_STS_IDLE | VCMD_RST_STS_PASS))
			{
				return 0;
			}

		msleep(1);
		timeout--;
	}while (timeout);
	v4l_err(client,"timeout ret=%d\n",ret);
	return -1;
}
*/
static int write_regs(struct i2c_client *client,  u16 reg, u8 *val,int len)
{
	struct i2c_msg msg[1];
	unsigned char *outbuf = (unsigned char *)kmalloc(sizeof(unsigned char)*(len+2), GFP_KERNEL);
	int ret=0;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = len+2;
	msg->buf = outbuf;
	outbuf[0] = reg>>8;
    outbuf[1] = reg&0xff;
	memcpy(outbuf+2, val, len);
	ret = i2c_transfer(client->adapter, msg, 1);
	kfree(outbuf);
	return ret;
	// if (reg & I2C_VD_CHECK_ACCESS){
	// 	return ret;
	// }else
	// {
	// 	return check_access_done(client,2000);//命令超时控制，由于应用层已经控制这里不需要了
	// }
}



struct ac020_framesize {
	u16 width;
	u16 height;
	struct v4l2_fract max_fps;
	u32 code;
	u32 field;
};

struct ac020 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	unsigned int xvclk_frequency;
	struct clk *xvclk;
	struct gpio_desc *pwdn_gpio;
	struct mutex lock;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *link_frequency;
	const struct ac020_framesize *frame_size;
	int streaming;
	u32 module_index;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
};
enum INT_TYPE{
	WN640_DVP=0,
	WN640_BT656_625_PAL,//the same as WN384_BT656_625_PAL
	WN640_BT656_525_NTSC,//the same as WN384_BT656_525_NTSC
	GL1280_BT1120,
	GL1280_DVP,
	WN384_DVP,
	WN640_BT656_625_PAL_P,
};
static const struct ac020_framesize ac020_framesizes[] = {

	{ /* dvp */
		.width		= 640,
		.height		= 512,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.field = V4L2_FIELD_NONE,
	},
	{ /* bt656 625 */
		.width		= 720,
		.height		= 576,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.field = V4L2_FIELD_INTERLACED,
	},

	{ /* bt656 525 */
		.width		= 720,
		.height		= 486,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.field = V4L2_FIELD_INTERLACED,
	},

	{ /* bt1120  */
		.width		= 1280,
		.height		= 1280,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.field = V4L2_FIELD_INTERLACED,
	},
	{ /* dvp  */
		.width		= 1280,
		.height		= 1024,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.field = V4L2_FIELD_NONE,
	},

	{ /* dvp  */
		.width		= 384,
		.height		= 288,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.field = V4L2_FIELD_NONE,
	},
	{ /* bt656 625 */
		.width		= 720,
		.height		= 576,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.field = V4L2_FIELD_NONE,
	},
};



static inline struct ac020 *to_ac020(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ac020, sd);
}




static void ac020_get_default_format(struct v4l2_mbus_framefmt *format)
{
	format->width = ac020_framesizes[mode].width;
	format->height = ac020_framesizes[mode].height;
	format->colorspace = V4L2_COLORSPACE_SRGB;
	format->code = ac020_framesizes[mode].code;
	format->field = ac020_framesizes[mode].field;

}

/*
 * V4L2 subdev video and pad level operations
 */

static int ac020_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (code->index >= ARRAY_SIZE(ac020_framesizes))
		return -EINVAL;

	code->code = ac020_framesizes[code->index].code;

	return 0;
}

static int ac020_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (fse->index >= ARRAY_SIZE(ac020_framesizes))
		return -EINVAL;

	fse->code = ac020_framesizes[fse->index].code;
	fse->min_width  = ac020_framesizes[fse->index].width;
	fse->max_width  = fse->min_width;
	fse->max_height = ac020_framesizes[fse->index].height;
	fse->min_height = fse->max_height;

	return 0;
}

static int ac020_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ac020 *ac020 = to_ac020(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		struct v4l2_mbus_framefmt *mf;

		mf = v4l2_subdev_get_try_format(sd, cfg, 0);
		mutex_lock(&ac020->lock);
		fmt->format = *mf;
		mutex_unlock(&ac020->lock);
		return 0;
#else
	return -ENOTTY;
#endif
	}
	ac020_get_default_format(&ac020->format);
	mutex_lock(&ac020->lock);
	fmt->format = ac020->format;
	mutex_unlock(&ac020->lock);

	return 0;
}

static void __ac020_try_frame_size(struct v4l2_mbus_framefmt *mf,
				    const struct ac020_framesize **size)
{
	const struct ac020_framesize *fsize = &ac020_framesizes[mode];
	const struct ac020_framesize *match = NULL;
	int i = ARRAY_SIZE(ac020_framesizes);
	unsigned int min_err = UINT_MAX;

	while (i--) {
		unsigned int err = abs(fsize->width - mf->width)
				+ abs(fsize->height - mf->height);
		if (err < min_err ) {
			min_err = err;
			match = fsize;
		}
		fsize++;
	}

	if (!match)
		match = &ac020_framesizes[mode];

	mf->width  = match->width;
	mf->height = match->height;
	
	if (size!=NULL)*size = match;
		start_regs[22]= match->width&0xff;
		start_regs[23]= match->width>>8;
		start_regs[24]= match->height&0xff;
		start_regs[25]= match->height>>8;
		start_regs[21]= match->max_fps.numerator;//denominator=1

}
static int ac020_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int index = ARRAY_SIZE(ac020_framesizes);
	struct v4l2_mbus_framefmt *mf = &fmt->format;
	const struct ac020_framesize *size = NULL;
	struct ac020 *ac020 = to_ac020(sd);
	int ret = 0;

	v4l_info(client, "%s enter\n", __func__);

	__ac020_try_frame_size(mf, &size);

	while (--index >= 0)
		if (ac020_framesizes[index].code == mf->code)
			break;

	if (index < 0)
		return -EINVAL;

	mf->colorspace = V4L2_COLORSPACE_SRGB;
	mf->code = ac020_framesizes[index].code;
	mf->field = ac020_framesizes[index].field;

	v4l_info(client,"tiny heigth=%d\n",size->height);
	mutex_lock(&ac020->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		mf = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		*mf = fmt->format;
#else
		return -ENOTTY;
#endif
	} else {
		if (ac020->streaming) {
			mutex_unlock(&ac020->lock);
			return -EBUSY;
		}

		ac020->frame_size = size;
		ac020->format = fmt->format;
	}

	mutex_unlock(&ac020->lock);
	return ret;
}

static int gl1280_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_selection *sel)
{
	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE||mode!=GL1280_BT1120)
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
		sel->r.left = 0;
		sel->r.top = 128;
		sel->r.width = 1280;
		sel->r.height = 1024;
		return 0;
	default:
		return -EINVAL;
	}
}
static void ac020_get_module_inf(struct ac020 *ac020,
				  struct rkmodule_inf *inf)
{
	memset(inf, 0, sizeof(*inf));
	strlcpy(inf->base.sensor, DRIVER_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, ac020->module_name,
		sizeof(inf->base.module));
	strlcpy(inf->base.lens, ac020->len_name, sizeof(inf->base.lens));
}

static long ac020_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct ac020 *ac020 = to_ac020(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	long ret = 0;
	unsigned char *data;
	struct ioctl_data * valp;

	valp=(struct ioctl_data *)arg;
	if((cmd==CMD_GET)||(cmd==CMD_SET)){
		if((valp!=NULL) &&(valp->data!=NULL) ){
			v4l_info(client,"ac020 %d %d %d  \n",cmd, valp->wIndex,valp->wLength);
		}else{
			v4l_err(client, "ac020 args error \n");
			return -EFAULT;
		}
	}
	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		ac020_get_module_inf(ac020, (struct rkmodule_inf *)arg);
		break;

	case CMD_GET:
		data = kmalloc(valp->wLength, GFP_KERNEL);
		read_regs(client,valp->wIndex,data,valp->wLength);

		if (copy_to_user(valp->data, data, valp->wLength))
		{
			ret = -EFAULT;
			v4l_err(client, "error stop ac020\n");
		}
		kfree(data);                                                                                                                                               
		break;
	case CMD_SET:
		write_regs(client,valp->wIndex,valp->data,valp->wLength);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long ac020_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct rkmodule_inf *inf;
	struct rkmodule_awb_cfg *cfg;
	long ret;
	printk("ac020 ioctl %d\n",cmd);
	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = ac020_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case RKMODULE_AWB_CFG:
		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(cfg, up, sizeof(*cfg));
		if (!ret)
			ret = ac020_ioctl(sd, cmd, cfg);
		kfree(cfg);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif

static int ac020_s_stream(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ac020 *ac020 = to_ac020(sd);
	unsigned short crcdata;
	int ret = 0;
	if(mode==GL1280_BT1120||mode==GL1280_DVP)return 0;
	if (on) {
		if (ac020->streaming == 1)
			return 0;
		ac020->streaming = 1;
		//startpreview
		v4l_info(client,"tiny startpreview\n");

		switch (mode)
		{
			case WN640_DVP:
			case GL1280_DVP:
			case WN384_DVP:
				start_regs[27] = 0;
				break;
			case WN640_BT656_625_PAL:
				start_regs[27] = 0x18;//隔行
				break;
			case WN640_BT656_625_PAL_P:
				start_regs[27] = 0x28;//逐行
				break;	
			case WN640_BT656_525_NTSC:
				start_regs[27] = 0x19;
				break;	
			
			default:
				break;
		}
		
		//update crc
		start_regs[21]= fps;
		start_regs[19]= type;
		crcdata= do_crc((uint8_t*)(start_regs+18),10);
		start_regs[14]=crcdata&0xff;
		start_regs[15]=crcdata>>8;
		
		crcdata= do_crc((uint8_t*)(start_regs),16);
		start_regs[16]=crcdata&0xff;
		start_regs[17]=crcdata>>8;
		
		if (write_regs(client, I2C_VD_BUFFER_RW,start_regs,sizeof(start_regs)) < 0) {
				v4l_err(client, "error start ac020\n");
				return -ENODEV;
		}
	}else {
		if (ac020->streaming == 0)
			return 0;
		ac020->streaming = 0;
		//stoppreview
		v4l_info(client,"tiny stoppreview\n");

		//停图再开时间太长如果没有多种模式切换可以去掉
		crcdata= do_crc((uint8_t*)(stop_regs+18),10);
		stop_regs[14]=crcdata&0xff;
		stop_regs[15]=crcdata>>8;
		
		crcdata= do_crc((uint8_t*)(stop_regs),16);
		stop_regs[16]=crcdata&0xff;
		stop_regs[17]=crcdata>>8;
		
		if (write_regs(client, I2C_VD_BUFFER_RW,stop_regs,sizeof(stop_regs)) < 0) {
				v4l_err(client, "error stop ac020\n");
				return -ENODEV;
		}
	}
	return ret;
}

static int ac020_set_test_pattern(struct ac020 *ac020, int value)
{
	return 0;
}

static int ac020_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ac020 *ac020 =
			container_of(ctrl->handler, struct ac020, ctrls);

	switch (ctrl->id) {
	case V4L2_CID_TEST_PATTERN:
		return ac020_set_test_pattern(ac020, ctrl->val);
	}

	return 0;
}

static const struct v4l2_ctrl_ops ac020_ctrl_ops = {
	.s_ctrl = ac020_s_ctrl,
};

static const char * const ac020_test_pattern_menu[] = {
	"Disabled",
	"Vertical Color Bars",
};

/* -----------------------------------------------------------------------------
 * V4L2 subdev internal operations
 */

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int ac020_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format =
				v4l2_subdev_get_try_format(sd, fh->pad, 0);

	dev_dbg(&client->dev, "%s:\n", __func__);

	ac020_get_default_format(format);

	return 0;
}
#endif

static int ac020_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *config)
{
		switch (mode)
	{
		case WN640_DVP:		
		case WN384_DVP:
			config->type = V4L2_MBUS_PARALLEL;
			config->flags = V4L2_MBUS_VSYNC_ACTIVE_HIGH |
							V4L2_MBUS_HSYNC_ACTIVE_HIGH |
							V4L2_MBUS_PCLK_SAMPLE_RISING;
			break;
		case GL1280_DVP:
			config->type = V4L2_MBUS_PARALLEL;
			config->flags = V4L2_MBUS_VSYNC_ACTIVE_HIGH |
							V4L2_MBUS_HSYNC_ACTIVE_HIGH |
							V4L2_MBUS_PCLK_SAMPLE_RISING;
			break;		
		case WN640_BT656_625_PAL:
		case WN640_BT656_525_NTSC:
		case WN640_BT656_625_PAL_P:
			config->type = V4L2_MBUS_BT656; 
			config->flags = V4L2_MBUS_PCLK_SAMPLE_RISING;
			break;
		case GL1280_BT1120:
			config->type = V4L2_MBUS_BT656; 
			config->flags = V4L2_MBUS_PCLK_SAMPLE_RISING; //可以设置双边沿触发，60hz
			break;
		default:
			break;
	}
	return 0;
}

static int ac020_enum_frame_interval(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_frame_interval_enum *fie)
{
	if (fie->index >= ARRAY_SIZE(ac020_framesizes))
		return -EINVAL;

	if (fie->code != MEDIA_BUS_FMT_YUYV8_2X8)
		return -EINVAL;

	fie->width = ac020_framesizes[fie->index].width;
	fie->height = ac020_framesizes[fie->index].height;
	fie->interval = ac020_framesizes[fie->index].max_fps;
	return 0;
}

static const struct v4l2_subdev_core_ops ac020_subdev_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.ioctl = ac020_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = ac020_compat_ioctl32,
#endif
};

static int ac020_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{

	fi->interval = ac020_framesizes[mode].max_fps;

	return 0;
}

static int ac020_querystd(struct v4l2_subdev *sd, v4l2_std_id *std) 
{ 
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	switch (mode)
	{
		case WN640_BT656_625_PAL:
		case WN640_BT656_625_PAL_P:
			*std = V4L2_STD_PAL; //656
			break;
		case WN640_BT656_525_NTSC:
			*std = V4L2_STD_NTSC; //525
			break;	
		case GL1280_BT1120:
			*std = V4L2_STD_ATSC; //1120
			break;
			
		default:
			break;
	}
	/*
				V4L2_STD_NTSC:INPUT_MODE_NTSC//525
			 	V4L2_STD_PAL: INPUT_MODE_PAL//625
				V4L2_STD_ATSC:INPUT_MODE_BT1120
				*/
	v4l_info(client,"tiny mode = %d\n",mode);		
	return 0; 
}

static const struct v4l2_subdev_video_ops ac020_subdev_video_ops = {
	.s_stream = ac020_s_stream,
	.g_mbus_config = ac020_g_mbus_config,
	.g_frame_interval = ac020_g_frame_interval,
	.querystd = ac020_querystd,
};

static const struct v4l2_subdev_pad_ops ac020_subdev_pad_ops = {
	.enum_mbus_code = ac020_enum_mbus_code,
	.enum_frame_size = ac020_enum_frame_sizes,
	.enum_frame_interval = ac020_enum_frame_interval,
	.get_fmt = ac020_get_fmt,
	.set_fmt = ac020_set_fmt,
	.get_selection = gl1280_get_selection,
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_ops ac020_subdev_ops = {
	.core  = &ac020_subdev_core_ops,
	.video = &ac020_subdev_video_ops,
	.pad   = &ac020_subdev_pad_ops,
};

static const struct v4l2_subdev_internal_ops ac020_subdev_internal_ops = {
	.open = ac020_open,
};
#endif




static int ac020_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	struct v4l2_subdev *sd;
	struct ac020 *ac020;
	char facing[2];
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	ac020 = devm_kzalloc(&client->dev, sizeof(*ac020), GFP_KERNEL);
	if (!ac020)
		return -ENOMEM;

	ret = of_property_read_u32(node, RKMODULE_CAMERA_MODULE_INDEX,
				   &ac020->module_index);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_FACING,
				       &ac020->module_facing);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_NAME,
				       &ac020->module_name);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_LENS_NAME,
				       &ac020->len_name);


	ac020->client = client;
 
 
	//获取设备树配置引脚，这里非常关键，RKISP1的驱动不会自动配置pinctrl切换引脚复用到dvp模式。
	//cif驱动包含引脚状态切换代码，无需再控制,注意引脚与网口复用
	/*
 	struct pinctrl *pinctrl=devm_pinctrl_get(&client->dev);
	struct pinctrl_state * pins_default = pinctrl_lookup_state(
					pinctrl,
					"rockchip,camera_default");
	if(IS_ERR(pins_default))
		dev_err(&client->dev, "%s: pinctrl_lookup_state error\n",
			__func__);				
	 ret = pinctrl_select_state(pinctrl, pins_default);
	if (ret < 0){
		dev_err(&client->dev, "%s: pinctrl_select_state error %d\n",
			__func__, ret);
		goto error;
	}*/
	v4l2_ctrl_handler_init(&ac020->ctrls, 2);
	ac020->link_frequency =
			v4l2_ctrl_new_std(&ac020->ctrls, &ac020_ctrl_ops,
					  V4L2_CID_PIXEL_RATE, 0,
					  ac020_PIXEL_RATE, 1,
					  ac020_PIXEL_RATE);

	v4l2_ctrl_new_std_menu_items(&ac020->ctrls, &ac020_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(ac020_test_pattern_menu) - 1,
				     0, 0, ac020_test_pattern_menu);
	ac020->sd.ctrl_handler = &ac020->ctrls;

	if (ac020->ctrls.error) {
		dev_err(&client->dev, "%s: control initialization error %d\n",
			__func__, ac020->ctrls.error);
		return  ac020->ctrls.error;
	}

	sd = &ac020->sd;
	v4l2_i2c_subdev_init(sd, client, &ac020_subdev_ops);


#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &ac020_subdev_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE ;
			 
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
	ac020->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &ac020->pad);
	if (ret < 0) {
		v4l2_ctrl_handler_free(&ac020->ctrls);
		return ret;
	}
#endif

	mutex_init(&ac020->lock);

	ac020_get_default_format(&ac020->format);
	ac020->frame_size = &ac020_framesizes[mode];


	memset(facing, 0, sizeof(facing));
	if (strcmp(ac020->module_facing, "back") == 0)
		facing[0] = 'b';
	else
		facing[0] = 'f';

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s_%s %s",
		 ac020->module_index, facing,
		 DRIVER_NAME, dev_name(sd->dev));
	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret)
		goto error;

	dev_info(&client->dev, "%s sensor driver registered !!\n", sd->name);

	return 0;

error:
	v4l2_ctrl_handler_free(&ac020->ctrls);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	mutex_destroy(&ac020->lock);
	return ret;
}

static int ac020_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ac020 *ac020 = to_ac020(sd);

	v4l2_ctrl_handler_free(&ac020->ctrls);
	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	mutex_destroy(&ac020->lock);

	return 0;
}

static const struct i2c_device_id ac020_id[] = {
	{ "ac020", 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, ac020_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ac020_of_match[] = {
	{ .compatible = "thermal_cam,ac020"  },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ac020_of_match);
#endif

static struct i2c_driver ac020_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(ac020_of_match),
	},
	.probe		= ac020_probe,
	.remove		= ac020_remove,
	.id_table	= ac020_id,
};

static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&ac020_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&ac020_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_AUTHOR("thermal_cam");
MODULE_DESCRIPTION(" ac020 ir camera driver");
MODULE_LICENSE("GPL v2");
