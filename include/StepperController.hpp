#ifndef STEPPERCONTROLLER_H
#define STEPPERCONTROLLER_H

#include <Arduino.h>

/**
 * StepperController - Control bipolar stepper motors using DRV8825 driver
 * 
 * The DRV8825 uses a STEP/DIR interface which is much simpler than the
 * DRV8871 H-bridge interface. It has built-in microstepping and current
 * limiting, making it ideal for NEMA 17 stepper motors.
 * 
 * Key Features:
 * - Simple STEP/DIR control interface
 * - Hardware microstepping (1, 1/2, 1/4, 1/8, 1/16, 1/32)
 * - Speed control in RPM
 * - Position tracking and control
 * - Non-blocking and blocking movement modes
 * - Acceleration/deceleration support
 * 
 * Hardware Connections (DRV8825):
 * - STEP -> ESP32 GPIO (generates step pulses)
 * - DIR  -> ESP32 GPIO (sets direction)
 * - ENABLE -> ESP32 GPIO (enables/disables motor, active LOW)
 * - M0, M1, M2 -> Set microstepping mode (can be hardwired or GPIO controlled)
 * - VMOT -> Motor power supply (8.2V - 45V)
 * - GND -> Common ground
 * - A1, A2, B1, B2 -> Stepper motor coils
 */

// Microstepping modes for DRV8825
enum MicrostepMode {
    FULL_STEP = 1,      // M0=L, M1=L, M2=L
    HALF_STEP = 2,      // M0=H, M1=L, M2=L
    QUARTER_STEP = 4,   // M0=L, M1=H, M2=L
    EIGHTH_STEP = 8,    // M0=H, M1=H, M2=L
    SIXTEENTH_STEP = 16,// M0=L, M1=L, M2=H
    THIRTYTWO_STEP = 32 // M0=H, M1=H, M2=H (1/32 microstepping)
};

class StepperController {
public:
    /**
     * Constructor
     * @param stepPin GPIO pin connected to DRV8825 STEP
     * @param dirPin GPIO pin connected to DRV8825 DIR
     * @param enablePin GPIO pin connected to DRV8825 ENABLE (optional, -1 to disable)
     * @param stepsPerRev Steps per revolution of motor (typically 200 for 1.8° motors)
     * @param m0Pin GPIO for M0 microstepping control (optional, -1 for hardwired)
     * @param m1Pin GPIO for M1 microstepping control (optional, -1 for hardwired)
     * @param m2Pin GPIO for M2 microstepping control (optional, -1 for hardwired)
     */
    StepperController(int stepPin, int dirPin, int enablePin = -1, 
                     int stepsPerRev = 200, int m0Pin = -1, int m1Pin = -1, int m2Pin = -1);
    
    /**
     * Initialize the stepper controller
     * Sets up GPIO pins and default settings
     */
    void init();
    
    /**
     * Set target speed in RPM
     * @param rpm Target speed in revolutions per minute (0.1 - 3000+ RPM)
     */
    void setSpeed(float rpm);
    
    /**
     * Set microstepping mode (if M0/M1/M2 pins are controlled by GPIO)
     * @param mode Microstepping mode (FULL_STEP to THIRTYTWO_STEP)
     * @return true if successful, false if pins not configured
     */
    bool setMicrostepMode(MicrostepMode mode);
    
    /**
     * Get current microstepping mode
     * @return Current microstepping mode
     */
    MicrostepMode getMicrostepMode() const { return currentMode; }
    
    /**
     * Set acceleration in steps per second squared
     * @param stepsPerSecSquared Acceleration rate (0 = infinite/no ramping)
     */
    void setAcceleration(float stepsPerSecSquared);
    
    /**
     * Set maximum speed in steps per second
     * @param stepsPerSec Maximum speed limit
     */
    void setMaxSpeed(float stepsPerSec);
    
    /**
     * Enable the motor (LOW signal to ENABLE pin)
     */
    void enable();
    
    /**
     * Disable the motor (HIGH signal to ENABLE pin, motor coasts)
     */
    void disable();
    
    /**
     * Check if motor is enabled
     * @return true if enabled, false if disabled
     */
    bool isEnabled() const { return enabled; }
    
    /**
     * Move to absolute position (non-blocking)
     * @param position Target position in steps
     */
    void moveTo(long position);
    
    /**
     * Move relative number of steps (non-blocking)
     * @param steps Number of steps to move (positive = forward, negative = reverse)
     */
    void move(long steps);
    
    /**
     * Move one step forward (non-blocking)
     */
    void stepForward();
    
    /**
     * Move one step backward (non-blocking)
     */
    void stepBackward();
    
    /**
     * Run the stepper motor (call this in loop() for non-blocking operation)
     * This handles timing and generates step pulses
     * @return true if motor is still moving, false if reached target
     */
    bool run();
    
    /**
     * Run motor until it reaches target position (blocking)
     * Blocks execution until target is reached
     */
    void runToPosition();
    
    /**
     * Run motor to new position (blocking)
     * @param position Target position in steps
     */
    void runToNewPosition(long position);
    
    /**
     * Check if motor is currently moving
     * @return true if moving, false if stopped
     */
    bool isRunning() const;
    
    /**
     * Stop the motor immediately
     * Cancels any pending movements
     */
    void stop();
    
    /**
     * Get current position
     * @return Current position in steps
     */
    long getCurrentPosition() const { return currentPosition; }
    
    /**
     * Get target position
     * @return Target position in steps
     */
    long getTargetPosition() const { return targetPosition; }
    
    /**
     * Set current position (resets position counter)
     * @param position New position value
     */
    void setCurrentPosition(long position);
    
    /**
     * Get distance to target position
     * @return Steps remaining to target (negative if moving backward)
     */
    long distanceToGo() const;
    
    /**
     * Get current speed in steps per second
     * @return Current speed
     */
    float getCurrentSpeed() const { return currentSpeed; }
    
    /**
     * Set direction
     * @param forward true for forward, false for reverse
     */
    void setDirection(bool forward);
    
    /**
     * Reverse current direction
     */
    void reverseDirection();
    
    /**
     * Get current direction
     * @return true if forward, false if reverse
     */
    bool getDirection() const { return directionForward; }

private:
    // Pin assignments
    int stepPin;
    int dirPin;
    int enablePin;
    int m0Pin, m1Pin, m2Pin;
    
    // Motor parameters
    int stepsPerRev;
    MicrostepMode currentMode;
    
    // Position tracking
    long currentPosition;
    long targetPosition;
    
    // Speed and timing
    float targetSpeed;           // Target speed in steps/sec
    float currentSpeed;          // Current speed in steps/sec (for acceleration)
    float maxSpeed;              // Maximum speed limit in steps/sec
    float acceleration;          // Acceleration in steps/sec²
    unsigned long stepInterval;  // Microseconds between steps
    unsigned long lastStepTime;  // Last step timestamp
    
    // Direction and state
    bool directionForward;
    bool enabled;
    
    // Internal methods
    void computeStepInterval();
    void updateMicrostepPins();
    float constrainSpeed(float speed);
};

#endif // STEPPERCONTROLLER_H
