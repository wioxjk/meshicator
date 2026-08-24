# Seeed Studio SenseCAP Indicator D1 (868MHz)

Board support for the LoRa-equipped Seeed Studio SenseCAP Indicator ("D1",
sometimes sold as D1S/D1L/D1Pro depending on which sensors/radios are
populated) -- an ESP32-S3 with a 480x480 IPS touchscreen and an SX1262
LoRa radio.

## Hardware bring-up: what's proven vs. what's new here

**Display, touch, I2C IO expander, user button** -- copied essentially
unchanged from MeshCore's own `variants/sensecap_indicator-espnow`, which
is already merged upstream and presumably running on real boards. Same
480x480 ST7701S panel, same FT5x06/FT6336U touch, same TCA9535 IO
expander. If you already have that variant working, the screen on this
one should behave identically.

**LoRa radio (SX1262)** -- this is new, and has **not been validated on
real hardware** (this was built without physical access to the board).
The base D1 model doesn't populate a LoRa radio at all -- only the
LoRa-equipped trim (D1S/D1L) does -- and Seeed hasn't published a
schematic for it. The pinout below is cross-checked between two
independent community reverse-engineering write-ups that arrived at the
same answer:

- [fdlamotte/MCSenseCAP_Indicator](https://github.com/fdlamotte/MCSenseCAP_Indicator)
- [ril3y/sensecap-indicator-d1l](https://github.com/ril3y/sensecap-indicator-d1l) --
  see `SENSECAP_INDICATOR_WIRING.txt` and `SENSECAP_INDICATOR_HARDWARE_REFERENCE.md`
  in that repo for the full reverse-engineered pinout this was built from.

If your board doesn't match this pinout, LoRa init will fail loudly (see
"Self-test" below) rather than silently doing the wrong thing.

## Pinout summary

```
ESP32-S3 <-> SX1262, via SPI + a TCA9535 I2C IO expander:

  SPI (native GPIO):       SCLK=41  MISO=47  MOSI=48
  IO expander (I2C 0x20):  SDA=39   SCL=40   INT=42 (shared, active-low)
    expander pin 0 -> SX1262 NSS      expander pin 4 -> LCD CS
    expander pin 1 -> SX1262 RESET    expander pin 5 -> LCD RESET
    expander pin 2 -> SX1262 BUSY     expander pin 7 -> touch RESET
    expander pin 3 -> SX1262 DIO1
```

See `pins.h` for the exact `#define`s.

## Why the radio needs a custom RadioLib Hal

RadioLib (the LoRa driver library MeshCore uses) expects to twiddle the
SX1262's NSS/RESET/BUSY/DIO1 lines as plain GPIOs, including a real edge
interrupt on DIO1 for fast RX/TX-done notification. On this board those
four lines are behind the I2C IO expander instead, and:

- toggling them means an I2C transaction, not a GPIO write -- so
  `IOExpanderRadioHal.h` subclasses RadioLib's `ArduinoHal` and routes
  just those four "pins" (given fake pin numbers >= `0x8000`) through the
  expander, leaving everything else at the normal Arduino GPIO/SPI path.
- there's no way to get a true, DIO1-specific hardware interrupt: the
  expander's single INT pin (GPIO42) is shared with BUSY and a
  TCXO-detect line, and reading *which* line changed needs another I2C
  transaction -- which is not safe to do from inside a GPIO ISR. So the
  real ISR only sets a flag, and `IOExpanderRadioHal::service()` -- called
  every `loop()` iteration via `radio_service()` -- does the actual I2C
  read and calls RadioLib's registered DIO1 callback when it sees the
  line go low-to-high.

Practical effect: RX/TX-done detection is polled at your sketch's loop()
rate rather than interrupt-latency, and BUSY-wait during SPI commands
does one I2C transaction per poll instead of a GPIO read. For LoRa's own
timescales (tens to hundreds of ms per symbol) this should be a total
non-issue, but if RX feels unexpectedly sluggish on real hardware, that's
the first place to look -- see `IOExpanderRadioHal.h` for the full
reasoning.

## Self-test before you trust it

Before relying on this for anything, watch the serial console (115200
baud) right after boot. `CustomSX1262::std_init()` logs the SX1262's
status/error registers:

```
SX1262 status=0x.. device_errors=0x....
```

- If the IO expander itself doesn't answer on I2C, `radio_init()` prints
  an error and the board halts -- check the I2C wiring/address first
  (`0x20`, see `pins.h`).
- If the expander answers but the radio never leaves a bad status /
  reports device errors, that's the SPI or NSS/RESET/BUSY wiring assumed
  above being wrong for your particular board revision.
- If both come up clean, send a test message on the channel you've
  configured (e.g. from the official MeshCore app on the "Public"
  channel) and confirm it shows up on screen.

## Building

Don't want to set up PlatformIO? Grab a pre-built binary from this repo's
[Releases page](../../releases) instead and flash it via WebSerial -- see
"Flashing it" in `examples/channel_board/README.md`.

To build from source:

```
pio run -e SenseCapIndicator_D1_channel_board -t upload
```

`-t mergebin` instead of `-t upload` produces a single flattened
`firmware-merged.bin` (bootloader+partitions+app, flashable at address
0x0) -- the format WebSerial flashers expect for a first-time install;
`.github/workflows/build-channel-board-firmware.yml` builds and publishes
one on every `channel-board-*` tag.

See `examples/channel_board/README.md` for what that firmware actually
does and how to configure it. This variant only defines the board
support (radio/display/IO expander) -- other MeshCore example apps
(`companion_radio`, `simple_repeater`, `simple_room_server`, ...) can be
wired up for it the same way other variants do, by adding an `[env:...]`
section to `platformio.ini` here that extends `SenseCapIndicator_D1` and
adds that example's source filter, but none of those are built by
default in this repo.

## Regulatory note

Defaults (inherited from MeshCore's `arduino_base`) are EU868-ish:
869.618MHz, BW 62.5kHz, SF8, and this variant sets 22dBm TX power. Check
your local LoRa band-plan and power limits before transmitting --
especially if you enable packet relaying (`relay on`, off by default),
which will have this board transmitting on other people's behalf too.
