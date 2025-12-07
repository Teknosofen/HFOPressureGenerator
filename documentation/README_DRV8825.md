# DRV8825 Stepper Motor Controller for ESP32

Complete stepper motor control library for ESP32 using the DRV8825 driver. This implementation provides professional-grade control with microstepping, acceleration, and both blocking and non-blocking operation modes.

## Hardware Overview

### DRV8825 Driver Features
- **Voltage Range**: 8.2V - 45V motor supply
- **Current Rating**: Up to 2.2A per coil (with proper cooling)
- **Microstepping**: 1, 1/2, 1/4, 1/8, 1/16, 1/32 steps
- **Interface**: Simple STEP/DIR control
- **Protection**: Overcurrent, undervoltage lockout, thermal shutdown

### Wiring Diagram

```
ESP32              DRV8825              NEMA 17 Motor
─────              ───────              ─────────────
GPIO 10 ───────→ STEP
GPIO 11 ───────→ DIR
GPIO 12 ───────→ !ENABLE
GPIO 13 ───────→ M0         (optional)
GPIO 14 ───────→ M1         (optional)
GPIO 15 ───────→ M2         (optional)
                 
GND ────────────→ GND ←───────────────── GND
                 VMOT ←──────────────── +12-24V PSU
                 
                 A1 ────────────────────→ Coil A+
                 A2 ────────────────────→ Coil A-
                 B1 ────────────────────→ Coil B+
                 B2 ────────────────────→ Coil B-
                 
                 !FAULT ─────────────────→ (optional LED)
                 !SLEEP ─────────┐
                 !RESET ─────────┴──────→ VCC (3.3V or 5V)
```

### Important Hardware Notes

1. **Decoupling Capacitor**: ALWAYS connect a 100µF electrolytic capacitor between VMOT and GND, as close to the driver as possible. This is CRITICAL for stability.

2. **Heatsink**: The DRV8825 WILL overheat at currents above 1A. Use a heatsink!

3. **Current Limiting**: Adjust the potentiometer on the DRV8825 to set motor current:
   - Measure voltage (Vref) on the potentiometer
   - Current Limit = Vref / (5 × Rsense)
   - For DRV8825 with 0.1Ω sense resistors: I = Vref × 2
   - For NEMA 17 (typically 1.7A rated): Set Vref to ~0.85V

4. **Power Supply**: 
   - 12V minimum for decent speed
   - 24V recommended for high-speed operation (600+ RPM)
   - 36V for maximum performance

5. **Microstepping Pins**:
   - Can be controlled by GPIO (as shown) for software control
   - OR hardwired with jumpers for fixed mode
   - Leave floating or tie to GND/VCC as needed

### Microstepping Mode Settings

| Mode | M0 | M1 | M2 | Steps/Rev* |
|------|----|----|----|-----------:|
| Full | L  | L  | L  |        200 |
| 1/2  | H  | L  | L  |        400 |
| 1/4  | L  | H  | L  |        800 |
| 1/8  | H  | H  | L  |      1,600 |
| 1/16 | L  | L  | H  |      3,200 |
| 1/32 | H  | H  | H  |      6,400 |

*For standard 200 step/rev motor (1.8° step angle)

## Software Features

### Core Capabilities
- ✅ **Simple STEP/DIR interface** - Much cleaner than H-bridge control
- ✅ **Hardware microstepping** - Up to 1/32 step resolution
- ✅ **Speed control in RPM** - Easy to understand units
- ✅ **Position tracking** - Accurate step counting
- ✅ **Non-blocking operation** - Doesn't halt your code
- ✅ **Blocking mode available** - When you need it
- ✅ **Acceleration/deceleration** - Smooth starts and stops
- ✅ **Direction control** - Easy forward/reverse
- ✅ **Enable/disable control** - Power saving and safety

### Performance
- **Maximum step rate**: ~20 kHz (depends on ESP32 load)
- **Speed range**: 0.1 - 3000+ RPM (motor dependent)
- **Position range**: ±2,147,483,647 steps (32-bit long)
- **Acceleration**: Configurable in steps/sec²

## Installation

1. Copy `StepperController.h` and `StepperController.cpp` to your project
2. Include in your main file:
   ```cpp
   #include "StepperController.h"
   ```

## Basic Usage

### 1. Simple Movement Example

```cpp
#include "StepperController.h"

// Create controller (STEP pin, DIR pin, ENABLE pin, steps/rev)
StepperController stepper(10, 11, 12, 200);

void setup() {
    Serial.begin(115200);
    
    // Initialize
    stepper.init();
    stepper.setSpeed(60.0);  // 60 RPM
    stepper.enable();
    
    // Move 1 revolution forward (blocking)
    stepper.move(200);
    stepper.runToPosition();
    
    Serial.println("Movement complete!");
}

void loop() {
    // Nothing needed for blocking mode
}
```

