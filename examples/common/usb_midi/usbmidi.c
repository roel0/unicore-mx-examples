/*
 * This file is part of the unicore-mx project.
 *
 * Copyright (C) 2014 Daniel Thompson <daniel@redfelineninja.org.uk>
 * Copyright (C) 2015 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
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
#include <string.h>
#include <unicore-mx/usbd/usbd.h>
#include <unicore-mx/usb/class/audio.h>
#include <unicore-mx/usb/class/midi.h>
#include <unicore-mx/cm3/scb.h>
#include "usbmidi-target.h"

/*
 * All references in this file come from Universal Serial Bus Device Class
 * Definition for MIDI Devices, release 1.0.
 */

/*
 * Midi specific endpoint descriptors.
 */
static const struct usb_midi_endpoint_descriptor midi_bulk_endp[] = {{
	/* Table B-12: MIDI Adapter Class-specific Bulk OUT Endpoint
	 * Descriptor
	 */
	.head = {
		.bLength = sizeof(struct usb_midi_endpoint_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
		.bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
		.bNumEmbMIDIJack = 1,
	},
	.jack[0] = {
		.baAssocJackID = 0x01,
	},
}, {
	/* Table B-14: MIDI Adapter Class-specific Bulk IN Endpoint
	 * Descriptor
	 */
	.head = {
		.bLength = sizeof(struct usb_midi_endpoint_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
		.bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
		.bNumEmbMIDIJack = 1,
	},
	.jack[0] = {
		.baAssocJackID = 0x03,
	},
} };

/*
 * Standard endpoint descriptors
 */
static const struct usb_endpoint_descriptor bulk_endp[] = {{
	/* Table B-11: MIDI Adapter Standard Bulk OUT Endpoint Descriptor */
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 0x40,
	.bInterval = 0x00,

	.extra = &midi_bulk_endp[0],
	.extra_len = sizeof(midi_bulk_endp[0])
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 0x40,
	.bInterval = 0x00,

	.extra = &midi_bulk_endp[1],
	.extra_len = sizeof(midi_bulk_endp[1])
} };

/*
 * Table B-4: MIDI Adapter Class-specific AC Interface Descriptor
 */
static const struct {
	struct usb_audio_header_descriptor_head header_head;
	struct usb_audio_header_descriptor_body header_body;
} __attribute__((packed)) audio_control_functional_descriptors = {
	.header_head = {
		.bLength = sizeof(struct usb_audio_header_descriptor_head) +
		           1 * sizeof(struct usb_audio_header_descriptor_body),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
		.bcdADC = 0x0100,
		.wTotalLength =
			   sizeof(struct usb_audio_header_descriptor_head) +
			   1 * sizeof(struct usb_audio_header_descriptor_body),
		.binCollection = 1,
	},
	.header_body = {
		.baInterfaceNr = 0x01,
	},
};

/*
 * Table B-3: MIDI Adapter Standard AC Interface Descriptor
 */
static const struct usb_interface_descriptor audio_control_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.extra = &audio_control_functional_descriptors,
	.extra_len = sizeof(audio_control_functional_descriptors)
} };

/*
 * Class-specific MIDI streaming interface descriptor
 */
static const struct {
	struct usb_midi_header_descriptor header;
	struct usb_midi_in_jack_descriptor in_embedded;
	struct usb_midi_in_jack_descriptor in_external;
	struct usb_midi_out_jack_descriptor out_embedded;
	struct usb_midi_out_jack_descriptor out_external;
} __attribute__((packed)) midi_streaming_functional_descriptors = {
	/* Table B-6: Midi Adapter Class-specific MS Interface Descriptor */
	.header = {
		.bLength = sizeof(struct usb_midi_header_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_MIDI_SUBTYPE_MS_HEADER,
		.bcdMSC = 0x0100,
		.wTotalLength = sizeof(midi_streaming_functional_descriptors),
	},
	/* Table B-7: MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
	.in_embedded = {
		.bLength = sizeof(struct usb_midi_in_jack_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
		.bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
		.bJackID = 0x01,
		.iJack = 0x00,
	},
	/* Table B-8: MIDI Adapter MIDI IN Jack Descriptor (External) */
	.in_external = {
		.bLength = sizeof(struct usb_midi_in_jack_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
		.bJackType = USB_MIDI_JACK_TYPE_EXTERNAL,
		.bJackID = 0x02,
		.iJack = 0x00,
	},
	/* Table B-9: MIDI Adapter MIDI OUT Jack Descriptor (Embedded) */
	.out_embedded = {
		.head = {
			.bLength = sizeof(struct usb_midi_out_jack_descriptor),
			.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
			.bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_OUT_JACK,
			.bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
			.bJackID = 0x03,
			.bNrInputPins = 1,
		},
		.source[0] = {
			.baSourceID = 0x02,
			.baSourcePin = 0x01,
		},
		.tail = {
			.iJack = 0x00,
		}
	},
	/* Table B-10: MIDI Adapter MIDI OUT Jack Descriptor (External) */
	.out_external = {
		.head = {
			.bLength = sizeof(struct usb_midi_out_jack_descriptor),
			.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
			.bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_OUT_JACK,
			.bJackType = USB_MIDI_JACK_TYPE_EXTERNAL,
			.bJackID = 0x04,
			.bNrInputPins = 1,
		},
		.source[0] = {
			.baSourceID = 0x01,
			.baSourcePin = 0x01,
		},
		.tail = {
			.iJack = 0x00,
		},
	},
};

/*
 * Table B-5: MIDI Adapter Standard MS Interface Descriptor
 */
static const struct usb_interface_descriptor midi_streaming_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_MIDISTREAMING,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = bulk_endp,

	.extra = &midi_streaming_functional_descriptors,
	.extra_len = sizeof(midi_streaming_functional_descriptors)
} };

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = audio_control_iface,
}, {
	.num_altsetting = 1,
	.altsetting = midi_streaming_iface,
} };

