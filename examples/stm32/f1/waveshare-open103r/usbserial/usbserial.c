/*
 * This file is part of the unicore-mx project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2013 Joshua Harlan Lifton <joshua.harlan.lifton@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/usbd/usbd.h>
#include <unicore-mx/usb/class/cdc.h>

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
  }
};

static const struct usb_endpoint_descriptor data_endp[] = {
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  },
  {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
  }
};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
  .header = {
    .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
    .bcdCDC = 0x0110,
  },
  .call_mgmt = {
    .bFunctionLength =
    sizeof(struct usb_cdc_call_management_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
    .bmCapabilities = 0,
    .bDataInterface = 1,
  },
  .acm = {
    .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_ACM,
    .bmCapabilities = 0,
  },
  .cdc_union = {
    .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
    .bDescriptorType = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_UNION,
    .bControlInterface = 0,
    .bSubordinateInterface0 = 1,
  }
};

static const struct usb_interface_descriptor comm_iface[] = {
  {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,
    .endpoint = comm_endp,
    .extra = &cdcacm_functional_descriptors,
    .extra_len = sizeof(cdcacm_functional_descriptors)
  }
};

static const struct usb_interface_descriptor data_iface[] = {
  {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
    .endpoint = data_endp,
  }
};

static const struct usb_interface ifaces[] = {
  {
    .num_altsetting = 1,
    .altsetting = comm_iface,
  },
  {
    .num_altsetting = 1,
    .altsetting = data_iface,
  }
};

static const uint8_t *usb_strings_ascii[] = {
  (uint8_t *) "Black Sphere Technologies",
  (uint8_t *) "CDC-ACM Demo",
  (uint8_t *) "DEMO",
};

const struct usb_string_utf8_data usb_strings[] = {{
	.data = usb_strings_ascii,
	.count = 3,
	.lang_id = USB_LANGID_ENGLISH_UNITED_STATES
}, {
	.data = NULL
}};

static const struct usb_config_descriptor config = {
  .bLength = USB_DT_CONFIGURATION_SIZE,
  .bDescriptorType = USB_DT_CONFIGURATION,
  .wTotalLength = 0,
  .bNumInterfaces = 2,
  .bConfigurationValue = 1,
  .iConfiguration = 0,
  .bmAttributes = 0x80,
  .bMaxPower = 0x32,
  .interface = ifaces,
  .string = usb_strings
};

static const struct usb_device_descriptor dev = {
  .bLength = USB_DT_DEVICE_SIZE,
  .bDescriptorType = USB_DT_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = USB_CLASS_CDC,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = 64,
  .idVendor = 0x0483,
  .idProduct = 0x5740,
  .bcdDevice = 0x0200,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1,

  .config = &config
  .string = usb_strings
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_control_result
cdcacm_control_request(usbd_device *usbd_dev, struct usbd_control_arg *arg)
{
  (void)usbd_dev;

  const uint8_t bmReqMask = USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT;
  const uint8_t bmReqVal = USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;

  if ((arg->setup->bmRequestType & bmReqMask) != bmReqVal) {
    return USBD_REQ_NEXT;
  }

  switch(arg->setup->bRequest) {
  case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
    /*
     * This Linux cdc_acm driver requires this to be implemented
     * even though it's optional in the CDC spec, and we don't
     * advertise it in the ACM functional descriptor.
     */
    char local_buf[10];
    struct usb_cdc_notification *notif = (void *)local_buf;

    /* We echo signals back to host as notification. */
    notif->bmRequestType = 0xA1;
    notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
    notif->wValue = 0;
    notif->wIndex = 0;
    notif->wLength = 2;
    local_buf[8] = arg->setup->wValue & 3;
    local_buf[9] = 0;
    // usbd_ep_write_packet(0x83, buf, 10);
    return USBD_REQ_HANDLED;
  }
  case USB_CDC_REQ_SET_LINE_CODING:
    if(arg->len < sizeof(struct usb_cdc_line_coding)) {
      return USBD_REQ_STALL;
    }
    return USBD_REQ_HANDLED;
  }
  return USBD_REQ_STALL;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
  (void)ep;
  char buf[64];
  int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

  if (len) {
    usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
    buf[len] = 0;
  }
}

static void cdcacm_set_config(usbd_device *usbd_dev,
		const struct usb_config_descriptor *cfg)
{
  (void)cfg;

  usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
  usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
  usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);
}

int main(void)
{
  usbd_device *usbd_dev;

  rcc_clock_setup_in_hsi_out_48mhz();
  rcc_periph_clock_enable(RCC_AFIO);

  AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

  usbd_dev = usbd_init(USBD_STM32_FSDEV_V1,
		       &dev,
		       usbd_control_buffer,
		       sizeof(usbd_control_buffer));
  usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
  usbd_register_control_callback(usbd_dev, cdcacm_control_request);

  while (1) {
    usbd_poll(usbd_dev);
  }

}
