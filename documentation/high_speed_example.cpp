#include <Arduino.h>
#include "StepperController.h"

// High-speed optimized example for NEMA 17 at 600+ RPM

// --- Pin Definitions ---
const int STEPPER_IN1_PIN = 10;
const int STEPPER_IN2_PIN = 11;
const int SPEED_POT_PIN = 1;        // Potentiometer for speed control
const int START_BUTTON = 12;
const int STOP_BUTTON = 13;

// --- Objects ---
StepperController stepper(STEPPER_IN1_PIN, STEPPER_IN2_PIN, 200);

// --- State ---
bool isRunning = false;
uint32_t lastSpeedUpdate = 0;
uint32_t lastStatusPrint = 0;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    
    // Stepper initialization for high speed
    stepper.init();
    stepper.setStepMode(FULL_STEP);  // Full-step for maximum speed
    stepper.setSpeed(600.0);          // Start at 600 RPM
    
    // Buttons
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(STOP_BUTTON, INPUT_PULLUP);
    
    Serial.println("High-Speed NEMA 17 Controller");
    Serial.println("Speed range: 100-1200 RPM");
    Serial.println("Use potentiometer to control speed");
    Serial.println("Press START to begin continuous rotation");
}

void loop() {
    // ===== CRITICAL: Call run() as frequently as possible =====
    stepper.run();
    
    // ===== Button handling (check less frequently) =====
    static uint32_t lastButtonCheck = 0;
    if (millis() - lastButtonCheck > 50) {  // Check every 50ms
        lastButtonCheck = millis();
        
        if (digitalRead(START_BUTTON) == LOW && !isRunning) {
            isRunning = true;
            stepper.enable();
            // Set a large target for continuous rotation
            stepper.step(100000);  // Move 500 revolutions
            Serial.println("Started!");
        }
        
        if (digitalRead(STOP_BUTTON) == LOW && isRunning) {
            isRunning = false;
            stepper.stop();
            stepper.disable();
            Serial.println("Stopped!");
        }
    }
    
    // ===== Speed control from potentiometer =====
    if (millis() - lastSpeedUpdate > 100) {  // Update speed every 100ms
        lastSpeedUpdate = millis();
        
        int adcValue = analogRead(SPEED_POT_PIN);
        // Map ADC to 100-1200 RPM
        float targetSpeed = map(adcValue, 0, 4095, 100, 1200);
        stepper.setSpeed(targetSpeed);
    }
    
    // ===== Status printing (very infrequent to not affect timing) =====
    if (millis() - lastStatusPrint > 1000) {  // Once per second
        lastStatusPrint = millis();
        
        Serial.printf("Speed: %.0f RPM | Position: %ld | %s\n",
                     stepper.getSpeed(),
                     stepper.getPosition(),
                     isRunning ? "RUNNING" : "STOPPED");
    }
}


// ===== ALTERNATIVE: DUAL CORE APPROACH =====
/*
// Use FreeRTOS to run stepper on dedicated core for maximum performance

TaskHandle_t stepperTaskHandle;

void stepperTask(void* parameter) {
    while(1) {
        stepper.run();
        // Minimal delay - stepper gets maximum CPU time
        delayMicroseconds(10);
    }
}

void setup() {
    Serial.begin(115200);
    
    stepper.init();
    stepper.setStepMode(FULL_STEP);
    stepper.setSpeed(1000.0);  // Can go higher with dedicated core
    stepper.enable();
    stepper.step(100000);
    
    // Create high-priority task on Core 0
    xTaskCreatePinnedToCore(
        stepperTask,           // Task function
        "StepperTask",         // Name
        4096,                  // Stack size
        NULL,                  // Parameters
        10,                    // Priority (high)
        &stepperTaskHandle,    // Task handle
        0                      // Core 0
    );
    
    Serial.println("Stepper running on dedicated core at 1000+ RPM");
}

void loop() {
    // This loop runs on Core 1
    // Can handle UI, serial, buttons without affecting stepper timing
    
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        Serial.printf("Position: %ld, Speed: %.0f RPM\n",
                     stepper.getPosition(),
                     stepper.getSpeed());
    }
    
    // Handle buttons, display, etc. here
    delay(10);  // This delay won't affect stepper timing!
}
*/


// ===== SPEED RAMPING EXAMPLE =====
/*
void rampToHighSpeed() {
    stepper.enable();
    stepper.setSpeed(100.0);  // Start at low speed
    stepper.step(10000);      // Large move
    
    float currentSpeed = 100.0;
    float targetSpeed = 1000.0;
    float acceleration = 20.0;  // RPM per update
    
    while(stepper.isRunning()) {
        stepper.run();
        
        // Ramp up speed gradually
        static uint32_t lastRamp = 0;
        if (millis() - lastRamp > 50 && currentSpeed < targetSpeed) {
            lastRamp = millis();
            currentSpeed += acceleration;
            stepper.setSpeed(currentSpeed);
            Serial.printf("Ramping: %.0f RPM\n", currentSpeed);
        }
    }
    
    Serial.println("Ramp complete!");
}
*/


// ===== SPEED TEST SEQUENCE =====
/*
void runSpeedTest() {
    int testSpeeds[] = {100, 300, 600, 900, 1200};
    
    Serial.println("Starting speed test sequence...");
    
    for(int testSpeed : testSpeeds) {
        Serial.printf("\n=== Testing %d RPM ===\n", testSpeed);
        
        stepper.enable();
        stepper.setSpeed(testSpeed);
        stepper.step(200);  // One full revolution
        
        uint32_t startTime = millis();
        long startPos = stepper.getPosition();
        
        while(stepper.isRunning()) {
            stepper.run();
        }
        
        uint32_t elapsed = millis() - startTime;
        long stepsCompleted = stepper.getPosition() - startPos;
        
        // Calculate actual speed achieved
        float actualRPM = (stepsCompleted * 60000.0) / (200.0 * elapsed);
        
        Serial.printf("Target: %d RPM, Actual: %.1f RPM, Time: %lu ms\n",
                     testSpeed, actualRPM, elapsed);
        
        if (abs(actualRPM - testSpeed) > testSpeed * 0.1) {
            Serial.println("WARNING: Speed deviation >10%");
        }
        
        delay(2000);  // Pause between tests
    }
    
    Serial.println("\nSpeed test complete!");
    stepper.disable();
}
*/


// ===== CONTINUOUS HIGH-SPEED ROTATION =====
/*
void continuousHighSpeed() {
    stepper.enable();
    stepper.setSpeed(800.0);  // 800 RPM continuous
    
    // Set very large target (essentially infinite rotation)
    stepper.moveTo(1000000);  // 5000 revolutions
    
    Serial.println("Continuous rotation at 800 RPM");
    Serial.println("Press any key to stop...");
    
    while(!Serial.available()) {
        stepper.run();
        
        // Print status occasionally
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint > 2000) {
            lastPrint = millis();
            float revolutions = stepper.getPosition() / 200.0;
            Serial.printf("Revolutions: %.1f\n", revolutions);
        }
    }
    
    // Deceleration
    Serial.println("Decelerating...");
    for(int rpm = 800; rpm >= 100; rpm -= 50) {
        stepper.setSpeed(rpm);
        delay(100);
    }
    
    stepper.stop();
    stepper.disable();
    Serial.println("Stopped");
}
*/