const uint8_t *usb_strings_english[] = {
	(uint8_t *) "unicore-mx",
	(uint8_t *) "MIDI Demo"
};

/*
 * MIDI = मिडी
 * Demo, DEMO = नमूना
 */
const uint8_t *usb_strings_hindi[] = {
	(uint8_t *) "unicore-mx",
	(uint8_t *) "मिडी नमूना"
};

const struct usb_string_utf8_data usb_strings[] = {{
	.data = usb_strings_english,
	.count = 2,
	.lang_id = USB_LANGID_ENGLISH_UNITED_STATES
}, {
	.data = usb_strings_hindi,
	.count = 2,
	.lang_id = USB_LANGID_HINDI
}, {
	.data = NULL
}};

/*
 * Table B-2: MIDI Adapter Configuration Descriptor
 */
static const struct usb_config_descriptor config[] = {{
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0, /* can be anything, it is updated automatically
			      when the usb code prepares the descriptor */
	.bNumInterfaces = 2, /* control and data */
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80, /* bus powered */
	.bMaxPower = 0x32,

	.interface = ifaces,
	.string = usb_strings
}};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

/* SysEx identity message, preformatted with correct USB framing information */
const uint8_t sysex_identity[] = {
	0x04,	/* USB Framing (3 byte SysEx) */
	0xf0,	/* SysEx start */
	0x7e,	/* non-realtime */
	0x00,	/* Channel 0 */
	0x04,	/* USB Framing (3 byte SysEx) */
	0x7d,	/* Educational/prototype manufacturer ID */
	0x66,	/* Family code (byte 1) */
	0x66,	/* Family code (byte 2) */
	0x04,	/* USB Framing (3 byte SysEx) */
	0x51,	/* Model number (byte 1) */
	0x19,	/* Model number (byte 2) */
	0x00,	/* Version number (byte 1) */
	0x04,	/* USB Framing (3 byte SysEx) */
	0x00,	/* Version number (byte 2) */
	0x01,	/* Version number (byte 3) */
	0x00,	/* Version number (byte 4) */
	0x05,	/* USB Framing (1 byte SysEx) */
	0xf7,	/* SysEx end */
	0x00,	/* Padding */
	0x00,	/* Padding */
};

/*
 * Table B-1: MIDI Adapter Device Descriptor
 */
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,    /* was 0x0110 in Table B-1 example descriptor */
	.bDeviceClass = 0,   /* device defined at interface level */
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x6666,  /* Prototype product vendor ID */
	.idProduct = 0x5119, /* dd if=/dev/random bs=2 count=1 | hexdump */
	.bcdDevice = 0x0100,
	.iManufacturer = 1,  /* index to string desc */
	.iProduct = 2,       /* index to string desc */
	.iSerialNumber = 0,
	.bNumConfigurations = 1,

	.config = config,
	.string = usb_strings
};

static bool error_recoverable(usbd_transfer_status status)
{
	switch (status) {
	case USBD_ERR_TIMEOUT:
	case USBD_ERR_IO:
	case USBD_ERR_BABBLE:
	case USBD_ERR_DTOG:
	case USBD_ERR_SHORT_PACKET:
	case USBD_ERR_OVERFLOW:
	return true;

	case USBD_ERR_RES_UNAVAIL:
	case USBD_SUCCESS:
	case USBD_ERR_SIZE:
	case USBD_ERR_CONN:
	case USBD_ERR_INVALID:
	case USBD_ERR_CONFIG_CHANGE:
	case USBD_ERR_CANCEL:
	default:
	return false;
	}
}

