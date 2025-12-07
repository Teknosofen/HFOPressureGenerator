#include <Arduino.h>
#include "main.hpp"
#include "ImageRenderer.hpp"
#include "MotorController.hpp"
#include "StepperController.hpp"

// ESC + DRV8825 Stepper Controller
// ESC: PWM only (no DSHOT, no direction)
// Stepper: DRV8825 driver with STEP/DIR interface
// Display: Shows both motor statuses

// ===== PIN DEFINITIONS =====

// ESC Motor (Brushless) - PWM only
#define ADC_INPUT_PIN          2    // GPIO2, ADC for ESC throttle
const int ESC_PIN = 44;             // ESC signal pin

// Stepper Motor (DRV8825)
const int STEPPER_STEP_PIN = 10;    // DRV8825 STEP pin
const int STEPPER_DIR_PIN = 11;     // DRV8825 DIR pin
const int STEPPER_ENABLE_PIN = 12;  // DRV8825 ENABLE pin (active LOW)

// Optional: Microstepping control (set to -1 if hardwired)
const int STEPPER_M0_PIN = -1;      // M0 pin (-1 = not used, hardwired)
const int STEPPER_M1_PIN = -1;      // M1 pin (-1 = not used, hardwired)
const int STEPPER_M2_PIN = -1;      // M2 pin (-1 = not used, hardwired)

// Control Buttons
#define STEPPER_FWD_BUTTON    13    // Button to move stepper forward
#define STEPPER_REV_BUTTON    14    // Button to move stepper reverse
#define STEPPER_STOP_BUTTON   15    // Button to stop stepper
#define STEPPER_ENABLE_BUTTON 16    // Button to enable/disable stepper

// Analog Inputs
#define STEPPER_SPEED_POT     1   // Potentiometer for stepper speed (GPIO1, ADC)

// ===== BUTTON STATE =====
// Stepper buttons
int  lastStepperFwdButton = HIGH;
int  lastStepperRevButton = HIGH;
int  lastStepperStopButton = HIGH;
int  lastStepperEnableButton = HIGH;
uint32_t lastStepperFwdMs = 0;
uint32_t lastStepperRevMs = 0;
uint32_t lastStepperStopMs = 0;
uint32_t lastStepperEnableMs = 0;

const uint32_t debounceMs = 60;

// ===== TIMING =====
uint32_t lastUIUpdateMs = 0;
uint32_t lastStepperSpeedUpdate = 0;

// ===== OBJECTS =====
ImageRenderer display;
MotorController escMotor(ESC_PIN);

// Create DRV8825 Stepper Controller
// Parameters: STEP pin, DIR pin, ENABLE pin, steps/rev, M0, M1, M2
StepperController stepper(STEPPER_STEP_PIN, STEPPER_DIR_PIN, STEPPER_ENABLE_PIN, 
                         200, STEPPER_M0_PIN, STEPPER_M1_PIN, STEPPER_M2_PIN);

void setup() {
    Serial.begin(115200);
    analogReadResolution(12); // 0..4095
    delay(500);

    Serial.println("\n========================================");
    Serial.println("ESC + DRV8825 Stepper Controller");
    Serial.println("========================================");
    Serial.println("ESC: PWM only, GPIO 44");
    Serial.println("Stepper: DRV8825 on GPIO 10/11/12");
    Serial.println("========================================\n");

    // ===== ESC MOTOR INITIALIZATION =====
    escMotor.init();
    Serial.println("ESC initialized - waiting 2 seconds for arming...");
    delay(2000);  // Give ESC time to arm
    Serial.println("ESC armed and ready");
    
    // ===== STEPPER MOTOR INITIALIZATION =====
    stepper.init();
    
    // Configure stepper settings
    stepper.setSpeed(100.0);         // Start at 100 RPM
    stepper.setAcceleration(500.0);  // Smooth acceleration at 500 steps/sec²
    stepper.setMaxSpeed(2000.0);     // Limit max speed to 2000 steps/sec
    
    // Set microstepping mode if GPIO pins are configured
    // If M0/M1/M2 are hardwired, this will have no effect
    if (STEPPER_M0_PIN >= 0) {
        stepper.setMicrostepMode(FULL_STEP);  // Can change to HALF_STEP, QUARTER_STEP, etc.
        Serial.printf("Microstepping mode: 1/%d\n", stepper.getMicrostepMode());
    } else {
        Serial.println("Microstepping pins hardwired (not controlled by software)");
    }
    
    stepper.enable();                // Enable stepper motor
    Serial.println("Stepper initialized and enabled");
    
    // ===== DISPLAY INITIALIZATION =====
    display.init();
    Serial.println("Display initialized");
    
    // ===== BUTTON SETUP =====
    pinMode(STEPPER_FWD_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_REV_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_STOP_BUTTON, INPUT_PULLUP);
    pinMode(STEPPER_ENABLE_BUTTON, INPUT_PULLUP);
    
    Serial.println("\n========================================");
    Serial.println("CONTROLS:");
    Serial.println("========================================");
    Serial.println("ESC Motor:");
    Serial.printf("  GPIO%d pot: ESC throttle control\n", ADC_INPUT_PIN);
    Serial.println("\nStepper Motor:");
    Serial.printf("  GPIO%d pot: Stepper speed (10-600 RPM)\n", STEPPER_SPEED_POT);
    Serial.printf("  GPIO%d: Move forward (50 steps)\n", STEPPER_FWD_BUTTON);
    Serial.printf("  GPIO%d: Move reverse (50 steps)\n", STEPPER_REV_BUTTON);
    Serial.printf("  GPIO%d: Stop immediately\n", STEPPER_STOP_BUTTON);
    Serial.printf("  GPIO%d: Enable/Disable motor\n", STEPPER_ENABLE_BUTTON);
    Serial.println("========================================\n");
    Serial.println("System ready!\n");
}

