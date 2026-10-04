/* Alarm vibration patterns and alarm sounds recreated from PebbleOS.
 *
 * Vibe patterns are converted from the PebbleOS vibe scores
 * (.json files in resources/normal/snowy/vibes) and sounds are copied from
 * fw/services/alarms/alarm_tones.c
 *
 * PebbleOS: SPDX-FileCopyrightText: 2026 Core Devices LLC
 *           SPDX-License-Identifier: Apache-2.0
 *
 * Apps can't set the vibration strength (that call isn't in the SDK), so each score
 * is reproduced as plain on/off timing. The rhythm is ok, accents are gone
 */

#include <pebble.h>
#include "common.h"

#if SYSTEM_VIBES

// On/off durations in ms, alternating and starting with "on"
static const uint16_t s_vibe_standard_long_high[] = {  // Standard
  500
};
static const uint16_t s_vibe_nudge_nudge[] = {  // Nudge Nudge
  30, 118, 30
};
static const uint16_t s_vibe_jackhammer[] = {  // Jackhammer
  50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};
static const uint16_t s_vibe_reveille[] = {  // Reveille
  111, 129, 105, 135, 102, 18, 105, 15, 111, 129, 102, 138, 105, 135, 102, 18, 105, 15, 111, 129,
  102, 138, 105, 135, 102, 18, 105, 15, 111, 129, 105, 135, 222, 258, 105, 135, 111, 129, 105, 135,
  102, 18, 105, 15, 111, 129, 102, 138, 105, 135, 102, 18, 105, 15, 111, 129, 102, 138, 105, 135,
  102, 18, 105, 15, 111, 129, 111, 129, 465, 255, 102, 138, 102, 138, 102, 138, 102, 138, 102, 138,
  220, 260, 102, 138, 102, 138, 102, 138, 105, 135, 102, 138, 105, 135, 462, 258, 102, 138, 102,
  138, 102, 138, 102, 138, 102, 138, 220, 260, 102, 138, 102, 138, 105, 135, 102, 18, 105, 15, 111,
  129, 111, 129, 465
};
static const uint16_t s_vibe_mario[] = {  // Mario
  102, 48, 102, 198, 102, 198, 105, 45, 102, 198, 100, 500, 111
};
static const uint16_t s_vibe_gentle[] = {  // Gentle (System)
  100, 4020, 100, 4020, 100, 4020, 100, 4020, 100, 4020, 100, 10000, 100, 20, 100, 4020, 100, 20,
  100, 4020, 100, 20, 100, 4020, 100, 20, 100, 4020, 100, 20, 100, 4020, 100, 20, 100, 8020, 100,
  20, 200, 1020, 100, 20, 200, 4020, 100, 20, 200, 1020, 100, 20, 200, 4020, 100, 20, 200, 1020,
  100, 20, 200, 4020, 100, 20, 200, 1020, 100, 20, 200, 8020, 100, 20, 200, 1020, 100, 20, 200,
  4020, 100, 20, 200, 1020, 100, 20, 200, 4020, 100, 20, 200, 1020, 100, 20, 200, 4020, 100, 20,
  200, 1020, 100, 20, 200, 8020, 200, 20, 300, 1020, 200, 20, 300, 2020, 200, 20, 300, 1020, 200,
  20, 300, 2020, 200, 20, 300, 1020, 200, 20, 300, 2020, 200, 20, 300, 1020, 200, 20, 300, 2020,
  200, 20, 300, 1020, 200, 20, 300, 6020, 200, 20, 300, 1020, 200, 20, 300, 1020, 200, 20, 300,
  1020, 200, 20, 300, 1020, 200, 20, 300, 1020, 200, 20, 300, 1020, 200, 20, 300, 1020, 200, 20,
  300, 1020, 200, 20, 300, 1020, 200, 20, 300
};
static const uint16_t s_vibe_double_pulse_medium[] = {  // Double Pulse
  400, 200, 400
};
static const uint16_t s_vibe_pebble_morse[] = {  // Pebble Morse
  60, 60, 180, 60, 180, 60, 60, 180, 60, 180, 180, 60, 60, 60, 60, 60, 60, 180, 180, 60, 60, 60,
  60, 60, 60, 180, 60, 60, 180, 60, 60, 60, 60, 180, 60
};
static const uint16_t s_vibe_heartbeat[] = {  // Heartbeat
  80, 100, 140, 510, 80, 100, 140
};
static const uint16_t s_vibe_double_tap[] = {  // Double Tap
  45, 75, 45
};
static const uint16_t s_vibe_imperial[] = {  // Imperial
  420, 70, 420, 70, 420, 70, 300, 70, 120, 70, 420, 70, 300, 70, 120, 70, 800
};

