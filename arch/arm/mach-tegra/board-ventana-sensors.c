/*
 * arch/arm/mach-tegra/board-ventana-sensors.c
 *
 * Copyright (c) 2011, NVIDIA CORPORATION, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of NVIDIA CORPORATION nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/mpu.h>
#include <linux/i2c/pca954x.h>
#include <linux/i2c/pca953x.h>
#include <linux/nct1008.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

#include <mach/gpio.h>

#include <media/ov5650.h>
#include <media/ov2710.h>
#ifdef CONFIG_VIDEO_YUV
#include <media/yuv_sensor.h>
#endif /* CONFIG_VIDEO_YUV */
#include <media/sh532u.h>
#include <media/ssl3250a.h>
#include <generated/mach-types.h>

#include "gpio-names.h"
#include "board.h"
#include "board-ventana.h"
#include "cpu-tegra.h"
#include <mach/board-ventana-misc.h>

#define LIGHT_IRQ_GPIO		TEGRA_GPIO_PZ2
#define AKM8975_IRQ_GPIO	TEGRA_GPIO_PN5
#define CAMERA_POWER_GPIO	TEGRA_GPIO_PK3
#define CAMERA_CSI_MUX_SEL_GPIO	TEGRA_GPIO_PBB4
#define CAMERA_FLASH_ACT_GPIO	TEGRA_GPIO_PD2
#define CAMERA_FLASH_STRB_GPIO	TEGRA_GPIO_PA0
#define NCT1008_THERM2_GPIO	TEGRA_GPIO_PN6
#define YUV_SENSOR_OE_L_GPIO    TEGRA_GPIO_PL2
#define CAMERA_FLASH_OP_MODE		0 /*0=I2C mode, 1=GPIO mode*/
#define CAMERA_FLASH_MAX_LED_AMP	7
#define CAMERA_FLASH_MAX_TORCH_AMP	11
#define CAMERA_FLASH_MAX_FLASH_AMP	31

#define YUV_SENSOR_RST_GPIO     TEGRA_GPIO_PU2
static struct regulator *reg_p_cam_avdd; /* LDO0 */
static struct regulator *reg_tegra_cam;    /* LDO6 */
enum {
	GPIO_FREE = 0,
	GPIO_REQUESTED,
};

struct camera_gpios {
	const char *name;
	int gpio;
	int enabled;
        int milliseconds;
        int requested;
};
#define CAMERA_GPIO(_name, _gpio, _enabled, _milliseconds, _requested)		\
	{						                        \
		.name = _name,				                        \
		.gpio = _gpio,				                        \
		.enabled = _enabled,			                        \
		.milliseconds = _milliseconds,				        \
		.requested = _requested,			                \
	}
static int ventana_camera_init(void)
{
#if 0
	int err;

	tegra_gpio_enable(CAMERA_POWER_GPIO);
	gpio_request(CAMERA_POWER_GPIO, "camera_power_en");
	gpio_direction_output(CAMERA_POWER_GPIO, 1);
	gpio_export(CAMERA_POWER_GPIO, false);

	tegra_gpio_enable(CAMERA_CSI_MUX_SEL_GPIO);
	gpio_request(CAMERA_CSI_MUX_SEL_GPIO, "camera_csi_sel");
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 0);
	gpio_export(CAMERA_CSI_MUX_SEL_GPIO, false);

	err = gpio_request(CAMERA_FLASH_ACT_GPIO, "torch_gpio_act");
	if (err < 0) {
		pr_err("gpio_request failed for gpio %d\n",
					CAMERA_FLASH_ACT_GPIO);
	} else {
		tegra_gpio_enable(CAMERA_FLASH_ACT_GPIO);
		gpio_direction_output(CAMERA_FLASH_ACT_GPIO, 0);
		gpio_export(CAMERA_FLASH_ACT_GPIO, false);
	}
#endif
	return 0;
}

#ifdef CONFIG_VIDEO_OV5650
/* left ov5650 is CAM2 which is on csi_a */
static int ventana_left_ov5650_power_on(void)
{
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 0);
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 1);
	gpio_direction_output(CAM2_LDO_SHUTDN_L_GPIO, 1);
	mdelay(5);
	gpio_direction_output(CAM2_PWR_DN_GPIO, 0);
	mdelay(5);
	gpio_direction_output(CAM2_RST_L_GPIO, 0);
	mdelay(1);
	gpio_direction_output(CAM2_RST_L_GPIO, 1);
	mdelay(20);
	return 0;
}

