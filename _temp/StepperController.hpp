#ifndef STEPPER_CONTROLLER_HPP
#define STEPPER_CONTROLLER_HPP

#include <Arduino.h>

enum StepperDirection {
    STEPPER_FORWARD,
    STEPPER_REVERSE
};

enum StepMode {
    FULL_STEP,
    HALF_STEP
};

class StepperController {
public:
    // Constructor: specify the two PWM pins for DRV8871
    StepperController(int in1Pin, int in2Pin, int stepsPerRevolution = 200);
    
    // Initialization
    void init();
    
    // Speed control (RPM)
    void setSpeed(float rpm);
    float getSpeed() const { return currentSpeedRPM; }
    
    // Direction control
    void setDirection(StepperDirection dir);
    StepperDirection getDirection() const { return currentDirection; }
    void reverseDirection();
    
    // Step mode
    void setStepMode(StepMode mode);
    StepMode getStepMode() const { return currentStepMode; }
    
    // Position control
    void step(int steps);                    // Move relative steps (+ or -)
    void moveTo(long targetPosition);        // Move to absolute position
    void setPosition(long position);         // Set current position without moving
    long getPosition() const { return currentPosition; }
    long getTargetPosition() const { return targetPosition; }
    
    // Incremental movement
    void incrementPosition(int steps = 1);   // Move forward by steps
    void decrementPosition(int steps = 1);   // Move backward by steps
    
    // Enable/disable motor
    void enable();
    void disable();
    bool isEnabled() const { return motorEnabled; }
    
    // Non-blocking operation
    void run();                              // Call this in loop() for non-blocking motion
    bool isRunning() const { return (currentPosition != targetPosition); }
    void stop();                             // Stop immediately
    
    // Blocking operation
    void runToPosition();                    // Block until target reached
    
    // Configuration
    void setAcceleration(float stepsPerSecondSquared);
    void setMaxSpeed(float stepsPerSecond);
    
private:
    // Pin configuration
    int pinIN1;
    int pinIN2;
    
    // Motor specifications
    int stepsPerRev;
    StepMode currentStepMode;
    
    // Speed and timing
    float currentSpeedRPM;
    unsigned long stepDelayMicros;
    unsigned long lastStepMicros;
    
    // Position tracking
    long currentPosition;
    long targetPosition;
    StepperDirection currentDirection;
    
    // Motor state
    bool motorEnabled;
    uint8_t currentStepPhase;  // 0-3 for full step, 0-7 for half step
    
    // PWM configuration
    static const int PWM_CHANNEL_1 = 2;  // Use different channels from motor controller
    static const int PWM_CHANNEL_2 = 3;
    static const int PWM_FREQ = 20000;   // 20 kHz PWM frequency
    static const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)
    
    // Step patterns for DRV8871 (PWM duty cycles)
    // DRV8871: IN1 and IN2 control direction and braking
    // Both LOW = coast, Both HIGH = brake, Different = forward/reverse
    
    // Helper methods
    void updateStepDelay();
    void executeStep(int direction);
    void setMotorOutput(uint8_t pwm1, uint8_t pwm2);
    void applyFullStep(uint8_t phase);
    void applyHalfStep(uint8_t phase);
};

#endif // STEPPER_CONTROLLER_HPP
