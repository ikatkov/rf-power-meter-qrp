#include "FmDisplay.h"
#include <TimerOne.h>
#include <OneButton.h>
#include <EEPROM.h>
#include "main.h"

#define DEBUG

const int CS_PIN = 11;
const int CLOCK_PIN = 12;
const int DATA_PIN = 10;

const int BUTTON_MENU_PIN = 8;
const int BUTTON_OK_PIN = 9;

const int POWER_LATCH_PIN = 7;

const int BUZZER_PIN = A3;
const int RF_PEAK_PIN = A2;
const int BATTERY_PIN = A0;

const float rf_voltage_divider_r1 = 11.0;     // 11k resistor
const float rf_voltage_divider_r2 = 2.5;      // 2.5k
const float REFERENCE_VOLTAGE = 4.762;        // measured reference voltage
const float DIODE_VOLTAGE_DROP = 0.42;        // TODO: apply curve fitting to get the actual value
const float ADC_CORRECTION = 0.1988 / 0.1930; // measured real voltage on ADC

const int TEXT_DBM[] = {_space, _d, _b, _space};
const int TEXT_PO[] = {_U, _a, _t, _t};
const int TEXT_FREQUENCY[] = {_F, _r, _E, _q};
const int TEXT_HOLD[] = {_H, _o, _1, _d};
const int TEXT_CONT[] = {_C, _o, _n, _t};
const int TEXT_BATTTERY[] = {_b, _a, _t, _t};
const int TEXT_OVERLOAD[] = {_8, _8, _8, _8};

const int TEXT_SAVE_ANIMATION1[] = {_space, segment_g, _space, _space};
const int TEXT_SAVE_ANIMATION2[] = {_space, segment_c, _space, _space};
const int TEXT_SAVE_ANIMATION3[] = {_space, segment_d, _space, _space};
const int TEXT_SAVE_ANIMATION4[] = {_space, segment_e, _space, _space};

const int TEXT_ZEROING_ANIMATION1[] = {_0, _dash, _dash, _dash};
const int TEXT_ZEROING_ANIMATION2[] = {_0, _0, _dash, _dash};
const int TEXT_ZEROING_ANIMATION3[] = {_0, _0, _0, _dash};
const int TEXT_ZEROING_ANIMATION4[] = {_0, _0, _0, _0};
const int TEXT_ZEROING_ANIMATION5[] = {_space, _0, _0, _0};
const int TEXT_ZEROING_ANIMATION6[] = {_space, _space, _0, _0};
const int TEXT_ZEROING_ANIMATION7[] = {_space, _space, _space, _0};

const int TEXT_BLANK[4] = {_space, _space, _space, _space};

const unsigned long COUNTDOWN_DELAY = 300000; // 5 minutes
const unsigned long SHUTDOWN_DELAY = 320000;  // 5 minutes 20 seconds

volatile bool displayOn = true;
volatile double rf_power_mw = 0;
volatile double rf_power_dbm = 0;

int display_measurement_number = 0;
uint16_t rf_adc_value = 0;
uint16_t old_rf_adc_value = 0;

unsigned long button_press_time;
bool turnoff_countdown;

enum MEASUREMENT_UNITS
{
    W,
    DBM
};
enum MEASUREMENT_MODE
{
    CONTINIOUS,
    HOLD_PEAK
};

// 3.7MHz, 7.2MHz, 14.2MHz, 18MHz, 21MHz, 24MHz, 28MHz, 50MHz
const uint16_t available_frequency_bands[8] = {37, 72, 142, 180, 211, 249, 280, 500};
uint8_t frequency_band = 1; // 7.2MHz

