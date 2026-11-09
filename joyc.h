// Hat Mini JoyC (STM32F030, I2C 0x54) on the StickC Plus2 hat header.
// Same sequence as M5Hat-Mini-JoyC: hardware I2C on SDA G0 / SCL G26, and a STOP
// between the register write and the read. A repeated start never ACKs
// on this firmware. The classic dual JoyC (0x38) is accepted if 0x54 is absent.

#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <driver/i2c.h>
#include <esp_timer.h>

extern int rotation;
extern bool joycHorizontal;
extern bool joycClick;

static const int JOYC_SDA = 0;
static const int JOYC_SCL = 26;
// The stick is a potentiometer, about -115..115. Inside ±79 nothing happens.
// Y past ±80 steps every 1.5 s. X past ±80 is one click (left back, right
// select) and stays locked until that axis returns to within ±10.
static const int JOYC_STEP = 80;
static const int JOYC_X_REARM = 10;
static const int JOYC_DEADZONE = JOYC_STEP - 1;
static const uint32_t JOYC_POLL_MS = 20;
static const uint32_t JOYC_REPEAT_MS = 1500;
static const uint32_t JOYC_BUTTON_MS = 50;

struct JoycState {
  bool present = false;
  bool classic = false;
  uint8_t addr = 0x54;
  int8_t screenX = 0;
  int8_t screenY = 0;
  bool button = false;
  bool prev = false;
  bool next = false;
  bool back = false;
  bool select = false;
  int holdDir = 0;
  bool xArmed = true;
  bool yArmed = true;
  int pendingDir = 0;
  uint32_t lastRepeat = 0;
  uint32_t pendingSince = 0;
  uint32_t idleSince = 0;
  uint32_t lastPoll = 0;
  bool buttonRaw = false;
  uint32_t buttonSince = 0;
  bool buttonArmed = false;
  uint32_t ledUntil = 0;
};

static JoycState joyc;
static bool joycWireReady = false;

// Portrait, screen facing you: +rawX is right, +rawY is up. The picture is
// turned in clockwise quarter-turns (1 = 90° clockwise, hat on the right).
// screenY > 0 is down the picture (next). screenX < 0 is left (back).
// screenX > 0 is right (select), same as the stick click.
inline void joycMapAxes(int rawX, int rawY, int rot, int8_t &screenX, int8_t &screenY) {
  int x = rawX;
  int y = -rawY;
  switch (rot & 3) {
    case 1:
      x = rawY;
      y = rawX;
      break;
    case 2:
      x = -rawX;
      y = rawY;
      break;
    case 3:
      x = -rawY;
      y = -rawX;
      break;
    default:
      break;
  }
  // Both axes came out reversed on the device: left read as right, up as down.
  x = -x;
  y = -y;
  if (x > 127) x = 127;
  if (x < -127) x = -127;
  if (y > 127) y = 127;
  if (y < -127) y = -127;
  screenX = (int8_t)x;
  screenY = (int8_t)y;
}

inline bool joycEnsureWire() {
  if (joycWireReady) {
    return true;
  }
  // M5.begin() owns port 0 for the grove header. The hat is G0/G26.
  M5.Ex_I2C.release();
  i2c_driver_delete(I2C_NUM_0);
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = (gpio_num_t)JOYC_SDA;
  conf.scl_io_num = (gpio_num_t)JOYC_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;
  if (i2c_param_config(I2C_NUM_0, &conf) != ESP_OK) {
    return false;
  }
  if (i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK) {
    return false;
  }
  joycWireReady = true;
  return true;
}

inline bool joycCmdDone(i2c_cmd_handle_t cmd) {
  esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
  i2c_cmd_link_delete(cmd);
  return err == ESP_OK;
}

inline bool joycPing(uint8_t addr) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_stop(cmd);
  return joycCmdDone(cmd);
}

// STOP between the register write and the read, matching M5Hat-Mini-JoyC.
inline bool joycReadReg(uint8_t reg, uint8_t *data, uint8_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (joyc.addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_stop(cmd);
  if (!joycCmdDone(cmd)) {
    return false;
  }
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (joyc.addr << 1) | I2C_MASTER_READ, true);
  for (uint8_t i = 0; i < len; i++) {
    i2c_master_read_byte(cmd, &data[i], i + 1 < len ? I2C_MASTER_ACK : I2C_MASTER_NACK);
  }
  i2c_master_stop(cmd);
  return joycCmdDone(cmd);
}

