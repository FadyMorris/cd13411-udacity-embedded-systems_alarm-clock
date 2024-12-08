#include "clock.h"

// Hardware pins for buttons, alarm switch and buzzer pin
// For devkit v4
#define MENU_PIN 16
#define PLUS_PIN 4
#define MINUS_PIN 2
#define OK_PIN 0

#define ALARM_PIN 15
#define BUZZER_PIN 12

TM1637 display(5, 18);
Clock clk;

// Interrupt Service Routines for buttons
void IRAM_ATTR buttonMenuInterrupt()
{
    clk.handleButtonMenuPress();
}

void IRAM_ATTR buttonOkInterrupt()
{
    clk.handleButtonOkPress();
}

void IRAM_ATTR buttonPlusInterrupt()
{
    clk.handleButtonPlusPress();
}

void IRAM_ATTR buttonMinusInterrupt()
{
    clk.handleButtonMinusPress();
}

// Interrupt Service Routine for the Alarm Switch
void IRAM_ATTR switchAlarmInterrupt()
{
    clk.handleSwitchAlarmChange(digitalRead(ALARM_PIN));
}

void setup()
{

    // Initiate the serial console
    Serial.begin(115200);
    // Configure buttons as inputs with pull-up
    pinMode(MENU_PIN, INPUT_PULLUP);
    pinMode(PLUS_PIN, INPUT_PULLUP);
    pinMode(MINUS_PIN, INPUT_PULLUP);
    pinMode(OK_PIN, INPUT_PULLUP);

    pinMode(ALARM_PIN, INPUT_PULLUP); // Alarm switch

    // Attach interrupt for the buttons
    attachInterrupt(digitalPinToInterrupt(MENU_PIN), buttonMenuInterrupt, FALLING); // Call the four buttons ISRs
    attachInterrupt(digitalPinToInterrupt(OK_PIN), buttonOkInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(PLUS_PIN), buttonPlusInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(MINUS_PIN), buttonMinusInterrupt, FALLING);

    attachInterrupt(digitalPinToInterrupt(ALARM_PIN), switchAlarmInterrupt, CHANGE); // Call the alarm switch ISR

    display.init();
    display.set(BRIGHT_TYPICAL);

    // Clock class init
    clk.init(&display, BUZZER_PIN);
    clk.handleSwitchAlarmChange(digitalRead(ALARM_PIN)); // Read the alarm switch pin and update the clock
    /* Uncomment the following lines to set the time
       and alarm for testing, it will set it to 23:02:55
       with alarm at 23:03. Remember to enable the alarm
       using the slide switch
    */
    clk.set_time(18, 56, 55);
    clk.set_alarm(18, 57);
    // clk.set_time(23, 02, 55);
    // clk.set_alarm(23, 03);

    // Start the clock
    clk.run();
}

void loop()
{
    // Delay to help with simulation running
    delay(100);
}
