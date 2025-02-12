/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"

/* define device tree */
/* TODO: modify temp device tree name */
#ifndef CORFU_GPIO_DTNAME
#define CORFU_GPIO_DTNAME "mediatek,flashlights_corfu_gpio"
#endif

/* TODO: define driver name */
#define CORFU_NAME "flashlights-corfu-gpio"

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(corfu_mutex);
static struct work_struct corfu_work;

/* define pinctrl */
/* TODO: define pinctrl */
#define CORFU_PINCTRL_PIN_EN 1
#define CORFU_PINCTRL_PIN_CE 0
#define CORFU_PINCTRL_PINSTATE_LOW 0
#define CORFU_PINCTRL_PINSTATE_HIGH 1
#define CORFU_PINCTRL_STATE_EN_HIGH "flashlight_en_high"
#define CORFU_PINCTRL_STATE_EN_LOW  "flashlight_en_low"
#define CORFU_PINCTRL_STATE_CE_HIGH "flashlight_ce_high"
#define CORFU_PINCTRL_STATE_CE_LOW  "flashlight_ce_low"
static struct pinctrl *corfu_pinctrl;
static struct pinctrl_state *corfu_en_high;
static struct pinctrl_state *corfu_en_low;
static struct pinctrl_state *corfu_ce_high;
static struct pinctrl_state *corfu_ce_low;

/* define usage count */
static int use_count;

extern int flash_mode;

/* platform data */
struct corfu_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int corfu_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	corfu_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(corfu_pinctrl)) {
		pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(corfu_pinctrl);
		return ret;
	}

	/* TODO: Flashlight EN pin initialization */
	corfu_en_high = pinctrl_lookup_state(
			corfu_pinctrl, CORFU_PINCTRL_STATE_EN_HIGH);
	if (IS_ERR(corfu_en_high)) {
		pr_err("Failed to init (%s)\n", CORFU_PINCTRL_STATE_EN_HIGH);
		ret = PTR_ERR(corfu_en_high);
	}
	corfu_en_low = pinctrl_lookup_state(
			corfu_pinctrl, CORFU_PINCTRL_STATE_EN_LOW);
	if (IS_ERR(corfu_en_low)) {
		pr_err("Failed to init (%s)\n", CORFU_PINCTRL_STATE_EN_LOW);
		ret = PTR_ERR(corfu_en_low);
	}

	/* TODO: Flashlight CE pin initialization */
	corfu_ce_high = pinctrl_lookup_state(
			corfu_pinctrl, CORFU_PINCTRL_STATE_CE_HIGH);
	if (IS_ERR(corfu_ce_high)) {
		pr_err("Failed to init (%s)\n", CORFU_PINCTRL_STATE_CE_HIGH);
		ret = PTR_ERR(corfu_ce_high);
	}
	corfu_ce_low = pinctrl_lookup_state(
			corfu_pinctrl, CORFU_PINCTRL_STATE_CE_LOW);
	if (IS_ERR(corfu_ce_low)) {
		pr_err("Failed to init (%s)\n", CORFU_PINCTRL_STATE_CE_LOW);
		ret = PTR_ERR(corfu_ce_low);
	}

	return ret;
}

