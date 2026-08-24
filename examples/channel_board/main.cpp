#include <Arduino.h>
#include <Mesh.h>
#include <target.h>
#include <helpers/IdentityStore.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <SPIFFS.h>

#include "ChannelConfig.h"
#include "MyMesh.h"
#include "UITask.h"

#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE   "23 Aug 2026"
#endif
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION   "v0.1.0"
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
ChannelConfig channel_config;
MyMesh the_mesh(radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables, channel_config);
UITask ui_task(display, the_mesh, channel_config);

static void printHelp() {
  Serial.println("Message Board setup commands:");
  Serial.println("  channel add <name> <base64 psk>   -- watch another #channel");
  Serial.println("  channel del <index>                -- stop watching a channel (see 'channel list')");
  Serial.println("  channel list                       -- show configured channels");
  Serial.println("  relay on|off                       -- also help relay mesh traffic (default: off)");
  Serial.println("  id                                 -- show this board's public key");
  Serial.println("  help                                -- show this message");
}

static void listChannels() {
  for (int i = 0; i < channel_config.count(); i++) {
    Serial.print("  ["); Serial.print(i); Serial.print("] #");
    Serial.println(channel_config.get(i).name);
  }
  if (channel_config.count() == 0) Serial.println("  (none configured)");
}

static void handleCommand(char* cmd) {
  char* saveptr;
  char* verb = strtok_r(cmd, " ", &saveptr);
  if (verb == NULL) return;

  if (strcmp(verb, "help") == 0) {
    printHelp();
  } else if (strcmp(verb, "id") == 0) {
    mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE);
    Serial.println();
  } else if (strcmp(verb, "relay") == 0) {
    char* arg = strtok_r(NULL, " ", &saveptr);
    if (arg && strcmp(arg, "on") == 0) { the_mesh.setRelayEnabled(true); Serial.println("relay: on"); }
    else if (arg && strcmp(arg, "off") == 0) { the_mesh.setRelayEnabled(false); Serial.println("relay: off"); }
    else Serial.println("usage: relay on|off");
  } else if (strcmp(verb, "channel") == 0) {
    char* sub = strtok_r(NULL, " ", &saveptr);
    if (sub && strcmp(sub, "list") == 0) {
      listChannels();
    } else if (sub && strcmp(sub, "add") == 0) {
      char* name = strtok_r(NULL, " ", &saveptr);
      char* psk = strtok_r(NULL, " ", &saveptr);
      if (name && psk && channel_config.add(name, psk)) {
        channel_config.save(SPIFFS);
        Serial.println("added.");
      } else {
        Serial.println("failed -- check the PSK is valid base64 (16 or 32 bytes) and the table isn't full");
      }
    } else if (sub && strcmp(sub, "del") == 0) {
      char* idx_s = strtok_r(NULL, " ", &saveptr);
      int idx = idx_s ? atoi(idx_s) : -1;
      if (channel_config.removeAt(idx)) {
        channel_config.save(SPIFFS);
        Serial.println("removed.");
      } else {
        Serial.println("no such channel -- see 'channel list'");
      }
    } else {
      Serial.println("usage: channel add|del|list ...");
    }
  } else {
    Serial.println("unknown command, try 'help'");
  }
}

static char command[128];

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();

  if (display.begin()) {
    display.startFrame();
    display.setCursor(20, 20);
    display.print("Starting up...");
    display.endFrame();
  }

  if (!radio_init()) {
    while (1) { delay(1000); }   // nothing useful we can do without the radio
  }
  fast_rng.begin(radio_driver.getRngSeed());

  // TEMP DEBUG: see the matching comment in variants/sensecap_indicator_d1/target.cpp --
  // same idea, covering the boot stages after radio_init() succeeds.
  display.startFrame(TFT_CYAN);
  display.endFrame();

  SPIFFS.begin(true);
  channel_config.load(SPIFFS);

  display.startFrame(TFT_BLUE);   // TEMP DEBUG: SPIFFS + channel config loaded ok
  display.endFrame();

  IdentityStore id_store(SPIFFS, "/identity");
  if (!id_store.load("_main", the_mesh.self_id)) {
    the_mesh.self_id = radio_new_identity();
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {
      the_mesh.self_id = radio_new_identity(); count++;
    }
    id_store.save("_main", the_mesh.self_id);
  }

  display.startFrame(TFT_MAGENTA);   // TEMP DEBUG: identity ready; setup() about to finish
  display.endFrame();

  Serial.print("Message Board ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();
  printHelp();
  listChannels();

  the_mesh.begin();
  ui_task.begin();

  command[0] = 0;
  board.onBootComplete();
}

void loop() {
  int len = strlen(command);
  while (Serial.available() && len < (int)sizeof(command) - 1) {
    char c = Serial.read();
    if (c != '\n') {   // terminal line endings are typically "\r\n" -- keep the \r, drop the \n
      command[len++] = c;
      command[len] = 0;
    }
    Serial.print(c);
  }
  if (len > 0 && command[len - 1] == '\r') {   // received complete line
    command[len - 1] = 0;
    handleCommand(command);
    command[0] = 0;
  }

  radio_service();   // let the LoRa Hal poll the IO expander for DIO1 (see IOExpanderRadioHal.h)
  the_mesh.loop();
  ui_task.loop();
  rtc_clock.tick();
}