const double fitting_function_coefficient[8][6] = {
    {2.0245749998638694e-15, -2.3699472615230126e-11, 9.070791422764846e-08, -0.00012760316726548302, 1.142113933556968, -8.307120539266869},   // 3MHz
    {-2.832639856233634e-15, 4.008791152785013e-11, -1.9347494506898958e-07, 0.0003762711505537496, 0.8649979956416018, 5.7510956015560994},    // 7.2MHz
    {-7.810853772218834e-16, 1.1557752449794416e-11, -5.7078681287238685e-08, 0.00011412767037503853, 1.0842364385975602, -0.4732162724031951}, // 14.2MHz
    {7.630398035762764e-15, -8.768622813420562e-11, 3.289916470982582e-07, -0.0004330479624294447, 1.3214604701574173, -7.765773398063638},     // 18MHz
    {-9.175671545215191e-15, 1.0720650694796922e-10, -4.32168272385996e-07, 0.0007161423096984246, 0.8881486345114411, 9.449364406775501},      // 21MHz
    {-4.63099664245735e-15, 5.860488098036351e-11, -2.638794351628782e-07, 0.0004933640336449699, 0.9393501818574546, 6.66894491307017},        // 24MHz
    {-9.804873517894107e-15, 1.173642971202226e-10, -4.897181239269411e-07, 0.0008448732362330482, 0.8542679565355191, 10.664231398894488},     // 28MHz
    {4.1232168516747e-14, -4.1479653503906627e-10, 1.350228380965077e-06, -0.0015175248806550679, 1.8014116551731685, -28.51094013300302}       // 50MHz
};

enum STATE
{
    MAIN,
    MEASUREMENT_MODE_SELECT,
    FREQUENCY_SELECT
};

STATE state = STATE::MAIN;

MEASUREMENT_UNITS measurement_units = MEASUREMENT_UNITS::W;
MEASUREMENT_MODE measurement_mode = MEASUREMENT_MODE::CONTINIOUS;

FmDisplay fmDisplay = FmDisplay(CLOCK_PIN, DATA_PIN, CS_PIN);

OneButton button_OK(BUTTON_OK_PIN, true);
OneButton button_MENU(BUTTON_MENU_PIN, true);

// Function to compute the correction
double compute_correction(const double measured, const double coefficients[6])
{
    return coefficients[0] * pow(measured, 5) +
           coefficients[1] * pow(measured, 4) +
           coefficients[2] * pow(measured, 3) +
           coefficients[3] * pow(measured, 2) +
           coefficients[4] * measured +
           coefficients[5];
}

void render_display();

void overload_tone()
{
    tone(BUZZER_PIN, 1700, 1000);
}

void startup_tone()
{
    tone(BUZZER_PIN, 1000, 200);
    delay(200);
    tone(BUZZER_PIN, 1500, 600);
}

void shutdown_tone()
{
    tone(BUZZER_PIN, 1500, 200);
    delay(200);
    tone(BUZZER_PIN, 1000, 600);
    delay(600);
}

void button_press_tone()
{
    tone(BUZZER_PIN, 1200, 200);
}

void int_to_symbols(uint16_t number, int digits[4], const bool leading_spaces = measurement_units == MEASUREMENT_UNITS::DBM)
{
    digits[0] = number / 1000; // Thousands place
    number %= 1000;            // Remaining number after thousands

    digits[1] = number / 100; // Hundreds place
    number %= 100;            // Remaining number after hundreds

    digits[2] = number / 10; // Tens place
    digits[3] = number % 10; // Ones place
    for (byte i = 0; i < 3; i++)
    {
        if (digits[i] == 0 && leading_spaces)
            digits[i] = 10;
        else
            break;
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        digits[i] = symbols[digits[i]];
    }
}

void display_slideout_animation()
{
    int symbols[4];
    int_to_symbols(display_measurement_number, symbols);
    for (int i = 3; i >= 0; i--)
    {
        fmDisplay.display_symbols(symbols);
        delay(100);
        // move the digits to the left
        for (int j = 0; j < 3; j++)
        {
            symbols[j] = symbols[j + 1];
            if (j >= i)
            {
                symbols[j] = _space;
            }
        }
        symbols[3] = _space;
    }
}

