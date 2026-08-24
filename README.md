# Meshicator

MeshCore firmware for the **Seeed Studio SenseCAP Indicator D1** (LoRa,
868MHz) -- a 480x480 touchscreen ESP32-S3 board -- built around one
feature: a calm, elegant on-screen display of messages from a small set
of MeshCore `#channel`s, simple enough for someone with zero interest in
chat apps to read at a glance.

<p>&nbsp;</p>

## What's here

- **`variants/sensecap_indicator_d1/`** -- board support for the
  Indicator D1: display/touch bring-up (reused from MeshCore's own
  `sensecap_indicator-espnow` variant) plus a new SX1262 LoRa radio driver
  for this board's I2C-IO-expander-based control wiring. See that
  directory's `README.md` for the hardware details, sourcing/attribution
  for the LoRa pinout, and how to sanity-check it before trusting it.
- **`examples/channel_board/`** -- the actual "message board" firmware:
  listens to configured channels and shows recent messages, one at a
  time, in large centered type with a soft color palette. See that
  directory's `README.md` for what it does and how to configure which
  channels it watches.

Everything else in this repository is [MeshCore](https://github.com/meshcore-dev/meshcore)
itself, vendored in unmodified (see "Built on MeshCore" below) so the new
variant/example can be built the normal MeshCore way.

## Flashing

Grab a pre-built `-merged.bin` from this repo's [Releases](../../releases)
and flash it from the browser via the official
[MeshCore flasher](https://flasher.meshcore.io)'s "Custom Firmware"
option (WebSerial, no install needed) -- or build it yourself:

```bash
# install PlatformIO first: https://docs.platformio.org
pio run -e SenseCapIndicator_D1_channel_board -t upload
```

Then connect over USB serial (115200 baud) to configure which channels it
watches -- see `examples/channel_board/README.md` for both.

## Built on MeshCore

This repository is a fork of [MeshCore](https://github.com/meshcore-dev/meshcore)
(MIT licensed), a lightweight LoRa mesh routing firmware. MeshCore's own
documentation, other supported devices, the companion apps, and the wire
protocol are all unchanged here -- see `docs/` in this repo, or
[MeshCore's docs site](https://docs.meshcore.io) and
[GitHub](https://github.com/meshcore-dev/meshcore) for all of that.

If you're looking for the *general-purpose* MeshCore firmware for some
other device, or the companion/repeater/room-server apps, you almost
certainly want upstream MeshCore rather than this repo -- this fork only
adds the one board and the one purpose-built display app described above.

## License

MIT, same as upstream MeshCore -- see `license.txt`.