inline bool joycWriteReg(uint8_t reg, const uint8_t *data, uint8_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (joyc.addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  if (len) {
    i2c_master_write(cmd, data, len, true);
  }
  i2c_master_stop(cmd);
  return joycCmdDone(cmd);
}

inline void joycLedOff() {
  if (!joyc.present || !joycWireReady) {
    return;
  }
  if (joyc.ledUntil && (int32_t)(joyc.ledUntil - (uint32_t)(esp_timer_get_time() / 1000ULL)) > 0) {
    return;
  }
  uint8_t rgb[3] = {0, 0, 0};
  joycWriteReg(joyc.classic ? 0x20 : 0x40, rgb, 3);
}

inline void joycLedGreen() {
  uint8_t rgb[3] = {0, 40, 0};
  if (joycWriteReg(joyc.classic ? 0x20 : 0x40, rgb, 3)) {
    joyc.ledUntil = (uint32_t)(esp_timer_get_time() / 1000ULL) + 800;
  }
}

inline void joycLedPulse() {
  if (!joyc.present || !joycWireReady) {
    return;
  }
  uint8_t rgb[3] = {0, 80, 0};
  if (joycWriteReg(joyc.classic ? 0x20 : 0x40, rgb, 3)) {
    joyc.ledUntil = (uint32_t)(esp_timer_get_time() / 1000ULL) + 120;
  }
}

inline bool joycProbe() {
  if (!joycEnsureWire()) {
    return false;
  }
  bool classic = false;
  uint8_t addr = 0x54;
  if (!joycPing(0x54)) {
    if (!joycPing(0x38)) {
      return false;
    }
    classic = true;
    addr = 0x38;
  }
  bool first = !joyc.present;
  joyc.present = true;
  joyc.classic = classic;
  joyc.addr = addr;
  if (first) {
    joyc.buttonArmed = false;
    joycLedGreen();
    Serial.printf("JoyC 0x%02X\n", addr);
  }
  return true;
}

inline bool joycBegin() {
  joyc.present = false;
  for (int attempt = 0; attempt < 10; attempt++) {
    if (joycProbe()) {
      return true;
    }
    delay(50);
  }
  Serial.println("JoyC ausente");
  return false;
}

inline void joycFire(int dir) {
  joycLedPulse();
  if (dir == 1) {
    joyc.prev = true;
  } else if (dir == 2) {
    joyc.next = true;
  } else if (dir == 3) {
    joyc.back = true;
  } else if (dir == 4) {
    joyc.select = true;
  }
}

inline bool joycReadAxes(int &rawX, int &rawY, bool &pressed) {
  if (!joyc.classic) {
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t button = 0;
    if (!joycReadReg(0x20, &x, 1) || !joycReadReg(0x21, &y, 1)) {
      return false;
    }
    joycReadReg(0x30, &button, 1);
    rawX = (int8_t)x;
    rawY = (int8_t)y;
    pressed = button != 0;
    return true;
  }
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t button = 0;
  if (!joycReadReg(0x60, &x, 1) || !joycReadReg(0x61, &y, 1)) {
    return false;
  }
  joycReadReg(0x64, &button, 1);
  rawX = (int)x - 100;
  rawY = (int)y - 100;
  pressed = (button & 0x11) != 0;
  return true;
}

// Menus drop the CPU to 80 MHz. millis() on this core follows that clock,
// so a 1 s wait and a 3 s wait can land on the same slow tick. The step
// timer uses the APB hardware timer instead, which does not change with the CPU.
inline uint32_t joycMs() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// 1 up, 2 down, 3 back, 4 select. The axis farther from center wins.
// Anything inside ±79 is idle. Both axes use the same ±80 line.
inline int joycPickDir(int x, int y, int &mag) {
  int ax = abs(x);
  int ay = abs(y);
  bool yStep = ay >= JOYC_STEP;
  bool xStep = ax >= JOYC_STEP;
  if (yStep && (!xStep || ay >= ax)) {
    mag = ay;
    return y < 0 ? 1 : 2;
  }
  if (x <= -JOYC_STEP) {
    mag = ax;
    return 3;
  }
  if (x >= JOYC_STEP) {
    mag = ax;
    return 4;
  }
  mag = 0;
  return 0;
}

inline void joycPoll() {
  uint32_t now = joycMs();
  if (joyc.present && joyc.ledUntil && (int32_t)(now - joyc.ledUntil) >= 0) {
    joyc.ledUntil = 0;
    joycLedOff();
  }
  if (!joyc.present) {
    if (now - joyc.lastPoll < 400) {
      return;
    }
    joyc.lastPoll = now;
    joycProbe();
    return;
  }
  if (now - joyc.lastPoll < JOYC_POLL_MS) {
    return;
  }
  joyc.lastPoll = now;

  int rawX = 0;
  int rawY = 0;
  bool pressed = false;
  if (!joycReadAxes(rawX, rawY, pressed)) {
    return;
  }

  int8_t sx = 0;
  int8_t sy = 0;
  joycMapAxes(rawX, rawY, rotation, sx, sy);
  joyc.screenX = sx;
  joyc.screenY = sy;

  if (abs((int)sx) <= JOYC_X_REARM) {
    joyc.xArmed = true;
  }
  if (abs((int)sy) <= JOYC_X_REARM) {
    joyc.yArmed = true;
    if (joyc.holdDir == 1 || joyc.holdDir == 2) {
      joyc.holdDir = 0;
    }
  }

  int mag = 0;
  int dir = joycPickDir(sx, sy, mag);
  (void)mag;
  if (!joycHorizontal && (dir == 3 || dir == 4)) {
    if (sy <= -JOYC_STEP) {
      dir = 1;
    } else if (sy >= JOYC_STEP) {
      dir = 2;
    } else {
      dir = 0;
    }
  }
  if (dir == 3 || dir == 4) {
    joyc.idleSince = 0;
    if (joyc.xArmed) {
      joyc.xArmed = false;
      joyc.holdDir = dir;
      joycFire(dir);
    }
  } else if (dir == 1 || dir == 2) {
    joyc.idleSince = 0;
    if (dir != joyc.holdDir) {
      if (joyc.yArmed) {
        joyc.yArmed = false;
        joyc.holdDir = dir;
        joyc.pendingDir = dir;
        joyc.lastRepeat = now;
        joycFire(dir);
      }
    } else if ((now - joyc.lastRepeat) >= JOYC_REPEAT_MS) {
      joyc.lastRepeat = now;
      joycFire(dir);
    }
  }

  if (pressed != joyc.buttonRaw) {
    joyc.buttonRaw = pressed;
    joyc.buttonSince = now;
  }
  if (!joyc.buttonArmed) {
    joyc.button = pressed;
    joyc.buttonArmed = true;
  } else if (pressed != joyc.button &&
             (now - joyc.buttonSince) >= JOYC_BUTTON_MS) {
    if (pressed && joycClick) {
      joyc.select = true;
      joycLedPulse();
    }
    joyc.button = pressed;
  }
}

inline bool joycTakePrev() {
  bool fired = joyc.prev;
  joyc.prev = false;
  return fired;
}

inline bool joycTakeNext() {
  bool fired = joyc.next;
  joyc.next = false;
  return fired;
}

inline bool joycTakeBack() {
  bool fired = joyc.back;
  joyc.back = false;
  return fired;
}

inline bool joycTakeSelect() {
  bool fired = joyc.select;
  joyc.select = false;
  return fired;
}

inline bool joycButtonHeld() {
  return joyc.present && joyc.button;
}

inline bool joycOutOfDeadzone() {
  if (!joyc.present) {
    return false;
  }
  return (joycClick && joyc.button) ||
         abs((int)joyc.screenX) > JOYC_DEADZONE ||
         abs((int)joyc.screenY) > JOYC_DEADZONE;
}

inline void joycDiscardEdges() {
  joyc.prev = false;
  joyc.next = false;
  joyc.back = false;
  joyc.select = false;
}
