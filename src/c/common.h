#pragma once
#include <pebble.h>
  
#define VERSION "4.1"
  
#ifdef PBL_COLOR
#define IF_COLOR(statement)   (statement)
#define IF_BW(statement)
#define IF_COLORBW(color, bw) (color)
#define COLOR_SCREEN 1
#else
#define IF_COLOR(statement)
#define IF_BW(statement)    (statement)
#define IF_COLORBW(color, bw) (bw)
#define COLOR_SCREEN 0
#endif

#ifdef PBL_SDK_2
#define IF_32(sdk3, sdk2) (sdk2)
#define IF_3(sdk3)
#define IF_2(sdk2) (sdk2)
#else
#define IF_32(sdk3, sdk2) (sdk3)
#define IF_3(sdk3) (sdk3)
#define IF_2(sdk2)
#endif

// Screen size class: "Big" screens are Pebble Time 2 (emery, 200x228) and Pebble Round 2 (gabbro, 260x260)
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO) || \
    (defined(PBL_DISPLAY_WIDTH) && PBL_DISPLAY_WIDTH >= 200)
#define BIG_SCREEN 1
#define IF_BIG_ELSE(big, small) (big)
#else
#define BIG_SCREEN 0
#define IF_BIG_ELSE(big, small) (small)
#endif

// Alarm sounds: only the Pebble Time 2 (emery) and Pebble 2 Duo (flint) have speakers.
// The SDK defines PBL_SPEAKER on other platforms too (e.g. basalt, and gabbro, which has no
// speaker), so the platforms have to be checked as well.
#if defined(PBL_SPEAKER) && (defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_FLINT))
#define ALARM_SOUND 1
#else
#define ALARM_SOUND 0
#endif

// Width of the action bar on rect watches. Every window uses the same width
#ifdef PBL_RECT
#undef ACTION_BAR_WIDTH
#if BIG_SCREEN
#define ACTION_BAR_WIDTH 26
#else
#define ACTION_BAR_WIDTH 20
#endif
#endif

typedef void (*SettingsClosedCallBack)();
  
typedef struct alarm {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
 } __attribute__((__packed__)) alarm;

typedef enum MoveSensitivity {
  MS_LOW = 1,
  MS_MEDIUM = 2,
  MS_HIGH = 3
} MoveSensitivty;

typedef enum VibePatterns {
  VP_Gentle = 0,
  VP_NSG = 1, // Not-So-Gentle
  VP_NSG2Snooze = 2,
  VP_SysFirst = 3   // System patterns (see alarmpatterns.c) are VP_SysFirst + index
} VibePatterns;

// Number of system vibration patterns and system sounds in alarmpatterns.c
#define SYS_VIBE_COUNT 11
#define SYS_TONE_COUNT 4

// The Reveille sound is written beat-for-beat to match the Reveille vibration, so when both are
// chosen they're kept in step (positions in the tables in alarmpatterns.c)
#define SYS_VIBE_REVEILLE 3
#define SYS_TONE_REVEILLE 0

// System vibration patterns aren't offered on aplite, which has no memory to spare
#ifdef PBL_PLATFORM_APLITE
#define SYSTEM_VIBES 0
#define VP_COUNT VP_SysFirst
#else
#define SYSTEM_VIBES 1
#define VP_COUNT (VP_SysFirst + SYS_VIBE_COUNT)
#endif

// Alarm sound options (only offered on watches with a speaker - see ALARM_SOUND above)
typedef enum AlarmSound {
  AS_Off = 0,        // Vibrate only
  AS_Chime = 1,      // Soft chime, timed to the vibration
  AS_Beeps = 2,      // Beeps, timed to the vibration
  AS_SysFirst = 3,   // System sounds (Reveille, Beacon, Bell, System Chime) are AS_SysFirst + index
  AS_Count = AS_SysFirst + SYS_TONE_COUNT
} AlarmSound;

#define DEFAULT_START_VOLUME 20   // used when sound_start_volume is 0 (not set yet)

typedef enum GooBMode {
  GM_Off = 0,
  GM_AfterAlarm = 1,
  GM_AfterStop = 2
} GooBMode;

struct Settings_st {
  uint8_t snooze_delay;
  bool dynamic_snooze;
  bool easy_light;
  bool smart_alarm;
  uint8_t monitor_period;
  MoveSensitivty sensitivity;
  WeekDay dst_check_day;
  uint8_t dst_check_hour;
  bool konamic_code_on;
  VibePatterns vibe_pattern;
  alarm one_time_alarm;
  uint8_t autoclose_timeout;
  GooBMode goob_mode;
  uint8_t goob_monitor_period;
  // Added in 4.1 at the end of the struct, so settings saved by older versions load these as 0
  uint8_t alarm_sound;          // AlarmSound value (0 = off)
  bool sound_only;              // true = don't vibrate when a sound is playing (0 = vibrate too)
  uint8_t sound_start_volume;   // starting volume in % (0 = DEFAULT_START_VOLUME)
} __attribute__((__packed__));

typedef enum AlarmDay {
  A_SUNDAY = 0,
  A_MONDAY = 1,
  A_TUESDAY = 2,
  A_WEDNESDAY = 3,
  A_THURSDAY = 4,
  A_FRIDAY = 5,
  A_SATURDAY = 6
} AlarmDay;

void dayname(uint8_t day, char *daystr, int slen);
void daynameshort(uint8_t day, char *daystr, int slen);
void gen_time_str(uint8_t hour, uint8_t min, char *timestr, int slen);
void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen);
time_t strip_time(time_t timestamp);
int64_t day_diff(time_t date1, time_t date2);
time_t get_UTC_offset(struct tm *t);
WeekDay ad2wd(AlarmDay alarmday);

// ---- System alarm vibration patterns and sounds, recreated from PebbleOS (alarmpatterns.c) ----

#if SYSTEM_VIBES
// Name of a system vibration pattern, for the Settings menu
const char* sys_vibe_name(uint8_t index);

// Copies a system vibration pattern into buffer (on/off durations, starting with on) and
// returns the number of segments. If max_ms is non-zero, the pattern is cut short to fit
// (for previews). play_ms receives the pattern's length and gap_ms the pause the system
// leaves before repeating it. Either can be NULL.
uint8_t sys_vibe_get(uint8_t index, uint32_t *buffer, uint8_t max_segments, uint32_t max_ms,
                     uint32_t *play_ms, uint32_t *gap_ms);
#endif

#if ALARM_SOUND
// Name of a system alarm sound, for the Settings menu
const char* sys_tone_name(uint8_t index);

// Points notes at a system alarm sound and returns the number of notes. If max_ms is
// non-zero, only the notes that fit within max_ms are counted (for previews).
uint16_t sys_tone_get(uint8_t index, const SpeakerNote **notes, uint32_t max_ms);
#endif