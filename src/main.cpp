#include <Arduino.h>
#include "main.hpp"
#include "ImageRenderer.hpp"
#include "MotorController.hpp"
#include "StepperController.hpp"

// Simplified ESC + Stepper Controller
// ESC: PWM only (no DSHOT, no direction)
// Display: Shows ADC value instead of direction

// ===== PIN DEFINITIONS =====

// ESC Motor (Brushless) - PWM only
#define ADC_INPUT_PIN          1    // GPIO1, ADC1_CH1 for ESC throttle
const int ESC_PIN = 44;             // ESC signal pin (adjust to your pin!)

// Stepper Motor (DRV8871)
const int STEPPER_IN1_PIN = 10;     // DRV8871 IN1
const int STEPPER_IN2_PIN = 11;     // DRV8871 IN2
#define STEPPER_FWD_BUTTON    12    // Button to move stepper forward
#define STEPPER_REV_BUTTON    13    // Button to move stepper reverse
#define STEPPER_ENABLE_BUTTON 15    // Button to enable/disable stepper
#define STEPPER_SPEED_POT     2     // Potentiometer for stepper speed (GPIO2, ADC1_CH2)

// ===== BUTTON STATE =====
// Stepper buttons
int  lastStepperFwdButton = HIGH;
int  lastStepperRevButton = HIGH;
int  lastStepperEnableButton = HIGH;
uint32_t lastStepperFwdMs = 0;
uint32_t lastStepperRevMs = 0;
uint32_t lastStepperEnableMs = 0;

const uint32_t debounceMs = 60;

// ===== TIMING =====
uint32_t lastUIUpdateMs = 0;
uint32_t lastStepperSpeedUpdate = 0;

// ===== OBJECTS =====
ImageRenderer display;
MotorController escMotor(ESC_PIN);
StepperController stepper(STEPPER_IN1_PIN, STEPPER_IN2_PIN, 200);

void setup() {
    Serial.begin(115200);
    analogReadResolution(12); // 0..4095

    Serial.println("=== ESC + Stepper Controller (Simplified) ===");
    Serial.println("ESC: PWM only, GPIO 44");
    Serial.println("Stepper: GPIO 10/11");

    // ESC Motor initialization
    escMotor.init();
    Serial.println("ESC initialized - waiting 2 seconds for arming...");
    delay(2000);  // Give ESC time to arm
    
    // Stepper Motor initialization
    stepper.init();
    stepper.setSpeed(100.0);         // Start at 100 RPM
    stepper.setStepMode(FULL_STEP);  // Full step mode
    stepper.enable();                // Enable stepper
    Serial.println("Stepper initialized");
    
    // Display initialization
    display.init();
    Serial.println("Display initialized");
    
    // Stepper Buttons
    pinMode(STEPPER_FWD_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_REV_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_ENABLE_BUTTON, INPUT_PULLUP);
    
    Serial.println("\nControls:");
    Serial.println("- GPIO1 pot: ESC throttle");
    Serial.println("- GPIO2 pot: Stepper speed");
    Serial.println("- GPIO12: Stepper forward");
    Serial.println("- GPIO13: Stepper reverse");
    Serial.println("- GPIO15: Stepper enable/disable");
    Serial.println("\nReady!\n");
}

void loop() {
    uint32_t nowMs = millis();

    // ===== STEPPER MOTOR - HIGH PRIORITY =====
    stepper.run();

    // ===== ESC MOTOR CONTROL =====
    int adcCounts = analogRead(ADC_INPUT_PIN);
    escMotor.setThrottleFromADC(adcCounts);

    // ===== STEPPER MOTOR CONTROL =====
    
    // Forward button
    int stepperFwdBtn = digitalRead(STEPPER_FWD_BUTTON);
    if (stepperFwdBtn == LOW && lastStepperFwdButton == HIGH && (nowMs - lastStepperFwdMs) > debounceMs) {
        stepper.incrementPosition(50);
        lastStepperFwdMs = nowMs;
        Serial.printf("Stepper: Forward to %ld\n", stepper.getTargetPosition());
    }
    lastStepperFwdButton = stepperFwdBtn;
    
    // Reverse button
    int stepperRevBtn = digitalRead(STEPPER_REV_BUTTON);
    if (stepperRevBtn == LOW && lastStepperRevButton == HIGH && (nowMs - lastStepperRevMs) > debounceMs) {
        stepper.decrementPosition(50);
        lastStepperRevMs = nowMs;
        Serial.printf("Stepper: Reverse to %ld\n", stepper.getTargetPosition());
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
    
    // Speed control from potentiometer
    if (nowMs - lastStepperSpeedUpdate > 100) {
        lastStepperSpeedUpdate = nowMs;
        
        int speedADC = analogRead(STEPPER_SPEED_POT);
        float targetSpeed = map(speedADC, 0, 4095, 10, 600);
        stepper.setSpeed(targetSpeed);
    }

    // ===== UI UPDATE =====
    if (nowMs - lastUIUpdateMs >= 200) {
        display.drawHeader(HFO_GEN_LABEL, HFO_GEN_VerLbl);
        display.drawDualMotorUI(
            escMotor.getThrottleUs(),
            escMotor.getThrottleValue(),
            stepper.getPosition(),
            stepper.getTargetPosition(),
            stepper.getSpeed(),
            stepper.isRunning(),
            stepper.isEnabled()
        );
        
        // Print status
        Serial.printf("ESC: %d us (ADC=%d) | Stepper: pos=%ld/%ld speed=%.0f RPM %s\n",
                     escMotor.getThrottleUs(),
                     escMotor.getThrottleValue(),
                     stepper.getPosition(),
                     stepper.getTargetPosition(),
                     stepper.getSpeed(),
                     stepper.isRunning() ? "MOVING" : "IDLE");
        
        lastUIUpdateMs = nowMs;
    }
}
