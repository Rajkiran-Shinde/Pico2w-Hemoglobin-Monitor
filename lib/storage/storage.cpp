#include "storage.h"
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace Storage {
static bool ready = false;
static const char *LOG_PATH = "/hb_log.csv";
static const char *HEADER = "timestamp,hb_g_dl,status";
static constexpr size_t MAX_LOG_RECORDS = 150;

bool begin() {
    // Do not auto-format on mount failure: that could destroy existing readings.
    ready = LittleFS.begin();
    if (!ready) return false;

    if (!LittleFS.exists(LOG_PATH)) {
        File f = LittleFS.open(LOG_PATH, "w");
        if (!f) { ready = false; return false; }
        f.println(HEADER);
        f.flush();
        f.close();
    }
    return true;
}
bool isAvailable() { return ready; }

size_t count() {
    if (!ready) return 0;
    File f = LittleFS.open(LOG_PATH, "r");
    if (!f) return 0;
    size_t n = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() && !line.startsWith("timestamp,")) ++n;
    }
    f.close();
    return n;
}

static bool rewriteNewest(size_t keepCount) {
    File input = LittleFS.open(LOG_PATH, "r");
    if (!input) return false;

    // Keep a bounded set of newest records in RAM before replacing the file.
    String lines[MAX_LOG_RECORDS];
    size_t n = 0;
    while (input.available()) {
        String line = input.readStringUntil('\n');
        line.trim();
        if (!line.length() || line.startsWith("timestamp,")) continue;
        if (n < MAX_LOG_RECORDS) lines[n++] = line;
        else {
            for (size_t i = 1; i < MAX_LOG_RECORDS; ++i) lines[i - 1] = lines[i];
            lines[MAX_LOG_RECORDS - 1] = line;
        }
    }
    input.close();

    File output = LittleFS.open("/hb_tmp.csv", "w");
    if (!output) return false;
    output.println(HEADER);
    size_t start = n > keepCount ? n - keepCount : 0;
    for (size_t i = start; i < n; ++i) output.println(lines[i]);
    output.flush();
    bool ok = !output.getWriteError();
    output.close();
    if (!ok) { LittleFS.remove("/hb_tmp.csv"); return false; }

    LittleFS.remove(LOG_PATH);
    if (!LittleFS.rename("/hb_tmp.csv", LOG_PATH)) return false;
    return true;
}

bool append(float hb, const char *timestamp, const char *status) {
    if (!ready || !timestamp || !status) return false;
    if (count() >= MAX_LOG_RECORDS) {
        // Retain the latest 149 records so this append creates a 150-record log.
        if (!rewriteNewest(MAX_LOG_RECORDS - 1)) return false;
    }

    File f = LittleFS.open(LOG_PATH, "a");
    if (!f) return false;
    f.print(timestamp);
    f.print(',');
    f.print(hb, 2);
    f.print(',');
    f.println(status);
    f.flush();
    bool ok = !f.getWriteError();
    f.close();
    return ok;
}
bool read(size_t index, Record &record) {
    if (!ready) return false;

    File f = LittleFS.open(LOG_PATH, "r");
    if (!f) return false;

    size_t current = 0;
    bool found = false;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();

        if (line.length() == 0 ||
            line.startsWith("timestamp,")) {
            continue;
        }

        if (current++ != index) {
            continue;
        }

        // Copy the CSV line into a writable buffer.
        char buffer[128] = {};
        size_t len = line.length();

        if (len >= sizeof(buffer)) {
            break;
        }

        memcpy(buffer, line.c_str(), len);
        buffer[len] = '\0';

        // Split: timestamp,hb_g_dl,status
        char *firstComma = strchr(buffer, ',');
        if (!firstComma) break;

        *firstComma = '\0';
        char *hbText = firstComma + 1;

        char *secondComma = strchr(hbText, ',');
        if (!secondComma) break;

        *secondComma = '\0';
        char *statusText = secondComma + 1;

        // Parse the numeric field separately from the CSV.
        char *endPtr = nullptr;
        float hbValue = strtof(hbText, &endPtr);

        if (endPtr == hbText || *endPtr != '\0') {
            break;
        }

        if (strlen(buffer) >= sizeof(record.timestamp) ||
            strlen(statusText) >= sizeof(record.status)) {
            break;
        }

        memset(&record, 0, sizeof(record));

        strncpy(record.timestamp, buffer,
                sizeof(record.timestamp) - 1);

        record.hb = hbValue;

        strncpy(record.status, statusText,
                sizeof(record.status) - 1);

        found = true;
        break;
    }

    f.close();
    return found;
}
bool clear() {
    if (!ready) return false;
    File f = LittleFS.open(LOG_PATH, "w");
    if (!f) return false;
    f.println(HEADER);
    f.flush();
    bool ok = !f.getWriteError();
    f.close();
    return ok;
}
}