void debug_print_symbols(const int symbols[4], const int display_text[4])
{
    Serial.print("Symbols: [");
    for (int k = 0; k < 4; k++)
    {
        Serial.print(symbols[k]);
        Serial.print(" ");
    }
    Serial.print("]\t[");
    for (int k = 0; k < 4; k++)
    {
        Serial.print(display_text[k]);
        Serial.print(" ");
    }
    Serial.println("]");
}

void display_scroll_animation(const int *scroll_text, int length)
{
    int symbols[4] = {_space, _space, _space, _space};

    for (int i = 0; i < length - 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i + j < length)
            {
                symbols[j] = scroll_text[i + j];
            }
            else
            {
                symbols[j] = _space;
            }
        }
        fmDisplay.display_symbols(symbols);
        delay(100);
    }
}

void display_save_animation()
{
    Timer1.detachInterrupt();
    fmDisplay.display_show_dot(false);

    const int *animations[] = {TEXT_SAVE_ANIMATION1, TEXT_SAVE_ANIMATION2, TEXT_SAVE_ANIMATION3, TEXT_SAVE_ANIMATION4};
    for (int i = 0; i < 8; ++i)
    {
        fmDisplay.display_symbols(animations[i % 4]);
        delay(100);
    }

    Timer1.attachInterrupt(render_display);
}

void display_zeroing_animation()
{
    Timer1.detachInterrupt();
    fmDisplay.display_show_dot(false);

    const int *animations[] = {TEXT_ZEROING_ANIMATION1, TEXT_ZEROING_ANIMATION2, TEXT_ZEROING_ANIMATION3, TEXT_ZEROING_ANIMATION4, TEXT_ZEROING_ANIMATION5, TEXT_ZEROING_ANIMATION6, TEXT_ZEROING_ANIMATION7};
    for (int i = 0; i < 7; ++i)
    {
        fmDisplay.display_symbols(animations[i]);
        delay(100);
    }

    Timer1.attachInterrupt(render_display);
}

void display_battery_voltage()
{
    int batteryVoltage = analogRead(BATTERY_PIN);
    Serial.print("Battery voltage ADC: ");
    Serial.print(batteryVoltage);
    float voltage = float(batteryVoltage + 0.5) * REFERENCE_VOLTAGE / 1024;
    Serial.print("\tBattery voltage: ");
    Serial.println(voltage);

    int symbols[4];
    int text_to_scroll[16];

    int_to_symbols(voltage * 1000, symbols);
    memcpy(text_to_scroll, TEXT_BLANK, sizeof(TEXT_BLANK));
    memcpy(text_to_scroll + 4, TEXT_BATTTERY, sizeof(TEXT_BATTTERY));
    memcpy(text_to_scroll + 8, TEXT_BLANK, sizeof(TEXT_BLANK));
    memcpy(text_to_scroll + 12, symbols, sizeof(symbols));
    display_scroll_animation(text_to_scroll, sizeof(text_to_scroll) / sizeof(text_to_scroll[0]));
}

void display_frequency_band()
{
    fmDisplay.display_show_dot(true);
    fmDisplay.display_int(available_frequency_bands[frequency_band], true);
}

