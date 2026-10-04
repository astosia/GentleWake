#include <pebble.h>
#include "common.h"

void dayname(uint8_t day, char *daystr, int slen) {
  switch (day) {
    case 0:
      strncpy(daystr, "Sunday", slen);
      break;
    case 1:
      strncpy(daystr, "Monday", slen);
      break;
    case 2:
      strncpy(daystr, "Tuesday", slen);
      break;
    case 3:
      strncpy(daystr, "Wednesday", slen);
      break;
    case 4:
      strncpy(daystr, "Thursday", slen);
      break;
    case 5:
      strncpy(daystr, "Friday", slen);
      break;
    case 6:
      strncpy(daystr, "Saturday", slen);
      break;
    default:
      strncpy(daystr, "", slen);
  }
}

void daynameshort(uint8_t day, char *daystr, int slen) {
  dayname(day, daystr, (slen < 4 ? slen : 3));
  daystr[(slen < 4 ? slen-1 : 3)] = '\0';
}

void gen_time_str(uint8_t hour, uint8_t min, char *timestr, int slen) {
  if (clock_is_24h_style())
      snprintf(timestr, slen, "%d:%.2d", hour, min);
    else
      snprintf(timestr, slen, "%d:%.2d%s", hour > 12 ? hour - 12 : hour == 0 ? 12 : hour, min,
               hour >= 12 ? "PM" : "AM");
}

void gen_alarm_str(alarm *alarmtime, char *alarmstr, int slen) {
  if (alarmtime->enabled) {
    gen_time_str(alarmtime->hour, alarmtime->minute, alarmstr, slen);
  } else {
    strncpy(alarmstr, "OFF", slen);
  }
}

// Strip time component from a timestamp (leaving the date part)
time_t strip_time(time_t timestamp) {
  return timestamp - (timestamp % (60*60*24));
}

// Calculates the number of days (not 24 hour periods) between 2 dates where date1 is older
// than date2 (if date2 is older a negative number will be returned)
int64_t day_diff(time_t date1, time_t date2) {
  return ((strip_time(date2) - strip_time(date1)) / (60*60*24));
}

// Gets the UTC offset of the local time in seconds 
// (pass in an existing localtime struct tm to save creating another one, or else pass NULL)
time_t get_UTC_offset(struct tm *t) {
#ifdef PBL_SDK_2
  // SDK2 uses localtime instead of UTC for all time functions so always return 0
  return 0; 
#else
  time_t now = time(NULL);
  if (t == NULL) t = localtime(&now);
  
  // Work the offset out from the local clock time itself, rather than from tm_gmtoff and
  // tm_isdst. Firmware versions disagree on those: older firmware gives tm_gmtoff without
  // summer time (so it had to be added), but current PebbleOS already includes it, which
  // made summer time count twice (e.g. UTC+2 instead of UTC+1 in the UK in summer).
  int32_t local_secs = (t->tm_hour * 3600) + (t->tm_min * 60) + t->tm_sec;
  int32_t utc_secs = now % (24 * 60 * 60);
  
  // Local time can be on the day before or after UTC: compare the days of the week
  // (1 Jan 1970, time 0, was a Thursday = 4)
  int8_t utc_wday = ((now / (24 * 60 * 60)) + 4) % 7;
  int8_t day_shift = t->tm_wday - utc_wday;
  if (day_shift > 1) day_shift -= 7;
  else if (day_shift < -1) day_shift += 7;
  
  int32_t offset = local_secs - utc_secs + (day_shift * 24 * 3600);
  
  // Every real time zone is a whole number of 15 minutes, so round to that. This also caters for
  // the clock ticking over a second between localtime() and time(NULL).
  offset = ((offset + (offset >= 0 ? 450 : -450)) / 900) * 900;
  
  return offset;
#endif 
}

WeekDay ad2wd(AlarmDay alarmday) {
  switch (alarmday) {
    case A_SUNDAY:
      return SUNDAY;
    case A_MONDAY:
      return MONDAY;
    case A_TUESDAY:
      return TUESDAY;
    case A_WEDNESDAY:
      return WEDNESDAY;
    case A_THURSDAY:
      return THURSDAY;
    case A_FRIDAY:
      return FRIDAY;
    case A_SATURDAY:
      return SATURDAY;
    default:
      return TODAY;
  }
}