static int ventana_left_ov5650_power_off(void)
{
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 0);
	gpio_direction_output(CAM2_RST_L_GPIO, 0);
	gpio_direction_output(CAM2_PWR_DN_GPIO, 1);
	gpio_direction_output(CAM2_LDO_SHUTDN_L_GPIO, 0);
	return 0;
}

struct ov5650_platform_data ventana_left_ov5650_data = {
	.power_on = ventana_left_ov5650_power_on,
	.power_off = ventana_left_ov5650_power_off,
};

/* right ov5650 is CAM1 which is on csi_b */
static int ventana_right_ov5650_power_on(void)
{
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 1);
	gpio_direction_output(CAM1_LDO_SHUTDN_L_GPIO, 1);
	mdelay(5);
	gpio_direction_output(CAM1_PWR_DN_GPIO, 0);
	mdelay(5);
	gpio_direction_output(CAM1_RST_L_GPIO, 0);
	mdelay(1);
	gpio_direction_output(CAM1_RST_L_GPIO, 1);
	mdelay(20);
	return 0;
}

static int ventana_right_ov5650_power_off(void)
{
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 0);
	gpio_direction_output(CAM1_RST_L_GPIO, 0);
	gpio_direction_output(CAM1_PWR_DN_GPIO, 1);
	gpio_direction_output(CAM1_LDO_SHUTDN_L_GPIO, 0);
	return 0;
}

struct ov5650_platform_data ventana_right_ov5650_data = {
	.power_on = ventana_right_ov5650_power_on,
	.power_off = ventana_right_ov5650_power_off,
};
#endif /*CONFIG_VIDEO_OV5650*/

#ifdef CONFIG_VIDEO_OV2710
static int ventana_ov2710_power_on(void)
{
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 1);
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 1);
	gpio_direction_output(CAM3_LDO_SHUTDN_L_GPIO, 1);
	mdelay(5);
	gpio_direction_output(CAM3_PWR_DN_GPIO, 0);
	mdelay(5);
	gpio_direction_output(CAM3_RST_L_GPIO, 0);
	mdelay(1);
	gpio_direction_output(CAM3_RST_L_GPIO, 1);
	mdelay(20);
	return 0;
}

static int ventana_ov2710_power_off(void)
{
	gpio_direction_output(CAM3_RST_L_GPIO, 0);
	gpio_direction_output(CAM3_PWR_DN_GPIO, 1);
	gpio_direction_output(CAM3_LDO_SHUTDN_L_GPIO, 0);
	gpio_direction_output(AVDD_DSI_CSI_ENB_GPIO, 0);
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 0);
	return 0;
}

struct ov2710_platform_data ventana_ov2710_data = {
	.power_on = ventana_ov2710_power_on,
	.power_off = ventana_ov2710_power_off,
};
#endif /*CONFIG_VIDEO_OV2710*/

#ifdef CONFIG_VIDEO_YUV
static struct camera_gpios yuv_sensor_gpio_keys[] = {
	[0] = CAMERA_GPIO("mipi_power_en", AVDD_DSI_CSI_ENB_GPIO, 1, 0, GPIO_FREE),
	[1] = CAMERA_GPIO("cam_power_en", CAMERA_POWER_GPIO, 0, 0, GPIO_FREE),
//	[1] = CAMERA_GPIO("yuv_sensor_oe_l", YUV_SENSOR_OE_L_GPIO, 0, 0, GPIO_FREE),
	[2] = CAMERA_GPIO("yuv_sensor_rst_lo", YUV_SENSOR_RST_GPIO, 1, 0, GPIO_FREE),	//Kenji+
};