static const struct {
  const uint16_t *segments;
  uint8_t num_segments;
  uint16_t gap_ms;     // pause after each repeat (trailing rest + repeat delay)
  const char *name;
} s_vibes[SYS_VIBE_COUNT] = {
  { s_vibe_standard_long_high, ARRAY_LENGTH(s_vibe_standard_long_high), 1000, "Standard" },
  { s_vibe_nudge_nudge, ARRAY_LENGTH(s_vibe_nudge_nudge), 1018, "Nudge Nudge" },
  { s_vibe_jackhammer, ARRAY_LENGTH(s_vibe_jackhammer), 1050, "Jackhammer" },
  { s_vibe_reveille, ARRAY_LENGTH(s_vibe_reveille), 1995, "Reveille" },
  { s_vibe_mario, ARRAY_LENGTH(s_vibe_mario), 1009, "Mario" },
  { s_vibe_gentle, ARRAY_LENGTH(s_vibe_gentle), 5020, "Gentle (System)" },
  { s_vibe_double_pulse_medium, ARRAY_LENGTH(s_vibe_double_pulse_medium), 1000, "Double Pulse" },
  { s_vibe_pebble_morse, ARRAY_LENGTH(s_vibe_pebble_morse), 1000, "Pebble Morse" },
  { s_vibe_heartbeat, ARRAY_LENGTH(s_vibe_heartbeat), 1010, "Heartbeat" },
  { s_vibe_double_tap, ARRAY_LENGTH(s_vibe_double_tap), 1015, "Double Tap" },
  { s_vibe_imperial, ARRAY_LENGTH(s_vibe_imperial), 1015, "Imperial" },
};

_Static_assert(ARRAY_LENGTH(s_vibes) == SYS_VIBE_COUNT, "s_vibes must have SYS_VIBE_COUNT entries");

const char* sys_vibe_name(uint8_t index) {
  return (index < SYS_VIBE_COUNT) ? s_vibes[index].name : "???";
}

uint8_t sys_vibe_get(uint8_t index, uint32_t *buffer, uint8_t max_segments, uint32_t max_ms,
                     uint32_t *play_ms, uint32_t *gap_ms) {
  if (index >= SYS_VIBE_COUNT) index = 0;
  uint8_t n = 0;
  uint32_t total = 0;
  for (uint8_t i = 0; i < s_vibes[index].num_segments && n < max_segments; i++) {
    uint32_t dur = s_vibes[index].segments[i];
    if (max_ms && total + dur > max_ms) {
      // Trimming a preview: end on an "on" segment cut to fit
      if (i % 2 == 0 && total < max_ms) buffer[n++] = max_ms - total;
      break;
    }
    buffer[n++] = dur;
    total += dur;
  }
  // A pattern must end on an "on" segment
  if (n > 0 && n % 2 == 0) total -= buffer[--n];
  if (play_ms) *play_ms = total;
  if (gap_ms) *gap_ms = s_vibes[index].gap_ms;
  return n;
}
#endif // SYSTEM_VIBES

#if ALARM_SOUND
#define NOTE(midi, wave, ms) {.midi_note = (midi), .waveform = (wave), .duration_ms = (ms)}

// MIDI: C4=60, D4=62, E4=64, F4=65, G4=67, A4=69, B4=71,
//       C5=72, D5=74, E5=76, F5=77, G5=79, A5=81, C6=84.

