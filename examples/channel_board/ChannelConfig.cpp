#include "ChannelConfig.h"

// NOTE: deliberately not #include <base64.hpp> here -- its functions
// aren't declared inline, so a second translation unit including it
// causes "multiple definition" link errors. src/helpers/BaseChatMesh.cpp
// already includes it (and is always compiled), so just declare the two
// functions we need against its real signatures and link against that
// one definition instead.
unsigned int decode_base64(const unsigned char input[], unsigned int input_length, unsigned char output[]);
unsigned int encode_base64(const unsigned char input[], unsigned int input_length, unsigned char output[]);

bool ChannelConfig::add(const char* name, const char* psk_base64) {
  if (_count >= MAX_GROUP_CHANNELS) return false;

  size_t b64_len = strlen(psk_base64);
  if (b64_len > 44) return false;   // longer than a 32-byte key can ever encode to -- reject before decoding

  uint8_t secret[PUB_KEY_SIZE];
  memset(secret, 0, sizeof(secret));
  int len = decode_base64((const unsigned char*)psk_base64, b64_len, secret);
  if (len != 16 && len != 32) return false;

  ChannelCfg& dest = _channels[_count];
  memset(&dest, 0, sizeof(dest));
  strncpy(dest.name, name, sizeof(dest.name) - 1);
  memcpy(dest.channel.secret, secret, sizeof(secret));
  dest.secret_len = (uint8_t)len;
  mesh::Utils::sha256(dest.channel.hash, sizeof(dest.channel.hash), secret, len);

  _count++;
  return true;
}

bool ChannelConfig::removeAt(int idx) {
  if (idx < 0 || idx >= _count) return false;
  for (int i = idx; i < _count - 1; i++) {
    _channels[i] = _channels[i + 1];
  }
  _count--;
  return true;
}

int ChannelConfig::findByHash(const uint8_t* hash, mesh::GroupChannel dest[], int max_matches) const {
  int n = 0;
  for (int i = 0; i < _count && n < max_matches; i++) {
    if (memcmp(_channels[i].channel.hash, hash, PATH_HASH_SIZE) == 0) {
      dest[n++] = _channels[i].channel;
    }
  }
  return n;
}

void ChannelConfig::load(FILESYSTEM& fs) {
  _count = 0;

  File f = fs.open(CHANNEL_CONFIG_FILE, "r");
  if (f) {
    while (f.available() && _count < MAX_GROUP_CHANNELS) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      int comma = line.indexOf(',');
      if (comma < 0) continue;

      String name = line.substring(0, comma);
      String psk = line.substring(comma + 1);
      add(name.c_str(), psk.c_str());
    }
    f.close();
  }

  if (_count == 0) {
    // first boot, or config file missing/empty -- seed the well-known
    // Public channel so the board isn't showing a totally blank screen
    add(DEFAULT_PUBLIC_CHANNEL_NAME, DEFAULT_PUBLIC_CHANNEL_PSK);
    save(fs);
  }
}

void ChannelConfig::save(FILESYSTEM& fs) {
  File f = fs.open(CHANNEL_CONFIG_FILE, "w");
  if (!f) return;

  char b64[64];
  for (int i = 0; i < _count; i++) {
    unsigned int enc_len = encode_base64(_channels[i].channel.secret, _channels[i].secret_len, (unsigned char*)b64);
    b64[enc_len] = 0;
    f.print(_channels[i].name);
    f.print(",");
    f.println(b64);
  }
  f.close();
}
