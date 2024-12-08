/// @file clock.cpp
/// Implementation of the Clock class.
///
/// This file contains the implementation to the clock class.
#include <Arduino.h>
#include "clock.h"
#include "stdio.h"

// Static function: Update time, show things on display
//                  and check alarm trigger
// static void update_time(void *clock)
// {
// }

/// @brief The interrupt service routine for the clock timer. This iterrupt is called every 0.5 seconds.
///
/// An explanation of how to use timer interrupts can be found in
/// [Arduino-ESP32 Timer API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html)
/// @return void
void ARDUINO_ISR_ATTR onTimer()
{
    clk.update_time();
    clk.check_alarm();
    clk.show();
}
//------------------------------------------------------------------------

/// @brief An empty Clock constructor.
Clock::Clock() {}

/// @brief Initialize internal variables,
///                set display to use and buzzer pin.
/// @param display The 7-segment display object, an instance of the TM1637 class.
/// @param buzzer_pin The buzzer output pin number.
void Clock::init(TM1637 *display, uint8_t buzzer_pin)
{
    this->display = display;
    this->alarm_tone = new AlarmTone();
    alarm_tone->init(buzzer_pin);
}

// Clock::set_time(): Set the time hour, minutes and seconds
//                    to internal binary representation

/// @brief Set the time hour, minutes and seconds
/// to internal binary representation.
///
/// The class member time variable is a `uint32_t` number which represents the hour|min|secs in a
/// binary format (17 bits):
///
/// 16 15 14 13 12 | 11  10  9  8  7  6 | 5  4  3  2  1  0
/// ---------------|--------------------|-----------------
///  H  H  H  H  H |  m  m   m  m  m  m | s  s  s  s  s  s
///
/// For example, the number: 76717 in binary:
/// ```
/// 1 0 0 1 0 | 1 0 1 1 1 0 | 1 0 1 1 0 1
/// ```
///
/// Means:
/// 1 0 0 1 0 -> 18 (hour)
/// 1 0 1 1 1 0 -> 46 (min)
/// 1 0 1 1 0 1 -> 45 (sec)
///
///
/// So this is 18:46:45
/// @param hours The hours (24 hour format).
/// @param minutes  Minutes
/// @param seconds  Seconds
void Clock::set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    time = 0x0000000 | hours << 12 | minutes << 6 | seconds;
    timestamp = (hours * 3600 + minutes * 60 + seconds) * 1000;
}

/// @brief Set the alarm hour, minutes and seconds.
///
/// See `set_time()` method.
/// @param hours Hours.
/// @param minutes Minutes.
void Clock::set_alarm(uint8_t hours, uint8_t minutes)
{
    this->alarm = 0x0000000 | hours << 12 | minutes << 6;
}

//
//
//

/// @brief A temporary variable to hold time adjusted by plus and minus buttons.The time isn't stored unless the OK button is pressed.
///        When in the set menus (for the alarm and the clock), this function modifies the time on the display by an offset.
/// @param offset An offset to increment the Clock::time variable with. Negative offset decrements the time.
void Clock::set_temp_time(int8_t offset)
{
    int8_t hours = temp_time >> 12;
    int8_t minutes = temp_time >> 6 & 0b00000111111;
    int8_t seconds = temp_time & 0b00000000000111111;

    if (set_digit == DIGITS_LEFT) // If the left digits (hours) are in focus
    {
        hours = (hours + 24 + offset) % 24; // Modify the hours variable, adding the offset value.
    }
    else // If the right digit is in focus,
    {
        minutes = (minutes + 60 + offset) % 60; // Modify the minutes variable, adding the offset value.
    }

    temp_time = 0x0000000 | hours << 12 | minutes << 6 | seconds; // Store the binary representation in the `temp_time` variable.
}

