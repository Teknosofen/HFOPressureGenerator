#include <Arduino.h>
#include "StepperController.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// DRV8825 Stepper Motor Control
const int STEP_PIN = 10;        // STEP pulse pin
const int DIR_PIN = 11;         // Direction pin
const int ENABLE_PIN = 12;      // Enable pin (active LOW)

// Optional: Microstepping control pins (can be hardwired instead)
const int M0_PIN = 13;          // Microstepping M0
const int M1_PIN = 14;          // Microstepping M1
const int M2_PIN = 15;          // Microstepping M2

// Control buttons
const int BTN_FWD = 16;         // Move forward button
const int BTN_REV = 17;         // Move backward button
const int BTN_STOP = 18;        // Stop button
const int BTN_MODE = 19;        // Change microstepping mode

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

// Create stepper controller
// NEMA 17 typically has 200 steps/rev (1.8° per step)
StepperController stepper(STEP_PIN, DIR_PIN, ENABLE_PIN, 200, M0_PIN, M1_PIN, M2_PIN);

// ============================================================================
// VARIABLES
// ============================================================================

unsigned long lastButtonCheck = 0;
const unsigned long BUTTON_DEBOUNCE = 50;
int currentModeIndex = 0;
MicrostepMode modes[] = {FULL_STEP, HALF_STEP, QUARTER_STEP, EIGHTH_STEP, SIXTEENTH_STEP, THIRTYTWO_STEP};
const char* modeNames[] = {"FULL", "1/2", "1/4", "1/8", "1/16", "1/32"};

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("DRV8825 Stepper Motor Controller");
    Serial.println("========================================\n");
    
    // Initialize stepper
    stepper.init();
    
    // Configure initial settings
    stepper.setSpeed(60.0);              // 60 RPM
    stepper.setAcceleration(500.0);      // 500 steps/sec² acceleration
    stepper.setMaxSpeed(2000.0);         // Maximum 2000 steps/sec
    stepper.setMicrostepMode(FULL_STEP); // Start with full step
    stepper.enable();                    // Enable motor
    
    // Setup buttons
    pinMode(BTN_FWD, INPUT_PULLUP);
    pinMode(BTN_REV, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_MODE, INPUT_PULLUP);
    
    Serial.println("\nControls:");
    Serial.println("  BTN_FWD  - Move 200 steps forward (1 revolution)");
    Serial.println("  BTN_REV  - Move 200 steps backward (1 revolution)");
    Serial.println("  BTN_STOP - Stop motor");
    Serial.println("  BTN_MODE - Cycle microstepping modes");
    Serial.println("\nReady!\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // CRITICAL: Call run() continuously for non-blocking movement
    stepper.run();
    
    // Handle buttons
    handleButtons();
    
    // Print status periodically
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        printStatus();
    }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtons() {
    if (millis() - lastButtonCheck < BUTTON_DEBOUNCE) return;
    
    // Forward button
    if (digitalRead(BTN_FWD) == LOW) {
        lastButtonCheck = millis();
        int stepsPerRev = 200 * stepper.getMicrostepMode();
        stepper.move(stepsPerRev);  // Move 1 revolution forward
        Serial.printf("Moving forward 1 revolution (%d steps)\n", stepsPerRev);
        delay(200);  // Prevent multiple triggers
    }
    
    // Reverse button
    if (digitalRead(BTN_REV) == LOW) {
        lastButtonCheck = millis();
        int stepsPerRev = 200 * stepper.getMicrostepMode();
        stepper.move(-stepsPerRev);  // Move 1 revolution backward
        Serial.printf("Moving backward 1 revolution (%d steps)\n", stepsPerRev);
        delay(200);
    }
    
    // Stop button
    if (digitalRead(BTN_STOP) == LOW) {
        lastButtonCheck = millis();
        stepper.stop();
        Serial.println("Motor stopped");
        delay(200);
    }
    
    // Mode change button
    if (digitalRead(BTN_MODE) == LOW) {
        lastButtonCheck = millis();
        currentModeIndex = (currentModeIndex + 1) % 6;
        stepper.setMicrostepMode(modes[currentModeIndex]);
        Serial.printf("Microstepping mode: %s\n", modeNames[currentModeIndex]);
        delay(200);
    }
}

// ============================================================================
// STATUS DISPLAY
// ============================================================================

void printStatus() {
    if (stepper.isRunning()) {
        Serial.printf("Position: %ld / Target: %ld | Distance: %ld | Speed: %.1f steps/sec\n",
                     stepper.getCurrentPosition(),
                     stepper.getTargetPosition(),
                     stepper.distanceToGo(),
                     stepper.getCurrentSpeed());
    }
}

// ============================================================================
// EXAMPLE FUNCTIONS (uncomment in setup() to use)
// ============================================================================

