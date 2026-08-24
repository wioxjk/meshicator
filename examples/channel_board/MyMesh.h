#pragma once

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/TxtDataHelpers.h>

#include "ChannelConfig.h"

#ifndef MAX_BOARD_MESSAGES
  #define MAX_BOARD_MESSAGES 12
#endif

#define MAX_MSG_TEXT_LEN   160

struct BoardMessage {
  char channel_name[32];
  char sender[32];     // best-effort -- taken from the "name: " prefix convention, may be blank
  char text[MAX_MSG_TEXT_LEN + 1];
  uint32_t received_at;   // our millis() at time of receipt, for "N minutes ago"
};

/**
 * A deliberately minimal, passive Mesh node: it has no contacts, sends no
 * messages, and (by default) doesn't repeat traffic for others -- it just
 * listens for the configured #channels and keeps a small rolling history
 * of what it heard, for the UI to display.
 */
class MyMesh : public mesh::Mesh {
  ChannelConfig* _channels;
  bool _relay_enabled = false;

  BoardMessage _messages[MAX_BOARD_MESSAGES];
  int _msg_count = 0;
  int _msg_next = 0;   // next slot to write (cyclic)
  uint32_t _total_received = 0;

protected:
  int searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel channels[], int max_matches) override {
    return _channels->findByHash(hash, channels, max_matches);
  }

  void onGroupDataRecv(mesh::Packet* packet, uint8_t type, const mesh::GroupChannel& channel, uint8_t* data, size_t len) override;

  // this board is a quiet appliance, not mesh infrastructure -- don't
  // spend its airtime relaying other people's packets by default.
  bool allowPacketForward(const mesh::Packet* packet) override { return _relay_enabled; }

public:
  MyMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng,
         mesh::RTCClock& rtc, mesh::MeshTables& tables, ChannelConfig& channels)
    : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(16), tables), _channels(&channels) { }

  void setRelayEnabled(bool en) { _relay_enabled = en; }
  bool getRelayEnabled() const { return _relay_enabled; }

  int getMessageCount() const { return _msg_count; }
  uint32_t getTotalReceived() const { return _total_received; }
  // idx 0 = most recently received
  const BoardMessage& getMessage(int idx_from_newest) const;
};