static int yuv_sensor_power_on(void)
{
	int ret;
	int i;

  printk("yuv_sensor_power_on+\n");
  if (!reg_tegra_cam) {
//    reg_tegra_cam = regulator_get(NULL, "tegra_camera");
    reg_tegra_cam = regulator_get(NULL, "vcsi");
    if (IS_ERR_OR_NULL(reg_tegra_cam)) {
      pr_err("EP101_ov5640_power_on LDO6: p_tegra_cam failed\n");
      regulator_put(reg_p_cam_avdd);
      reg_tegra_cam = NULL;
      return PTR_ERR(reg_tegra_cam);
    }
    regulator_set_voltage(reg_tegra_cam, 1800000, 1800000);
    pr_err("EP101_ov5640_power_on LDO6: p_tegra_cam OK\n");
    regulator_enable(reg_tegra_cam);
  }
  msleep(1);
  if (!reg_p_cam_avdd) {
    reg_p_cam_avdd = regulator_get(NULL, "p_cam_avdd");
    if (IS_ERR_OR_NULL(reg_p_cam_avdd)) {
      pr_err("EP101_ov5640_power_on LDO0: p_cam_avdd failed\n");
      reg_p_cam_avdd = NULL;
      return PTR_ERR(reg_p_cam_avdd);
    }
    regulator_set_voltage(reg_p_cam_avdd, 2850000, 2850000);
    pr_err("EP101_ov5640_power_on LDO0: p_cam_avdd OK\n");
    regulator_enable(reg_p_cam_avdd);
  }
  msleep(5);
  for (i = 0; i < ARRAY_SIZE(yuv_sensor_gpio_keys); i++) {
    tegra_gpio_enable(yuv_sensor_gpio_keys[i].gpio);
    pr_info("gpio %d set to %d\n",yuv_sensor_gpio_keys[i].gpio, yuv_sensor_gpio_keys[i].enabled);
    ret = gpio_request(yuv_sensor_gpio_keys[i].gpio,
    yuv_sensor_gpio_keys[i].name);
    if (ret < 0) {
      pr_err("%s: gpio_request failed for gpio #%d\n",
      __func__, i);
      goto fail;
    }
    msleep(1);
    gpio_direction_output(yuv_sensor_gpio_keys[i].gpio,
    yuv_sensor_gpio_keys[i].enabled);
    gpio_export(yuv_sensor_gpio_keys[i].gpio, false);
  }
  printk("yuv_sensor_power_on-\n");
	return 0;
fail:
  regulator_disable(reg_p_cam_avdd);
  regulator_disable(reg_tegra_cam);
  regulator_put(reg_p_cam_avdd);
  regulator_put(reg_tegra_cam);
	while (i--)
		gpio_free(yuv_sensor_gpio_keys[i].gpio);
	return ret;
}

static int yuv_sensor_power_off(void)
{
  int i;

  printk("yuv_sensor_power_off+\n");
  for (i=ARRAY_SIZE(yuv_sensor_gpio_keys)-1;i>=0; i--) {
    pr_info("gpio %d set to %d\n",yuv_sensor_gpio_keys[i].gpio, !(yuv_sensor_gpio_keys[i].enabled));
    gpio_direction_output(yuv_sensor_gpio_keys[i].gpio,
    !(yuv_sensor_gpio_keys[i].enabled));
    gpio_export(yuv_sensor_gpio_keys[i].gpio, false);
  }
  msleep(1);
  if(reg_p_cam_avdd){
    regulator_disable(reg_p_cam_avdd);
    regulator_put(reg_p_cam_avdd);
    reg_p_cam_avdd = NULL;
  	pr_err("EP101_ov5640_power_off LDO0: p_cam_avdd OK\n");
  }
  msleep(35);
  if(reg_tegra_cam){
  	regulator_disable(reg_tegra_cam);
  	regulator_put(reg_tegra_cam);
    reg_tegra_cam = NULL;
  	pr_err("EP101_ov5640_power_off LDO6: p_tegra_cam OK\n");
  }

  i = ARRAY_SIZE(yuv_sensor_gpio_keys);
  while (i--)
    gpio_free(yuv_sensor_gpio_keys[i].gpio);
  printk("yuv_sensor_power_off-\n");
  return 0;
}

struct yuv_sensor_platform_data yuv_sensor_data = {
	.power_on = yuv_sensor_power_on,
	.power_off = yuv_sensor_power_off,
};

#define FRONT_CAMERA_POWER_GPIO	TEGRA_GPIO_PK4
#define FRONT_YUV_SENSOR_RST_GPIO     TEGRA_GPIO_PU3

static struct camera_gpios yuv_front_sensor_gpio_keys[] = {
	[0] = CAMERA_GPIO("mipi_power_en", AVDD_DSI_CSI_ENB_GPIO, 1, 0, GPIO_FREE),
	[1] = CAMERA_GPIO("cam_power_en", FRONT_CAMERA_POWER_GPIO, 0, 0, GPIO_FREE),
	[2] = CAMERA_GPIO("yuv_sensor_rst_lo", FRONT_YUV_SENSOR_RST_GPIO, 1, 0, GPIO_FREE),
};

