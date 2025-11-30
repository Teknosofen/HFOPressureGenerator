/* 
#include <Arduino.h>

// ESC signal pin
const int escPin = 43;

// DSHOT settings
#define DSHOT_THROTTLE_MIN 48
#define DSHOT_THROTTLE_MAX 2047
#define DSHOT_BITCOUNT     16

// Use DSHOT150 timing for reliable bit-bang
// Bit period ≈ 6.67 µs → ≈1600 cycles at 240 MHz
static inline void gpioHigh(int pin) { GPIO.out_w1ts = (1UL << pin); }
static inline void gpioLow(int pin)  { GPIO.out_w1tc = (1UL << pin); }
static inline void burnCycles(uint32_t cycles) {
  while (cycles--) { asm volatile("nop"); }
}

portMUX_TYPE dshotMux = portMUX_INITIALIZER_UNLOCKED;

// Ratio targets for DSHOT “1” and “0” bits at DSHOT150
const uint32_t BIT_CYCLES = 1600;       // ≈6.67 µs at 240 MHz
const uint32_t ONE_HI     = (BIT_CYCLES * 3) / 4;  // 1200 cycles
const uint32_t ONE_LO     = BIT_CYCLES - ONE_HI;   // 400 cycles
const uint32_t ZERO_HI    = (BIT_CYCLES * 3) / 8;  // 600 cycles
const uint32_t ZERO_LO    = BIT_CYCLES - ZERO_HI;  // 1000 cycles

void sendDSHOTPacket(uint16_t payload11bits, bool telemetry) {
  // Build packet with checksum
  uint16_t packet = (payload11bits << 1) | (telemetry ? 1 : 0);
  uint8_t csum = 0; uint16_t d = packet;
  for (int i = 0; i < 3; i++) { csum ^= (d & 0xF); d >>= 4; }
  packet = (packet << 4) | (csum & 0xF);

  portENTER_CRITICAL(&dshotMux);
  for (int i = DSHOT_BITCOUNT - 1; i >= 0; i--) {
    bool bit = (packet >> i) & 1;
    if (bit) {
      gpioHigh(escPin); burnCycles(ONE_HI);
      gpioLow(escPin);  burnCycles(ONE_LO);
    } else {
      gpioHigh(escPin); burnCycles(ZERO_HI);
      gpioLow(escPin);  burnCycles(ZERO_LO);
    }
  }
  portEXIT_CRITICAL(&dshotMux);
}

void sendDSHOTThrottle(int throttle) {
  throttle = constrain(throttle, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
  sendDSHOTPacket((uint16_t)throttle, false);
}

void setup() {
  pinMode(escPin, OUTPUT);
  Serial.begin(115200);
  delay(4000);
  Serial.println("DSHOT150 test starting...");
}

void loop() {
  // Fixed throttle value for testing
  int throttlePkt = 300; // mid‑range packet
  sendDSHOTThrottle(throttlePkt);

  // Send at ~1 kHz (every 1 ms)
  delayMicroseconds(1000);
} */








#include <Arduino.h>
#include <TFT_eSPI.h>   // Graphics library for T-Display S3

// --- Pins ---
#define INTERACTION_BUTTON_PIN 14   // GPIO14, Key 2: toggle PWM/DSHOT
#define DIRECTION_BUTTON_PIN   0    // GPIO0, Key 1: toggle forward/reverse
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1
const int escPin = 43;              // ESC signal pin (white wire)

// --- PWM settings ---
const int pwmChannel = 0;
const int pwmFreq = 50;             // 50 Hz for RC PWM
const int pwmResolution = 14;       // max supported on ESP32-S3

// --- DSHOT settings ---
#define DSHOT_THROTTLE_MIN 48
#define DSHOT_THROTTLE_MAX 2047
#define DSHOT_BITCOUNT     16