/// @brief Commit (store) the value of the temporary time variable in the final storage.
///
/// The final storage can be either the `time` or `alarm` varialbe.
/// Depends on the `time_to_set` class member (pointer variable).
void Clock::commit_temp_time()
{
    int8_t hours = temp_time >> 12;
    int8_t minutes = temp_time >> 6 & 0b00000111111;
    int8_t seconds = temp_time & 0b00000000000111111;
    if (time_to_set == &time) // If the `time_to_set` pointer points to the clock time
    {
        set_time(hours, minutes, seconds);
    }
    else // Else, if the time_to_set pointer points to the alarm time
    {
        set_alarm(hours, minutes);
    }
}

// -------------------- Handlers for Buttons and Switch Interrupt Service Routines --------------------

/// @brief Handles Menu button press.
void Clock::handleButtonMenuPress()
{
    display_state = DIGITS_LEFT | POINT | DIGITS_RIGHT;
    switch (state)
    {
    case STATE_CLOCK:
    case STATE_MENU_SET:
    case STATE_MENU_ALARM:
        state = (state + 1) % 3; // Cycle between the first three states, CLOCK, SET, and ALARM
        break;
    case STATE_SET_CLOCK:        // If the current state is SET_CLOCK
    case STATE_SET_ALARM:        // or SET_ALARM
        state = STATE_CLOCK;     // Pressing the menu button cancels the setting, returning back to the STATE_CLOCK
        set_digit = DIGITS_LEFT; // Put the focus back on the left digits of the display (hours).
        break;
    }
}

/// @brief Handles OK button press.
void Clock::handleButtonOkPress()
{
    display_state = DIGITS_LEFT | POINT | DIGITS_RIGHT; // The dipslay state will be 0b111. Displaying hours, minutes, and the middle colon
    switch (state)
    {
    case STATE_MENU_SET:         // If The current state is MENU_SET (Displaying "SET" on the 7-segment)
        temp_time = time;        // Copy the current `time` to `temp_time`
        time_to_set = &time;     // Set the `time_to_set` pointer to the `time`
        state = STATE_SET_CLOCK; // Move to SET_CLOCK state (Display the time setting). The hours will be blinking.
        break;
    case STATE_MENU_ALARM:    // If The current state is ALARM_SET (Displaying "AL" on the 7-segment)
        temp_time = alarm;    // Copy the current `alarm` time to `temp_time`
        time_to_set = &alarm; // Set the `time_to_set` pointer to the `alarm`
        if (alarm_enabled)    // If the alarm switch is on.
        {
            state = STATE_SET_ALARM; // Move to SET_ALARM state.
        }
        else // Else if the alarm switch is turned off
        {
            state = STATE_ALARM_OFF; // Move to the ALARM_OFF state, display "OFF" message to the user.
            alarm_off_counter = 6;   // Set the counter to keep the displayed message to 6 * 0.5 = 3 seconds
        }
        break;
    case STATE_SET_CLOCK:
    case STATE_SET_ALARM:
        if (set_digit == DIGITS_RIGHT) // If the right digits are in focus and the OK button is pressed.
        {
            state = STATE_CLOCK; // Move to the default state displaying the clock time.
            commit_temp_time();  // and commit the time, storing the temporary time variable that is being set in either time or alarm.
        }
        set_digit ^= DIGITS_LEFT | DIGITS_RIGHT; // toggle the state of digit from left (hours) to right (mintues) or vice versa.
        break;
    case STATE_ALARM:
        state = STATE_CLOCK; // If the alarm is triggered, pressing the OK button returns to the clock state.
        break;
    }
}

/// @brief Handles `+` button press.
void Clock::handleButtonPlusPress()
{
    set_temp_time(+1); // Increment the temporary time on the display
}

/// @brief Handles `-` button press.
void Clock::handleButtonMinusPress()
{
    set_temp_time(-1); // Decrement the temporary time on the display
}

/// @brief Enables or disables alarm.
///        Handles the alarm switch change.
void Clock::handleSwitchAlarmChange(bool alarm_pin)
{
    alarm_enabled = alarm_pin; // Set the `alarm_poin` variable to either true or false, depends on whether the alarm switch is on or off.
}

