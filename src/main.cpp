#include <Arduino.h>
#include "main.hpp"
#include "ImageRendererEnhanced.hpp"
#include "MotorController.hpp"
#include "StepperController.hpp"

// in the TFT_eSPI TFT_eSPI_User_setup_Select.H ensure that the 
// line 133 #include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT

// In the TFT_eSPI_User_setup.H 
// line 55 #define ST7789_DRIVER      // Full configuration option, define additional parameters below for this display
// line 87 #define TFT_WIDTH  170 // ST7789 170 x 320
// line 92 #define TFT_HEIGHT 320 // ST7789 240 x 320

// ===== PIN DEFINITIONS =====

// ESC Motor (Brushless)
#define INTERACTION_BUTTON_PIN 14   // GPIO14, Key 2: toggle PWM/DSHOT
#define DIRECTION_BUTTON_PIN   0    // GPIO0, Key 1: toggle forward/reverse
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1 for ESC throttle
const int ESC_PIN = 44;             // ESC signal pin (white wire)

// Stepper Motor (DRV8871)
const int STEPPER_IN1_PIN = 10;     // DRV8871 IN1
const int STEPPER_IN2_PIN = 11;     // DRV8871 IN2
#define STEPPER_FWD_BUTTON    12    // Button to move stepper forward
#define STEPPER_REV_BUTTON    13    // Button to move stepper reverse
#define STEPPER_ENABLE_BUTTON 15    // Button to enable/disable stepper
#define STEPPER_SPEED_POT     2     // Potentiometer for stepper speed control (GPIO2, ADC1_CH2)

// ===== BUTTON STATE =====
// ESC buttons
int  lastModeButton = HIGH;
int  lastDirButton  = HIGH;
uint32_t lastModeToggleMs = 0;
uint32_t lastDirToggleMs  = 0;

// Stepper buttons
int  lastStepperFwdButton = HIGH;
int  lastStepperRevButton = HIGH;
int  lastStepperEnableButton = HIGH;
uint32_t lastStepperFwdMs = 0;
uint32_t lastStepperRevMs = 0;
uint32_t lastStepperEnableMs = 0;

const uint32_t debounceMs = 60;

// ===== TIMING =====
uint32_t lastUIUpdateMs = 0;        // for display refresh
uint32_t lastStepperSpeedUpdate = 0; // for stepper speed adjustment

// ===== OBJECTS =====
ImageRendererEnhanced display;
MotorController escMotor(ESC_PIN);
StepperController stepper(STEPPER_IN1_PIN, STEPPER_IN2_PIN, 200);  // 200 steps/rev NEMA 17

void setup() {
    Serial.begin(115200);
    analogReadResolution(12); // 0..4095

    // ESC Motor initialization
    escMotor.init();
    
    // Stepper Motor initialization
    stepper.init();
    stepper.setSpeed(100.0);         // Start at 100 RPM
    stepper.setStepMode(FULL_STEP);  // Full step mode
    stepper.enable();                // Enable stepper
    
    // Display initialization
    display.init();
    display.setPositions(
        DisplayPos(10, 100),   // mode position
        DisplayPos(10, 120),   // value position
        DisplayPos(10, 140),   // direction position
        DisplayPos(10, 160)    // ADC position
    );
    
    // ESC Buttons
    pinMode(INTERACTION_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DIRECTION_BUTTON_PIN, INPUT_PULLUP);
    
    // Stepper Buttons
    pinMode(STEPPER_FWD_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_REV_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_ENABLE_BUTTON, INPUT_PULLUP);
    
    // Initial display
    // display.drawFullUI(escMotor.getModeString(), 
                    //    escMotor.getDirectionString(),
                    //    escMotor.getThrottleLabel(), 
                    //    1000, 
                    //    0);
    
    Serial.println("=== Dual Motor Controller ===");
    Serial.println("ESC Motor (Brushless) + Stepper Motor");
    Serial.println("ESC: GPIO14=Mode, GPIO0=Direction, GPIO1=Throttle");
    Serial.println("Stepper: GPIO12=Forward, GPIO13=Reverse, GPIO15=Enable, GPIO2=Speed");
}