void read_rf_power()
{
    rf_adc_value = analogRead(RF_PEAK_PIN);
    if (measurement_mode == MEASUREMENT_MODE::HOLD_PEAK && rf_adc_value < old_rf_adc_value)
    {
        return;
    }
    old_rf_adc_value = rf_adc_value;

    if (rf_adc_value == 0)
    {
        rf_power_dbm = 0;
        rf_power_mw = 0;
    }
    else
    {
#ifdef DEBUG
        Serial.print("RF power ADC: ");
        Serial.print(rf_adc_value);
#endif

        double rf_voltage = ADC_CORRECTION * rf_adc_value * REFERENCE_VOLTAGE / 1023.0;
#ifdef DEBUG
        // Serial.print("\t RF power @ADC, V: ");
        // Serial.print(rf_voltage, 4);
#endif
        rf_voltage = rf_voltage * (rf_voltage_divider_r1 + rf_voltage_divider_r2) / rf_voltage_divider_r2;
#ifdef DEBUG
        // Serial.print("\t RF power before divider, V: ");
        // Serial.print(rf_voltage, 4);
#endif

        rf_voltage = DIODE_VOLTAGE_DROP + rf_voltage;
#ifdef DEBUG
        Serial.print("\t RF power, Vp: ");
        Serial.print(rf_voltage, 4);
        Serial.print("\t RF power, Vpp: ");
        Serial.print(2 * rf_voltage, 4);
#endif

        double rf_voltage_rms = rf_voltage / sqrt(2);
#ifdef DEBUG
        Serial.print("\t RF power, Vrms: ");
        Serial.print(rf_voltage_rms, 4);
#endif
        rf_power_mw = rf_voltage_rms * rf_voltage_rms * 100.0 / 5;

#ifdef DEBUG
        Serial.print("\t RF power, mW: ");
        Serial.print(rf_power_mw, 4);
#endif

        rf_power_mw = compute_correction(rf_power_mw, fitting_function_coefficient[frequency_band]);

#ifdef DEBUG
        Serial.print("\t Corrected RF power, mW: ");
        Serial.print(rf_power_mw, 4);
#endif

        rf_power_dbm = 10 * log10(rf_power_mw) * 10;
#ifdef DEBUG
        Serial.print("\t RF power dBm: ");
        Serial.print(rf_power_dbm, 4);
        Serial.println("");
#endif
    }
    if (measurement_units == MEASUREMENT_UNITS::DBM)
    {
        display_measurement_number = round(rf_power_dbm);
    }
    else if (measurement_units == MEASUREMENT_UNITS::W)
    {
        display_measurement_number = round(rf_power_mw);
    }
    if (rf_adc_value >= 1022)
    {
        overload_tone();
    }
}

void render_display()
{
    // Serial.println("render_display, state:");

    // switch (state)
    // {
    // case STATE::MAIN:
    //     Serial.println("MAIN");
    //     break;
    // case STATE::MEASUREMENT_MODE_SELECT:
    //     Serial.println("MEASUREMENT_MODE_SELECT");
    //     break;
    // case STATE::FREQUENCY_SELECT:
    //     Serial.println("FREQUENCY_SELECT");
    //     break;
    // default:
    //     Serial.println("UNKNOWN");
    //     break;
    // }

    if (state == STATE::MAIN)
    {
        fmDisplay.display_show_dot(false);
        if (measurement_mode == MEASUREMENT_MODE::HOLD_PEAK)
        {
            if (displayOn)
            {
                fmDisplay.clear();
                digitalWrite(LED_BUILTIN, LOW);
            }
            else
            {
                fmDisplay.display_show_dot(false);
                fmDisplay.display_int(display_measurement_number, measurement_units == MEASUREMENT_UNITS::DBM);
                digitalWrite(LED_BUILTIN, HIGH);
            }
            displayOn = !displayOn;
        }
        else if (measurement_mode == MEASUREMENT_MODE::CONTINIOUS)
        {
            fmDisplay.display_int(display_measurement_number, measurement_units == MEASUREMENT_UNITS::DBM);
        }
    }
    else if (state == STATE::FREQUENCY_SELECT)
    {
        display_frequency_band();
    }
    else if (state == STATE::MEASUREMENT_MODE_SELECT)
    {
        if (measurement_mode == MEASUREMENT_MODE::HOLD_PEAK)
        {
            fmDisplay.display_symbols(TEXT_HOLD);
        }
        else if (measurement_mode == MEASUREMENT_MODE::CONTINIOUS)
        {
            fmDisplay.display_symbols(TEXT_CONT);
        }
    }
}

void handle_ok_button_click()
{
    Serial.println("handle_ok_button_click");
    button_press_time = millis();
    turnoff_countdown = false;
    if (state == STATE::MAIN)
    {
        display_zeroing_animation();
        rf_power_dbm = 0;
        rf_power_mw = 0;
        rf_adc_value = 0;
        old_rf_adc_value = 0;
    }

    if (state == STATE::MEASUREMENT_MODE_SELECT)
    {
        state = STATE::MAIN;
    }
    else if (state == STATE::FREQUENCY_SELECT)
    {
        fmDisplay.display_show_dot(false);
        state = STATE::MAIN;
        display_save_animation();
        EEPROM.write(0, frequency_band);
    }
}

