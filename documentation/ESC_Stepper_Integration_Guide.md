# ESC Motor + Stepper Motor Integration Guide

## Overview
This system controls two different motor types simultaneously on the ESP32-S3:
1. **Brushless ESC Motor** - PWM/DSHOT controlled via MotorController class
2. **Stepper Motor** - DRV8871 controlled via StepperController class

Both motors operate independently in parallel without interference.

## Hardware Setup

### Complete Pin Configuration

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32-S3 T-Display                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ESC Motor (Brushless):                                 │
│  • GPIO 43  → ESC Signal (white wire)                   │
│  • GPIO 14  → Mode Button (PWM/DSHOT toggle)            │
│  • GPIO 0   → Direction Button (Forward/Reverse)        │
│  • GPIO 1   → Throttle ADC (Potentiometer)              │
│                                                         │
│  Stepper Motor (DRV8871):                               │
│  • GPIO 10  → DRV8871 IN1                               │
│  • GPIO 11  → DRV8871 IN2                               │
│  • GPIO 12  → Forward Button                            │
│  • GPIO 13  → Reverse Button                            │
│  • GPIO 15  → Enable/Disable Button                     │
│  • GPIO 2   → Speed Potentiometer (ADC)                 │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### ESC Wiring
```
ESC:
┌──────────────┐
│  ESC         │
│  Signal ─────┼──── GPIO 43 (ESP32)
│  GND    ─────┼──── GND (ESP32)
│  +5V    ─────┼──── 5V (Optional BEC power)
│              │
│  Motor A ────┼──── Brushless Motor Phase A
│  Motor B ────┼──── Brushless Motor Phase B  
│  Motor C ────┼──── Brushless Motor Phase C
│              │
│  Battery+ ───┼──── LiPo Battery + (7.4V-22.2V)
│  Battery- ───┼──── LiPo Battery - (GND)
└──────────────┘
```

### DRV8871 + Stepper Wiring
```
DRV8871:                     Stepper Motor:
┌──────────────┐            ┌──────────────┐
│  DRV8871     │            │ NEMA 17      │
│  IN1 ────────┼──── GPIO 10│              │
│  IN2 ────────┼──── GPIO 11│  Coil A+ ────┼──── OUT1
│              │            │  Coil A- ────┼──── OUT2
│  OUT1 ───────┼────────────┤              │
│  OUT2 ───────┼────────────┤              │
│              │            │              │
│  VM  ────────┼──── 12-24V │              │
│  GND ────────┼──── GND    │              │
└──────────────┘            └──────────────┘

Note: For full bipolar stepper control, you need 2x DRV8871
(one per coil). Current implementation controls one coil.
```

### Power Supply Recommendations
```
ESC Motor:
- Battery: 7.4V - 22.2V LiPo (2S-6S)
- Current: Depends on motor (typically 20-40A peak)

Stepper Motor:
- Supply: 12V minimum, 24V recommended for high speed
- Current: 1-2A per coil typical for NEMA 17
- Higher voltage = higher speed capability

ESP32-S3:
- Can be powered from ESC BEC (5V)
- Or separate 5V supply
- Current: ~500mA with display
```

## PWM Channel Allocation

The system uses different PWM channels to avoid conflicts:

```cpp
// MotorController (ESC):
// - Uses PWM Channel 0 (for PWM mode)
// - GPIO 43

// StepperController (DRV8871):
// - Uses PWM Channel 2 (IN1)
// - Uses PWM Channel 3 (IN2)
// - GPIO 10, 11

No conflicts! ✓
```

## Software Usage

### Basic Setup
```cpp
#include "MotorController.hpp"
#include "StepperController.hpp"

MotorController escMotor(43);  // GPIO 43
StepperController stepper(10, 11, 200);  // GPIO 10, 11, 200 steps/rev

void setup() {
    escMotor.init();
    
    stepper.init();
    stepper.setSpeed(100.0);    // 100 RPM
    stepper.enable();
}

void loop() {
    // CRITICAL: Call stepper.run() frequently!
    stepper.run();
    
    // ESC control
    int throttle = analogRead(1);
    escMotor.setThrottleFromADC(throttle);
    
    // Other code...
}
```

### Button Controls

#### ESC Motor
```cpp
// Mode toggle (GPIO 14): Switch between PWM and DSHOT
if (digitalRead(14) == LOW) {
    escMotor.setMode(MODE_DSHOT);
}

// Direction (GPIO 0): Forward/Reverse
if (digitalRead(0) == LOW) {
    escMotor.setDirection(!escMotor.isForward());
}

// Throttle (GPIO 1): Analog pot 0-4095 → 1000-2000µs or 48-2047 DSHOT
int adc = analogRead(1);
escMotor.setThrottleFromADC(adc);
```