// Use DSHOT150 timing for reliable bit-bang
// Bit period ≈ 6.67 µs → ≈1600 cycles at 240 MHz
static inline void gpioHigh(int pin) { GPIO.out_w1ts = (1UL << pin); }
static inline void gpioLow(int pin)  { GPIO.out_w1tc = (1UL << pin); }
static inline void burnCycles(uint32_t cycles) {
  while (cycles--) { asm volatile("nop"); }
}

portMUX_TYPE dshotMux = portMUX_INITIALIZER_UNLOCKED;

// Ratio targets for DSHOT “1” and “0” bits at DSHOT150
const uint32_t BIT_CYCLES = 1600;       // ≈6.67 µs at 240 MHz
const uint32_t ONE_HI     = (BIT_CYCLES * 3) / 4;  // 1200 cycles
const uint32_t ONE_LO     = BIT_CYCLES - ONE_HI;   // 400 cycles
const uint32_t ZERO_HI    = (BIT_CYCLES * 3) / 8;  // 600 cycles
const uint32_t ZERO_LO    = BIT_CYCLES - ZERO_HI;  // 1000 cycles

// --- State ---
bool useDSHOT = false;
bool motorForward = true;
int  lastModeButton = HIGH;
int  lastDirButton  = HIGH;
uint32_t lastModeToggleMs = 0;
uint32_t lastDirToggleMs  = 0;
const uint32_t debounceMs = 60;

// --- Timing ---
uint32_t lastUIUpdateMs = 0;   // for 5 Hz screen refresh

// --- TFT setup ---
TFT_eSPI tft;

// Colours for PressureGenerator UI
#define TFT_LOGOBACKGROUND       0x85BA // note, if you pick another color from the image, note that you will have to flip the bytes here
#define TFT_LOGOBLUE             0x5497
#define TFT_DARKERBLUE           0x3A97 // A muted steel blue
#define TFT_DEEPBLUE             0x1A6F // A darker steel blue
#define TFT_SLATEBLUE            0x2B4F // A lighter steel blue
#define TFT_MIDNIGHTBLUE         0x1028 // A light steel blue
#define TFT_REDDISH_TINT         0xA4B2   
#define TFT_GREENISH_TINT        0x5DAD

// --- UI helpers ---
void drawUI(const char* modeText, const char* dirText,
            const char* valueLabel, int value, int adcCounts) {

  static const char* OldModeText   = "";
  static const char* OldDirText    = "";
  static const char* OldValueLabel = "";
  static int OldValue              = -1;
  static int OldAdcCounts          = -1;

  // Mode text
  if (strcmp(modeText, OldModeText) != 0) {
    tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND); // erase old
    tft.setCursor(10, 30);
    tft.println(OldModeText);
    OldModeText = modeText;
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);          // draw new
    tft.setCursor(10, 30);
    tft.println(modeText);
  }

  // Value label + value
  if (value != OldValue || strcmp(valueLabel, OldValueLabel) != 0) {
    tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND); // erase old
    tft.setCursor(10, 70);
    tft.printf("%s: %d", OldValueLabel, OldValue);
    OldValueLabel = valueLabel;
    OldValue = value;
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);          // draw new
    tft.setCursor(10, 70);
    tft.printf("%s: %d", valueLabel, value);
  }

  // Direction
  if (strcmp(dirText, OldDirText) != 0) {
    tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND); // erase old
    tft.setCursor(10, 110);
    tft.printf("Direction: %s", OldDirText);
    OldDirText = dirText;
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);          // draw new
    tft.setCursor(10, 110);
    tft.printf("Direction: %s", dirText);
  }

  // ADC counts
  if (adcCounts != OldAdcCounts) {
    tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND); // erase old
    tft.setCursor(10, 150);
    tft.printf("ADC counts: %d", OldAdcCounts);
    OldAdcCounts = adcCounts;
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);          // draw new
    tft.setCursor(10, 150);
    tft.printf("ADC counts: %d", adcCounts);

  }
}

