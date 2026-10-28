#include "display.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "../../include/config.h"

#ifndef OLED_CONTROLLER_SH1106
#define OLED_CONTROLLER_SH1106 1
#endif
#if OLED_CONTROLLER_SH1106
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
#else
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
#endif

namespace Display {
static bool ready = false;

bool begin() {
    oled.setI2CAddress(OLED_I2C_ADDRESS << 1);
    ready = oled.begin();
    oled.setContrast(200);
    return ready;
}
bool available() { return ready; }

// Format the float without printf's floating-point support. The scaled integer
// is rounded first, then split into whole and fractional parts.
static void formatHb(char *out, size_t outSize, float hb, bool withUnit = true) {
    if (!out || outSize == 0) return;
    if (!(hb == hb) || hb > 999.0f || hb < -99.0f) {
        snprintf(out, outSize, "--.--%s", withUnit ? " g/dL" : "");
        return;
    }
    const bool negative = hb < 0.0f;
    float absolute = negative ? -hb : hb;
    int32_t scaled = (int32_t)(absolute * 100.0f + 0.5f);
    int32_t whole = scaled / 100;
    int32_t fraction = scaled % 100;
    snprintf(out, outSize, "%s%ld.%02ld%s", negative ? "-" : "",
             (long)whole, (long)fraction, withUnit ? " g/dL" : "");
}

static void drawHeader(const char *timestamp, bool rtcAvailable) {
    oled.setDrawColor(1);
    oled.setFont(u8g2_font_5x7_tf);
    if (rtcAvailable && timestamp && strlen(timestamp) >= 16) {
        // Keep the header readable and bounded on a 128 px display.
        char header[22];
        snprintf(header, sizeof(header), "%.19s", timestamp);
        oled.drawStr(0, 7, header);
    } else {
        oled.drawStr(0, 7, "HB MONITOR");
    }
    oled.drawHLine(0, 10, 128);
}

static void drawMenuItem(uint8_t y, const char *label, bool selected) {
    if (selected) {
        oled.drawBox(0, y - 10, 128, 12);
        oled.setDrawColor(0);
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(6, y, label);
        oled.setDrawColor(1);
    } else {
        oled.setFont(u8g2_font_6x12_tf);
        oled.drawStr(6, y, label);
    }
}

static void shortDateTime(const char *timestamp, char *out, size_t outSize) {
    if (!out || outSize == 0) return;
    out[0] = '\0';
    if (!timestamp || !timestamp[0]) {
        snprintf(out, outSize, "No timestamp");
        return;
    }
    // Expected RTC format: YYYY-MM-DD HH:MM:SS. Also tolerate other strings.
    if (strlen(timestamp) >= 16 && timestamp[4] == '-' && timestamp[7] == '-') {
        snprintf(out, outSize, "%c%c/%c%c %c%c:%c%c",
                 timestamp[5], timestamp[6], timestamp[8], timestamp[9],
                 timestamp[11], timestamp[12], timestamp[14], timestamp[15]);
    } else {
        snprintf(out, outSize, "%.15s", timestamp);
    }
}

static void drawLogList() {
    const size_t n = Storage::count();
    oled.setFont(u8g2_font_6x13_tf);;
    oled.drawStr(0, 23, "SAVED READINGS");
    oled.setFont(u8g2_font_5x7_tf);
    if (n == 0) {
        oled.drawStr(0, 40, "No saved readings yet");
        oled.drawStr(0, 62, "B1: back");
        return;
    }

    const size_t selected = Mode::logIndex() < n ? Mode::logIndex() : 0;
    // Show up to four compact rows. The selected record is kept in view.
    size_t first = selected > 3 ? selected - 3 : 0;
    if (first + 4 > n && n > 4) first = n - 4;
    const size_t end = (first + 4 < n) ? first + 4 : n;
    uint8_t y = 33;
    for (size_t i = first; i < end; ++i, y += 9) {
        Storage::Record rec;
        if (!Storage::read(i, rec)) continue;
        char dt[16], hb[20], line[32];
        shortDateTime(rec.timestamp, dt, sizeof(dt));
        formatHb(hb, sizeof(hb), rec.hb, false);
        // Fixed, compact single-line row: selection marker, index, date/time, Hb.
        snprintf(line, sizeof(line), "%c%02u %s %s", i == selected ? '>' : ' ',
                 (unsigned)(i + 1), dt, hb);
        if (i == selected) {
            oled.drawBox(0, y - 7, 128, 9);
            oled.setDrawColor(0);
            oled.drawStr(1, y, line);
            oled.setDrawColor(1);
        } else {
            oled.drawStr(1, y, line);
        }
    }
    oled.drawStr(0, 63, "B1: details   B2: next");
}

static void drawLogDetail() {
    const size_t n = Storage::count();
    oled.setFont(u8g2_font_6x13_tf);;
    char heading[24];
    snprintf(heading, sizeof(heading), "READING %u/%u",
             (unsigned)(n ? Mode::logIndex() + 1 : 0), (unsigned)n);
    oled.drawStr(0, 23, heading);

    Storage::Record rec;
    if (!n || !Storage::read(Mode::logIndex(), rec)) {
        oled.setFont(u8g2_font_6x10_tf);
        oled.drawStr(0, 41, "Record unavailable");
        oled.setFont(u8g2_font_5x7_tf);
        oled.drawStr(0, 63, "B1: list");
        return;
    }

    oled.setFont(u8g2_font_5x7_tf);
    oled.drawStr(0, 34, rec.timestamp);
    oled.setFont(u8g2_font_7x13B_tf);
    char value[24];
    formatHb(value, sizeof(value), rec.hb, true);
    oled.drawStr(0, 49, value);
    oled.setFont(u8g2_font_5x7_tf);

    oled.drawStr(0, 63, "B1: list  B2: next");
}

void update(
    Mode::Screen screen, Mode::MeasureMode mode, const char *message,
    const char *timestamp, bool rtcAvailable, float hb, bool hbAvailable,
    const char *measurementStatus, uint8_t samplesPercent,
    uint8_t menuSelection
) {
    (void)mode;
    if (!ready) return;

    oled.clearBuffer();
    drawHeader(timestamp, rtcAvailable);

    switch (screen) {
    case Mode::Screen::HOME:
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(0, 23, "MENU");
        drawMenuItem(36, "HB MEASUREMENT", menuSelection == 0);
        drawMenuItem(49, "VIEW LOGS", menuSelection == 1);
        drawMenuItem(62, "CLEAR LOGS", menuSelection == 2);
        break;

    case Mode::Screen::HB_READY:
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(0, 24, "HB MEASUREMENT");
        oled.setFont(u8g2_font_6x10_tf);
        oled.drawStr(0, 39, "Place finger on sensor");
        oled.drawStr(0, 51, "Press B1 to start");
        oled.setFont(u8g2_font_5x7_tf);
        oled.drawStr(0, 63, "B2: cancel");
        break;

    case Mode::Screen::MEASURE:
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(0, 22, Mode::singleMeasurementComplete() ? "MEASUREMENT DONE" : "MEASURING");
        if (hbAvailable) {
            oled.setFont(u8g2_font_7x13B_tf);
            char value[24];
            formatHb(value, sizeof(value), hb, true);
            oled.drawStr(0, 42, value);
            oled.setFont(u8g2_font_5x7_tf);
            oled.drawStr(0, 53, "Experimental estimate");
            oled.drawStr(0, 63, Mode::singleMeasurementComplete()
                                      ? "Saved | hold B1 to exit"
                                      : "Hold still - saving");
        } else {
            oled.setFont(u8g2_font_6x10_tf);
            oled.drawStr(0, 38, "Keep finger still");
            char progress[24];
            snprintf(progress, sizeof(progress), "Signal window %u%%", (unsigned)samplesPercent);
            oled.drawStr(0, 51, progress);
            oled.setFont(u8g2_font_5x7_tf);
            if (measurementStatus) oled.drawStr(0, 63, measurementStatus);
        }
        break;

    case Mode::Screen::LOGS:
        drawLogList();
        break;

    case Mode::Screen::LOG_DETAIL:
        drawLogDetail();
        break;

    case Mode::Screen::CLEAR_CONFIRM:
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(0, 25, "CLEAR ALL LOGS?");
        oled.setFont(u8g2_font_6x10_tf);
        if (Mode::clearCancelSelected()) {
            oled.drawStr(8, 42, "> NO, KEEP LOGS");
            oled.drawStr(8, 56, "  YES, DELETE");
        } else {
            oled.drawStr(8, 42, "> YES, DELETE");
            oled.drawStr(8, 56, "  NO, KEEP LOGS");
        }
        break;

    case Mode::Screen::MESSAGE:
        oled.setFont(u8g2_font_6x13_tf);;
        oled.drawStr(0, 25, "STATUS");
        oled.setFont(u8g2_font_6x10_tf);
        if (message) oled.drawUTF8(0, 43, message);
        oled.setFont(u8g2_font_5x7_tf);
        oled.drawStr(0, 63, "B1: continue");
        break;
    }
    oled.sendBuffer();
}
} // namespace Display
