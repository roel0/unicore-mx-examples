# README

This example implements a USB CDC-ACM device (aka Virtual Serial Port)
to demonstrate the use of the USB device stack.

## Configuration
at compile time, `#define USB_OTG_HS` to enable USB_OTG_HS.
Only one port will work at a time.

at compile time, `#define SHOW_HIGH_SPEED_DEMO` to enumerate as
HIGH speed device (only for OTG_HS).

## Board connections

| Port  | Function       | Description                               |
| ----- | -------------- | ------------------------------------------ |
| `CN13`| `(USB_OTG_FS)` | USB acting as device, connect to computer |
| `CN12`| `(USB_OTG_HS)` | USB acting as device, connect to computer |
