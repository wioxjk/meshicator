#pragma once

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/IdentityStore.h>   // for the FILESYSTEM macro

#ifndef MAX_GROUP_CHANNELS
  #define MAX_GROUP_CHANNELS 8
#endif

#ifndef CHANNEL_CONFIG_FILE
  #define CHANNEL_CONFIG_FILE "/channel_board.cfg"
#endif

// the standard, publicly-documented MeshCore "Public" channel key -- see
// docs/companion_protocol.md ("Uses a publicly known 16-byte key:
// 8b3387e9c5cdea6ac9e5edbaa115cd72"). Pre-seeded on first boot so the
// board shows something useful before anyone configures a private channel.
#define DEFAULT_PUBLIC_CHANNEL_NAME  "Public"
#define DEFAULT_PUBLIC_CHANNEL_PSK   "izOH6cXN6mrJ5e26oRXNcg=="

struct ChannelCfg {
  char name[32];
  mesh::GroupChannel channel;
  uint8_t secret_len;   // 16 or 32 -- which of `channel.secret`'s bytes are real key material
};

/**
 * Small, self-contained store for "which #channels does this board watch".
 * Persisted as a plain text file (one "name,base64psk" line per channel)
 * on the ESP32's SPIFFS partition -- deliberately not using the fancier
 * ChannelDetails/BaseChatMesh machinery, since this board never needs
 * contacts, DMs or the rest of a full companion radio.
 */
class ChannelConfig {
  ChannelCfg _channels[MAX_GROUP_CHANNELS];
  int _count = 0;

public:
  int count() const { return _count; }
  const ChannelCfg& get(int i) const { return _channels[i]; }

  // returns false if the PSK doesn't decode to a valid 16 or 32 byte key,
  // or the table is already full
  bool add(const char* name, const char* psk_base64);
  bool removeAt(int idx);

  void load(FILESYSTEM& fs);
  void save(FILESYSTEM& fs);

  // populates `dest` with every configured channel whose hash matches;
  // returns the number of matches (0, or occasionally >1 on a hash
  // collision -- caller tries each until decryption succeeds)
  int findByHash(const uint8_t* hash, mesh::GroupChannel dest[], int max_matches) const;
};