static int yuv_front_sensor_power_on(void)
{
	int ret;
	int i;
  printk("yuv_front_sensor_power_on+\n");
  if (!reg_tegra_cam) {
//    reg_tegra_cam = regulator_get(NULL, "tegra_camera");
    reg_tegra_cam = regulator_get(NULL, "vcsi");
    if (IS_ERR_OR_NULL(reg_tegra_cam)) {
      pr_err("IO Power Failed LDO6: p_tegra_cam failed\n");
      regulator_put(reg_p_cam_avdd);
      reg_tegra_cam = NULL;
      return PTR_ERR(reg_tegra_cam);
    }
    regulator_set_voltage(reg_tegra_cam, 1800000, 1800000);
    pr_err("EP101_mi1040_power_on LDO6: p_tegra_cam OK\n");
    regulator_enable(reg_tegra_cam);
  }
  if (!reg_p_cam_avdd) {
    reg_p_cam_avdd = regulator_get(NULL, "p_cam_avdd");
    if (IS_ERR_OR_NULL(reg_p_cam_avdd)) {
      pr_err("AVDD Failed: p_cam_avdd failed\n");
      reg_p_cam_avdd = NULL;
      return PTR_ERR(reg_p_cam_avdd);
    }
    regulator_set_voltage(reg_p_cam_avdd, 2850000, 2850000);
    pr_err("EP101_mi1040_power_on LDO0: p_cam_avdd OK\n");
    regulator_enable(reg_p_cam_avdd);
  }

  for (i = 0; i < ARRAY_SIZE(yuv_front_sensor_gpio_keys); i++) {
    tegra_gpio_enable(yuv_front_sensor_gpio_keys[i].gpio);
    pr_info("gpio %d set to %d\n",yuv_front_sensor_gpio_keys[i].gpio,
      yuv_front_sensor_gpio_keys[i].enabled);
    ret = gpio_request(yuv_front_sensor_gpio_keys[i].gpio,
      yuv_front_sensor_gpio_keys[i].name);
    if (ret < 0) {
      pr_err("%s: gpio_request failed for gpio #%d\n",
      __func__, i);
      goto fail;
    }
    gpio_direction_output(yuv_front_sensor_gpio_keys[i].gpio,
    yuv_front_sensor_gpio_keys[i].enabled);
    gpio_export(yuv_front_sensor_gpio_keys[i].gpio, false);
  }
  printk("yuv_front_sensor_power_on-\n");
	return 0;
fail:
  regulator_disable(reg_p_cam_avdd);
  regulator_disable(reg_tegra_cam);
  regulator_put(reg_p_cam_avdd);
  regulator_put(reg_tegra_cam);
	while (i--)
		gpio_free(yuv_sensor_gpio_keys[i].gpio);
	return ret;
}

static int yuv_front_sensor_power_off(void)
{
  int i;

  printk("yuv_front_sensor_power_off+\n");
  for (i=ARRAY_SIZE(yuv_front_sensor_gpio_keys)-1;i>=0; i--) {
    pr_info("gpio %d set to %d\n",yuv_front_sensor_gpio_keys[i].gpio, !(yuv_front_sensor_gpio_keys[i].enabled));
    gpio_direction_output(yuv_front_sensor_gpio_keys[i].gpio,
    !(yuv_front_sensor_gpio_keys[i].enabled));
    gpio_export(yuv_front_sensor_gpio_keys[i].gpio, false);
  }
  if(reg_p_cam_avdd){
  	regulator_disable(reg_p_cam_avdd);
    regulator_put(reg_p_cam_avdd);
    reg_p_cam_avdd = NULL;
  	pr_err("EP101_mi1040_power_off LDO0: p_cam_avdd OK\n");
  }

  msleep(20);

  if(reg_tegra_cam){
  	regulator_disable(reg_tegra_cam);
  	regulator_put(reg_tegra_cam);
    reg_tegra_cam = NULL;
  	pr_err("EP101_mi1040_power_off LDO6: p_tegra_cam OK\n");
  }

  i = ARRAY_SIZE(yuv_front_sensor_gpio_keys);
  while (i--)
    gpio_free(yuv_front_sensor_gpio_keys[i].gpio);
  printk("yuv_front_sensor_power_off-\n");
  return 0;
}

struct yuv_sensor_platform_data yuv_front_sensor_data = {
	.power_on = yuv_front_sensor_power_on,
	.power_off = yuv_front_sensor_power_off,
};

struct yuv_sensor_platform_data yuv_rear_sensor2_data = {
	.power_on = yuv_sensor_power_on,
	.power_off = yuv_sensor_power_off,
};

#endif /* CONFIG_VIDEO_YUV */

