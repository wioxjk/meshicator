#include "UITask.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#define AUTO_ADVANCE_MS     15000
#define REFRESH_MS          30000   // keep "N minutes ago" from going stale even when idle

// soft, warm palette -- deliberately not MeshCore's default UIColor scheme,
// which is tuned for dense status/contact-list screens rather than a
// single big readable message.
#define COL_BG            0xffbd   // warm cream
#define COL_CARD          0xfffe   // near-white
#define COL_CARD_BORDER   0xef3b   // soft cream border
#define COL_TEXT          0x39a6   // warm charcoal (not pure black)
#define COL_ACCENT        0x4b91   // dusty blue
#define COL_ACCENT_SOFT   0xdf3d   // pale dusty blue (pill background)
#define COL_MUTED         0x9cb1   // warm gray
#define COL_DOT_ACTIVE    0x4b91
#define COL_DOT_INACTIVE  0xe6d9

void UITask::begin() {
  _last_seen_total = _mesh->getTotalReceived();
  _dirty = true;
}

void UITask::formatAgo(uint32_t received_at, char* out, size_t out_sz) {
  uint32_t elapsed_s = (millis() - received_at) / 1000;
  if (elapsed_s < 45) {
    snprintf(out, out_sz, "just now");
  } else if (elapsed_s < 3600) {
    snprintf(out, out_sz, "%u min ago", (unsigned)(elapsed_s / 60));
  } else if (elapsed_s < 86400) {
    snprintf(out, out_sz, "%u hr ago", (unsigned)(elapsed_s / 3600));
  } else {
    snprintf(out, out_sz, "%u days ago", (unsigned)(elapsed_s / 86400));
  }
}

void UITask::renderWaiting() {
  auto& canvas = _display->canvas();

  _display->startFrame(COL_BG);

  canvas.setTextColor(COL_TEXT);
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  const char* headline = "Waiting for the first message" "\xE2\x80\xA6";  // UTF-8 ellipsis
  int w = canvas.textWidth(headline);
  canvas.setCursor((480 - w) / 2, 210);
  canvas.print(headline);

  canvas.setTextColor(COL_MUTED);
  canvas.setFont(&fonts::FreeSans12pt7b);

  char listening[160] = "Listening on: ";
  for (int i = 0; i < _channels->count(); i++) {
    if (i > 0) strncat(listening, ", ", sizeof(listening) - strlen(listening) - 1);
    strncat(listening, "#", sizeof(listening) - strlen(listening) - 1);
    strncat(listening, _channels->get(i).name, sizeof(listening) - strlen(listening) - 1);
  }
  w = canvas.textWidth(listening);
  canvas.setCursor((480 - w) / 2, 270);
  canvas.print(listening);

  _display->endFrame();
}

void UITask::renderMessage(const BoardMessage& m, int idx, int count) {
  auto& canvas = _display->canvas();

  _display->startFrame(COL_BG);

  // channel pill, top center
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  char pill[40];
  snprintf(pill, sizeof(pill), "#%s", m.channel_name);
  int pill_text_w = canvas.textWidth(pill);
  int pill_w = pill_text_w + 40;
  int pill_h = 44;
  int pill_x = (480 - pill_w) / 2;
  int pill_y = 20;
  _display->fillRoundRect(pill_x, pill_y, pill_w, pill_h, pill_h / 2, COL_ACCENT_SOFT);
  canvas.setTextColor(COL_ACCENT);
  canvas.setCursor(pill_x + 20, pill_y + 10);
  canvas.print(pill);

  // message card
  int card_x = 24, card_y = 84, card_w = 432, card_h = 320;
  _display->fillRoundRect(card_x, card_y, card_w, card_h, 24, COL_CARD_BORDER);
  _display->fillRoundRect(card_x + 2, card_y + 2, card_w - 4, card_h - 4, 22, COL_CARD);

  int inner_x = card_x + 28, inner_w = card_w - 56;
  int y = card_y + 26;

  if (m.sender[0]) {
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextColor(COL_ACCENT);
    canvas.setCursor(inner_x, y);
    canvas.print(m.sender);
    y += 40;
  }

  // pick a body font size that keeps longer messages from overflowing the card
  size_t text_len = strlen(m.text);
  const lgfx::GFXfont* body_font;
  int line_h;
  if (text_len <= 60) { body_font = &fonts::FreeSans24pt7b; line_h = 46; }
  else if (text_len <= 120) { body_font = &fonts::FreeSans18pt7b; line_h = 36; }
  else { body_font = &fonts::FreeSans12pt7b; line_h = 26; }
  canvas.setFont(body_font);
  canvas.setTextColor(COL_TEXT);

  int body_h = card_y + card_h - 56 - y;   // leave room for the timestamp line below
  _display->drawWrappedCentered(m.text, inner_x, y, inner_w, body_h, line_h);

  char ago[32];
  formatAgo(m.received_at, ago, sizeof(ago));
  canvas.setFont(&fonts::FreeSans9pt7b);
  canvas.setTextColor(COL_MUTED);
  int ago_w = canvas.textWidth(ago);
  canvas.setCursor(card_x + card_w - ago_w - 24, card_y + card_h - 34);
  canvas.print(ago);

  // pagination dots
  if (count > 1) {
    int dot_r = 5, dot_gap = 20;
    int total_w = (count - 1) * dot_gap;
    int start_x = (480 - total_w) / 2;
    int dot_y = 440;
    for (int i = 0; i < count; i++) {
      _display->fillCircle(start_x + i * dot_gap, dot_y, dot_r, i == idx ? COL_DOT_ACTIVE : COL_DOT_INACTIVE);
    }
  }

  _display->endFrame();
}

void UITask::loop() {
  unsigned long now = millis();

  // new message arrived -> jump straight to it
  uint32_t total = _mesh->getTotalReceived();
  if (total != _last_seen_total) {
    _last_seen_total = total;
    _view_idx = 0;
    _next_auto_advance = now + AUTO_ADVANCE_MS;
    _dirty = true;
  }

  // single tap anywhere: advance to the next (older) message, and restart
  // the auto-advance timer so a manual browse isn't immediately undone
  int tx, ty;
  bool touched = _display->getTouch(&tx, &ty);
  if (touched && !_was_touched && _mesh->getMessageCount() > 1) {
    _view_idx = (_view_idx + 1) % _mesh->getMessageCount();
    _next_auto_advance = now + AUTO_ADVANCE_MS;
    _dirty = true;
  }
  _was_touched = touched;

  if (_mesh->getMessageCount() > 1 && (long)(now - _next_auto_advance) >= 0) {
    _view_idx = (_view_idx + 1) % _mesh->getMessageCount();
    _next_auto_advance = now + AUTO_ADVANCE_MS;
    _dirty = true;
  }

  if ((long)(now - _next_refresh) >= 0) {
    _next_refresh = now + REFRESH_MS;
    _dirty = true;
  }

  if (!_dirty) return;
  _dirty = false;

  if (_mesh->getMessageCount() == 0) {
    renderWaiting();
  } else {
    renderMessage(_mesh->getMessage(_view_idx), _view_idx, _mesh->getMessageCount());
  }
}