void handle_menu_button_click()
{
    Serial.println("handle_menu_button_click");
    button_press_time = millis();
    turnoff_countdown = false;

    int symbols[4];
    int text_to_scroll[20];
    if (state == STATE::MAIN)
    {
        Timer1.detachInterrupt();
        if (measurement_units == MEASUREMENT_UNITS::W)
        {
            measurement_units = MEASUREMENT_UNITS::DBM;
            int_to_symbols(display_measurement_number, symbols, false);
            memcpy(text_to_scroll, symbols, sizeof(symbols));
            memcpy(text_to_scroll + 4, TEXT_BLANK, sizeof(TEXT_BLANK));
            memcpy(text_to_scroll + 8, TEXT_DBM, sizeof(TEXT_DBM));
            memcpy(text_to_scroll + 12, TEXT_BLANK, sizeof(TEXT_BLANK));
            int_to_symbols(round(rf_power_dbm), symbols);
            memcpy(text_to_scroll + 16, symbols, sizeof(symbols));
            display_scroll_animation(text_to_scroll, sizeof(text_to_scroll) / sizeof(text_to_scroll[0]));
            delay(500);
            display_measurement_number = round(rf_power_dbm);
        }
        else if (measurement_units == MEASUREMENT_UNITS::DBM)
        {
            measurement_units = MEASUREMENT_UNITS::W;

            int_to_symbols(display_measurement_number, symbols, true);
            memcpy(text_to_scroll, symbols, sizeof(symbols));
            memcpy(text_to_scroll + 4, TEXT_BLANK, sizeof(TEXT_BLANK));
            memcpy(text_to_scroll + 8, TEXT_PO, sizeof(TEXT_PO));
            memcpy(text_to_scroll + 12, TEXT_BLANK, sizeof(TEXT_BLANK));
            int_to_symbols(round(rf_power_mw), symbols);
            memcpy(text_to_scroll + 16, symbols, sizeof(symbols));
            display_scroll_animation(text_to_scroll, sizeof(text_to_scroll) / sizeof(text_to_scroll[0]));
            delay(500);
            display_measurement_number = round(rf_power_mw);
        }
        Timer1.attachInterrupt(render_display);
    }
    else if (state == STATE::MEASUREMENT_MODE_SELECT)
    {
        if (measurement_mode == MEASUREMENT_MODE::HOLD_PEAK)
        {
            measurement_mode = MEASUREMENT_MODE::CONTINIOUS;
        }
        else if (measurement_mode == MEASUREMENT_MODE::CONTINIOUS)
        {
            measurement_mode = MEASUREMENT_MODE::HOLD_PEAK;
        }
    }
    else if (state == STATE::FREQUENCY_SELECT)
    {
        frequency_band++;
        if (frequency_band >= sizeof(available_frequency_bands) / sizeof(available_frequency_bands[0]))
        {
            frequency_band = 0;
        }
        display_frequency_band();
    }
}

void handle_menu_button_doubleclick()
{
    Serial.println("handle_menu_button_doubleclick");
    button_press_time = millis();
    turnoff_countdown = false;

    int symbols[4];
    int text_to_scroll[12];

    if (state == STATE::MAIN)
    {
        Timer1.detachInterrupt();
        state = STATE::MEASUREMENT_MODE_SELECT;

        int_to_symbols(display_measurement_number, symbols);
        memcpy(text_to_scroll, symbols, sizeof(symbols));
        memcpy(text_to_scroll + 4, TEXT_BLANK, sizeof(TEXT_BLANK));

        if (measurement_mode == MEASUREMENT_MODE::HOLD_PEAK)
        {
            memcpy(text_to_scroll + 8, TEXT_HOLD, sizeof(TEXT_HOLD));
        }
        else if (measurement_mode == MEASUREMENT_MODE::CONTINIOUS)
        {
            memcpy(text_to_scroll + 8, TEXT_CONT, sizeof(TEXT_CONT));
        }
        display_scroll_animation(text_to_scroll, sizeof(text_to_scroll) / sizeof(text_to_scroll[0]));

        Timer1.attachInterrupt(render_display);
    }
}