static int corfu_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(corfu_pinctrl)) {
		pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case CORFU_PINCTRL_PIN_EN:
		if (state == CORFU_PINCTRL_PINSTATE_LOW &&
				!IS_ERR(corfu_en_low))
			pinctrl_select_state(corfu_pinctrl, corfu_en_low);
		else if (state == CORFU_PINCTRL_PINSTATE_HIGH &&
				!IS_ERR(corfu_en_high))
			pinctrl_select_state(corfu_pinctrl, corfu_en_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case CORFU_PINCTRL_PIN_CE:
		if (state == CORFU_PINCTRL_PINSTATE_LOW &&
				!IS_ERR(corfu_ce_low))
			pinctrl_select_state(corfu_pinctrl, corfu_ce_low);
		else if (state == CORFU_PINCTRL_PINSTATE_HIGH &&
				!IS_ERR(corfu_ce_high))
			pinctrl_select_state(corfu_pinctrl, corfu_ce_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d)\n", pin, state);

	return ret;
}


/******************************************************************************
 * corfu operations
 *****************************************************************************/
/* flashlight enable function */
static int corfu_enable(void)
{
	/* TODO: wrap enable function */

	if(flash_mode)
		return corfu_pinctrl_set(CORFU_PINCTRL_PIN_EN, CORFU_PINCTRL_PINSTATE_HIGH);
	else
		return corfu_pinctrl_set(CORFU_PINCTRL_PIN_CE, CORFU_PINCTRL_PINSTATE_HIGH);
}

/* flashlight disable function */
static int corfu_disable(void)
{
	/* TODO: wrap disable function */
	corfu_pinctrl_set(CORFU_PINCTRL_PIN_EN, CORFU_PINCTRL_PINSTATE_LOW);
	return corfu_pinctrl_set(CORFU_PINCTRL_PIN_CE, CORFU_PINCTRL_PINSTATE_LOW);
}

/* set flashlight level */
static int corfu_set_level(int level)
{
	/* TODO: wrap set level function */
	return 0;
}

/* flashlight init */
static int corfu_init(void)
{
	/* TODO: wrap init function */
	corfu_pinctrl_set(CORFU_PINCTRL_PIN_CE,0);
	return corfu_pinctrl_set(CORFU_PINCTRL_PIN_EN, 0);
}

/* flashlight uninit */
static int corfu_uninit(void)
{
	/* TODO: wrap uninit function */
	corfu_pinctrl_set(CORFU_PINCTRL_PIN_CE,0);
	return corfu_pinctrl_set(CORFU_PINCTRL_PIN_EN, 0);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer corfu_timer;
static unsigned int corfu_timeout_ms;

static void corfu_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	corfu_disable();
}

static enum hrtimer_restart corfu_timer_func(struct hrtimer *timer)
{
	schedule_work(&corfu_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int corfu_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		corfu_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		corfu_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (corfu_timeout_ms) {
				s = corfu_timeout_ms / 1000;
				ns = corfu_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&corfu_timer, ktime,
						HRTIMER_MODE_REL);
			}
			corfu_enable();
		} else {
			corfu_disable();
			hrtimer_cancel(&corfu_timer);
		}
		break;
	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int corfu_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int corfu_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int corfu_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&corfu_mutex);
	if (set) {
		if (!use_count)
			ret = corfu_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = corfu_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&corfu_mutex);

	return ret;
}

static ssize_t corfu_strobe_store(struct flashlight_arg arg)
{
	corfu_set_driver(1);
	corfu_set_level(arg.level);
	corfu_timeout_ms = 0;
	corfu_enable();
	msleep(arg.dur);
	corfu_disable();
	corfu_set_driver(0);

	return 0;
}

static struct flashlight_operations corfu_ops = {
	corfu_open,
	corfu_release,
	corfu_ioctl,
	corfu_strobe_store,
	corfu_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int corfu_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * corfu_init();
	 */

	return 0;
}

static int corfu_parse_dt(struct device *dev,
		struct corfu_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		pr_info("Parse no dt, node.\n");
		return 0;
	}
	pr_info("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		pr_info("Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num *
			sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
				CORFU_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int corfu_probe(struct platform_device *pdev)
{
	struct corfu_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;

	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (corfu_pinctrl_init(pdev)) {
		pr_debug("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		err = corfu_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&corfu_work, corfu_work_disable);

	/* init timer */
	hrtimer_init(&corfu_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	corfu_timer.function = corfu_timer_func;
	corfu_timeout_ms = 100;

	/* init chip hw */
	corfu_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&corfu_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(CORFU_NAME, &corfu_ops)) {
			err = -EFAULT;
			goto err;
		}
	}

	pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int corfu_remove(struct platform_device *pdev)
{
	struct corfu_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(CORFU_NAME);

	/* flush work queue */
	flush_work(&corfu_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id corfu_gpio_of_match[] = {
	{.compatible = CORFU_GPIO_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, corfu_gpio_of_match);
#else
static struct platform_device corfu_gpio_platform_device[] = {
	{
		.name = CORFU_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, corfu_gpio_platform_device);
#endif

static struct platform_driver corfu_platform_driver = {
	.probe = corfu_probe,
	.remove = corfu_remove,
	.driver = {
		.name = CORFU_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = corfu_gpio_of_match,
#endif
	},
};

static int __init flashlight_corfu_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&corfu_gpio_platform_device);
	if (ret) {
		pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&corfu_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_corfu_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&corfu_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_corfu_init);
module_exit(flashlight_corfu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight CORFU GPIO Driver");

