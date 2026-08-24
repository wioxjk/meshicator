# Getting Started

A short, non-technical guide to going from "box with a screen" to "message
board that shows what the family's saying." Two parts: flash it once,
then tell it which channel(s) to watch. After that, you never touch it
again.

## What you need

- The SenseCAP Indicator D1 (LoRa, 868MHz)
- A USB-C cable
- A computer running **Chrome or Edge** (the flashing step uses
  WebSerial, which only those two browsers support)
- 5 minutes

## Step 1: Flash the firmware

1. Plug the Indicator into your computer via USB-C.
2. Go to this repo's [Releases page](https://github.com/wioxjk/meshicator/releases)
   and download the file ending in **`-merged.bin`** from the latest
   `channel-board-*` release.
3. Open **[flasher.meshcore.io](https://flasher.meshcore.io)** in Chrome
   or Edge.
4. Choose **"Custom Firmware"**, select the `-merged.bin` file you just
   downloaded, and follow the prompts. It'll ask to connect to the
   device (pick the Indicator from the browser's port list) and then
   erase + flash — this takes under a minute.
5. When it's done, the screen should show "Waiting for the first
   message..." along with "Listening on: #Public". That means it's
   alive and already listening on MeshCore's public channel.

If you'd rather build it from source instead of using the pre-built
release, see `variants/sensecap_indicator_d1/README.md`.

## Step 2: Tell it which channel(s) to show

This is the only setup step, and you only do it once. There's
deliberately no menu for this on the touchscreen — it's done over a USB
serial connection, the same cable you used to flash it.

1. Keep the Indicator plugged into your computer.
2. Open a serial terminal at **115200 baud**. A few easy options:
   - In the [flasher.meshcore.io](https://flasher.meshcore.io) page you
     already have open, there's usually a **Console** feature — click it
     and select the Indicator's port.
   - Or, on the command line: `screen /dev/ttyACM0 115200` (Linux/Mac,
     port name varies) or any terminal app that speaks serial
     (PuTTY/Tera Term on Windows).
3. Type `help` and press Enter. You should see:
   ```
   channel add <name> <base64 psk>   -- watch another #channel
   channel del <index>                -- stop watching a channel (see 'channel list')
   channel list                       -- show configured channels
   relay on|off                       -- also help relay mesh traffic for others (default: off)
   id                                  -- show this board's public key
   help                                -- show this list
   ```

### Adding a private family channel

If your family already has a private group channel set up in one of the
MeshCore apps (the web app, Android, or iOS app), you'll need its name
and its **PSK** (a short string of letters/numbers/symbols ending in
`==`):

1. In the MeshCore app, open the channel, and find the option to
   **export / share** it — this shows the PSK.
2. On the Indicator's serial console, type:
   ```
   channel add Family izOH6cXN6mrJ5e26oRXNcg==
   ```
   (replace `Family` with whatever you want it labeled on screen, and
   the long string with your actual channel's PSK)
3. Type `channel list` to confirm it's there.
4. That's it — messages sent to that channel from anyone's phone will
   now show up on the board within moments of arriving.

You can add up to 8 channels this way; messages from all of them show
up interleaved, newest first, each one tagged with which channel it
came from.

### Removing the public channel (optional)

The board starts out listening to MeshCore's shared "Public" channel,
which anyone on the mesh can post to. If you only want your private
family channel to show up:

```
channel list
```
(note the index number next to "Public", usually `0`), then:
```
channel del 0
```

## Where to put it

It's meant to be left plugged in and forgotten about — a notice board on
a kitchen counter or hallway table. It doesn't need Wi-Fi, doesn't need
a phone nearby, and doesn't need anyone to interact with it. A gentle tap
on the screen skips to the next recent message if there's more than one;
otherwise it just cycles through them on its own every 15 seconds.

## Something not right?

- **Screen stays on "Waiting for the first message..." forever** —
  double check the channel/PSK you added is exactly what the sending app
  shows (typos in the PSK are the most common issue — `channel list`
  won't catch a wrong PSK, since it can't tell a wrong key from a right
  one, it'll just never decrypt anything).
- **Nothing shows up in the serial console when you type `help`** — make
  sure the terminal is set to 115200 baud, and try pressing Enter once
  first to clear any partial line.
- **Anything more fundamental** (blank screen, radio not responding) —
  see the "Self-test before you trust it" section in
  `variants/sensecap_indicator_d1/README.md`.
