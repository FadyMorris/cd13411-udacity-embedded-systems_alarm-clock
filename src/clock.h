/// @file clock.h
/// Interfaces the Clock class.
///
/// This header defines the interface to the clock class.
#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>
#include <Arduino.h>
#include "tm1637.h"
#include "alarm_tone.h"

// ----------- By Fady -------------------
//

/// @brief An enum to define the states of the clock.
enum ClockState
{
    // Note: the first three states need to be in this exact order
    STATE_CLOCK = 0,      ///< Normal state. Display clock.
    STATE_MENU_SET = 1,   ///< Menu (Displaying "SET")
    STATE_MENU_ALARM = 2, ///< Menu (Displaying "AL")
                          // ------------------------
    STATE_SET_CLOCK = 3,  ///< Set clock state. Blinking the selected digit being set.
    STATE_SET_ALARM = 4,  ///< Set alarm state. Blinking the selected digit being set.
    STATE_ALARM_OFF = 5,  ///< Menu after selecting alarm if the alarm is off (Displaying "OFF")
    STATE_ALARM = 6,      ///< The alarm state. The buzzer sounds and the display is blinking with the alarm time.
};

/// @enum The digit state (Whether the digit in focus is the left or right state).
///        Also used to indicate which parts of the display is blinking
enum DigitState
{
    DIGITS_LEFT = 0b10,  ///< left digits of the display (hours)
    DIGITS_RIGHT = 0b01, ///< right digits of the display (minutes)
    POINT = 0b100        ///< Point (middle colon)
};
// ------------------------------

/// @brief Button type enum.
enum ButtonType
{
    BUTTON_MENU,
    BUTTON_PLUS,
    BUTTON_MINUS,
    BUTTON_OK,
};

class Clock
{
private:
    TM1637 *display = NULL; ///< 7-segment Display object

    hw_timer_t *timer = NULL; ///< Timer variable to count time

    // TODO: Add other private variables here
    AlarmTone *alarm_tone; ///< The buzzer variable. Pointing to the buzzer object.
    uint32_t time = 0;
    uint32_t alarm = 0;
    uint32_t *time_to_set = nullptr;                            ///< A pointer of the current time to set. Points to either clock or alarm
    uint32_t temp_time = 0;                                     ///< The variable on display that is being modified in the set menu.
                                                                /// This variable isn't stored unless the OK button is pressed. Pressing the menu button cancels the variable storage.
    uint32_t timestamp = 0;                                     ///< timestamp in milliseconds. Used for incrementing the time counter and advancing the clock.
    uint8_t state = STATE_CLOCK;                                ///< Current state of the clock
    uint8_t set_digit = DIGITS_LEFT;                            ///< The current digit in focus in the SET or Alarm Menu.
    bool alarm_enabled = 0;                                     ///< The state of the alarm enable switch.
    uint8_t blink_state = POINT;                                ///< Blinking state: middle point (colon), left two digits, right two digits
    uint8_t display_state = DIGITS_LEFT | POINT | DIGITS_RIGHT; ///< Display state: middle point (colon), left two digits, right two digits

    uint8_t alarm_off_counter; ///< Counter for Alarm off display message
    uint8_t alarm_counter;     ///< Counter for Alarm sound and display

public:
    // Constructor
    Clock();

    // Init function
    void init(TM1637 *display, uint8_t buzzer_pin);

    // Set time and alarm time
    void set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void set_alarm(uint8_t hours, uint8_t minutes);

    // Alarm functions
    void check_alarm();

    // Clock functions
    void show();
    void run();

    // TODO: Add other public variables/functions here
    void setup_timer();                // Attaches the class member timer to the interrupt service routine to run the interrupt every 0.5 seconds.
    void update_time();                // Increments the timestamp by 0.5 seconds for every call.
    void set_temp_time(int8_t offset); // When in the set menus (for the alarm and the clock), this function modifies the time on the display by an offset.
    void commit_temp_time();

    void handleButtonMenuPress();
    void handleButtonOkPress();
    void handleButtonPlusPress();
    void handleButtonMinusPress();
    void handleSwitchAlarmChange(bool alarm_pin);
};

extern Clock clk;
#endif