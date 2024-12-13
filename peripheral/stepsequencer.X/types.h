#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int note;     // note value (0-127)
    int velocity; // velocity (0-127)
    int duration; // duration (1-100)
    bool active;  // is active
} NoteState;

extern const NoteState DEFAULT_SEQUENCE[2][4];

typedef const char *(*format_value_func)(int);

typedef struct
{
    const char *descriptor;
    int value;
    int min_value;
    int max_value;
    format_value_func format_value;
} SettingInfo;

typedef enum
{
    SETTINGS_MODE_NOTE,
    SETTINGS_MODE_VELOCITY,
    SETTINGS_MODE_DURATION,
    SETTINGS_MODE_ACTIVE
} SettingsMode;

typedef struct
{
    volatile bool red_down;
    volatile bool yellow_down;
    volatile int red_held_counter;
    volatile int yellow_held_counter;
    volatile bool handle_red;
    volatile bool handle_yellow;
} ButtonState;

typedef struct
{
    volatile int16_t value;
    volatile uint8_t enabled;
    volatile bool value_ready;
    volatile int16_t initial_value;
    volatile int initial_setting_value;
} ADCState;

typedef struct
{
    uint8_t current_row;
    uint8_t current_col;
    int previous_row;
    int previous_col;
} StepPosition;

typedef struct
{
    bool unlock_settings;
    SettingsMode mode;
    StepPosition editing; // step currently editing
    StepPosition playing; // step currently playing
} SequencerState;

typedef struct
{
    bool blink;
    int blink_counter;
} DisplayState;

#endif /* TYPES_H */