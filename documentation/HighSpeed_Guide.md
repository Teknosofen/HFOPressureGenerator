# High-Speed Operation Guide for NEMA 17 Steppers

## Speed Capabilities

### NEMA 17 Typical Specifications
- **Standard Speed**: 0-600 RPM (reliable for most applications)
- **High Speed**: 600-1200 RPM (requires optimization)
- **Maximum Speed**: Up to 1500 RPM (motor and driver dependent)

The StepperController now supports speeds up to **1500 RPM**.

## Achieving 600+ RPM

### 1. Hardware Requirements

#### Power Supply
```
Voltage matters for high speed!
- Low speed (0-300 RPM): 12V adequate
- Medium speed (300-600 RPM): 24V recommended
- High speed (600+ RPM): 24-36V optimal

Higher voltage = faster current rise = higher speed capability
```

#### DRV8871 Considerations
```
DRV8871 specs:
- Max voltage: 45V
- PWM frequency: 20 kHz (already set)
- Current limit: 3.3A peak

For NEMA 17 at high speed:
- Use 24V or higher supply
- Ensure adequate cooling (heatsink recommended)
- Monitor driver temperature
```

#### Motor Selection
```
Not all NEMA 17s are equal for high speed:

LOW INDUCTANCE (better for high speed):
- Inductance: 2-4 mH
- Resistance: 1-2 Ω
- Holding torque: 40-50 Ncm
- Example: 17HS4401, 17HS8401

HIGH INDUCTANCE (better for low-speed torque):
- Inductance: 8-15 mH
- Resistance: 3-8 Ω
- Holding torque: 60+ Ncm
- Better for <300 RPM
```

### 2. Software Configuration

#### Basic High-Speed Setup
```cpp
StepperController stepper(10, 11, 200);

void setup() {
    stepper.init();
    
    // High-speed configuration
    stepper.setSpeed(600.0);       // 600 RPM
    stepper.setStepMode(FULL_STEP); // Full step for speed
    stepper.enable();
}

void loop() {
    stepper.run();  // Call frequently for timing
    // Avoid long delays in loop!
}
```

#### Very High Speed (1000+ RPM)
```cpp
void setupHighSpeed() {
    stepper.init();
    stepper.setSpeed(1200.0);      // 1200 RPM
    stepper.setStepMode(FULL_STEP); // Half-step too slow
    stepper.enable();
    
    // At 1200 RPM with 200 steps:
    // - 4000 steps/sec
    // - 250 microseconds per step
    // - Need tight timing!
}
```

### 3. Code Optimization for High Speed

#### Critical: Minimize Loop Time
```cpp
void loop() {
    // GOOD: Tight loop for high-speed stepping
    stepper.run();
    
    // Read inputs less frequently
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 10) {  // Check every 10ms
        handleButtons();
        lastCheck = millis();
    }
}

void badLoop() {
    // BAD: Long operations block timing
    stepper.run();
    delay(100);           // ❌ Ruins timing
    Serial.println();     // ❌ Takes too long at high speed
    display.update();     // ❌ May cause missed steps
}
```

#### Optimize Display Updates
```cpp
void loop() {
    stepper.run();  // Always prioritize this
    
    // Update display infrequently
    static uint32_t lastUI = 0;
    if (millis() - lastUI > 200) {  // Only every 200ms
        display.drawFullUI(...);
        lastUI = millis();
    }
}
```

#### Use Task Separation (Advanced)
```cpp
// Run stepper on Core 0, UI on Core 1
void stepperTask(void* parameter) {
    while(1) {
        stepper.run();
        delayMicroseconds(10);  // Minimal delay
    }
}

void setup() {
    stepper.init();
    stepper.setSpeed(1000.0);
    stepper.enable();
    
    // Create high-priority task on Core 0
    xTaskCreatePinnedToCore(
        stepperTask,
        "StepperTask",
        2048,
        NULL,
        10,  // High priority
        NULL,
        0    // Core 0
    );
}
```

### 4. Speed vs Torque Trade-offs

```
Speed (RPM)    Torque      Applications
-----------    ------      ------------
0-300          100%        Positioning, heavy loads
300-600        70-80%      General purpose, moderate loads
600-1000       40-50%      Fast positioning, light loads
1000-1500      20-30%      High speed, minimal load

Key Point: Torque drops dramatically with speed!
```

