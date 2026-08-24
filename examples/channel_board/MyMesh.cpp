#include "MyMesh.h"

// mirrors BaseChatMesh::onGroupDataRecv()'s PAYLOAD_TYPE_GRP_TXT parsing
// (timestamp[4] + txt_type[1] + "name: message" or plain "message")
void MyMesh::onGroupDataRecv(mesh::Packet* packet, uint8_t type, const mesh::GroupChannel& channel, uint8_t* data, size_t len) {
  if (type != PAYLOAD_TYPE_GRP_TXT) return;   // we don't do anything with PAYLOAD_TYPE_GRP_DATA (telemetry etc)
  if (len < 5) return;

  uint8_t txt_type = data[4];
  if ((txt_type >> 2) != 0) return;   // unsupported text sub-type

  data[len] = 0;   // decrypted buffer has room for this; makes &data[5] a proper C string
  const char* body = (const char*)&data[5];

  BoardMessage& m = _messages[_msg_next];
  memset(&m, 0, sizeof(m));

  int ch_idx = -1;
  for (int i = 0; i < _channels->count(); i++) {
    if (memcmp(_channels->get(i).channel.hash, channel.hash, PATH_HASH_SIZE) == 0) { ch_idx = i; break; }
  }
  StrHelper::strncpy(m.channel_name, ch_idx >= 0 ? _channels->get(ch_idx).name : "?", sizeof(m.channel_name));

  // convention used by the companion apps: "sender_name: message text"
  const char* colon = strstr(body, ": ");
  if (colon != NULL && colon - body < (int)sizeof(m.sender)) {
    size_t name_len = colon - body;
    memcpy(m.sender, body, name_len);
    m.sender[name_len] = 0;
    StrHelper::strncpy(m.text, colon + 2, sizeof(m.text));
  } else {
    StrHelper::strncpy(m.text, body, sizeof(m.text));
  }
  m.received_at = _ms->getMillis();

  _msg_next = (_msg_next + 1) % MAX_BOARD_MESSAGES;
  if (_msg_count < MAX_BOARD_MESSAGES) _msg_count++;
  _total_received++;
}

const BoardMessage& MyMesh::getMessage(int idx_from_newest) const {
  // _msg_next points at the slot that will be overwritten next, i.e. one
  // past the most recently written message.
  int idx = (_msg_next - 1 - idx_from_newest + MAX_BOARD_MESSAGES * 2) % MAX_BOARD_MESSAGES;
  return _messages[idx];
}
