#include "StepperController.hpp"

StepperController::StepperController(int in1Pin, int in2Pin, int stepsPerRevolution)
    : pinIN1(in1Pin),
      pinIN2(in2Pin),
      stepsPerRev(stepsPerRevolution),
      currentStepMode(FULL_STEP),
      currentSpeedRPM(60.0),
      stepDelayMicros(0),
      lastStepMicros(0),
      currentPosition(0),
      targetPosition(0),
      currentDirection(STEPPER_FORWARD),
      motorEnabled(false),
      currentStepPhase(0) {
    updateStepDelay();
}

void StepperController::init() {
    // Setup PWM channels for both DRV8871 inputs
    ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
    
    ledcAttachPin(pinIN1, PWM_CHANNEL_1);
    ledcAttachPin(pinIN2, PWM_CHANNEL_2);
    
    // Start with motor disabled (coast mode)
    disable();
}

void StepperController::setSpeed(float rpm) {
    currentSpeedRPM = constrain(rpm, 0.1, 1500.0);  // Support up to 1500 RPM for NEMA 17
    updateStepDelay();
}

void StepperController::updateStepDelay() {
    // Calculate microseconds per step based on RPM
    int effectiveStepsPerRev = (currentStepMode == HALF_STEP) ? stepsPerRev * 2 : stepsPerRev;
    float stepsPerSecond = (currentSpeedRPM * effectiveStepsPerRev) / 60.0;
    stepDelayMicros = (stepsPerSecond > 0) ? (1000000.0 / stepsPerSecond) : 10000;
}

void StepperController::setDirection(StepperDirection dir) {
    currentDirection = dir;
}

void StepperController::reverseDirection() {
    currentDirection = (currentDirection == STEPPER_FORWARD) ? STEPPER_REVERSE : STEPPER_FORWARD;
}

void StepperController::setStepMode(StepMode mode) {
    currentStepMode = mode;
    currentStepPhase = 0;  // Reset phase when changing modes
    updateStepDelay();
}

void StepperController::step(int steps) {
    targetPosition = currentPosition + steps;
}

void StepperController::moveTo(long target) {
    targetPosition = target;
}

void StepperController::setPosition(long position) {
    currentPosition = position;
    targetPosition = position;
}

void StepperController::incrementPosition(int steps) {
    targetPosition = currentPosition + abs(steps);
}

void StepperController::decrementPosition(int steps) {
    targetPosition = currentPosition - abs(steps);
}

void StepperController::enable() {
    motorEnabled = true;
}

void StepperController::disable() {
    motorEnabled = false;
    // Coast mode: both outputs LOW
    setMotorOutput(0, 0);
}

void StepperController::stop() {
    targetPosition = currentPosition;
    // Optional: apply brake by setting both HIGH
    // setMotorOutput(255, 255);
}

void StepperController::run() {
    if (!motorEnabled || currentPosition == targetPosition) {
        return;
    }
    
    unsigned long currentMicros = micros();
    
    // For high-speed operation, check timing more precisely
    if (currentMicros - lastStepMicros >= stepDelayMicros) {
        lastStepMicros = currentMicros;
        
        // Determine direction
        int direction = (targetPosition > currentPosition) ? 1 : -1;
        
        executeStep(direction);
        currentPosition += direction;
    }
}

void StepperController::runToPosition() {
    while (currentPosition != targetPosition && motorEnabled) {
        run();
        delayMicroseconds(10);  // Small delay to prevent tight loop
    }
}

void StepperController::executeStep(int direction) {
    if (currentStepMode == FULL_STEP) {
        // Full step mode: 4 phases
        if (direction > 0) {
            currentStepPhase = (currentStepPhase + 1) % 4;
        } else {
            currentStepPhase = (currentStepPhase + 3) % 4;  // +3 is same as -1 mod 4
        }
        applyFullStep(currentStepPhase);
        
    } else {
        // Half step mode: 8 phases
        if (direction > 0) {
            currentStepPhase = (currentStepPhase + 1) % 8;
        } else {
            currentStepPhase = (currentStepPhase + 7) % 8;  // +7 is same as -1 mod 8
        }
        applyHalfStep(currentStepPhase);
    }
}

void StepperController::applyFullStep(uint8_t phase) {
    // Full step sequence for bipolar stepper with DRV8871
    // Phase 0: IN1=HIGH, IN2=LOW  (coil A+)
    // Phase 1: IN1=LOW,  IN2=HIGH (coil A-)
    // Phase 2: IN1=HIGH, IN2=LOW  (coil B+)
    // Phase 3: IN1=LOW,  IN2=HIGH (coil B-)
    
    // Note: This is a simplified approach. For actual bipolar stepper,
    // you need two DRV8871 drivers (one per coil) or H-bridge per coil.
    // This implementation assumes unipolar or demonstrates the concept.
    
    switch (phase) {
        case 0:
            setMotorOutput(255, 0);    // Forward
            break;
        case 1:
            setMotorOutput(0, 255);    // Reverse
            break;
        case 2:
            setMotorOutput(255, 0);    // Forward
            break;
        case 3:
            setMotorOutput(0, 255);    // Reverse
            break;
    }
}

void StepperController::applyHalfStep(uint8_t phase) {
    // Half step sequence - alternates between full steps and half power
    uint8_t pwmLevel = 180;  // Reduced power for half steps
    
    switch (phase) {
        case 0:
            setMotorOutput(255, 0);
            break;
        case 1:
            setMotorOutput(pwmLevel, 0);
            break;
        case 2:
            setMotorOutput(0, 255);
            break;
        case 3:
            setMotorOutput(0, pwmLevel);
            break;
        case 4:
            setMotorOutput(255, 0);
            break;
        case 5:
            setMotorOutput(pwmLevel, 0);
            break;
        case 6:
            setMotorOutput(0, 255);
            break;
        case 7:
            setMotorOutput(0, pwmLevel);
            break;
    }
}

void StepperController::setMotorOutput(uint8_t pwm1, uint8_t pwm2) {
    ledcWrite(PWM_CHANNEL_1, pwm1);
    ledcWrite(PWM_CHANNEL_2, pwm2);
}

void StepperController::setAcceleration(float stepsPerSecondSquared) {
    // Placeholder for acceleration implementation
    // This would require velocity ramping logic in run()
    // For now, this is a simple implementation without acceleration
}

void StepperController::setMaxSpeed(float stepsPerSecond) {
    // Convert steps per second to RPM
    int effectiveStepsPerRev = (currentStepMode == HALF_STEP) ? stepsPerRev * 2 : stepsPerRev;
    float rpm = (stepsPerSecond * 60.0) / effectiveStepsPerRev;
    setSpeed(rpm);
}