#ifdef CONFIG_VIDEO_SH532U
static struct sh532u_platform_data sh532u_left_pdata = {
	.num		= 1,
	.sync		= 2,
	.dev_name	= "focuser",
	.gpio_reset	= CAM2_RST_L_GPIO,
	.gpio_en	= CAM2_LDO_SHUTDN_L_GPIO,
};

static struct sh532u_platform_data sh532u_right_pdata = {
	.num		= 2,
	.sync		= 1,
	.dev_name	= "focuser",
	.gpio_reset	= CAM1_RST_L_GPIO,
	.gpio_en	= CAM1_LDO_SHUTDN_L_GPIO,
};
#endif

#ifdef CONFIG_TORCH_SSL3250A
static struct nvc_torch_pin_state ventana_ssl3250a_pinstate = {
	.mask		= 0x0040, /* VGP6 */
	.values		= 0x0040,
};

static struct ssl3250a_platform_data ventana_ssl3250a_pdata = {
	.dev_name	= "torch",
	.pinstate	= &ventana_ssl3250a_pinstate,
	.gpio_act	= CAMERA_FLASH_ACT_GPIO,
};
#endif

#ifdef CONFIG_SENSORS_AK8975
static void ventana_akm8975_init(void)
{
	tegra_gpio_enable(AKM8975_IRQ_GPIO);
	gpio_request(AKM8975_IRQ_GPIO, "akm8975");
	gpio_direction_input(AKM8975_IRQ_GPIO);
}
#endif

static void ventana_nct1008_init(void)
{
	tegra_gpio_enable(NCT1008_THERM2_GPIO);
	gpio_request(NCT1008_THERM2_GPIO, "temp_alert");
	gpio_direction_input(NCT1008_THERM2_GPIO);
}

static struct nct1008_platform_data ventana_nct1008_pdata = {
	.supported_hwrev = true,
	.ext_range = false,
	.conv_rate = 0x08,
	.offset = 0,
	.hysteresis = 0,
	.shutdown_ext_limit = 115,
	.shutdown_local_limit = 120,
	.throttling_ext_limit = 90,
	.alarm_fn = tegra_throttling_enable,
};

static const struct i2c_board_info ventana_i2c0_board_info[] = {
	{
		I2C_BOARD_INFO("isl29018", 0x44),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PZ2),
	},

	{
		I2C_BOARD_INFO("ami304", 0x0E),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PN5),
	},

	{
		I2C_BOARD_INFO("kxtf9", 0x0F),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PN4),
	},
};

static const struct i2c_board_info ventana_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO("bq20z45-battery", 0x0B),
		//.irq = TEGRA_GPIO_TO_IRQ(AC_PRESENT_GPIO),
	},
	{
                I2C_BOARD_INFO("asusec", 0x19),
                .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PS2),
        },

};

static struct pca953x_platform_data ventana_tca6416_data = {
	.gpio_base      = TEGRA_NR_GPIOS + 4, /* 4 gpios are already requested by tps6586x */
};

static struct pca954x_platform_mode ventana_pca9546_modes[] = {
	{ .adap_id = 6, .deselect_on_exit = 1 }, /* REAR CAM1 */
	{ .adap_id = 7, .deselect_on_exit = 1 }, /* REAR CAM2 */
	{ .adap_id = 8, .deselect_on_exit = 1 }, /* FRONT CAM3 */
};

static struct pca954x_platform_data ventana_pca9546_data = {
	.modes	  = ventana_pca9546_modes,
	.num_modes      = ARRAY_SIZE(ventana_pca9546_modes),
};

static const struct i2c_board_info ventana_i2c3_board_info_tca6416[] = {
	{
		I2C_BOARD_INFO("tca6416", 0x20),
		.platform_data = &ventana_tca6416_data,
	},
};

static const struct i2c_board_info ventana_i2c3_board_info_pca9546[] = {
	{
		I2C_BOARD_INFO("pca9546", 0x70),
		.platform_data = &ventana_pca9546_data,
	},
};

#ifdef CONFIG_TORCH_SSL3250A
static const struct i2c_board_info ventana_i2c3_board_info_ssl3250a[] = {
	{
		I2C_BOARD_INFO("ssl3250a", 0x30),
		.platform_data = &ventana_ssl3250a_pdata,
	},
};
#endif

static struct i2c_board_info ventana_i2c4_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.irq = TEGRA_GPIO_TO_IRQ(NCT1008_THERM2_GPIO),
		.platform_data = &ventana_nct1008_pdata,
	},

#ifdef CONFIG_SENSORS_AK8975
	{
		I2C_BOARD_INFO("akm8975", 0x0C),
		.irq = TEGRA_GPIO_TO_IRQ(AKM8975_IRQ_GPIO),
	},