static void resubmit_for_recoverable_error(usbd_device *usbd_dev,
		const usbd_transfer *transfer, usbd_transfer_status status,
		usbd_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBD_SUCCESS) {
		if (error_recoverable(status)) {
			usbd_transfer_submit(usbd_dev, transfer);
		}
	}
}

static void send_sysex_identify(usbd_device *usbd_dev)
{
	const usbd_transfer transfer = {
		.ep_type = USBD_EP_BULK,
		.ep_addr = 0x81,
		.ep_size = 64,
		.ep_interval = USBD_INTERVAL_NA,
		.buffer = (void *) sysex_identity,
		.length = sizeof(sysex_identity),
		.flags = USBD_FLAG_NO_SUCCESS_CALLBACK,
		.timeout = USBD_TIMEOUT_NEVER,
		.callback = resubmit_for_recoverable_error
	};

	usbd_transfer_submit(usbd_dev, &transfer);
}

static void usbmidi_data_rx_cb(usbd_device *usbd_dev,
		const usbd_transfer *transfer, usbd_transfer_status status,
		usbd_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBD_SUCCESS) {
		return;
	}

	/* This implementation treats any message from the host as a SysEx
	 * identity request. This works well enough providing the host
	 * packs the identify request in a single 8 byte USB message.
	 */
	if (transfer->transferred) {
		send_sysex_identify(usbd_dev);
	}

	/* Accept more data from host */
	usbd_transfer_submit(usbd_dev, transfer);

	usbmidi_target_data_rx_cb();
}

static void usbmidi_set_config(usbd_device *usbd_dev,
				const struct usb_config_descriptor *cfg)
{
	(void)cfg;

	usbd_ep_prepare(usbd_dev, 0x01, USBD_EP_BULK, 64, USBD_INTERVAL_NA,
					USBD_EP_NONE);
	usbd_ep_prepare(usbd_dev, 0x81, USBD_EP_BULK, 64, USBD_INTERVAL_NA,
					USBD_EP_NONE);

	static uint8_t buf[64];

	const usbd_transfer transfer = {
		.ep_type = USBD_EP_BULK,
		.ep_addr = 0x01,
		.ep_size = 64,
		.ep_interval = USBD_INTERVAL_NA,
		.buffer = buf,
		.length = sizeof(buf),
		.flags = USBD_FLAG_NONE,
		.timeout = USBD_TIMEOUT_NEVER,
		.callback = usbmidi_data_rx_cb,
	};

	usbd_transfer_submit(usbd_dev, &transfer);
}

static volatile usbd_urb_id button_event_urb_id = USBD_INVALID_URB_ID;

static void button_send_event_callback(usbd_device *usbd_dev,
		const usbd_transfer *transfer, usbd_transfer_status status,
		usbd_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBD_SUCCESS) {
		if (error_recoverable(status)) {
			button_event_urb_id = usbd_transfer_submit(usbd_dev, transfer);
			return;
		}
	}

	button_event_urb_id = USBD_INVALID_URB_ID;
}

static void button_send_event(usbd_device *usbd_dev, int pressed)
{
	static char buf[4] = { 0x08, /* USB framing: virtual cable 0, note on */
			0x80, /* MIDI command: note on, channel 1 */
			60,   /* Note 60 (middle C) */
			64,   /* "Normal" velocity */
	};

	buf[0] |= pressed;
	buf[1] |= pressed << 4;

	if (button_event_urb_id != USBD_INVALID_URB_ID) {
		/* already in progress */
		return;
	}

	const usbd_transfer transfer = {
		.ep_type = USBD_EP_BULK,
		.ep_addr = 0x81,
		.ep_size = 64,
		.ep_interval = USBD_INTERVAL_NA,
		.buffer = buf,
		.length = sizeof(buf),
		.flags = USBD_FLAG_NONE,
		.timeout = USBD_TIMEOUT_NEVER,
		.callback = button_send_event_callback
	};

	button_event_urb_id = usbd_transfer_submit(usbd_dev, &transfer);
}

static void button_poll(usbd_device *usbd_dev)
{
	static uint32_t button_state = 0;

	/* This is a simple shift based debounce. It's simplistic because
	 * although this implements debounce adequately it does not have any
	 * noise suppression. It is also very wide (32-bits) because it can
	 * be polled in a very tight loop (no debounce timer).
	 */
	uint32_t old_button_state = button_state;
	button_state = (button_state << 1) | usbmidi_target_button_state();
	if ((0 == button_state) != (0 == old_button_state)) {
		button_send_event(usbd_dev, !!button_state);
	}
}

int main(void)
{
	usbd_device *usbd_dev;

	usbmidi_target_init();

	usbd_dev = usbd_init(usbmidi_target_usb_driver(), NULL, &dev,
			usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, usbmidi_set_config);

	while (1) {
		usbd_poll(usbd_dev, 0);
		button_poll(usbd_dev);
	}
}
