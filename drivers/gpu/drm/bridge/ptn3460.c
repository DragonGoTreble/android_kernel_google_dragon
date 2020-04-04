/*
 * NXP PTN3460 DP/LVDS bridge driver
 *
 * Copyright (C) 2013 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include "bridge/ptn3460.h"

#include "drm_crtc.h"
#include "drm_crtc_helper.h"
#include "drm_edid.h"
#include "drmP.h"

#define PTN3460_EDID_ADDR			0x0
#define PTN3460_EDID_EMULATION_ADDR		0x84
#define PTN3460_EDID_ENABLE_EMULATION		0
#define PTN3460_EDID_EMULATION_SELECTION	1
#define PTN3460_EDID_SRAM_LOAD_ADDR		0x85

struct ptn3460_bridge {
	struct drm_connector connector;
	struct i2c_client *client;
	struct drm_encoder *encoder;
	struct drm_bridge bridge;
	struct edid *edid;
	int gpio_pd_n;
	int gpio_rst_n;
	u32 edid_emulation;
	bool enabled;
};

static inline struct ptn3460_bridge *
		bridge_to_ptn3460(struct drm_bridge *bridge)
{
	return container_of(bridge, struct ptn3460_bridge, bridge);
}

static inline struct ptn3460_bridge *
		connector_to_ptn3460(struct drm_connector *connector)
{
	return container_of(connector, struct ptn3460_bridge, connector);
}

static int ptn3460_read_bytes(struct ptn3460_bridge *ptn_bridge, char addr,
		u8 *buf, int len)
{
	int ret;

	ret = i2c_master_send(ptn_bridge->client, &addr, 1);
	if (ret <= 0) {
		DRM_ERROR("Failed to send i2c command, ret=%d\n", ret);
		return ret;
	}

	ret = i2c_master_recv(ptn_bridge->client, buf, len);
	if (ret <= 0) {
		DRM_ERROR("Failed to recv i2c data, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int ptn3460_write_byte(struct ptn3460_bridge *ptn_bridge, char addr,
		char val)
{
	int ret;
	char buf[2];

	buf[0] = addr;
	buf[1] = val;

	ret = i2c_master_send(ptn_bridge->client, buf, ARRAY_SIZE(buf));
	if (ret <= 0) {
		DRM_ERROR("Failed to send i2c command, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int ptn3460_select_edid(struct ptn3460_bridge *ptn_bridge)
{
	int ret;
	char val;

	/* Load the selected edid into SRAM (accessed at PTN3460_EDID_ADDR) */
	ret = ptn3460_write_byte(ptn_bridge, PTN3460_EDID_SRAM_LOAD_ADDR,
			ptn_bridge->edid_emulation);
	if (ret) {
		DRM_ERROR("Failed to transfer EDID to sram, ret=%d\n", ret);
		return ret;
	}

	/* Enable EDID emulation and select the desired EDID */
	val = 1 << PTN3460_EDID_ENABLE_EMULATION |
		ptn_bridge->edid_emulation << PTN3460_EDID_EMULATION_SELECTION;

	ret = ptn3460_write_byte(ptn_bridge, PTN3460_EDID_EMULATION_ADDR, val);
	if (ret) {
		DRM_ERROR("Failed to write EDID value, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static void ptn3460_pre_enable(struct drm_bridge *bridge)
{
	struct ptn3460_bridge *ptn_bridge = bridge_to_ptn3460(bridge);
	int ret;

	if (ptn_bridge->enabled)
		return;

	if (gpio_is_valid(ptn_bridge->gpio_pd_n))
		gpio_set_value(ptn_bridge->gpio_pd_n, 1);

	if (gpio_is_valid(ptn_bridge->gpio_rst_n)) {
		gpio_set_value(ptn_bridge->gpio_rst_n, 0);
		usleep_range(10, 20);
		gpio_set_value(ptn_bridge->gpio_rst_n, 1);
	}

	/*
	 * There's a bug in the PTN chip where it falsely asserts hotplug before
	 * it is fully functional. We're forced to wait for the maximum start up
	 * time specified in the chip's datasheet to make sure we're really up.
	 */
	msleep(90);

	ret = ptn3460_select_edid(ptn_bridge);
	if (ret)
		DRM_ERROR("Select EDID failed ret=%d\n", ret);

	ptn_bridge->enabled = true;
}

static void ptn3460_enable(struct drm_bridge *bridge)
{
}

static void ptn3460_disable(struct drm_bridge *bridge)
{
	struct ptn3460_bridge *ptn_bridge = bridge_to_ptn3460(bridge);

	if (!ptn_bridge->enabled)
		return;

	ptn_bridge->enabled = false;

	if (gpio_is_valid(ptn_bridge->gpio_rst_n))
		gpio_set_value(ptn_bridge->gpio_rst_n, 1);

	if (gpio_is_valid(ptn_bridge->gpio_pd_n))
		gpio_set_value(ptn_bridge->gpio_pd_n, 0);
}

static void ptn3460_post_disable(struct drm_bridge *bridge)
{
}

static struct drm_bridge_funcs ptn3460_bridge_funcs = {
	.pre_enable = ptn3460_pre_enable,
	.enable = ptn3460_enable,
	.disable = ptn3460_disable,
	.post_disable = ptn3460_post_disable,
};

static int ptn3460_get_modes(struct drm_connector *connector)
{
	struct ptn3460_bridge *ptn_bridge;
	u8 *edid;
	int ret, num_modes = 0;
	bool power_off;

	ptn_bridge = connector_to_ptn3460(connector);

	if (ptn_bridge->edid)
		return drm_add_edid_modes(connector, ptn_bridge->edid);

	power_off = !ptn_bridge->enabled;
	ptn3460_pre_enable(&ptn_bridge->bridge);

	edid = kmalloc(EDID_LENGTH, GFP_KERNEL);
	if (!edid) {
		DRM_ERROR("Failed to allocate EDID\n");
		return 0;
	}

	ret = ptn3460_read_bytes(ptn_bridge, PTN3460_EDID_ADDR, edid,
			EDID_LENGTH);
	if (ret) {
		kfree(edid);
		goto out;
	}

	ptn_bridge->edid = (struct edid *)edid;
	drm_mode_connector_update_edid_property(connector, ptn_bridge->edid);

	num_modes = drm_add_edid_modes(connector, ptn_bridge->edid);

out:
	if (power_off)
		ptn3460_disable(&ptn_bridge->bridge);

	return num_modes;
}

static struct drm_encoder *ptn3460_best_encoder(struct drm_connector *connector)
{
	struct ptn3460_bridge *ptn_bridge = connector_to_ptn3460(connector);

	return ptn_bridge->encoder;
}

static struct drm_connector_helper_funcs ptn3460_connector_helper_funcs = {
	.get_modes = ptn3460_get_modes,
	.best_encoder = ptn3460_best_encoder,
};

static enum drm_connector_status ptn3460_detect(struct drm_connector *connector,
		bool force)
{
	return connector_status_connected;
}

static void ptn3460_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
}

static struct drm_connector_funcs ptn3460_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = ptn3460_detect,
	.destroy = ptn3460_connector_destroy,
};

