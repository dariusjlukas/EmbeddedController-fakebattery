/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI TUSB544 USB Type-C Multi-Protocol Linear Redriver
 */
#include "i2c.h"
#include "tusb544.h"
#include "usb_mux.h"

static int tusb544_write(int port, int offset, int data)
{
	return i2c_write8(usb_retimers[port].i2c_port,
			  usb_retimers[port].i2c_addr_flags,
			  offset, data);
}

static int tusb544_read(int port, int offset, int *data)
{
	return i2c_read8(usb_retimers[port].i2c_port,
			  usb_retimers[port].i2c_addr_flags,
			  offset, data);
}

static int tusb544_enter_low_power_mode(int port)
{
	int reg;
	int rv;

	rv = tusb544_read(port, TUSB544_REG_GENERAL4, &reg);
	if (rv)
		return rv;

	/* Setting CTL_SEL[0,1] to 0 powers down, per Table 5 */
	reg &= ~TUSB544_GEN4_CTL_SEL;

	return tusb544_write(port, TUSB544_REG_GENERAL4, reg);
}

static int tusb544_init(int port)
{
	return EC_SUCCESS;
}

static int tusb544_set_mux(int port, mux_state_t mux_state)
{
	int reg;
	int rv;

	if (mux_state == USB_PD_MUX_NONE)
		return tusb544_enter_low_power_mode(port);

	rv = tusb544_read(port, TUSB544_REG_GENERAL4, &reg);
	if (rv)
		return rv;

	if (mux_state & USB_PD_MUX_POLARITY_INVERTED)
		reg |= TUSB544_GEN4_FLIP_SEL;
	else
		reg &= ~TUSB544_GEN4_FLIP_SEL;

	reg &= ~TUSB544_GEN4_CTL_SEL;

	if ((mux_state & USB_PD_MUX_USB_ENABLED) &&
	    (mux_state & USB_PD_MUX_DP_ENABLED))
		reg |= TUSB544_CTL_SEL_DP_USB;
	else if (mux_state & USB_PD_MUX_DP_ENABLED)
		reg |= TUSB544_CTL_SEL_DP_ONLY;
	else if (mux_state & USB_PD_MUX_USB_ENABLED)
		reg |= TUSB544_CTL_SEL_USB_ONLY;

	rv = tusb544_write(port, TUSB544_REG_GENERAL4, reg);
	if (rv)
		return rv;

	rv = tusb544_read(port, TUSB544_REG_GENERAL6, &reg);
	if (rv)
		return rv;

	reg &= ~TUSB544_GEN6_DIR_SEL;
	if (pd_get_power_role(port) == PD_ROLE_SOURCE)
		reg |= TUSB544_DIR_SEL_USB_DP_SRC;
	else
		reg |= TUSB544_DIR_SEL_USB_DP_SNK;

	return tusb544_write(port, TUSB544_REG_GENERAL6, reg);
}

const struct usb_retimer_driver tusb544_usb_redriver_drv = {
	.enter_low_power_mode = &tusb544_enter_low_power_mode,
	.init = &tusb544_init,
	.set = &tusb544_set_mux,
};