#endif
};

#ifdef CONFIG_VIDEO_OV5650
static struct i2c_board_info ventana_i2c6_board_info[] = {
	{
		I2C_BOARD_INFO("ov5650R", 0x36),
		.platform_data = &ventana_right_ov5650_data,
	},
	{
		I2C_BOARD_INFO("sh532u", 0x72),
		.platform_data = &sh532u_right_pdata,
	},
};

static struct i2c_board_info ventana_i2c7_board_info[] = {
	{
		I2C_BOARD_INFO("ov5650L", 0x36),
		.platform_data = &ventana_left_ov5650_data,
	},
	{
		I2C_BOARD_INFO("sh532u", 0x72),
		.platform_data = &sh532u_left_pdata,
	},
};
#endif /*CONFIG_VIDEO_OV5650*/

static struct i2c_board_info ventana_i2c3_board_info[] = {
#ifdef CONFIG_VIDEO_OV5650
               I2C_BOARD_INFO("ov5650", 0x36),
               .platform_data = &ventana_ov5650_data,
#endif
#ifdef CONFIG_VIDEO_YUV
	{
		I2C_BOARD_INFO("ov5640", 0x3C),
		.platform_data = &yuv_sensor_data,
	},
#endif /* CONFIG_VIDEO_YUV */
};

#ifdef CONFIG_VIDEO_YUV
static struct i2c_board_info front_sensor_i2c3_board_info[] = {  //ddebug
	{
		I2C_BOARD_INFO("mi1040", 0x48),
		.platform_data = &yuv_front_sensor_data,
	},
};
#ifdef CONFIG_VIDEO_YUV_MI5140
static struct i2c_board_info rear_sensor2_i2c3_board_info[] = {  //ddebug
	{
		I2C_BOARD_INFO("mi5140", 0x3D),
		.platform_data = &yuv_rear_sensor2_data,
	}
};
#endif /* CONFIG_VIDEO_YUV_MI5140 */
#endif /* CONFIG_VIDEO_YUV */

#ifdef CONFIG_VIDEO_OV2710
static struct i2c_board_info ventana_i2c8_board_info[] = {
	{
		I2C_BOARD_INFO("ov2710", 0x36),
		.platform_data = &ventana_ov2710_data,
	},
};
#endif /* CONFIG_VIDEO_OV2710 */

#ifdef CONFIG_MPU_SENSORS_MPU3050
static struct mpu_platform_data mpu3050_data = {
	.int_config  = 0x10,
	.orientation = MPU_GYRO_ORIENTATION,  /* Orientation matrix for MPU on ventana */
	.level_shifter = 0,
	.accel = {
#ifdef CONFIG_MPU_SENSORS_KXTF9
	.get_slave_descr = get_accel_slave_descr,
#else
	.get_slave_descr = NULL,
#endif
	.adapt_num   = 0,
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	.address     = MPU_ACCEL_ADDR,
	.orientation = MPU_ACCEL_ORIENTATION,  /* Orientation matrix for Kionix on ventana */
	.irq =  TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PN4),
	},

	.compass = {
#ifdef CONFIG_MPU_SENSORS_AMI304
	.get_slave_descr = get_compass_slave_descr,
	.irq =  TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PN5),
#else
	.get_slave_descr = NULL,
#endif
	.adapt_num   = 0,            /* bus number 4 on ventana */
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	.address     = MPU_COMPASS_ADDR,
	.orientation = MPU_COMPASS_ORIENTATION,  /* Orientation matrix for AKM on ventana */
	},
};

static struct i2c_board_info __initdata mpu3050_i2c0_boardinfo[] = {
	{
		I2C_BOARD_INFO(MPU_GYRO_NAME, MPU_GYRO_ADDR),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PZ4),
		.platform_data = &mpu3050_data,
	},
};

struct mpu_orientation_def{
	__s8 gyro_orientation[9];
	__s8 accel_orientation[9];
	__s8 compass_orientation[9];
};
static void ventana_mpuirq_init(void)
{
	int ret = 0;
	pr_info("*** MPU START *** ventana_mpuirq_init...\n");

#if	MPU_ACCEL_IRQ_GPIO	/* ACCEL-IRQ assignment */
	pr_info("Setting MPU_ACCEL_IRQ_GPIO ...\n");
	tegra_gpio_enable(MPU_ACCEL_IRQ_GPIO);
	ret = gpio_request(MPU_ACCEL_IRQ_GPIO, MPU_ACCEL_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}
	ret = gpio_direction_input(MPU_ACCEL_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_ACCEL_IRQ_GPIO);
		return;
	}