// Reveille: beat-for-beat with the Reveille vibe score
static const SpeakerNote s_reveille[] = {
  // note_1, _
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6, _, note_10, note_6, note_1, _, note_10, _
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // (repeat)
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6, _, note_10, note_6, note_1, _, note_6, _
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_10_ (held), _, _, note_6, _, note_1, _
  NOTE(76, SpeakerWaveformSquare, 240),
  NOTE(0, SpeakerWaveformSquare, 240),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6, _, note_10, note_6, note_1, _, note_10, _
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // (repeat)
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6, _, note_10, note_6, note_1, _, note_1, _
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6___ (long held), _, _, note_10, _
  NOTE(72, SpeakerWaveformSquare, 480),
  NOTE(0, SpeakerWaveformSquare, 240),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_10 × 4
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_13_ (apex G5), _, _, note_10, _, note_10, _
  NOTE(79, SpeakerWaveformSquare, 240),
  NOTE(0, SpeakerWaveformSquare, 240),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_10, _, note_6, _, note_10, _, note_6, _
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_10___ (long E5), _, _, note_10, _
  NOTE(76, SpeakerWaveformSquare, 480),
  NOTE(0, SpeakerWaveformSquare, 240),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_10 × 4
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_13_ (apex G5), _, _, note_10, _, note_10, _
  NOTE(79, SpeakerWaveformSquare, 240),
  NOTE(0, SpeakerWaveformSquare, 240),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6, _, note_10, note_6, note_1, _, note_1, _
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(76, SpeakerWaveformSquare, 120),
  NOTE(72, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  NOTE(67, SpeakerWaveformSquare, 120),
  NOTE(0, SpeakerWaveformSquare, 120),
  // note_6___ (final C5), then the 480ms of trailing rests inside the score.
  NOTE(72, SpeakerWaveformSquare, 480),
  NOTE(0, SpeakerWaveformSquare, 480),
};

// Beacon: alternating C5/G5 squares, six cycles
static const SpeakerNote s_beacon[] = {
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
  NOTE(72, SpeakerWaveformSquare, 200), NOTE(79, SpeakerWaveformSquare, 200),
};

// Bell: sine wave descending, gentler than Reveille
static const SpeakerNote s_bell[] = {
  NOTE(81, SpeakerWaveformSine, 500), // A5
  NOTE(77, SpeakerWaveformSine, 500), // F5
  NOTE(74, SpeakerWaveformSine, 500), // D5
  NOTE(69, SpeakerWaveformSine, 700), // A4
};

// Chime: triangle ascending with brief rests, lighter than Reveille & Bell
static const SpeakerNote s_chime[] = {
  NOTE(72, SpeakerWaveformTriangle, 250),                                         // C5
  NOTE(0, SpeakerWaveformTriangle, 50),   NOTE(76, SpeakerWaveformTriangle, 250), // E5
  NOTE(0, SpeakerWaveformTriangle, 50),   NOTE(79, SpeakerWaveformTriangle, 250), // G5
  NOTE(0, SpeakerWaveformTriangle, 50),   NOTE(84, SpeakerWaveformTriangle, 500), // C6
};

#undef NOTE

static const struct {
  const SpeakerNote *notes;
  uint16_t count;
  const char *name;
} s_tones[SYS_TONE_COUNT] = {
  { s_reveille, ARRAY_LENGTH(s_reveille), "Reveille" },
  { s_beacon,   ARRAY_LENGTH(s_beacon),   "Beacon" },
  { s_bell,     ARRAY_LENGTH(s_bell),     "Bell" },
  { s_chime,    ARRAY_LENGTH(s_chime),    "System Chime" },
};

const char* sys_tone_name(uint8_t index) {
  return (index < SYS_TONE_COUNT) ? s_tones[index].name : "???";
}

uint16_t sys_tone_get(uint8_t index, const SpeakerNote **notes, uint32_t max_ms) {
  if (index >= SYS_TONE_COUNT) index = 0;
  *notes = s_tones[index].notes;
  if (max_ms == 0) return s_tones[index].count;
  // Count the notes that fit within max_ms (used for short previews)
  uint32_t total = 0;
  uint16_t n = 0;
  while (n < s_tones[index].count && total + s_tones[index].notes[n].duration_ms <= max_ms)
    total += s_tones[index].notes[n++].duration_ms;
  return n > 0 ? n : 1;
}
#endif // ALARM_SOUND

// The option counts in common.h must match the tables above
#if SYSTEM_VIBES
_Static_assert(VP_COUNT - VP_SysFirst == SYS_VIBE_COUNT, "VP_COUNT in common.h must match SYS_VIBE_COUNT");
#endif
#if ALARM_SOUND
_Static_assert(AS_Count - AS_SysFirst == SYS_TONE_COUNT, "AS_Count in common.h must match SYS_TONE_COUNT");
#endif