### 5. Acceleration/Ramping (Important for High Speed)

For speeds above 600 RPM, you should implement acceleration:

```cpp
void rampToSpeed(float targetRPM) {
    float currentRPM = 100.0;  // Start slow
    stepper.setSpeed(currentRPM);
    
    stepper.step(10000);  // Large move
    
    while(stepper.isRunning()) {
        stepper.run();
        
        // Gradually increase speed
        if (currentRPM < targetRPM) {
            currentRPM += 10.0;  // Increment by 10 RPM
            stepper.setSpeed(currentRPM);
            delay(10);  // Short delay between increments
        }
    }
}

// Usage
void highSpeedMove() {
    stepper.setSpeed(100.0);  // Start slow
    stepper.step(1000);
    
    // Ramp up while moving
    rampToSpeed(1000.0);
}
```

### 6. Microstepping Considerations

```
At high speeds:
- FULL_STEP: Fastest, recommended for 600+ RPM
- HALF_STEP: Max ~400 RPM before timing issues

Why?
- Half-step doubles the step rate
- At 1000 RPM with half-step = 6666 steps/sec
- Step period = 150 microseconds (challenging)
```

### 7. Practical Speed Limits

#### With Standard ESP32 (240 MHz)
```
Theoretical maximum with tight loop:
- Full-step: ~2000 RPM (limited by motor, not MCU)
- Half-step: ~1000 RPM (timing becomes tight)

Practical maximum with other code running:
- Full-step: ~1500 RPM
- Half-step: ~600 RPM

With display updates and serial:
- Full-step: ~1000 RPM
- Half-step: ~400 RPM
```

### 8. Testing Procedure

Start slow and work up:

```cpp
void speedTest() {
    stepper.enable();
    
    int testSpeeds[] = {100, 300, 600, 900, 1200};
    
    for(int rpm : testSpeeds) {
        Serial.printf("Testing %d RPM\n", rpm);
        
        stepper.setSpeed(rpm);
        stepper.step(200);  // 1 revolution
        
        while(stepper.isRunning()) {
            stepper.run();
        }
        
        delay(1000);  // Pause between tests
        
        // Check for issues:
        // - Motor stalling
        // - Excessive vibration
        // - Driver overheating
        // - Missed steps
    }
}
```

### 9. Troubleshooting High-Speed Issues

**Motor stalls at high speed:**
- Increase supply voltage (24V better than 12V)
- Reduce load/inertia
- Implement acceleration ramping
- Check for mechanical binding

**Inconsistent speed/missed steps:**
- Loop() taking too long - profile your code
- Serial.print() calls - reduce frequency
- Display updates - make less frequent
- Add task separation on different cores

**Driver overheating:**
- Add heatsink to DRV8871
- Reduce current if possible
- Improve airflow
- Consider driver upgrade (e.g., TMC2209)

**Excessive vibration:**
- Try different speed (may hit resonance)
- Add damper to motor shaft
- Microstepping helps (but limits max speed)
- Check motor mounting

### 10. Recommended Settings for Different Speed Ranges

```cpp
// Low speed, high torque (positioning)
stepper.setSpeed(100.0);
stepper.setStepMode(HALF_STEP);  // Smooth
// Update display every 100ms OK

// Medium speed (general purpose)
stepper.setSpeed(400.0);
stepper.setStepMode(FULL_STEP);
// Update display every 200ms

// High speed (rapid movements)
stepper.setSpeed(800.0);
stepper.setStepMode(FULL_STEP);
// Update display every 500ms
// Minimize serial output

// Maximum speed (if achievable)
stepper.setSpeed(1200.0);
stepper.setStepMode(FULL_STEP);
// Dedicated task on separate core
// No display updates during motion
```

## Summary

**Yes, 600+ RPM is absolutely possible with NEMA 17 steppers!**

Key requirements:
1. ✅ **24V+ power supply** (most important)
2. ✅ **Low inductance motor** (2-4 mH ideal)
3. ✅ **Tight loop timing** (call run() frequently)
4. ✅ **Full-step mode** (half-step limits speed)
5. ✅ **Acceleration ramping** (for >600 RPM)
6. ✅ **Minimal blocking code** (reduce delays and prints)

The updated StepperController now supports up to 1500 RPM. Test your specific motor and power supply to find the optimal speed for your application!