#endif

	tegra_gpio_enable(TEGRA_GPIO_PZ4);
	gpio_request(TEGRA_GPIO_PZ4, MPU_GYRO_NAME);
	gpio_direction_input(TEGRA_GPIO_PZ4);
	/*
	  Check Project ID for load different orientation
	  TF-101 : ASUSGetProjectID()==101
	  SL-101 : ASUSGetProjectID()==102
	*/
	if(ASUSGetProjectID()==101)
	{
		pr_info("initial mpu with TF-101 config...\n");
	}
	else if(ASUSGetProjectID()==102)
	{
		struct mpu_orientation_def SL_101 = {
			SL101_GYRO_ORIENTATION,
			SL101_ACCEL_ORIENTATION,
			SL101_COMPASS_ORIENTATION,};

		pr_info("initial mpu with SL-101 config...\n");
		memcpy(mpu3050_data.orientation, SL_101.gyro_orientation, sizeof(mpu3050_data.orientation));
		memcpy(mpu3050_data.accel.orientation, SL_101.accel_orientation, sizeof(mpu3050_data.accel.orientation));
		memcpy(mpu3050_data.compass.orientation, SL_101.compass_orientation, sizeof(mpu3050_data.compass.orientation));
	}
	pr_info("*** MPU END *** ventana_mpuirq_init...\n");
	i2c_register_board_info(0, ventana_i2c0_board_info,
		ARRAY_SIZE(ventana_i2c0_board_info));
	i2c_register_board_info(0, mpu3050_i2c0_boardinfo,
		ARRAY_SIZE(mpu3050_i2c0_boardinfo));
}
#endif

static const struct i2c_board_info light_i2c1_board_info[] = {
#ifdef CONFIG_SENSORS_AL3000
	{
		I2C_BOARD_INFO("al3000a", 0x1c),
		.irq = TEGRA_GPIO_TO_IRQ(LIGHT_IRQ_GPIO),
	},
#endif
};

int __init ventana_sensors_init(void)
{
	struct board_info BoardInfo;

#ifdef CONFIG_SENSORS_AK8975
	ventana_akm8975_init();
#endif
#ifdef CONFIG_MPU_SENSORS_MPU3050
	ventana_mpuirq_init();
#endif
	ventana_camera_init();
	ventana_nct1008_init();

	tegra_get_board_info(&BoardInfo);

	/*
	 * battery driver is supported on FAB.D boards and above only,
	 * since they have the necessary hardware rework
	 */
	//if (BoardInfo.sku > 0) {
		i2c_register_board_info(2, ventana_i2c2_board_info,
			ARRAY_SIZE(ventana_i2c2_board_info));
	//}

	i2c_register_board_info(2, light_i2c1_board_info,
		ARRAY_SIZE(light_i2c1_board_info));

#ifdef CONFIG_TORCH_SSL3250A
	i2c_register_board_info(3, ventana_i2c3_board_info_ssl3250a,
		ARRAY_SIZE(ventana_i2c3_board_info_ssl3250a));
#endif
//+ ov5640 rear camera
	i2c_register_board_info(3, ventana_i2c3_board_info,
		ARRAY_SIZE(ventana_i2c3_board_info));
//-
//+ Front camera
	i2c_register_board_info(3, front_sensor_i2c3_board_info,
		ARRAY_SIZE(front_sensor_i2c3_board_info));
//-
#ifdef CONFIG_VIDEO_YUV_MI5140
	i2c_register_board_info(3, rear_sensor2_i2c3_board_info,
		ARRAY_SIZE(rear_sensor2_i2c3_board_info));
#endif

	i2c_register_board_info(4, ventana_i2c4_board_info,
		ARRAY_SIZE(ventana_i2c4_board_info));

#ifdef CONFIG_VIDEO_OV5650
	i2c_register_board_info(6, ventana_i2c6_board_info,
		ARRAY_SIZE(ventana_i2c6_board_info));

	i2c_register_board_info(7, ventana_i2c7_board_info,
		ARRAY_SIZE(ventana_i2c7_board_info));
#endif
#ifdef CONFIG_VIDEO_OV2710
	i2c_register_board_info(8, ventana_i2c8_board_info,
		ARRAY_SIZE(ventana_i2c8_board_info));
#endif
	return 0;
}