void handle_menu_button_longpress()
{
    Serial.println("handle_menu_button_longpress");
    button_press_time = millis();
    turnoff_countdown = false;

    int symbols[4];
    int text_to_scroll[20];

    if (state == STATE::MAIN)
    {
        Timer1.detachInterrupt();
        state = STATE::FREQUENCY_SELECT;

        int_to_symbols(display_measurement_number, symbols);
        memcpy(text_to_scroll, symbols, sizeof(symbols));
        memcpy(text_to_scroll + 4, TEXT_BLANK, sizeof(TEXT_BLANK));
        memcpy(text_to_scroll + 8, TEXT_FREQUENCY, sizeof(TEXT_FREQUENCY));
        memcpy(text_to_scroll + 12, TEXT_BLANK, sizeof(TEXT_BLANK));
        int_to_symbols(available_frequency_bands[frequency_band], symbols, true);
        memcpy(text_to_scroll + 16, symbols, sizeof(symbols));
        display_scroll_animation(text_to_scroll, sizeof(text_to_scroll) / sizeof(text_to_scroll[0]));

        Timer1.attachInterrupt(render_display);
    }
}

void handle_ok_button_longpress()
{
    Serial.println("handle_ok_button_longpress");
    shutdown_tone();
    // shut down
    digitalWrite(POWER_LATCH_PIN, LOW);
}

void setup()
{
    // Launch the power supply
    pinMode(POWER_LATCH_PIN, OUTPUT);
    digitalWrite(POWER_LATCH_PIN, HIGH);

    pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);
    pinMode(BUTTON_OK_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    pinMode(RF_PEAK_PIN, INPUT);

    analogReference(EXTERNAL);

    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(57600);
    fmDisplay.start();
    fmDisplay.clear();

    startup_tone();
    display_battery_voltage();
    delay(1000);

    // Read frequency_band from EEPROM
    frequency_band = EEPROM.read(0);
    if (frequency_band >= sizeof(available_frequency_bands) / sizeof(available_frequency_bands[0]))
    {
        frequency_band = 1; // Default to 7.2MHz if the value is out of range
        EEPROM.write(0, frequency_band);
    }
    display_frequency_band();
    delay(1000);

    fmDisplay.display_show_dot(false);

    button_OK.setLongPressIntervalMs(2000);
    button_OK.attachPress(button_press_tone);
    button_OK.attachClick(handle_ok_button_click);
    button_OK.attachLongPressStart(handle_ok_button_longpress);

    button_MENU.setLongPressIntervalMs(2000);
    button_MENU.attachPress(button_press_tone);
    button_MENU.attachClick(handle_menu_button_click);
    button_MENU.attachDoubleClick(handle_menu_button_doubleclick);
    button_MENU.attachLongPressStart(handle_menu_button_longpress);

    Timer1.initialize(400000);              // Initialize timer to 500ms
    Timer1.attachInterrupt(render_display); // Attach the interrupt service routine
}

void loop()
{
    read_rf_power();
    button_OK.tick();
    button_MENU.tick();

    if (!turnoff_countdown && (millis() - button_press_time) > COUNTDOWN_DELAY)
    {
        Serial.println("Shutdown count down");
        tone(BUZZER_PIN, 2000, 300);
        delay(500);
        tone(BUZZER_PIN, 2000, 300);
        delay(500);
        tone(BUZZER_PIN, 2000, 300);
        delay(500);
        turnoff_countdown = true;
    }

    if (turnoff_countdown && (millis() - button_press_time) > SHUTDOWN_DELAY)
    {
        Serial.println("Shutting down after inactivity");
        shutdown_tone();
        // shut down
        digitalWrite(POWER_LATCH_PIN, LOW);
        delay(1000);
    }

    delay(10);
}