### 2. Non-Blocking Movement

```cpp
void setup() {
    stepper.init();
    stepper.setSpeed(120.0);  // 120 RPM
    stepper.enable();
    
    // Set target position
    stepper.moveTo(1000);  // Move to position 1000
}

void loop() {
    // CRITICAL: Must call run() continuously
    stepper.run();
    
    // Check if still moving
    if (stepper.isRunning()) {
        Serial.println("Moving...");
    } else {
        Serial.println("Reached target");
        delay(1000);
        
        // Set new target
        stepper.moveTo(0);
    }
}
```

### 3. With Microstepping Control

```cpp
// Create with microstepping pins
StepperController stepper(10, 11, 12, 200, 13, 14, 15);
//                        ^   ^   ^   ^    ^   ^   ^
//                        |   |   |   |    |   |   M2
//                        |   |   |   |    |   M1
//                        |   |   |   |    M0
//                        |   |   |   steps/rev
//                        |   |   ENABLE
//                        |   DIR
//                        STEP

void setup() {
    stepper.init();
    stepper.enable();
    
    // Set microstepping mode
    stepper.setMicrostepMode(EIGHTH_STEP);  // 1/8 microstepping
    stepper.setSpeed(60.0);
    
    // Move 1 physical revolution (1600 microsteps in 1/8 mode)
    stepper.move(200 * 8);
    stepper.runToPosition();
}
```

### 4. With Acceleration

```cpp
void setup() {
    stepper.init();
    stepper.enable();
    
    stepper.setSpeed(240.0);           // Target speed: 240 RPM
    stepper.setAcceleration(1000.0);   // Ramp up at 1000 steps/sec²
    stepper.setMaxSpeed(2000.0);       // Never exceed 2000 steps/sec
    
    // Long move with smooth acceleration/deceleration
    stepper.move(2000);
}

void loop() {
    stepper.run();  // Handles acceleration automatically
    
    if (!stepper.isRunning()) {
        // Reverse direction
        stepper.move(-2000);
        delay(1000);
    }
}
```

## API Reference

### Initialization

```cpp
// Constructor
StepperController(int stepPin, int dirPin, int enablePin = -1, 
                 int stepsPerRev = 200, int m0Pin = -1, 
                 int m1Pin = -1, int m2Pin = -1);

// Initialize hardware
void init();
```

### Motor Control

```cpp
void enable();                    // Enable motor (energize coils)
void disable();                   // Disable motor (coast, save power)
bool isEnabled();                 // Check if motor is enabled
void stop();                      // Stop immediately, cancel movements
```

### Speed and Movement

```cpp
void setSpeed(float rpm);         // Set speed in RPM (0.1 - 3000)
void setMaxSpeed(float stepsPerSec); // Set maximum speed limit
void setAcceleration(float stepsPerSecSquared); // Enable acceleration

void moveTo(long position);       // Move to absolute position
void move(long steps);            // Move relative steps (+/-)
void stepForward();               // Move 1 step forward
void stepBackward();              // Move 1 step backward
```

### Execution Modes

```cpp
bool run();                       // Non-blocking: call in loop()
void runToPosition();             // Blocking: wait until target reached
void runToNewPosition(long pos);  // Blocking: move to position and wait
```

### Position Tracking

```cpp
long getCurrentPosition();        // Get current position
long getTargetPosition();         // Get target position
long distanceToGo();             // Steps remaining to target
void setCurrentPosition(long pos); // Reset position counter
```

### Status

```cpp
bool isRunning();                 // Check if motor is moving
float getCurrentSpeed();          // Get current speed (steps/sec)
bool getDirection();              // Get direction (true = forward)
```

### Direction

```cpp
void setDirection(bool forward);  // Set direction (true/false)
void reverseDirection();          // Flip direction
```

### Microstepping

```cpp
bool setMicrostepMode(MicrostepMode mode); // Set microstepping mode
MicrostepMode getMicrostepMode();          // Get current mode

// Available modes:
// FULL_STEP, HALF_STEP, QUARTER_STEP, EIGHTH_STEP, 
// SIXTEENTH_STEP, THIRTYTWO_STEP
```

## Advanced Examples

### Continuous Rotation

```cpp
void continuousRotation() {
    stepper.enable();
    stepper.setSpeed(180.0);      // 180 RPM
    stepper.moveTo(100000000);    // Very far target = continuous
    
    while (true) {
        stepper.run();
        
        // Can do other work here
        checkSensors();
        updateDisplay();
        
        yield();
    }
}
```

### Position Control with Waypoints

```cpp
void followWaypoints() {
    stepper.enable();
    stepper.setSpeed(120.0);
    stepper.setAcceleration(800.0);
    
    long waypoints[] = {0, 500, 1200, 800, 0};
    
    for (long pos : waypoints) {
        Serial.printf("Moving to %ld\n", pos);
        stepper.runToNewPosition(pos);
        delay(500);  // Pause at each waypoint
    }
}
```

