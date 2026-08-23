# Channel Board

A calm, always-on "message board" firmware for the SenseCAP Indicator D1:
it silently listens to a small set of MeshCore `#channel`s and shows the
messages on screen, one at a time, big and easy to read from across a
room. That's it -- no contacts, no menus, no configuration screens. The
idea is a device you could leave on a kitchen counter for a relative who
has no interest in learning a chat app: messages just appear.

<p>&nbsp;</p>

## What it does

- Listens for text messages on whichever `#channel`s you configure (see
  below). Group-channel messages don't require joining/discovery/routing
  tables -- anything encrypted with a channel's shared key that reaches
  the radio gets shown, MeshCore-flood-network-wide.
- Shows the newest message full-screen: sender name, the message text in
  large centered type, and a soft "3 min ago"-style timestamp.
- Auto-advances through the last few messages (12 by default) every 15
  seconds if there's more than one; a single tap anywhere on the screen
  jumps to the next one immediately.
- Small dots along the bottom show which message (of how many) you're
  looking at.
- Before the first message ever arrives, it shows a plain "waiting"
  screen naming which channels it's listening on, so you know it's alive.
- Does **not** relay/repeat other people's mesh traffic by default (see
  `relay on`/`relay off` below) -- it's meant to be a quiet appliance, not
  infrastructure. It also never transmits on its own initiative: it only
  ever receives.

## First boot

On first boot (or if its config file is missing/corrupt) it pre-seeds the
standard, publicly-documented MeshCore "Public" channel -- the same
16-byte key (`izOH6cXN6mrJ5e26oRXNcg==` base64, documented in
[`docs/companion_protocol.md`](../../docs/companion_protocol.md)) that
every MeshCore companion app joins by default -- so you'll see *something*
without any setup. Anyone on the mesh can read the Public channel, so
don't rely on it for anything private.

## Configuring which channels it watches

There's no touchscreen UI for this on purpose -- it's meant to be set up
once and then left alone. Connect over USB serial (115200 baud) and use:

```
channel add <name> <base64 psk>     add/watch another channel
channel del <index>                 stop watching one (see 'channel list')
channel list                        show what's currently configured
relay on|off                        also help relay mesh traffic for others (default: off)
id                                  show this board's public key
help                                show this list
```

The PSK is whatever your MeshCore app shows when you export/share a
private group channel (a base64-encoded 16- or 32-byte AES key). Up to 8
channels can be configured at once (`MAX_GROUP_CHANNELS` in
`platformio.ini`); messages from all of them are shown interleaved,
newest first, tagged with which channel they came from.

Settings persist across reboots/power loss (written to the ESP32's
SPIFFS partition), so this is genuinely a one-time setup step.

## What it deliberately doesn't do

- No way to *send* a message from the board itself -- it's a read-only
  display. Use any existing MeshCore app to post to the channel(s) it's
  watching.
- No contacts, DMs, or repeater/room-server admin features -- if you want
  those on this hardware, see the note at the bottom of
  `variants/sensecap_indicator_d1/README.md` about wiring up MeshCore's
  other example apps (`companion_radio`, etc.) instead.
- No absolute clock/date -- there's no GPS or NTP source here, so
  timestamps are relative ("3 min ago", based on when *this board*
  received the message), not the sender's wall-clock time.