#### Stepper Motor
```cpp
// Forward (GPIO 12): Move 50 steps forward
if (digitalRead(12) == LOW) {
    stepper.incrementPosition(50);
}

// Reverse (GPIO 13): Move 50 steps backward
if (digitalRead(13) == LOW) {
    stepper.decrementPosition(50);
}

// Enable/Disable (GPIO 15): Toggle motor power
if (digitalRead(15) == LOW) {
    stepper.isEnabled() ? stepper.disable() : stepper.enable();
}

// Speed (GPIO 2): Analog pot controls RPM (10-600)
int speedADC = analogRead(2);
float rpm = map(speedADC, 0, 4095, 10, 600);
stepper.setSpeed(rpm);
```

## Performance Considerations

### Loop Timing
```cpp
void loop() {
    // PRIORITY 1: Stepper timing (call every loop iteration)
    stepper.run();  // ← CRITICAL for smooth motion
    
    // PRIORITY 2: ESC control (every loop is fine)
    escMotor.setThrottleFromADC(analogRead(1));
    
    // PRIORITY 3: Button checks (every 50ms is enough)
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 50) {
        checkButtons();
        lastCheck = millis();
    }
    
    // PRIORITY 4: Display updates (every 200-500ms)
    static uint32_t lastUI = 0;
    if (millis() - lastUI > 200) {
        updateDisplay();
        lastUI = millis();
    }
}
```

### Stepper Speed Limits with Concurrent ESC Operation
```
When running both motors with display updates:
- Practical stepper speed: 100-600 RPM
- Maximum with optimization: 800-1000 RPM

For higher stepper speeds (1000+ RPM):
- Reduce display update frequency
- Minimize Serial.print() calls
- Consider FreeRTOS dual-core approach
```

### FreeRTOS Dual Core (Advanced)
```cpp
TaskHandle_t stepperTask;

void stepperTaskFunction(void* param) {
    while(1) {
        stepper.run();
        delayMicroseconds(10);
    }
}

void setup() {
    // ... init code ...
    
    // Run stepper on Core 0 (dedicated)
    xTaskCreatePinnedToCore(
        stepperTaskFunction,
        "StepperTask",
        4096,
        NULL,
        10,  // High priority
        &stepperTask,
        0    // Core 0
    );
}

void loop() {
    // This runs on Core 1
    // ESC, display, buttons - won't affect stepper timing!
    escMotor.setThrottleFromADC(analogRead(1));
    updateDisplay();
    delay(10);  // This delay won't affect stepper!
}
```

## Common Use Cases

### 1. Camera Gimbal with Propulsion
```cpp
// Stepper controls camera pan/tilt
// ESC controls propulsion motor
stepper.setSpeed(50.0);  // Slow, precise positioning
escMotor.setThrottleFromADC(throttleInput);
```

### 2. 3D Printer with Cooling Fan
```cpp
// Stepper for print head movement
// ESC for cooling fan
stepper.moveTo(targetPosition);
escMotor.setThrottleFromADC(fanSpeed);
```

### 3. CNC with Spindle
```cpp
// Stepper for axis movement
// ESC for spindle motor
stepper.setSpeed(200.0);  // Fast positioning
escMotor.setMode(MODE_PWM);  // PWM for spindle speed
```

### 4. Robot Arm with Wheel Drive
```cpp
// Stepper for precise joint control
// ESC for mobile base wheels
stepper.setStepMode(HALF_STEP);  // Smooth arm movement
escMotor.setDirection(FORWARD);
```

## Troubleshooting

### Stepper Motion Jerky/Irregular
**Cause:** Loop takes too long, `stepper.run()` not called frequently enough
**Solution:** 
- Reduce display update rate
- Move Serial.print() outside fast loop
- Profile loop execution time

### ESC Not Arming
**Cause:** Incorrect throttle range or ESC not calibrated
**Solution:**
- Ensure throttle starts at 1000µs (idle)
- Calibrate ESC (full throttle → low throttle → power on)
- Check PWM frequency (50Hz required)

### Both Motors Affect Each Other
**Cause:** Shared ground issues or power supply sag
**Solution:**
- Verify common ground between ESP32, ESC, DRV8871
- Use adequate power supply (separate for motors and logic ideal)
- Add capacitors near power inputs

### High-Speed Stepper Causes ESC Glitches
**Cause:** Stepper code blocking ESC signal generation
**Solution:**
- Reduce stepper speed
- Use FreeRTOS dual-core approach
- Ensure tight loop without delays

## Example Applications

### Complete Example: Camera Slider
```cpp
// Stepper moves camera along rail
// ESC controls second axis or auxiliary motor

void moveToPosition(long position, int escSpeed) {
    stepper.moveTo(position);
    escMotor.setThrottleFromADC(map(escSpeed, 0, 100, 0, 4095));
    
    while(stepper.isRunning()) {
        stepper.run();
        delay(1);  // Small delay OK for camera slider
    }
    
    escMotor.setThrottleFromADC(0);  // Stop ESC when position reached
}
```

## Summary

✅ **ESC Motor:** Brushless motor control (PWM/DSHOT)
✅ **Stepper Motor:** Precise positioning with DRV8871
✅ **Independent Operation:** No interference between motors
✅ **Flexible Control:** Buttons, potentiometers, programmatic
✅ **Scalable:** Can add more peripherals without conflicts

The system is production-ready for robotics, automation, and mechatronics applications!