void example_BasicMovement() {
    Serial.println("\n=== Basic Movement Example ===");
    
    stepper.enable();
    stepper.setSpeed(60.0);  // 60 RPM
    
    // Move 1 revolution forward
    Serial.println("Moving forward 1 revolution...");
    stepper.move(200);  // 200 steps = 1 rev for NEMA 17
    stepper.runToPosition();  // Block until complete
    
    delay(1000);
    
    // Move back to start
    Serial.println("Moving back to start...");
    stepper.moveTo(0);
    stepper.runToPosition();
    
    Serial.println("Done!");
}

void example_SpeedTest() {
    Serial.println("\n=== Speed Test Example ===");
    
    stepper.enable();
    
    float testSpeeds[] = {30, 60, 120, 240, 480};
    
    for (float rpm : testSpeeds) {
        Serial.printf("\nTesting speed: %.0f RPM\n", rpm);
        stepper.setSpeed(rpm);
        stepper.move(200);  // 1 revolution
        stepper.runToPosition();
        delay(1000);
    }
    
    Serial.println("Speed test complete!");
}

void example_MicrosteppingComparison() {
    Serial.println("\n=== Microstepping Comparison ===");
    
    stepper.enable();
    stepper.setSpeed(60.0);
    
    MicrostepMode testModes[] = {FULL_STEP, HALF_STEP, QUARTER_STEP, EIGHTH_STEP};
    const char* names[] = {"Full Step", "Half Step", "Quarter Step", "Eighth Step"};
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("\nMode: %s\n", names[i]);
        stepper.setMicrostepMode(testModes[i]);
        
        // Move 1 revolution (adjusted for microstepping)
        int steps = 200 * testModes[i];
        stepper.move(steps);
        stepper.runToPosition();
        
        delay(1000);
        
        // Return to start
        stepper.move(-steps);
        stepper.runToPosition();
        
        delay(1000);
    }
    
    Serial.println("Microstepping test complete!");
}

void example_AccelerationDemo() {
    Serial.println("\n=== Acceleration Demo ===");
    
    stepper.enable();
    stepper.setSpeed(240.0);  // High speed
    
    // Without acceleration
    Serial.println("\nMoving without acceleration (instant start)...");
    stepper.setAcceleration(0);
    stepper.move(400);
    stepper.runToPosition();
    delay(1000);
    
    // Return to start
    stepper.moveTo(0);
    stepper.runToPosition();
    delay(2000);
    
    // With acceleration
    Serial.println("\nMoving with acceleration (smooth start/stop)...");
    stepper.setAcceleration(800.0);  // 800 steps/sec²
    stepper.move(400);
    stepper.runToPosition();
    delay(1000);
    
    // Return to start
    stepper.moveTo(0);
    stepper.runToPosition();
    
    Serial.println("Acceleration demo complete!");
}

void example_ContinuousRotation() {
    Serial.println("\n=== Continuous Rotation Example ===");
    Serial.println("Motor will rotate continuously. Press BTN_STOP to stop.");
    
    stepper.enable();
    stepper.setSpeed(120.0);  // 120 RPM
    stepper.setAcceleration(1000.0);
    
    // Set a very far target position to run "forever"
    stepper.moveTo(1000000);
    
    // Run continuously in main loop until stopped
    while (stepper.isRunning()) {
        stepper.run();
        
        // Check stop button
        if (digitalRead(BTN_STOP) == LOW) {
            stepper.stop();
            Serial.println("Stopped by user");
            delay(200);
            break;
        }
        
        yield();
    }
    
    Serial.println("Continuous rotation stopped");
}

void example_PositionControl() {
    Serial.println("\n=== Position Control Example ===");
    
    stepper.enable();
    stepper.setSpeed(120.0);
    stepper.setAcceleration(500.0);
    
    // Define waypoints
    long waypoints[] = {0, 200, 600, 800, 400, 0};
    
    Serial.println("Moving through waypoints:");
    for (long pos : waypoints) {
        Serial.printf("  Moving to position %ld\n", pos);
        stepper.runToNewPosition(pos);
        delay(500);
    }
    
    Serial.println("Position control demo complete!");
}

// ============================================================================
// SERIAL COMMAND INTERFACE (optional)
// ============================================================================

void serialCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 'e':  // Enable
                stepper.enable();
                Serial.println("Motor enabled");
                break;
            case 'd':  // Disable
                stepper.disable();
                Serial.println("Motor disabled");
                break;
            case 'f':  // Move forward
                stepper.move(200);
                Serial.println("Moving forward");
                break;
            case 'r':  // Move reverse
                stepper.move(-200);
                Serial.println("Moving reverse");
                break;
            case 's':  // Stop
                stepper.stop();
                Serial.println("Stopped");
                break;
            case 'h':  // Home (reset position)
                stepper.setCurrentPosition(0);
                Serial.println("Position reset to 0");
                break;
            case '?':  // Status
                printStatus();
                break;
        }
    }
}