// -------------------- End Handlers for Buttons and Switch Interrupt Service Routines --------------------

/// @brief Show the time, alarm, or menu on display.
///
/// This function checks the current state stored in the class member
/// variable `state` and changes the 7-segment display accordingly.
/// The blinking is controlled by the `blink_state` variable.
/// For example:
/// If `blink_state = 0b100` (blinking the middle colon), and `display_state` = 0b111 (display all the objects hours, minutes, and middle colon),
/// a call to the function every 0.5 seconds will cycle the display state betwen `0b111` to `0b011`, creating the blinking effect.
///
/// The xor operation is used to toggle the display:
/// \f[
///     \mathrm{display\_state} = \mathrm{display\_state}  \oplus \mathrm{blink\_state}
/// \f]
void Clock::show()
{
    uint32_t *time_on_display = nullptr; // A pointer either to clock, alarm, or temporary setting time

    switch (state)
    {
    case STATE_CLOCK:
        blink_state = POINT;     // Blink only the midddle colon
        time_on_display = &time; // Display the time.
        break;
    case STATE_SET_CLOCK:
    case STATE_SET_ALARM:
        blink_state = set_digit;      // Blink only the digit in focus (blink either the hours or minutes)
        time_on_display = &temp_time; // Display the `temp time` variable.
        break;
    case STATE_ALARM: // Alarm triggered state
        // Serial.println("Alarm Counter:");
        // Serial.println(alarm_counter);
        time_on_display = &alarm;                         // Display the alarm time.
        blink_state = DIGITS_LEFT | POINT | DIGITS_RIGHT; // Blink everything on the display (hours, minutes, and the middle colon)
        alarm_tone->play();                               // Play the buzzer sound.
        if (not --alarm_counter)                          // Decrement and check the alarm counter. When the `alarm_counter` is 0, execute the following:
        {
            state = STATE_CLOCK;                                // Return back to the clock state.
            display_state = DIGITS_LEFT | POINT | DIGITS_RIGHT; // Display all objects.
            blink_state = POINT;                                // Blink only the middle colon.
        }
        break;
    }

    int8_t hours, minutes, seconds;
    int8_t data[4];    // An array of four digits to be sent to the display (two for hours and two for minutes)
    display->point(0); // Turn off the middle colon

    switch (state)
    {
    case STATE_MENU_SET:
        display->displayStr("SET", 0);
        break;
    case STATE_MENU_ALARM:
        display->displayStr("AL", 0);
        break;
    case STATE_ALARM_OFF:
        display->displayStr("OFF", 0);
        if (not --alarm_off_counter) // Decrement the counter, and check if it is zero execute the following block
        {
            state = STATE_CLOCK; // Return to the clock state.
        }
        break;
    case STATE_CLOCK:
    case STATE_ALARM:
    case STATE_SET_CLOCK:
    case STATE_SET_ALARM:
        display_state ^= blink_state; // The logic behind blinking the display. Every call to the function flips the objects being displayed.
        hours = *time_on_display >> 12;
        minutes = *time_on_display >> 6 & 0b00000111111;
        seconds = *time_on_display & 0b00000000000111111;

        // // For Debug
        // if (state == STATE_CLOCK)
        // {
        //     Serial.print(seconds);
        //     Serial.print(" ");
        // }
        // // -------------------

        if ((display_state & DIGITS_LEFT) >> 1) // If the display state contains the hours (left digit),
        {                                       // display the hours
            data[0] = hours / 10;               // Display the first digit of the hours.
            data[1] = hours % 10;               // Display the right digit of the hours.
        }
        else
        {
            data[0] = data[1] = 0x7f; // Turn off the hours digits in the 7-segment display
        }

        display->point((display_state & POINT) >> 2); // Check the state of the middle colon and display it accordingly

        if (display_state & DIGITS_RIGHT) // If the display state contains the minutes (right digit),
        {                                 // display the minutes
            data[2] = minutes / 10;       // Display the first digit of the minutes.
            data[3] = minutes % 10;       // Display the right digit of the minutes.
        }
        else
        {
            data[2] = data[3] = 0x7f; // Turn off the minutes digits in the 7-segment display
        }

        display->display(data); // Send the data array to the 7-segment display.
        break;
    }
}

