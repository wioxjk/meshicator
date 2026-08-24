#pragma once

#include <IndicatorDisplay.h>
#include "MyMesh.h"
#include "ChannelConfig.h"

/**
 * The "calm, elegant" message board screen: one message at a time, big and
 * centered, auto-advancing through recent history every AUTO_ADVANCE_MS, or
 * advance-on-tap for anyone who wants to browse manually. No lists, no
 * menus, nothing to configure from the touchscreen -- everything about it
 * is meant to be readable at a glance from across a room.
 */
class UITask {
  IndicatorDisplay* _display;
  MyMesh* _mesh;
  ChannelConfig* _channels;

  int _view_idx = 0;              // 0 = newest message
  uint32_t _last_seen_total = 0;
  unsigned long _next_auto_advance = 0;
  unsigned long _next_refresh = 0;
  bool _dirty = true;
  bool _was_touched = false;

  void renderMessage(const BoardMessage& m, int idx, int count);
  void renderWaiting();
  void formatAgo(uint32_t received_at, char* out, size_t out_sz);

public:
  UITask(IndicatorDisplay& display, MyMesh& mesh, ChannelConfig& channels)
    : _display(&display), _mesh(&mesh), _channels(&channels) { }

  void begin();
  void loop();
};
