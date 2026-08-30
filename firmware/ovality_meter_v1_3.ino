/*
 ╔══════════════════════════════════════════════════════════════╗
 ║         OVALITY METER — Final Firmware v1.3                  ║
 ║   ESP32-C3 SuperMini + LDC1612 + 1.3" OLED (SH1106)          ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  LIBRARIES (install via Manage Libraries):                   ║
 ║    1. Adafruit SH110X   by Adafruit                          ║
 ║    2. Adafruit GFX Library  by Adafruit                      ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  BOARD SETTINGS:                                             ║
 ║    Board  → ESP32C3 Dev Module                               ║
 ║    USB CDC On Boot → Enabled  (REQUIRED for Serial Monitor)  ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  PIN CONNECTIONS:                                            ║
 ║    GPIO4  → SDA  (LDC1612 + OLED, shared I2C)               ║
 ║    GPIO5  → SCL  (LDC1612 + OLED, shared I2C)               ║
 ║    GPIO2  → Button MEASURE  (other leg → GND)               ║
 ║    GPIO3  → Button RESET    (other leg → GND)               ║
 ║    3V3    → LDC1612 VCC + OLED VCC  (via 1N5819 diode)      ║
 ║    GND    → GND rail                                        ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  TO RECALIBRATE:                                             ║
 ║    Set CALIBRATION_DONE to false                             ║
 ║    Clear cal_count=0, CAL_POS={0}, CAL_RAW={0}              ║
 ║    Upload → Serial Monitor 115200 baud                       ║
 ║    c0  c2.5  c10  c22.8  c34  → p → paste → true → upload   ║
 ╠══════════════════════════════════════════════════════════════╣
 ║  MEASUREMENT FLOW:                                           ║
 ║    Press MEAS (1st) → live Dmax → press to lock              ║
 ║    Press MEAS (2nd) → live Dmin → press to lock              ║
 ║    Press MEAS (3rd) → live D0   → press to lock              ║
 ║    Press MEAS (4th) → shows ovality result                   ║
 ║    Press RESET      → clears everything, back to start       ║
 ╚══════════════════════════════════════════════════════════════╝
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define WHITE SH110X_WHITE
#define BLACK SH110X_BLACK

// ─── DISPLAY ────────────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H  64
#define OLED_ADDR 0x3C   // try 0x3D if display stays blank

Adafruit_SH1106G oled(SCREEN_W, SCREEN_H, &Wire, -1);

// ─── PINS ───────────────────────────────────────────────────
#define PIN_SDA     4
#define PIN_SCL     5
#define BTN_MEASURE 2
#define BTN_RESET   3

// ─── LDC1612 REGISTERS ──────────────────────────────────────
#define LDC_ADDR            0x2B   // try 0x2A if sensor not found
#define REG_DATA_CH0_MSB    0x00
#define REG_DATA_CH0_LSB    0x01
#define REG_RCOUNT_CH0      0x08
#define REG_SETTLECOUNT_CH0 0x10
#define REG_CLOCK_DIVIDERS  0x14
#define REG_CONFIG          0x1A
#define REG_MUX_CONFIG      0x1B
#define REG_RESET_DEV       0x1C
#define REG_DRIVE_CURRENT   0x1E
#define REG_MANUFACTURER_ID 0x7E

// ─── CALIBRATION ────────────────────────────────────────────
// Physics-informed 1mm-step calibration
// Anchored to no-hand measurements: 0mm=22783000, 34.6mm=21874000
// Curve shape derived from 35-point measured session
// To recalibrate: set CALIBRATION_DONE false, clear arrays, upload
#define CALIBRATION_DONE true
#define MAX_CAL_POINTS 80

int cal_count = 35;
float CAL_POS[MAX_CAL_POINTS] = {
  0.00,1.00,2.00,3.00,4.00,5.00,6.00,7.00,8.00,9.00,
  10.00,11.00,12.00,13.00,14.00,15.00,16.00,17.00,18.00,19.00,
  20.00,21.00,22.00,23.00,24.00,25.00,26.00,27.00,28.00,29.00,
  30.00,31.00,32.00,33.00,34.00
};
uint32_t CAL_RAW[MAX_CAL_POINTS] = {
  22783000,22729470,22675940,22622410,22568880,22520752,22473244,
  22413966,22368846,22320178,22276053,22247565,22217824,22184771,
  22165065,22133963,22109759,22093985,22069663,22045355,22025174,
  22005677,21990245,21974106,21959014,21947256,21935429,21923652,
  21914420,21905692,21897438,21890545,21885549,21881167,21876067
};

// ─── STATE MACHINE ──────────────────────────────────────────
enum State {
  ST_BOOT,
  ST_READY,
  ST_DMAX, ST_DMAX_LOCKED,
  ST_DMIN, ST_DMIN_LOCKED,
  ST_D0,   ST_D0_LOCKED,
  ST_RESULT,
  ST_CAL,
  ST_ERROR
};
State state = ST_BOOT;

float    dmax = 0, dmin = 0, d0 = 0, ovality = 0;
uint32_t live_raw = 0;
float    live_mm  = 0;

// ─── CALIBRATION TEMP STORAGE ───────────────────────────────
float    temp_pos[MAX_CAL_POINTS];
uint32_t temp_raw[MAX_CAL_POINTS];
int      temp_count = 0;

// ─── BUTTON LATCH ───────────────────────────────────────────
bool m_press_latch = false;

// ─── LDC1612 ────────────────────────────────────────────────
void ldc_write(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(LDC_ADDR);
  Wire.write(reg);
  Wire.write((val >> 8) & 0xFF);
  Wire.write(val & 0xFF);
  Wire.endTransmission();
}

uint16_t ldc_read(uint8_t reg) {
  Wire.beginTransmission(LDC_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LDC_ADDR, (uint8_t)2);
  uint16_t v = 0;
  if (Wire.available()) v  = Wire.read() << 8;
  if (Wire.available()) v |= Wire.read();
  return v;
}

bool ldc_init() {
  if (ldc_read(REG_MANUFACTURER_ID) != 0x5449) return false;
  ldc_write(REG_RESET_DEV,       0x8000); delay(100);
  ldc_write(REG_RCOUNT_CH0,      0xFFFF);
  ldc_write(REG_SETTLECOUNT_CH0, 0x0064);
  ldc_write(REG_CLOCK_DIVIDERS,  0x1001);
  ldc_write(REG_DRIVE_CURRENT,   0x9000);
  ldc_write(REG_MUX_CONFIG,      0x020C);
  ldc_write(REG_CONFIG,          0x1601);
  delay(100);
  return true;
}

uint32_t ldc_read_ch0() {
  uint16_t msb = ldc_read(REG_DATA_CH0_MSB);
  uint16_t lsb = ldc_read(REG_DATA_CH0_LSB);
  if (msb & 0xF000) return 0;
  return ((uint32_t)(msb & 0x0FFF) << 16) | lsb;
}

// ─── RAW → MM ───────────────────────────────────────────────
float raw_to_mm(uint32_t raw) {
  if (!CALIBRATION_DONE || cal_count < 2) return raw / 1000000.0f;
  for (int i = 0; i < cal_count - 1; i++) {
    uint32_t lo = CAL_RAW[i], hi = CAL_RAW[i+1];
    bool inc      = (hi > lo);
    bool in_range = inc ? (raw >= lo && raw <= hi)
                        : (raw <= lo && raw >= hi);
    if (in_range) {
      float t = (float)((int32_t)raw - (int32_t)lo)
              / (float)((int32_t)hi  - (int32_t)lo);
      return CAL_POS[i] + t * (CAL_POS[i+1] - CAL_POS[i]);
    }
  }
  if ((CAL_RAW[0] > CAL_RAW[cal_count-1] && raw > CAL_RAW[0]) ||
      (CAL_RAW[0] < CAL_RAW[cal_count-1] && raw < CAL_RAW[0]))
    return CAL_POS[0];
  return CAL_POS[cal_count-1];
}

// ─── AVERAGED SENSOR READ ───────────────────────────────────
// Takes N readings and returns the average raw value
// Filters out hand interference and electrical noise
uint32_t ldc_read_averaged(int n, int delay_ms) {
  uint64_t sum = 0; int cnt = 0;
  for (int i = 0; i < n; i++) {
    uint32_t r = ldc_read_ch0();
    if (r > 0) { sum += r; cnt++; }
    delay(delay_ms);
  }
  return cnt > 0 ? (uint32_t)(sum / cnt) : 0;
}

// ─── BUTTONS ────────────────────────────────────────────────
unsigned long btn_m_last = 0, btn_r_last = 0;
bool btn_m_prev = HIGH, btn_r_prev = HIGH;
#define DEBOUNCE 220

bool btn_measure_pressed() {
  bool cur = digitalRead(BTN_MEASURE);
  if (btn_m_prev == HIGH && cur == LOW && millis()-btn_m_last > DEBOUNCE) {
    btn_m_last = millis(); btn_m_prev = cur; return true;
  }
  btn_m_prev = cur; return false;
}

bool btn_reset_pressed() {
  bool cur = digitalRead(BTN_RESET);
  if (btn_r_prev == HIGH && cur == LOW && millis()-btn_r_last > DEBOUNCE) {
    btn_r_last = millis(); btn_r_prev = cur; return true;
  }
  btn_r_prev = cur; return false;
}

// ─── DISPLAY HELPERS ────────────────────────────────────────
void clr()  { oled.clearDisplay(); }
void show() { oled.display(); }

void draw_title_bar(const char* title) {
  oled.fillRect(0, 0, SCREEN_W, 11, WHITE);
  oled.setTextColor(BLACK); oled.setTextSize(1);
  oled.setCursor(3, 2); oled.print(title);
  oled.setTextColor(WHITE);
}

void draw_bottom_hint(const char* hint) {
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(2, 57); oled.print(hint);
}

void draw_bar(int x, int y, int w, int h, int pct) {
  oled.drawRect(x, y, w, h, WHITE);
  int fill = (w-2)*pct/100;
  if (fill > 0) oled.fillRect(x+1, y+1, fill, h-2, WHITE);
}

void draw_step_dots(int current) {
  int sx = (SCREEN_W - 40) / 2;
  for (int i = 0; i < 4; i++) {
    int cx = sx + i*10 + 4;
    if      (i < current)  oled.fillCircle(cx, 53, 3, WHITE);
    else if (i == current) oled.drawCircle(cx, 53, 3, WHITE);
    else                   oled.drawPixel(cx, 53, WHITE);
  }
}

void draw_live_value(float mm) {
  oled.setTextColor(WHITE);
  char buf[12];
  if (CALIBRATION_DONE) {
    oled.setTextSize(2);
    dtostrf(mm, 5, 2, buf);
    oled.setCursor(20, 26); oled.print(buf);
    oled.setTextSize(1);
    oled.setCursor(96, 36); oled.print("mm");
  } else {
    oled.setTextSize(1);
    oled.setCursor(4, 28);
    oled.printf("Raw: %.3fM", mm);
  }
  if ((millis()/400)%2 == 0) oled.fillCircle(6, 34, 3, WHITE);
}

void draw_locked_value(const char* label, float val) {
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(4, 15); oled.print(label); oled.print(" LOCKED");
  oled.drawLine(100, 12, 104, 18, WHITE);
  oled.drawLine(104, 18, 114,  8, WHITE);
  oled.setTextSize(2);
  char buf[10]; dtostrf(val, 5, 2, buf);
  oled.setCursor(16, 28); oled.print(buf);
  oled.setTextSize(1);
  oled.setCursor(98, 38); oled.print("mm");
}

// ─── SCREENS ────────────────────────────────────────────────
void screen_boot() {
  for (int x = SCREEN_W; x >= 0; x -= 8) {
    clr();
    oled.setTextSize(2); oled.setTextColor(WHITE);
    oled.setCursor(x, 10); oled.print("OVALITY");
    oled.setTextSize(1);
    oled.setCursor(x+8,  32); oled.print("MEASUREMENT");
    oled.setCursor(x+18, 44); oled.print("INSTRUMENT");
    show(); delay(18);
  }
  for (int i = 0; i < 3; i++) {
    clr();
    oled.setTextSize(2); oled.setTextColor(WHITE);
    oled.setCursor(0, 10); oled.print("OVALITY");
    oled.setTextSize(1);
    oled.setCursor(8,  32); oled.print("MEASUREMENT");
    oled.setCursor(18, 44); oled.print("INSTRUMENT");
    show(); delay(200);
    clr(); show(); delay(120);
  }
  for (int p = 0; p <= 100; p += 5) {
    clr();
    oled.setTextSize(1); oled.setTextColor(WHITE);
    oled.setCursor(26, 8); oled.print("INITIALISING");
    draw_bar(14, 34, 100, 10, p);
    oled.setCursor(54, 50); oled.printf("%d%%", p);
    show(); delay(20);
  }
  delay(300);
}

void screen_ready() {
  clr(); draw_title_bar("OVALITY METER");
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(10, 18); oled.print("Ready to measure");
  oled.setCursor(46+(millis()/300)%8, 30); oled.print("> PRESS MEAS");
  oled.setCursor(6,  44); oled.print("1:Dmax 2:Dmin 3:D0");
  draw_bottom_hint("MEAS=start  RST=reset");
  show();
}

void screen_measuring(const char* label, int step) {
  clr(); draw_title_bar(label);
  draw_live_value(live_mm);
  draw_step_dots(step);
  draw_bottom_hint("MEAS=lock   RST=reset");
  show();
}

void screen_locked(const char* label, float val, int step) {
  clr(); draw_title_bar("VALUE LOCKED");
  draw_locked_value(label, val);
  draw_step_dots(step);
  draw_bottom_hint("MEAS=next   RST=reset");
  show();
}

// ── Locking animation + high-quality averaged reading ────────
// Takes 20+ readings during the animation for best accuracy
// Moves hand effect: even if hand was near during press,
// the 0.5s average settles to a clean reading
void screen_locking_anim() {
  uint64_t sum = 0; int cnt = 0;
  for (int r = 0; r <= 28; r += 4) {
    clr();
    oled.setTextSize(1); oled.setTextColor(WHITE);
    oled.setCursor(34, 24); oled.print("LOCKING...");
    oled.drawCircle(64, 44, r, WHITE);
    show();
    // Take 3 readings per animation frame
    for (int i = 0; i < 3; i++) {
      uint32_t v = ldc_read_ch0();
      if (v > 0) { sum += v; cnt++; }
      delay(13);
    }
  }
  // Store high-quality averaged reading
  if (cnt > 0) {
    live_raw = (uint32_t)(sum / cnt);
    live_mm  = raw_to_mm(live_raw);
  }
  delay(100);
}

void screen_result() {
  clr();
  oled.fillRect(0, 0, SCREEN_W, 11, WHITE);
  oled.setTextColor(BLACK); oled.setTextSize(1);
  oled.setCursor(18, 2); oled.print("OVALITY RESULT");
  oled.setTextColor(WHITE);
  char buf[12];
  oled.setTextSize(2);
  dtostrf(ovality, 5, 2, buf);
  oled.setCursor(4, 14); oled.print(buf);
  oled.setTextSize(1);
  oled.setCursor(90, 22); oled.print("%");
  oled.drawLine(0, 33, SCREEN_W, 33, WHITE);
  char b2[8];
  oled.setCursor(0,  37); oled.print("D0:");
  dtostrf(d0,   4, 1, b2); oled.print(b2);
  oled.setCursor(44, 37); oled.print("D+:");
  dtostrf(dmax, 4, 1, b2); oled.print(b2);
  oled.setCursor(88, 37); oled.print("D-:");
  dtostrf(dmin, 4, 1, b2); oled.print(b2);
  int gauge = (int)constrain(ovality/20.0f*100.0f, 0, 100);
  draw_bar(0, 50, SCREEN_W, 8, gauge);
  oled.setTextColor(gauge > 30 ? BLACK : WHITE);
  oled.setCursor(4, 52);
  if      (ovality < 3)  oled.print("GOOD");
  else if (ovality < 8)  oled.print("MODERATE");
  else if (ovality < 15) oled.print("HIGH");
  else                   oled.print("CRITICAL");
  oled.setTextColor(WHITE);
  show();
}

void screen_cal_standby() {
  static unsigned long last_draw = 0;
  if (millis() - last_draw < 3000) return;
  last_draw = millis();
  clr(); draw_title_bar("CALIBRATION");
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(2, 16); oled.print("Check Serial Monitor");
  oled.setCursor(2, 30); oled.print("115200 baud");
  oled.setCursor(2, 44); oled.printf("Points: %d", temp_count);
  show();
}

void screen_error(const char* msg) {
  clr();
  if ((millis()/500)%2 == 0) {
    oled.drawRect(0, 0, SCREEN_W, SCREEN_H, WHITE);
    oled.drawRect(2, 2, SCREEN_W-4, SCREEN_H-4, WHITE);
  }
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(34, 8);  oled.print("! ERROR !");
  oled.drawLine(0, 18, SCREEN_W, 18, WHITE);
  oled.setCursor(4, 24);  oled.print(msg);
  oled.setCursor(4, 40);  oled.print("Check wiring:");
  oled.setCursor(4, 52);  oled.print("SDA=GPIO4  SCL=GPIO5");
  show();
}

// ─── CALIBRATION (Serial Monitor only) ──────────────────────
void run_calibration(uint32_t raw) {
  Serial.printf("Raw: %8u  |  %.4fM\n", raw, raw/1000000.0f);
  if (!Serial.available()) { screen_cal_standby(); return; }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim(); cmd.toLowerCase();

  if (cmd == "t") {
    Serial.printf("Raw: %u  (%.4fM)\n", raw, raw/1000000.0f);
  }
  else if (cmd == "r") {
    temp_count = 0;
    Serial.println("Cleared. Start fresh.");
  }
  else if (cmd == "p") {
    if (temp_count < 2) { Serial.println("Need at least 2 points."); return; }
    float    ps[MAX_CAL_POINTS];
    uint32_t rs[MAX_CAL_POINTS];
    for (int i = 0; i < temp_count; i++) { ps[i]=temp_pos[i]; rs[i]=temp_raw[i]; }
    for (int i = 1; i < temp_count; i++) {
      float kp=ps[i]; uint32_t kr=rs[i]; int j=i-1;
      while (j>=0 && ps[j]>kp) { ps[j+1]=ps[j]; rs[j+1]=rs[j]; j--; }
      ps[j+1]=kp; rs[j+1]=kr;
    }
    Serial.println("\n========== PASTE THIS INTO FIRMWARE ==========");
    Serial.printf("int cal_count = %d;\n", temp_count);
    Serial.print("float CAL_POS[MAX_CAL_POINTS] = {");
    for (int i = 0; i < temp_count; i++) {
      Serial.printf("%.2f", ps[i]);
      if (i < temp_count-1) Serial.print(",");
    }
    Serial.println("};");
    Serial.print("uint32_t CAL_RAW[MAX_CAL_POINTS] = {");
    for (int i = 0; i < temp_count; i++) {
      Serial.print(rs[i]);
      if (i < temp_count-1) Serial.print(",");
    }
    Serial.println("};");
    Serial.println("==============================================");
    Serial.println("1. Copy 3 lines above into firmware");
    Serial.println("2. Set CALIBRATION_DONE = true");
    Serial.println("3. Re-upload");
    Serial.printf("Total: %d points\n\n", temp_count);
  }
  else if (cmd.startsWith("c")) {
    float pos = cmd.substring(1).toFloat();
    if (pos < 0 || pos > 40) { Serial.println("Range: 0 to 40mm"); return; }
    if (temp_count >= MAX_CAL_POINTS) { Serial.println("Max 80 points reached"); return; }
    for (int i = 0; i < temp_count; i++) {
      if (fabs(temp_pos[i]-pos) < 0.01f) {
        temp_raw[i] = raw;
        Serial.printf("%.2fmm UPDATED: %u  (%d total)\n", pos, raw, temp_count);
        return;
      }
    }
    temp_pos[temp_count] = pos;
    temp_raw[temp_count] = raw;
    temp_count++;
    Serial.printf("%.2fmm recorded: %u  (%d total)\n", pos, raw, temp_count);
  }
  else {
    Serial.println("c0  c2.5  c17.3 → record | p → print | r → clear | t → raw");
  }
}

// ─── SETUP ──────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(BTN_MEASURE, INPUT_PULLUP);
  pinMode(BTN_RESET,   INPUT_PULLUP);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  if (!oled.begin(OLED_ADDR, true)) {
    if (!oled.begin(0x3D, true)) {
      Serial.println("OLED not found");
      while (1) delay(1000);
    }
  }
  oled.setTextWrap(false);
  screen_boot();
  Serial.print("LDC1612... ");
  for (int tries = 1; tries <= 6; tries++) {
    if (ldc_init()) { Serial.println("OK"); break; }
    Serial.printf("retry %d/6\n", tries);
    clr(); oled.setTextSize(1); oled.setTextColor(WHITE);
    oled.setCursor(10, 28); oled.printf("Sensor retry %d/6", tries);
    show(); delay(1000);
    if (tries == 6) { state = ST_ERROR; return; }
  }
  if (CALIBRATION_DONE) {
    state = ST_READY;
    Serial.println("Measurement mode — ready.");
  } else {
    state = ST_CAL;
    Serial.println("Calibration mode.");
    Serial.println("c0  c2.5  c10.3  c22.8  c34 → p → paste → true → upload");
    Serial.println("─────────────────────────────────────────────────────────");
  }
}

// ─── LOOP ───────────────────────────────────────────────────
unsigned long last_read  = 0;
unsigned long last_frame = 0;
#define READ_INTERVAL  200
#define FRAME_INTERVAL 80

void loop() {
  if (btn_measure_pressed()) m_press_latch = true;
  bool r_press = btn_reset_pressed();

  // ── Averaged sensor read (8 samples) ─────────────────────
  // Filters hand interference and electrical noise
  if (millis()-last_read > READ_INTERVAL) {
    last_read = millis();
    uint64_t sum = 0; int cnt = 0;
    for (int i = 0; i < 8; i++) {
      uint32_t r = ldc_read_ch0();
      if (r > 0) { sum += r; cnt++; }
      delay(8);
    }
    if (cnt > 0) {
      live_raw = (uint32_t)(sum / cnt);
      live_mm  = raw_to_mm(live_raw);
    }
  }

  if (r_press && state != ST_CAL) {
    state = ST_READY;
    dmax = dmin = d0 = ovality = 0;
    m_press_latch = false;
    Serial.println("RESET");
  }

  if (millis()-last_frame > FRAME_INTERVAL) {
    last_frame = millis();
    bool m_press  = m_press_latch;
    m_press_latch = false;

    switch (state) {

      case ST_BOOT:
        state = CALIBRATION_DONE ? ST_READY : ST_CAL;
        break;

      case ST_READY:
        screen_ready();
        if (m_press) { state = ST_DMAX; Serial.println("→ Dmax"); }
        break;

      case ST_DMAX:
        screen_measuring("MEASURE Dmax", 0);
        if (CALIBRATION_DONE) Serial.printf("Dmax live: %.3fmm\n", live_mm);
        if (m_press) {
          screen_locking_anim();   // averages 20+ readings
          dmax = live_mm;
          Serial.printf("Dmax LOCKED = %.3fmm\n", dmax);
          state = ST_DMAX_LOCKED;
        }
        break;

      case ST_DMAX_LOCKED:
        screen_locked("Dmax", dmax, 0);
        if (m_press) { state = ST_DMIN; Serial.println("→ Dmin"); }
        break;

      case ST_DMIN:
        screen_measuring("MEASURE Dmin", 1);
        if (CALIBRATION_DONE) Serial.printf("Dmin live: %.3fmm\n", live_mm);
        if (m_press) {
          screen_locking_anim();
          dmin = live_mm;
          Serial.printf("Dmin LOCKED = %.3fmm\n", dmin);
          state = ST_DMIN_LOCKED;
        }
        break;

      case ST_DMIN_LOCKED:
        screen_locked("Dmin", dmin, 1);
        if (m_press) { state = ST_D0; Serial.println("→ D0"); }
        break;

      case ST_D0:
        screen_measuring("MEASURE D0", 2);
        if (CALIBRATION_DONE) Serial.printf("D0 live: %.3fmm\n", live_mm);
        if (m_press) {
          screen_locking_anim();
          d0 = live_mm;
          Serial.printf("D0 LOCKED = %.3fmm\n", d0);
          state = ST_D0_LOCKED;
        }
        break;

      case ST_D0_LOCKED:
        screen_locked("D0", d0, 2);
        if (m_press) {
          ovality = (d0 > 0.1f) ? fabsf(dmax-dmin)/d0*100.0f : 0;
          Serial.println("\n══════════════════════════════");
          Serial.printf("  Dmax    = %.3f mm\n", dmax);
          Serial.printf("  Dmin    = %.3f mm\n", dmin);
          Serial.printf("  D0      = %.3f mm\n", d0);
          Serial.println("──────────────────────────────");
          Serial.printf("  OVALITY = %.2f%%\n", ovality);
          Serial.println("══════════════════════════════\n");
          state = ST_RESULT;
        }
        break;

      case ST_RESULT:
        screen_result();
        if (m_press) {
          dmax = dmin = d0 = ovality = 0;
          state = ST_READY;
          Serial.println("New measurement. Press MEAS.");
        }
        break;

      case ST_CAL:
        run_calibration(live_raw);
        break;

      case ST_ERROR:
        screen_error("LDC1612 not found");
        if (millis()%3000 < 50 && ldc_init())
          state = CALIBRATION_DONE ? ST_READY : ST_CAL;
        break;
    }
  }
}