/// @brief Check if alarm needs to be triggered.
///        Called by the ISR. If the current time equals the alarm time. It changes the state to `STATE_ALARM`
///        and sets the alarm down counter to 30 seconds.
///        also modifies the blinking state to blink both the left and rigth digits and the midddle colon.
void Clock::check_alarm()
{
    if (alarm_enabled && time == alarm)
    {
        state = STATE_ALARM;
        alarm_counter = 60; // Set the counter for 0.5 * 60 = 30 seconds
        display_state = DIGITS_LEFT | POINT | DIGITS_RIGHT;
    }
}

// VSCode with Platform IO Version

/// @brief Attaches the class member timer to the interrupt service routine to run the interrupt every 0.5 seconds.
///
/// Source: https://www.electronicwings.com/esp32/esp32-timer-interrupts
void Clock::setup_timer()
{

    // The next function returns a pointer to a structure of type hw_timer_t, which we
    // will define as the timer global variable.  We pass three values to this
    // function, the first one is the timer to use. ESP32 has 4 hardware timers. We
    // can put 0 to 3 values to use any which we need.  The second value is
    // “prescaler”, i.e. divider value to the clock frequency i.e. 80MHz. We have a
    // 16bit prescaler so we can set any value from 2 to 65536. Here we used ‘80’,
    // so the value after dividing will be 80MHz/80= 1MHz.
    timer = timerBegin(0, 80, true); // timer 0, prescalar: 80, UP counting

    // Attach onTimer function to our timer.
    timerAttachInterrupt(timer, &onTimer, true); // Attach interrupt

    // Set alarm to call onTimer function every half second (value in microseconds).
    timerAlarmWrite(timer, 500000, true); // Match value= 500000 for 0.5 sec. delay.
    timerAlarmEnable(timer);              // Enable Timer with interrupt (Alarm Enable)
}

// // WokWi Online Simulator

/// @brief Attaches the class member timer to the interrupt service routine to run the interrupt every 0.5 seconds.
///
/// Source: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
// void Clock::setup_timer()
// {
//     // Set timer frequency to 1Mhz
//     timer = timerBegin(1000000);

//     // Attach onTimer function to our timer.
//     timerAttachInterrupt(timer, &onTimer);

//     // Set alarm to call onTimer function every half second (value in microseconds).
//     // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
//     timerAlarm(timer, 500000, true, 0); // Match value= 500000 for 0.5 sec. delay.
// }

/// @brief  Increments the timestamp by 0.5 seconds on every call.
///
/// Adds 0.5 seconds (500 milliseconds). Resets the counter every day.
/// @f$\mathrm{timestamp\;} = (\mathrm{\;timestamp\;} + 500) \mathrm{\;mod\;} (24 \times 60 \times 60 \times 1000)@f$
void Clock::update_time()
{
    timestamp = (timestamp + 500) % (24 * 60 * 60 * 1000); // Add 0.5 seconds (500 milliseconds). Reset the counter after 1 day
                                                           // The timestamp variable is in milliseconds.

    uint8_t hour = timestamp / 3600000;
    uint8_t minutes = (timestamp % 3600000) / 60000;
    uint8_t seconds = (timestamp % 60000) / 1000;

    time = 0x0000000 | hour << 12 | minutes << 6 | seconds;
}

/// @brief Start running the clock
///               This function MUST not block, everything should be handled
///               by interrupts
void Clock::run()
{
    this->show();
    this->setup_timer();
}
