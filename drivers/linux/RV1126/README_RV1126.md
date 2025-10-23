# RV1126开发板使用及配置： 

## 基础命令

- 检测机芯：adb devices
- 连接机芯：adb shell
- 上传文件：adb push 源地址（PC上地址） 目标地址（开发板上地址）
- 下载文件：adb pull 源地址（开发板上地址） 目标地址（PC上地址）

## 修改内核驱动

当基于RV1126进行显示出图和发送命令时，需要修改内核以支持需要的出图及命令通路的接口。以下为基于RV1126的显示出图，支持MIPI和UART的内核修改流程。

### 打开串口配置

内核文件 kernel/drivers/usb/serial/Makefile 把原来里面的配置项直接改为y,表示编译进内核，需要添加使能下面几款芯片的驱动

```
obj-y    += ch341.o
obj-y    += cp210x.o
obj-y    += ftdi_sio.o
obj-y    += p12303.o
```

举例：

```
obj-$(CONFIG_USB_SERIAL_CH341) += ch341.o
#修改为
obj-y += ch341.o
```

### 修改配置和设备树

配置文件aio-rv1126-opemwrt.mk 放置在内核目录device/rockchip/rv1126_rv1109下。
新设备树文件rv1126-firefly-openwrt.dts放置在内核目录kernel/arch/arm/boot/dts下。

### 添加驱动文件

#### 替换USB驱动

需要把驱动文件uvc_v4l2.c.c 替换driver/media/usb

#### 添加MIPI驱动

需要把驱动文件ac020-mipi.c 放到driver/media/i2c下，**ac020-mipi.c 支持wn640和GL1280,I2C通道无法区分，可以同时出图**

在内核driver/media/i2c/Makefile里面添加自定义的驱动

```
obj-y += ac020.o
```

需要注意，驱动的宽高需要添加机芯中对应的宽高（line313）

```c
static  struct ac020_framesize ac020_framesizes[] = {
    { /* 640*/
        .width        = 640,
        .height        = 900,
        .max_fps = {
            .numerator = 30,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,
    },
    { /* 1280 */
        .width        = 1280,
        .height        = 1797,
        .max_fps = {
            .numerator = 30,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_2X8
    }
};
```

**修改MIPI出图datatype**

rv1126当前驱动，按照如下修改以支持datatype 0x2A。
mipi通道0：kernel/drivers/media/platform/rockchip/cif/capture.c 修改get_data_type函数，将0x1e改为0x2a 
mipi通道1：kernel/drivers/media/platform/rockchip/isp/regs.h 398行里面改成如下：

```c
#define CIF_CSI2_DT_YUV422_8b            0x2A
```

#### 添加DVP驱动

需要把驱动文件ac020-all.c 放到driver/media/i2c下，**ac020-all.c 支持dvp bt656 bt1120 逐行、隔行**

在内核driver/media/i2c/Makefile里面添加自定义的驱动

```
obj-y += ac020.o
```

需要注意，驱动的宽高需要添加机芯中对应的宽高（line313）

```c
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
```

### 编译内核

需要参考RV1126内核的[官网编译方式]([源码编译 — Firefly Wiki (t-firefly.com)](https://wiki.t-firefly.com/zh_CN/CORE-1126-JD4/Source_code.html#))，获取源码并编译。

在内核目录下调用build.sh文件如下操作：

./build.sh device/rockchip/rv1126_rv1109/aio-rv1126-opemwrt.mk #进行切换配置
./build.sh kernel  #编译内核,生成新内核文件kernel/zboot.img

编译的内核为在kernel目录下的zboot.img

### 更新内核

```shell
adb push ./zboot.img / #推送内核
adb shell    #进入shell
dd if=zboot.img of=/dev/mmcblk0p3  #更新内核，直接内核文件覆盖分区3即可
reboot    #重启系统
```

## 注意事项：

1.自动出图的机芯无法直接切换出图需要先停图再出图，应用可以先发送stop命令后再start

2.1280显示需要缩小

3.生成的节点是/dev/video0，命令可以通过/dev/v4l-subdev1节点发送，可以 media-ctl -p -d /dev/media0命令查询

4.板子程序rkmedia_vi_venc_rtsp_vo_test可以出图显示测试

5.CIF默认最新分辨率640*480限制，更小分辨率支持需修改kernel/drivers/media/platform/rockchip/cif/dev.h：配置成最小分辨率支持

#define RKCIF_DEFAULT_WIDTH	160

#define RKCIF_DEFAULT_HEIGHT	120