int ptn3460_init(struct drm_device *dev, struct drm_encoder *encoder,
		struct i2c_client *client, struct device_node *node)
{
	int ret;
	struct ptn3460_bridge *ptn_bridge;

	ptn_bridge = devm_kzalloc(dev->dev, sizeof(*ptn_bridge), GFP_KERNEL);
	if (!ptn_bridge) {
		return -ENOMEM;
	}

	ptn_bridge->client = client;
	ptn_bridge->encoder = encoder;
	ptn_bridge->gpio_pd_n = of_get_named_gpio(node, "powerdown-gpio", 0);
	if (gpio_is_valid(ptn_bridge->gpio_pd_n)) {
		ret = gpio_request_one(ptn_bridge->gpio_pd_n,
				GPIOF_OUT_INIT_HIGH, "PTN3460_PD_N");
		if (ret) {
			dev_err(&client->dev,
				"Request powerdown-gpio failed (%d)\n", ret);
			return ret;
		}
	}

	ptn_bridge->gpio_rst_n = of_get_named_gpio(node, "reset-gpio", 0);
	if (gpio_is_valid(ptn_bridge->gpio_rst_n)) {
		/*
		 * Request the reset pin low to avoid the bridge being
		 * initialized prematurely
		 */
		ret = gpio_request_one(ptn_bridge->gpio_rst_n,
				GPIOF_OUT_INIT_LOW, "PTN3460_RST_N");
		if (ret) {
			dev_err(&client->dev,
				"Request reset-gpio failed (%d)\n", ret);
			gpio_free(ptn_bridge->gpio_pd_n);
			return ret;
		}
	}

	ret = of_property_read_u32(node, "edid-emulation",
			&ptn_bridge->edid_emulation);
	if (ret) {
		dev_err(&client->dev, "Can't read EDID emulation value\n");
		goto err;
	}

	ptn_bridge->bridge.funcs = &ptn3460_bridge_funcs;
	ret = drm_bridge_attach(dev, &ptn_bridge->bridge);
	if (ret) {
		DRM_ERROR("Failed to initialize bridge with drm\n");
		goto err;
	}

	encoder->bridge = &ptn_bridge->bridge;

	ret = drm_connector_init(dev, &ptn_bridge->connector,
			&ptn3460_connector_funcs, DRM_MODE_CONNECTOR_LVDS);
	if (ret) {
		DRM_ERROR("Failed to initialize connector with drm\n");
		goto err;
	}
	drm_connector_helper_add(&ptn_bridge->connector,
			&ptn3460_connector_helper_funcs);
	drm_connector_register(&ptn_bridge->connector);
	drm_mode_connector_attach_encoder(&ptn_bridge->connector, encoder);

	return 0;

err:
	if (gpio_is_valid(ptn_bridge->gpio_pd_n))
		gpio_free(ptn_bridge->gpio_pd_n);
	if (gpio_is_valid(ptn_bridge->gpio_rst_n))
		gpio_free(ptn_bridge->gpio_rst_n);
	return ret;
}
EXPORT_SYMBOL(ptn3460_init);

void ptn3460_destroy(struct drm_bridge *bridge)
{
	struct ptn3460_bridge *ptn_bridge = bridge->driver_private;

	if (gpio_is_valid(ptn_bridge->gpio_pd_n))
		gpio_free(ptn_bridge->gpio_pd_n);
	if (gpio_is_valid(ptn_bridge->gpio_rst_n))
		gpio_free(ptn_bridge->gpio_rst_n);
	/* Nothing else to free, we've got devm allocated memory */
}
EXPORT_SYMBOL(ptn3460_destroy);
