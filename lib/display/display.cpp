
#include "display.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <string.h>
#include <stdio.h>
#include "../../include/config.h"

// Set OLED_CONTROLLER_SH1106=0 if your display uses SSD1306.
#ifndef OLED_CONTROLLER_SH1106
#define OLED_CONTROLLER_SH1106 1
#endif

#if OLED_CONTROLLER_SH1106
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
    U8G2_R0, U8X8_PIN_NONE);
#else
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(
    U8G2_R0, U8X8_PIN_NONE);
#endif

namespace Display {

static bool ready = false;

// Hb API currently supplies values in g/dL.
// Convert to g/L for the OLED and measurement history.
static float toGL(float hbGdL) {
    return hbGdL * 10.0f;
}

bool begin() {
    oled.setI2CAddress(OLED_I2C_ADDRESS << 1);
    oled.begin();
    oled.setContrast(180);
    ready = true;
    return ready;
}

bool available() {
    return ready;
}

static void header(
    const char *timestamp,
    bool rtcAvailable,
    float soc
) {
    oled.setFont(u8g2_font_5x7_tf);

    if (rtcAvailable && timestamp &&
        strlen(timestamp) >= 16) {
        char date[11];
        char time[6];

        snprintf(date, sizeof(date), "%.10s", timestamp);
        snprintf(time, sizeof(time), "%.5s", timestamp + 11);

        oled.drawStr(0, 7, date);
        oled.drawStr(65, 7, time);
    } else {
        oled.drawStr(0, 7, "RTC unavailable");
    }

    char battery[12];
    snprintf(battery, sizeof(battery), "S:%02.0f%%", soc);
    oled.drawStr(101, 7, battery);

    oled.drawHLine(0, 10, 128);
}

void update(
    Mode::Screen screen,
    Mode::MeasureMode mode,
    const char *message,
    const char *timestamp,
    bool rtcAvailable,
    float hb,
    bool hbAvailable,
    const char *measurementStatus,
    uint8_t samplesPercent,
    uint8_t menuSelection,
    uint8_t settingsSelection
) {
    if (!ready) {
        return;
    }

    oled.clearBuffer();
    header(timestamp, rtcAvailable, Mode::soc());

    oled.setFont(u8g2_font_6x12_tf);

    switch (screen) {

        // -------------------------------------------------
        // HOME MENU
        // -------------------------------------------------
        case Mode::Screen::HOME: {
            const char *items[] = {
                "Continuous mode",
                "Single measurement",
                "Logs",
                "Settings"
            };

            char line[28];

            for (uint8_t i = 0; i < 4; i++) {
                snprintf(
                    line,
                    sizeof(line),
                    "%s%s",
                    menuSelection == i ? ">" : " ",
                    items[i]
                );

                oled.drawStr(0, 22 + i * 11, line);
            }
            break;
        }

        // -------------------------------------------------
        // MEASUREMENT SCREEN
        // -------------------------------------------------
        case Mode::Screen::MEASURE: {
            oled.drawStr(
                0, 23,
                mode == Mode::MeasureMode::SINGLE
                    ? "SINGLE MEASUREMENT"
                    : "CONTINUOUS MODE"
            );

            if (hbAvailable) {
                oled.setFont(u8g2_font_7x14B_tf);

                char value[24];
                snprintf(
                    value,
                    sizeof(value),
                    "Hb: %.1f g/L",
                    toGL(hb)
                );

                oled.drawStr(0, 40, value);
                oled.setFont(u8g2_font_5x7_tf);
                oled.drawStr(0, 51, "Estimated Hb result");

            } else {
                oled.drawStr(0, 35, "Keep finger still");

                char progress[24];
                snprintf(
                    progress,
                    sizeof(progress),
                    "Signal window: %u%%",
                    samplesPercent
                );

                oled.drawStr(0, 48, progress);
            }

            oled.setFont(u8g2_font_5x7_tf);

            if (measurementStatus) {
                oled.drawStr(0, 62, measurementStatus);
            } else {
                oled.drawStr(0, 62, "B1: back  B2: mode");
            }
            break;
        }

        // -------------------------------------------------
        // SAVED MEASUREMENT LOGS
        // -------------------------------------------------
        case Mode::Screen::LOGS: {
            oled.drawStr(0, 22, "SAVED MEASUREMENTS");

            Storage::Record rec;

            if (Storage::read(Mode::logIndex(), rec)) {
                oled.drawStr(0, 35, rec.timestamp);

                char value[24];
                snprintf(
                    value,
                    sizeof(value),
                    "Hb: %.1f g/L",
                    toGL(rec.hb)
                );

                oled.drawStr(0, 48, value);

                char index[24];
                snprintf(
                    index,
                    sizeof(index),
                    "%u/%u  B2: next",
                    (unsigned)(Mode::logIndex() + 1),
                    (unsigned)Storage::count()
                );

                oled.drawStr(0, 62, index);
            } else {
                oled.drawStr(0, 42, "No saved readings");
                oled.drawStr(0, 60, "B1: back");
            }
            break;
        }

        // -------------------------------------------------
        // SETTINGS / BATTERY
        // -------------------------------------------------
        case Mode::Screen::SETTINGS: {
            oled.drawStr(0, 23, "SETTINGS / BATTERY");

            char value[28];

            snprintf(
                value, sizeof(value),
                "%cSOC: %.0f%% demo",
                settingsSelection == 0 ? '>' : ' ',
                Mode::soc()
            );
            oled.drawStr(0, 36, value);

            snprintf(
                value, sizeof(value),
                "%cSOH: %.0f%% demo",
                settingsSelection == 1 ? '>' : ' ',
                Mode::soh()
            );
            oled.drawStr(0, 47, value);

            snprintf(
                value, sizeof(value),
                "%cClear logs",
                settingsSelection == 2 ? '>' : ' '
            );
            oled.drawStr(0, 58, value);
            break;
        }

        // -------------------------------------------------
        // CONFIRM CLEARING HISTORY
        // -------------------------------------------------
        case Mode::Screen::CLEAR_CONFIRM: {
            oled.drawStr(0, 25, "CLEAR ALL LOGS?");

            oled.drawStr(
                0, 40,
                Mode::clearCancelSelected()
                    ? "> NO / cancel"
                    : "> YES / clear"
            );

            oled.drawStr(0, 54, "B1: confirm B2: toggle");
            break;
        }

        // -------------------------------------------------
        // STATUS / ERROR MESSAGE
        // -------------------------------------------------
        case Mode::Screen::MESSAGE: {
            oled.setFont(u8g2_font_7x14B_tf);
            oled.drawStr(0, 29, "STATUS");

            oled.setFont(u8g2_font_6x12_tf);

            if (message) {
                // Keep messages within the 128x64 display.
                oled.drawUTF8(0, 47, message);
            }

            oled.setFont(u8g2_font_5x7_tf);
            oled.drawStr(0, 62, "B1: return");
            break;
        }
    }

    oled.sendBuffer();
}

} // namespace Display