#ifdef CONFIG_TEGRA_CAMERA
#ifdef CONFIG_VIDEO_OV5650
struct tegra_camera_gpios {
	const char *name;
	int gpio;
	int enabled;
};

#define TEGRA_CAMERA_GPIO(_name, _gpio, _enabled)		\
	{						\
		.name = _name,				\
		.gpio = _gpio,				\
		.enabled = _enabled,			\
	}

static struct tegra_camera_gpios ventana_camera_gpio_keys[] = {
	[0] = TEGRA_CAMERA_GPIO("en_avdd_csi", AVDD_DSI_CSI_ENB_GPIO, 1),
	[1] = TEGRA_CAMERA_GPIO("cam_i2c_mux_rst_lo", CAM_I2C_MUX_RST_GPIO, 1),

	[2] = TEGRA_CAMERA_GPIO("cam2_ldo_shdn_lo", CAM2_LDO_SHUTDN_L_GPIO, 0),
	[3] = TEGRA_CAMERA_GPIO("cam2_af_pwdn_lo", CAM2_AF_PWR_DN_L_GPIO, 0),
	[4] = TEGRA_CAMERA_GPIO("cam2_pwdn", CAM2_PWR_DN_GPIO, 0),
	[5] = TEGRA_CAMERA_GPIO("cam2_rst_lo", CAM2_RST_L_GPIO, 1),

	[6] = TEGRA_CAMERA_GPIO("cam3_ldo_shdn_lo", CAM3_LDO_SHUTDN_L_GPIO, 0),
	[7] = TEGRA_CAMERA_GPIO("cam3_af_pwdn_lo", CAM3_AF_PWR_DN_L_GPIO, 0),
	[8] = TEGRA_CAMERA_GPIO("cam3_pwdn", CAM3_PWR_DN_GPIO, 0),
	[9] = TEGRA_CAMERA_GPIO("cam3_rst_lo", CAM3_RST_L_GPIO, 1),

	[10] = TEGRA_CAMERA_GPIO("cam1_ldo_shdn_lo", CAM1_LDO_SHUTDN_L_GPIO, 0),
	[11] = TEGRA_CAMERA_GPIO("cam1_af_pwdn_lo", CAM1_AF_PWR_DN_L_GPIO, 0),
	[12] = TEGRA_CAMERA_GPIO("cam1_pwdn", CAM1_PWR_DN_GPIO, 0),
	[13] = TEGRA_CAMERA_GPIO("cam1_rst_lo", CAM1_RST_L_GPIO, 1),
};

int __init ventana_camera_late_init(void)
{
	int ret;
	int i;
	struct regulator *cam_ldo6 = NULL;

	if (!machine_is_ventana())
		return 0;

	cam_ldo6 = regulator_get(NULL, "vdd_ldo6");
	if (IS_ERR_OR_NULL(cam_ldo6)) {
		pr_err("%s: Couldn't get regulator ldo6\n", __func__);
		return PTR_ERR(cam_ldo6);
	}

	ret = regulator_enable(cam_ldo6);
	if (ret){
		pr_err("%s: Failed to enable ldo6\n", __func__);
		goto fail_put_regulator;
	}

	i2c_new_device(i2c_get_adapter(3), ventana_i2c3_board_info_tca6416);

	for (i = 0; i < ARRAY_SIZE(ventana_camera_gpio_keys); i++) {
		ret = gpio_request(ventana_camera_gpio_keys[i].gpio,
			ventana_camera_gpio_keys[i].name);
		if (ret < 0) {
			pr_err("%s: gpio_request failed for gpio #%d\n",
				__func__, i);
			goto fail_free_gpio;
		}
		gpio_direction_output(ventana_camera_gpio_keys[i].gpio,
			ventana_camera_gpio_keys[i].enabled);
		gpio_export(ventana_camera_gpio_keys[i].gpio, false);
	}

	i2c_new_device(i2c_get_adapter(3), ventana_i2c3_board_info_pca9546);

	ventana_ov2710_power_off();
	ventana_left_ov5650_power_off();
	ventana_right_ov5650_power_off();

	ret = regulator_disable(cam_ldo6);
	if (ret){
		pr_err("%s: Failed to disable ldo6\n", __func__);
		goto fail_free_gpio;
	}

	regulator_put(cam_ldo6);
	return 0;

fail_free_gpio:
	while (i--)
		gpio_free(ventana_camera_gpio_keys[i].gpio);

fail_put_regulator:
	regulator_put(cam_ldo6);
	return ret;
}

late_initcall(ventana_camera_late_init);
#endif

#endif /* CONFIG_TEGRA_CAMERA */