void loop() {
    uint32_t nowMs = millis();

    // ===== STEPPER MOTOR - HIGH PRIORITY =====
    // Call run() frequently for smooth stepper operation
    stepper.run();

    // ===== ESC MOTOR CONTROL =====
    
    // Mode toggle (PWM <-> DSHOT)
    int modeBtn = digitalRead(INTERACTION_BUTTON_PIN);
    if (modeBtn == LOW && lastModeButton == HIGH && (nowMs - lastModeToggleMs) > debounceMs) {
        MotorMode newMode = (escMotor.getMode() == MODE_PWM) ? MODE_DSHOT : MODE_PWM;
        escMotor.setMode(newMode);
        lastModeToggleMs = nowMs;
        
        Serial.print("ESC Mode: ");
        Serial.println(escMotor.getModeString());
    }
    lastModeButton = modeBtn;

    // Direction toggle
    int dirBtn = digitalRead(DIRECTION_BUTTON_PIN);
    if (dirBtn == LOW && lastDirButton == HIGH && (nowMs - lastDirToggleMs) > debounceMs) {
        escMotor.setDirection(!escMotor.isForward());
        lastDirToggleMs = nowMs;
        
        Serial.print("ESC Direction: ");
        Serial.println(escMotor.getDirectionString());
    }
    lastDirButton = dirBtn;

    // ADC read and ESC throttle control
    int adcCounts = analogRead(ADC_INPUT_PIN);
    escMotor.setThrottleFromADC(adcCounts);

    // ===== STEPPER MOTOR CONTROL =====
    
    // Forward button - increment position
    int stepperFwdBtn = digitalRead(STEPPER_FWD_BUTTON);
    if (stepperFwdBtn == LOW && lastStepperFwdButton == HIGH && (nowMs - lastStepperFwdMs) > debounceMs) {
        stepper.incrementPosition(50);  // Move 50 steps forward (90° for 200 step motor)
        lastStepperFwdMs = nowMs;
        Serial.printf("Stepper: Target position %ld\n", stepper.getTargetPosition());
    }
    lastStepperFwdButton = stepperFwdBtn;
    
    // Reverse button - decrement position
    int stepperRevBtn = digitalRead(STEPPER_REV_BUTTON);
    if (stepperRevBtn == LOW && lastStepperRevButton == HIGH && (nowMs - lastStepperRevMs) > debounceMs) {
        stepper.decrementPosition(50);  // Move 50 steps backward
        lastStepperRevMs = nowMs;
        Serial.printf("Stepper: Target position %ld\n", stepper.getTargetPosition());
    }
    lastStepperRevButton = stepperRevBtn;
    
    // Enable/Disable button
    int stepperEnableBtn = digitalRead(STEPPER_ENABLE_BUTTON);
    if (stepperEnableBtn == LOW && lastStepperEnableButton == HIGH && (nowMs - lastStepperEnableMs) > debounceMs) {
        if (stepper.isEnabled()) {
            stepper.disable();
            Serial.println("Stepper: DISABLED");
        } else {
            stepper.enable();
            Serial.println("Stepper: ENABLED");
        }
        lastStepperEnableMs = nowMs;
    }
    lastStepperEnableButton = stepperEnableBtn;
    
    // Speed control from potentiometer (update every 100ms)
    if (nowMs - lastStepperSpeedUpdate > 100) {
        lastStepperSpeedUpdate = nowMs;
        
        int speedADC = analogRead(STEPPER_SPEED_POT);
        // Map ADC to reasonable RPM range: 10-600 RPM
        float targetSpeed = map(speedADC, 0, 4095, 10, 600);
        stepper.setSpeed(targetSpeed);
    }

    // ===== UI UPDATE =====
    
    if (nowMs - lastUIUpdateMs >= 200) {
        display.drawHeader(HFO_GEN_LABEL, HFO_GEN_VerLbl);
        // display.drawFullUI(escMotor.getModeString(),
                        //   escMotor.getDirectionString(),
                        //   escMotor.getThrottleLabel(),
                        //   escMotor.getThrottleValue(),
                        //   adcCounts);
        display.drawDualMotorUI(
            escMotor.getModeString(),
            escMotor.getDirectionString(),
            escMotor.getThrottleValue(),
            stepper.getPosition(),
            stepper.getTargetPosition(),
            stepper.getSpeed(),
            stepper.isRunning(),
            stepper.isEnabled()
        );
        // Print status of both motors
        Serial.printf("ESC: %s %s=%d ADC=%d | Stepper: pos=%ld/%ld speed=%.0f RPM %s %s\n",
                     escMotor.getModeString(),
                     escMotor.getThrottleLabel(),
                     escMotor.getThrottleValue(),
                     adcCounts,
                     stepper.getPosition(),
                     stepper.getTargetPosition(),
                     stepper.getSpeed(),
                     stepper.isRunning() ? "MOVING" : "IDLE",
                     stepper.isEnabled() ? "ENABLED" : "DISABLED");
        
        lastUIUpdateMs = nowMs;
    }
} 