### Speed Ramping

```cpp
void speedRamp() {
    stepper.enable();
    stepper.setAcceleration(500.0);
    
    // Gradually increase speed
    for (int rpm = 30; rpm <= 300; rpm += 30) {
        stepper.setSpeed(rpm);
        stepper.move(200);  // 1 revolution at each speed
        stepper.runToPosition();
        delay(500);
    }
}
```

### Jog Mode

```cpp
void jogMode() {
    const int JOG_STEPS = 10;  // Steps per jog
    
    if (digitalRead(BTN_JOG_FWD) == LOW) {
        stepper.move(JOG_STEPS);
    }
    if (digitalRead(BTN_JOG_REV) == LOW) {
        stepper.move(-JOG_STEPS);
    }
    
    stepper.run();  // Must call continuously
}
```

## Troubleshooting

### Motor doesn't move
- ✓ Check `enable()` is called
- ✓ Verify power supply to VMOT
- ✓ Check GPIO connections
- ✓ Ensure `run()` is called in loop (non-blocking mode)
- ✓ Verify target position ≠ current position
- ✓ Check motor coil connections
- ✓ Measure Vref for proper current setting

### Motor stutters or stalls
- ✓ Reduce speed (lower RPM)
- ✓ Increase motor voltage (12V → 24V)
- ✓ Increase current limit (adjust Vref)
- ✓ Add acceleration (`setAcceleration()`)
- ✓ Reduce microstepping
- ✓ Check for mechanical binding
- ✓ Reduce load/inertia

### Motor overheats
- ✓ Add heatsink to DRV8825
- ✓ Reduce current limit (lower Vref)
- ✓ Use `disable()` when not moving
- ✓ Improve airflow
- ✓ Check for short circuits

### Missed steps / position errors
- ✓ Speed too high - reduce RPM
- ✓ Acceleration too aggressive
- ✓ Load too heavy
- ✓ Insufficient motor current
- ✓ Power supply voltage drop
- ✓ Use lower microstepping for speed

### DRV8825 gets too hot
- ✓ **ALWAYS use a heatsink** above 1A
- ✓ Reduce current via Vref adjustment
- ✓ Ensure good air circulation
- ✓ Check motor isn't stalling
- ✓ Verify motor coils aren't shorted

### Jerky motion
- ✓ Enable acceleration
- ✓ Use higher microstepping
- ✓ Check power supply stability
- ✓ Add capacitor (100µF) if missing
- ✓ Reduce EMI with twisted pair wiring

## Performance Guidelines

### Speed vs. Microstepping

| Mode | Max Practical Speed | Notes |
|------|---------------------|-------|
| Full Step | 2000+ RPM | Fastest, most torque |
| 1/2 Step | 1000 RPM | Good balance |
| 1/4 Step | 600 RPM | Smooth, moderate speed |
| 1/8 Step | 400 RPM | Very smooth |
| 1/16 Step | 200 RPM | Ultra smooth, positioning |
| 1/32 Step | 100 RPM | Maximum smoothness, slow |

### Voltage vs. Speed

- **8-12V**: Good for <200 RPM
- **12-18V**: Good for 200-400 RPM
- **24V**: Good for 400-800 RPM
- **36V**: Maximum speed (1000+ RPM)

### Current Setting Guidelines

For NEMA 17 motors:
- **0.4-0.8A rated**: Set to 70% of rating
- **1.0-1.5A rated**: Set to 70-80% of rating
- **1.7-2.0A rated**: Set to 70% with heatsink

## Comparison: DRV8825 vs DRV8871

| Feature | DRV8825 | DRV8871 (previous) |
|---------|---------|-------------------|
| **Purpose** | Stepper driver | DC/H-bridge |
| **Outputs** | 4 (A1,A2,B1,B2) | 2 (OUT1, OUT2) |
| **Interface** | STEP/DIR | PWM/H-bridge |
| **For Steppers** | ✅ Perfect | ❌ Need 2x |
| **Microstepping** | ✅ Built-in | ❌ No |
| **Complexity** | ✅ Simple | ❌ Complex |
| **Code** | ✅ Easy | ❌ Complicated |

The DRV8825 is **dramatically simpler** for stepper control!

## Additional Resources

- [DRV8825 Datasheet](https://www.ti.com/lit/ds/symlink/drv8825.pdf)
- [Current Adjustment Guide](https://www.pololu.com/product/2133)
- [Microstepping Explained](https://www.motioncontroltips.com/what-is-microstepping/)

## License

MIT License - Free to use and modify

## Support

For issues or questions, refer to the example code in `main_drv8825.cpp` which demonstrates all features.
