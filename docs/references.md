# Reading behind the first drivers

These are technical references, not dependencies of the runtime.

- [USB HID class specification 1.11](https://www.usb.org/sites/default/files/hid1_11.pdf):
  Appendix B's boot report format and Appendix F's keyboard requirements. A
  device's ordinary report descriptor is not necessarily its boot-protocol format.
- [Linux SPI userspace API](https://docs.kernel.org/spi/spidev.html): composite
  `SPI_IOC_MESSAGE` transactions preserve chip select over command and response.
- [Linux hidraw interface](https://docs.kernel.org/hid/hidraw.html): raw report
  delivery is distinct from descriptor interpretation and keyboard layout.
- [Nelua overview](https://nelua.io/overview/): C generation, fixed-width types,
  compile-time programming and C exports.
- [Pinned compiler source](https://github.com/Spaceghost/nelua-lang/tree/3c29419776dfdca194fa7e9c572498264fe5c654):
  exact fork revision used for the initial generated artifacts.

For hardware qualification, use the exact flash part's manufacturer datasheet
and record its JEDEC read-ID command, timing, supported modes and returned IDs.
The supplied `ef4018` fixture is an example response, not part identification
or wiring validation performed on physical hardware.
