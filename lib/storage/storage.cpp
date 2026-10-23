#include "storage.h"
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>
namespace Storage {
static bool ready = false;
static const char *LOG_PATH = "/hb_log.csv";
bool begin() {
  ready = LittleFS.begin();
  if (!ready) return false;
  if (!LittleFS.exists(LOG_PATH)) {
    File f = LittleFS.open(LOG_PATH, "w");
    if (!f) return false;
    f.println("timestamp,hb_g_dl,status"); f.close();
  }
  return true;
}
bool isAvailable() { return ready; }
bool append(float hb, const char *timestamp, const char *status) {
  if (!ready || !timestamp || !status) return false;
  File f = LittleFS.open(LOG_PATH, "a"); if (!f) return false;
  // Timestamp/status are internally generated and contain no commas.
  f.print(timestamp); f.print(','); f.print(hb, 2); f.print(','); f.println(status);
  f.flush(); f.close(); return true;
}
size_t count() {
  if (!ready) return 0;
  File f = LittleFS.open(LOG_PATH, "r"); if (!f) return 0;
  size_t n = 0; while (f.available()) { String line = f.readStringUntil('\n'); if (line.length() && !line.startsWith("timestamp,")) ++n; }
  f.close(); return n;
}
bool read(size_t index, Record &record) {
  if (!ready) return false;
  File f = LittleFS.open(LOG_PATH, "r"); if (!f) return false;
  size_t current = 0; bool found = false;
  while (f.available()) {
    String line = f.readStringUntil('\n'); line.trim();
    if (!line.length() || line.startsWith("timestamp,")) continue;
    if (current++ != index) continue;
    char ts[32] = {}, status[32] = {}; float hb = 0;
    if (sscanf(line.c_str(), "%31[^,],%f,%31s", ts, &hb, status) == 3) {
      memset(&record, 0, sizeof(record)); strncpy(record.timestamp, ts, sizeof(record.timestamp)-1);
      record.hb = hb; strncpy(record.status, status, sizeof(record.status)-1); found = true;
    }
    break;
  }
  f.close(); return found;
}
bool clear() {
  if (!ready) return false;
  File f = LittleFS.open(LOG_PATH, "w"); if (!f) return false;
  f.println("timestamp,hb_g_dl,status"); f.close(); return true;
}
}
