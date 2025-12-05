#include <Arduino.h>
#include "main.hpp"
#include "ImageRenderer.hpp"
#include "MotorController.hpp"

// in the TFT_eSPI TFT_eSPI_User_setup_Select.H ensure that the 
// line 133 #include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT

// In the TFT_eSPI_User_setup.H 
// line 55 #define ST7789_DRIVER      // Full configuration option, define additional parameters below for this display
// line 87 #define TFT_WIDTH  170 // ST7789 170 x 320
// line 92 #define TFT_HEIGHT 320 // ST7789 240 x 320

// --- Pins ---
#define INTERACTION_BUTTON_PIN 14   // GPIO14, Key 2: toggle PWM/DSHOT
#define DIRECTION_BUTTON_PIN   0    // GPIO0, Key 1: toggle forward/reverse
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1
const int ESC_PIN = 43;             // ESC signal pin (white wire)

// --- Button State ---
int  lastModeButton = HIGH;
int  lastDirButton  = HIGH;
uint32_t lastModeToggleMs = 0;
uint32_t lastDirToggleMs  = 0;
const uint32_t debounceMs = 60;

// --- Timing ---
uint32_t lastUIUpdateMs = 0;   // for 5 Hz screen refresh

// --- Objects ---
ImageRenderer display;
MotorController motor(ESC_PIN);

void setup() {
    Serial.begin(115200);
    analogReadResolution(12); // 0..4095

    // Motor initialization
    motor.init();
    
    // Display initialization
    display.init();
    display.setPositions(
        DisplayPos(10, 100),   // mode position
        DisplayPos(10, 120),   // value position
        DisplayPos(10, 140),  // direction position
        DisplayPos(10, 160)   // ADC position
    );
    
    // Buttons
    pinMode(INTERACTION_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DIRECTION_BUTTON_PIN, INPUT_PULLUP);
    
    // Initial display
    display.drawFullUI(motor.getModeString(), 
                       motor.getDirectionString(),
                       motor.getThrottleLabel(), 
                       1000, 
                       0);
}

void loop() {
  uint32_t nowMs = millis();

  // --- Mode toggle (PWM <-> DSHOT) ---
  int modeBtn = digitalRead(INTERACTION_BUTTON_PIN);
  if (modeBtn == LOW && lastModeButton == HIGH && (nowMs - lastModeToggleMs) > debounceMs) {
      MotorMode newMode = (motor.getMode() == MODE_PWM) ? MODE_DSHOT : MODE_PWM;
      motor.setMode(newMode);
      lastModeToggleMs = nowMs;
      
      Serial.print("Mode switched to: ");
      Serial.println(motor.getModeString());
  }
  lastModeButton = modeBtn;

  // --- Direction toggle (Forward <-> Reverse) ---
  int dirBtn = digitalRead(DIRECTION_BUTTON_PIN);
  if (dirBtn == LOW && lastDirButton == HIGH && (nowMs - lastDirToggleMs) > debounceMs) {
      motor.setDirection(!motor.isForward());
      lastDirToggleMs = nowMs;
      
      Serial.print("Direction toggled: ");
      Serial.println(motor.getDirectionString());
  }
  lastDirButton = dirBtn;

  // --- ADC read and motor control ---
  int adcCounts = analogRead(ADC_INPUT_PIN);
  motor.setThrottleFromADC(adcCounts);

  // --- UI update at 5 Hz (200ms) ---
  if (nowMs - lastUIUpdateMs >= 200) {
    display.drawHeader(HFO_GEN_LABEL, HFO_GEN_VerLbl);
    display.drawFullUI(motor.getModeString(),
                      motor.getDirectionString(),
                      motor.getThrottleLabel(),
                      motor.getThrottleValue(),
                      adcCounts);
    
    Serial.printf("%s %s: %d, ADC: %d\n",
                  motor.getModeString(),
                  motor.getThrottleLabel(),
                  motor.getThrottleValue(),
                  adcCounts);
    
    lastUIUpdateMs = nowMs;
  }
}