// --- DSHOT packet ---
void sendDSHOTPacket(uint16_t payload11bits, bool telemetry) {
  uint16_t packet = (payload11bits << 1) | (telemetry ? 1 : 0);
  uint8_t csum = 0; uint16_t d = packet;
  for (int i = 0; i < 3; i++) { csum ^= (d & 0xF); d >>= 4; }
  packet = (packet << 4) | (csum & 0xF);

  portENTER_CRITICAL(&dshotMux);
  for (int i = DSHOT_BITCOUNT - 1; i >= 0; i--) {
    bool bit = (packet >> i) & 1;
    if (bit) {
      gpioHigh(escPin); burnCycles(ONE_HI);
      gpioLow(escPin);  burnCycles(ONE_LO);
    } else {
      gpioHigh(escPin); burnCycles(ZERO_HI);
      gpioLow(escPin);  burnCycles(ZERO_LO);
    }
  }
  portEXIT_CRITICAL(&dshotMux);
}

void sendDSHOTThrottle(int throttle) {
  throttle = constrain(throttle, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
  sendDSHOTPacket((uint16_t)throttle, false);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 0..4095

  // PWM init
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(escPin, pwmChannel);
  float frameUs = 1000000.0 / pwmFreq;   // 20,000 µs
  int dutyIdle = (int)((1000.0 / frameUs) * ((1 << pwmResolution) - 1));
  ledcWrite(pwmChannel, dutyIdle);

  // TFT init
  tft.init();
  tft.setRotation(1);
  tft.setFreeFont(&FreeSansBold12pt7b); // Set font to Free Sans Bold 18
  tft.fillScreen(TFT_LOGOBACKGROUND);
  tft.setTextColor(TFT_GREEN, TFT_LOGOBACKGROUND);
  tft.setTextSize(1);

  // Buttons
  pinMode(INTERACTION_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIRECTION_BUTTON_PIN, INPUT_PULLUP);
  pinMode(escPin, OUTPUT); // needed for bit-bang DSHOT

  drawUI("Mode: PWM", "Forward", "Throttle (us)", 1000, 0);
}

void loop() {
  uint32_t nowMs = millis();

  // --- Mode toggle ---
  int modeBtn = digitalRead(INTERACTION_BUTTON_PIN);
  if (modeBtn == LOW && lastModeButton == HIGH && (nowMs - lastModeToggleMs) > debounceMs) {
    useDSHOT = !useDSHOT;
    lastModeToggleMs = nowMs;
    if (useDSHOT) {
      ledcDetachPin(escPin);
      pinMode(escPin, OUTPUT);
    } else {
      ledcAttachPin(escPin, pwmChannel);
    }
    Serial.print("Mode switched to: ");
    Serial.println(useDSHOT ? "DSHOT" : "PWM");
  }
  lastModeButton = modeBtn;

  // --- Direction toggle ---
  int dirBtn = digitalRead(DIRECTION_BUTTON_PIN);
  if (dirBtn == LOW && lastDirButton == HIGH && (nowMs - lastDirToggleMs) > debounceMs) {
    motorForward = !motorForward;
    lastDirToggleMs = nowMs;
    Serial.print("Direction toggled: ");
    Serial.println(motorForward ? "Forward" : "Reverse");
  }
  lastDirButton = dirBtn;

  // --- ADC read ---
  int adcCounts = analogRead(ADC_INPUT_PIN);

  if (!useDSHOT) {
    int throttleUs = map(adcCounts, 0, 4095, 1000, 2000);
    float frameUs = 1000000.0 / pwmFreq;
    int duty = (int)((throttleUs / frameUs) * ((1 << pwmResolution) - 1));
    ledcWrite(pwmChannel, duty);

    if (nowMs - lastUIUpdateMs >= 200) {
      drawUI("Mode: PWM", motorForward ? "Forward" : "Reverse",
             "Throttle (us)", throttleUs, adcCounts);
      Serial.printf("PWM throttle: %d us, ADC: %d\n", throttleUs, adcCounts);
      lastUIUpdateMs = nowMs;
    }
  } else {
    int throttlePkt = map(adcCounts, 0, 4095, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
    sendDSHOTThrottle(throttlePkt);

    if (nowMs - lastUIUpdateMs >= 200) {
      drawUI("Mode: DSHOT", motorForward ? "Forward" : "Reverse",
             "Throttle (pkt)", throttlePkt, adcCounts);
      Serial.printf("DSHOT throttle: %d pkt, ADC: %d\n", throttlePkt, adcCounts);
      lastUIUpdateMs = nowMs;
    }
  }
}





/*
#include <Arduino.h>
#include <TFT_eSPI.h>   // Graphics library for T-Display S3

// --- Pins ---
#define DIRECTION_BUTTON_PIN   0    // GPIO0, Key 1: toggle forward/reverse
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1
const int escPin = 43;              // ESC signal pin (white wire)

// --- PWM settings ---
const int pwmChannel = 0;
const int pwmFreq = 50;             // 50 Hz for RC PWM
const int pwmResolution = 14;       // max supported on ESP32-S3

// --- State ---
bool motorForward = true;
int  lastDirButton  = HIGH;
uint32_t lastDirToggleMs  = 0;
const uint32_t debounceMs = 60;

// --- TFT setup ---
TFT_eSPI tft;

// --- UI helpers ---
void drawUI(const char* dirText, int throttleUs, int adcCounts) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 30);
  tft.println("Mode: PWM");
  tft.setCursor(10, 70);
  tft.printf("Throttle: %d us", throttleUs);
  tft.setCursor(10, 110);
  tft.printf("Direction: %s", dirText);
  tft.setCursor(10, 150);
  tft.printf("ADC counts: %d", adcCounts);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 0..4095

  // PWM init
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(escPin, pwmChannel);
  float frameUs = 1000000.0 / pwmFreq;   // 20,000 µs
  int dutyIdle = (int)((1000.0 / frameUs) * ((1 << pwmResolution) - 1));
  ledcWrite(pwmChannel, dutyIdle);

  // TFT init
  tft.init();
  tft.setRotation(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  // Buttons
  pinMode(DIRECTION_BUTTON_PIN, INPUT_PULLUP);

  drawUI("Forward", 1000, 0);
}

void loop() {
  // --- Direction toggle ---
  int dirBtn = digitalRead(DIRECTION_BUTTON_PIN);
  if (dirBtn == LOW && lastDirButton == HIGH && (millis() - lastDirToggleMs) > debounceMs) {
    motorForward = !motorForward;
    lastDirToggleMs = millis();
    Serial.print("Direction toggled: ");
    Serial.println(motorForward ? "Forward" : "Reverse");
  }
  lastDirButton = dirBtn;

  // --- ADC read ---
  int adcCounts = analogRead(ADC_INPUT_PIN);

  // Map ADC counts to throttle pulse width (1000–2000 µs)
  int throttleUs = map(adcCounts, 0, 4095, 1000, 2500);

  // Convert to duty cycle for LEDC
  float frameUs = 1000000.0 / pwmFreq;   // 20,000 µs
  int duty = (int)((throttleUs / frameUs) * ((1 << pwmResolution) - 1));
  ledcWrite(pwmChannel, duty);

  // --- Serial debug ---
  Serial.printf("ADC=%d, Throttle=%d us, Direction=%s\n",
                adcCounts, throttleUs, motorForward ? "Forward" : "Reverse");

  // --- TFT update ---
  drawUI(motorForward ? "Forward" : "Reverse", throttleUs, adcCounts);

  delay(20); // update ~50 Hz
}
*/





/* 
#include <Arduino.h>
#include <TFT_eSPI.h>   // Graphics library for T-Display S3

// --- Pins ---
#define INTERACTION_BUTTON_PIN 14   // GPIO14, Key 2 (not used in PWM test)
#define DIRECTION_BUTTON_PIN   0    // GPIO0, Key 1 (not used in PWM test)
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1
const int escPin = 18;              // ESC signal pin (white wire)

// --- PWM settings ---
const int pwmChannel = 0;
const int pwmFreq = 50;             // 50 Hz for RC PWM
const int pwmResolution = 16;       // 16-bit resolution

// --- TFT setup ---
TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 0..4095 counts
  pinMode(escPin, OUTPUT);

  // PWM init
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(escPin, pwmChannel);

  // Start idle PWM low (1000us) for arming safety
  int dutyIdle = (int)((1000.0 / 20000.0) * 65535); // 1000us of 20ms frame
  ledcWrite(pwmChannel, dutyIdle);

  // TFT init
  tft.init();
  tft.setRotation(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);

  tft.setCursor(10, 30);
  tft.println("PWM Test Mode");
}

void loop() {
  // --- ADC read ---
  int adcCounts = analogRead(ADC_INPUT_PIN);

  // Map ADC counts to throttle pulse width (1000–2000 µs)
  int throttleUs = map(adcCounts, 0, 4095, 1000, 2000);

  // Convert to duty cycle for LEDC
  int duty = (int)((throttleUs / 20000.0) * 65535);
  ledcWrite(pwmChannel, duty);

  // --- Serial debug ---
  Serial.printf("ADC: %d, PWM throttle: %d us\n", adcCounts, throttleUs);

  // --- TFT update ---
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 30);
  tft.println("Mode: PWM");
  tft.setCursor(10, 70);
  tft.printf("Throttle: %d us", throttleUs);
  tft.setCursor(10, 110);
  tft.printf("ADC counts: %d", adcCounts);

  delay(20); // 50 Hz update
}
*/







/*

#include <Arduino.h>
#include <TFT_eSPI.h>     // Graphics library for T-Display S3
#include "driver/rmt.h"

// --- Pins ---
#define INTERACTION_BUTTON_PIN 14   // GPIO14, Key 2: toggle PWM/DSHOT
#define DIRECTION_BUTTON_PIN   0    // GPIO0,  Key 1: toggle forward/reverse
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1
const int escPin = 18;              // ESC signal pin (white wire)

// --- PWM settings ---
const int pwmChannel = 0;
const int pwmFreq = 50;             // 50 Hz for RC PWM
const int pwmResolution = 16;       // 16-bit resolution

// --- DSHOT settings ---
#define DSHOT_THROTTLE_MIN 48
#define DSHOT_THROTTLE_MAX 2047
#define DSHOT_BITCOUNT     16
#define DSHOT600_TICKS     80   // adjust for timing (80MHz APB clock)

// --- Mode & direction state ---
bool useDSHOT = false;      // false = PWM, true = DSHOT
bool motorForward = true;   // direction flag
int  lastModeButton = HIGH;
int  lastDirButton  = HIGH;
uint32_t lastModeToggleMs = 0;
uint32_t lastDirToggleMs  = 0;
const uint32_t debounceMs = 60;

// --- RMT channel ---
rmt_channel_t channel = RMT_CHANNEL_0;

// --- TFT setup ---
TFT_eSPI tft;

// --- UI helpers ---
void drawUI(const char* modeText, const char* dirText,
            const char* valueLabel, int value, int adcCounts) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 30);
  tft.println(modeText);
  tft.setCursor(10, 70);
  tft.printf("%s: %d", valueLabel, value);
  tft.setCursor(10, 110);
  tft.printf("Direction: %s", dirText);
  tft.setCursor(10, 150);
  tft.printf("ADC counts: %d", adcCounts);
}

void setupRMT() {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = channel;
  config.gpio_num = (gpio_num_t)escPin;
  config.mem_block_num = 1;
  config.clk_div = 1;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  rmt_config(&config);
  rmt_driver_install(channel, 0, 0);
}

void sendDSHOTPacket(uint16_t payload11bits, bool telemetry) {
  uint16_t packet = (payload11bits << 1) | (telemetry ? 1 : 0);
  uint8_t csum = 0;
  uint16_t csum_data = packet;
  for (int i = 0; i < 3; i++) {
    csum ^= (csum_data & 0xF);
    csum_data >>= 4;
  }
  csum &= 0xF;
  packet = (packet << 4) | csum;

  rmt_item32_t items[DSHOT_BITCOUNT];
  for (int i = DSHOT_BITCOUNT - 1; i >= 0; i--) {
    bool bit = (packet >> i) & 1;
    rmt_item32_t &it = items[DSHOT_BITCOUNT - 1 - i];
    it.level0 = 1;
    it.level1 = 0;
    if (bit) {
      it.duration0 = DSHOT600_TICKS * 3 / 4;
      it.duration1 = DSHOT600_TICKS / 4;
    } else {
      it.duration0 = DSHOT600_TICKS / 4;
      it.duration1 = DSHOT600_TICKS * 3 / 4;
    }
  }
  rmt_write_items(channel, items, DSHOT_BITCOUNT, true);
}

void sendDSHOTThrottle(int throttle) {
  throttle = constrain(throttle, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
  sendDSHOTPacket((uint16_t)throttle, false);
}

void sendDSHOTCommand(uint8_t command) {
  command = constrain(command, 0, 47);
  sendDSHOTPacket(command, false);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 0..4095

  // PWM init
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(escPin, pwmChannel);
  int dutyIdle = (int)((1000.0 / 20000.0) * 65535);
  ledcWrite(pwmChannel, dutyIdle);

  // RMT init
  setupRMT();

  // TFT init
  tft.init();
  tft.setRotation(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  // Buttons
  pinMode(INTERACTION_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIRECTION_BUTTON_PIN,   INPUT_PULLUP);

  drawUI("Mode: PWM", "Forward", "Throttle (us)", 1000, 0);
}

void loop() {
  // --- Buttons ---
  int modeBtn = digitalRead(INTERACTION_BUTTON_PIN);
  if (modeBtn == LOW && lastModeButton == HIGH && (millis() - lastModeToggleMs) > debounceMs) {
    useDSHOT = !useDSHOT;
    lastModeToggleMs = millis();
    Serial.print("Mode switched to: ");
    Serial.println(useDSHOT ? "DSHOT" : "PWM");
  }
  lastModeButton = modeBtn;

  int dirBtn = digitalRead(DIRECTION_BUTTON_PIN);
  if (dirBtn == LOW && lastDirButton == HIGH && (millis() - lastDirToggleMs) > debounceMs) {
    motorForward = !motorForward;
    lastDirToggleMs = millis();
    Serial.print("Direction toggled: ");
    Serial.println(motorForward ? "Forward" : "Reverse");
    if (useDSHOT) {
      for (int i = 0; i < 4; i++) {
        sendDSHOTCommand(20); // reverse toggle
        delay(3);
      }
    }
  }
  lastDirButton = dirBtn;

  // --- ADC read ---
  int adcCounts = analogRead(ADC_INPUT_PIN);

  if (!useDSHOT) {
    int throttleUs = map(adcCounts, 0, 4095, 1000, 2000);
    int duty = (int)((throttleUs / 20000.0) * 65535);
    ledcWrite(pwmChannel, duty);

    drawUI("Mode: PWM", motorForward ? "Forward" : "Reverse",
           "Throttle (us)", throttleUs, adcCounts);
    Serial.printf("PWM throttle: %d us, ADC: %d\n", throttleUs, adcCounts);

    delay(20);
  } else {
    int throttlePkt = map(adcCounts, 0, 4095, DSHOT_THROTTLE_MIN, DSHOT_THROTTLE_MAX);
    sendDSHOTThrottle(throttlePkt);

    drawUI("Mode: DSHOT", motorForward ? "Forward" : "Reverse",
           "Throttle (pkt)", throttlePkt, adcCounts);
    Serial.printf("DSHOT throttle: %d pkt, ADC: %d\n", throttlePkt, adcCounts);

    delay(2);
  }
}





*/