void loop() {
    uint32_t nowMs = millis();

    // ===== STEPPER MOTOR - HIGHEST PRIORITY =====
    // Must be called continuously for non-blocking movement
    stepper.run();

    // ===== ESC MOTOR CONTROL =====
    int adcCounts = analogRead(ADC_INPUT_PIN);
    escMotor.setThrottleFromADC(adcCounts);

    // ===== STEPPER MOTOR BUTTON CONTROL =====
    
    // Forward button - Move 50 steps forward
    int stepperFwdBtn = digitalRead(STEPPER_FWD_BUTTON);
    if (stepperFwdBtn == LOW && lastStepperFwdButton == HIGH && (nowMs - lastStepperFwdMs) > debounceMs) {
        if (stepper.isEnabled()) {
            stepper.move(50);  // Relative movement
            Serial.printf("Stepper: Moving +50 steps (target: %ld)\n", stepper.getTargetPosition());
        } else {
            Serial.println("Stepper: Cannot move - motor disabled");
        }
        lastStepperFwdMs = nowMs;
    }
    lastStepperFwdButton = stepperFwdBtn;
    
    // Reverse button - Move 50 steps backward
    int stepperRevBtn = digitalRead(STEPPER_REV_BUTTON);
    if (stepperRevBtn == LOW && lastStepperRevButton == HIGH && (nowMs - lastStepperRevMs) > debounceMs) {
        if (stepper.isEnabled()) {
            stepper.move(-50);  // Relative movement
            Serial.printf("Stepper: Moving -50 steps (target: %ld)\n", stepper.getTargetPosition());
        } else {
            Serial.println("Stepper: Cannot move - motor disabled");
        }
        lastStepperRevMs = nowMs;
    }
    lastStepperRevButton = stepperRevBtn;
    
    // Stop button - Immediately stop motor
    int stepperStopBtn = digitalRead(STEPPER_STOP_BUTTON);
    if (stepperStopBtn == LOW && lastStepperStopButton == HIGH && (nowMs - lastStepperStopMs) > debounceMs) {
        stepper.stop();
        Serial.printf("Stepper: STOPPED at position %ld\n", stepper.getCurrentPosition());
        lastStepperStopMs = nowMs;
    }
    lastStepperStopButton = stepperStopBtn;
    
    // Enable/Disable button
    int stepperEnableBtn = digitalRead(STEPPER_ENABLE_BUTTON);
    if (stepperEnableBtn == LOW && lastStepperEnableButton == HIGH && (nowMs - lastStepperEnableMs) > debounceMs) {
        if (stepper.isEnabled()) {
            stepper.disable();
            Serial.println("Stepper: DISABLED (motor coasts, saves power)");
        } else {
            stepper.enable();
            Serial.println("Stepper: ENABLED (motor energized)");
        }
        lastStepperEnableMs = nowMs;
    }
    lastStepperEnableButton = stepperEnableBtn;
    
    // ===== STEPPER SPEED CONTROL FROM POTENTIOMETER =====
    if (nowMs - lastStepperSpeedUpdate > 100) {
        lastStepperSpeedUpdate = nowMs;
        
        int speedADC = analogRead(STEPPER_SPEED_POT);
        // Map ADC to speed range: 10-600 RPM
        float targetSpeed = map(speedADC, 0, 4095, 10, 600);
        Serial.println("Stepper speed ADC: " + String(speedADC) + " -> Target RPM: " + String(targetSpeed));
        // Only update if speed changed significantly (avoid jitter)
        static float lastSetSpeed = 0;
        if (abs(targetSpeed - lastSetSpeed) > 5.0) {
            stepper.setSpeed(targetSpeed);
            lastSetSpeed = targetSpeed;
            
            // Optional: Print speed changes
            // Serial.printf("Stepper speed set to: %.0f RPM\n", targetSpeed);
        }
    }

    // ===== UI UPDATE =====
    if (nowMs - lastUIUpdateMs >= 200) {
        display.drawHeader(HFO_GEN_LABEL, HFO_GEN_VerLbl);
        display.drawDualMotorUI(
            escMotor.getThrottleUs(),
            escMotor.getThrottleValue(),
            stepper.getCurrentPosition(),
            stepper.getTargetPosition(),
            stepper.getCurrentSpeed(),  // Now returns actual current speed (for acceleration)
            stepper.isRunning(),
            stepper.isEnabled()
        );
        
        // Print comprehensive status
        Serial.printf("[ESC: %4d us (ADC=%4d)] [Stepper: pos=%6ld/%6ld | speed=%5.0f RPM | dist=%5ld | %s | %s]\n",
                     escMotor.getThrottleUs(),
                     escMotor.getThrottleValue(),
                     stepper.getCurrentPosition(),
                     stepper.getTargetPosition(),
                     (stepper.getCurrentSpeed() * 60.0) / (200 * stepper.getMicrostepMode()), // Convert steps/sec to RPM
                     stepper.distanceToGo(),
                     stepper.isRunning() ? "MOVING" : "IDLE  ",
                     stepper.isEnabled() ? "ENABLED " : "DISABLED");
        
        lastUIUpdateMs = nowMs;
    }
}
