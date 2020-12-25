// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Texas Instruments Incorporated - https://www.ti.com/
 * Author: Tomi Valkeinen <tomi.valkeinen@ti.com>
 */

#include <linux/export.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_of.h>

#include "tidss_crtc.h"
#include "tidss_drv.h"
#include "tidss_encoder.h"

static int tidss_encoder_atomic_check(struct drm_encoder *encoder,
				      struct drm_crtc_state *crtc_state,
				      struct drm_connector_state *conn_state)
{
	struct drm_device *ddev = encoder->dev;
	struct tidss_crtc_state *tcrtc_state = to_tidss_crtc_state(crtc_state);
	struct drm_bridge_state *bstate;
	struct drm_bridge *bridge;

	dev_dbg(ddev->dev, "%s\n", __func__);

	/* Copy the bus_format and flags from the first bridge's state */
	bridge = drm_bridge_chain_get_first_bridge(encoder);
	bstate = drm_atomic_get_new_bridge_state(crtc_state->state, bridge);
	if (!bstate) {
		dev_err(ddev->dev, "Could not get the bridge state\n");
		return -EINVAL;
	}

	tcrtc_state->bus_format = bstate->input_bus_cfg.format;
	tcrtc_state->bus_flags = bstate->input_bus_cfg.flags;

	if (tcrtc_state->bus_format == 0 ||
	    tcrtc_state->bus_format == MEDIA_BUS_FMT_FIXED) {

		dev_err(ddev->dev, "Bridge connected to the encoder did not specify media bus format\n");
		return -EINVAL;
	}

	return 0;
}

static void tidss_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
	kfree(encoder);
}

static const struct drm_encoder_helper_funcs encoder_helper_funcs = {
	.atomic_check = tidss_encoder_atomic_check,
};

static const struct drm_encoder_funcs encoder_funcs = {
	.destroy = tidss_encoder_destroy,
};

struct drm_encoder *tidss_encoder_create(struct tidss_device *tidss,
					 u32 encoder_type, u32 possible_crtcs)
{
	struct drm_encoder *enc;
	int ret;

	enc = kzalloc(sizeof(*enc), GFP_KERNEL);
	if (!enc)
		return ERR_PTR(-ENOMEM);

	enc->possible_crtcs = possible_crtcs;

	ret = drm_encoder_init(&tidss->ddev, enc, &encoder_funcs,
			       encoder_type, NULL);
	if (ret < 0) {
		kfree(enc);
		return ERR_PTR(ret);
	}

	drm_encoder_helper_add(enc, &encoder_helper_funcs);

	dev_dbg(tidss->dev, "Encoder create done\n");

	return enc;
}
