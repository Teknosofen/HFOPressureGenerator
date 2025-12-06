#include "ImageRendererEnhanced.hpp"

ImageRendererEnhanced::ImageRendererEnhanced() 
    : oldValue(-1), 
      oldAdcCounts(-1),
      oldESCThrottle(-1),
      oldESCADC(-1),
      oldStepperPos(-1),
      oldStepperTarget(-1),
      oldStepperSpeed(-1.0),
      oldStepperRunning(false),
      oldStepperEnabled(false) {
}

void ImageRendererEnhanced::init() {
    tft.init();
    tft.setRotation(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.fillScreen(TFT_LOGOBACKGROUND);
    tft.setTextColor(TFT_GREEN, TFT_LOGOBACKGROUND);
    tft.setTextSize(1);
}

void ImageRendererEnhanced::setPositions(DisplayPos mode, DisplayPos value, DisplayPos dir, DisplayPos adc) {
    modePos = mode;
    valuePos = value;
    dirPos = dir;
    adcPos = adc;
}

void ImageRendererEnhanced::drawHeader(const char* title, const char* version) {
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
    tft.setTextSize(1);
    tft.setCursor(10, 20);
    tft.printf("%s %s", title, version);
}

void ImageRendererEnhanced::updateMode(const char* modeText) {
    if (oldModeText != modeText) {
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(modePos.x, modePos.y);
        tft.println(oldModeText.c_str());
        
        oldModeText = modeText;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(modePos.x, modePos.y);
        tft.println(modeText);
    }
}

void ImageRendererEnhanced::updateValue(const char* valueLabel, int value) {
    if (value != oldValue || oldValueLabel != valueLabel) {
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(valuePos.x, valuePos.y);
        tft.printf("%s: %d", oldValueLabel.c_str(), oldValue);
        
        oldValueLabel = valueLabel;
        oldValue = value;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(valuePos.x, valuePos.y);
        tft.printf("%s: %d", valueLabel, value);
    }
}

void ImageRendererEnhanced::updateDirection(const char* dirText) {
    if (oldDirText != dirText) {
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(dirPos.x, dirPos.y);
        tft.printf("Direction: %s", oldDirText.c_str());
        
        oldDirText = dirText;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(dirPos.x, dirPos.y);
        tft.printf("Direction: %s", dirText);
    }
}

void ImageRendererEnhanced::updateADC(int adcCounts) {
    if (adcCounts != oldAdcCounts) {
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(adcPos.x, adcPos.y);
        tft.printf("ADC counts: %d", oldAdcCounts);
        
        oldAdcCounts = adcCounts;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(adcPos.x, adcPos.y);
        tft.printf("ADC counts: %d", adcCounts);
    }
}

void ImageRendererEnhanced::drawFullUI(const char* modeText, const char* dirText,
                                const char* valueLabel, int value, int adcCounts) {
    updateMode(modeText);
    updateValue(valueLabel, value);
    updateDirection(dirText);
    updateADC(adcCounts);
}

void ImageRendererEnhanced::drawESCStatus(int x, int y, const char* mode, const char* dir, 
                                         int throttle, int adc) {
    // Mode
    if (oldESCMode != mode) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Mode: %s", oldESCMode.c_str());
        
        // Draw new
        oldESCMode = mode;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Mode: %s", mode);
    }
    
    // Throttle
    if (throttle != oldESCThrottle) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("Throttle: %d", oldESCThrottle);
        
        // Draw new
        oldESCThrottle = throttle;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("Throttle: %d", throttle);
    }
    
    // Direction
    if (oldESCDir != dir) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 60);
        tft.printf("Dir: %s", oldESCDir.c_str());
        
        // Draw new
        oldESCDir = dir;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 60);
        tft.printf("Dir: %s", dir);
    }
    
    // Draw header once (doesn't change)
    static bool headerDrawn = false;
    if (!headerDrawn) {
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y);
        tft.println("=== ESC Motor ===");
        headerDrawn = true;
    }
}

void ImageRendererEnhanced::drawStepperStatus(int x, int y, long pos, long target, 
                                              float speed, bool running, bool enabled) {
    // Position
    if (pos != oldStepperPos || target != oldStepperTarget) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Pos: %ld/%ld    ", oldStepperPos, oldStepperTarget);
        
        // Draw new
        oldStepperPos = pos;
        oldStepperTarget = target;
        tft.setTextColor(TFT_GREENISH_TINT, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Pos: %ld/%ld", pos, target);
    }
    
    // Speed
    if (speed != oldStepperSpeed) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("Speed: %.0f RPM   ", oldStepperSpeed);
        
        // Draw new
        oldStepperSpeed = speed;
        tft.setTextColor(TFT_GREENISH_TINT, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("Speed: %.0f RPM", speed);
    }
    
    // Running status
    if (running != oldStepperRunning) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 60);
        tft.printf("%s      ", oldStepperRunning ? "MOVING" : "IDLE");
        
        // Draw new
        oldStepperRunning = running;
        tft.setTextColor(TFT_GREENISH_TINT, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 60);
        tft.printf("%s", running ? "MOVING" : "IDLE");
    }
    
    // Enabled status
    if (enabled != oldStepperEnabled) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 80);
        tft.printf("%s   ", oldStepperEnabled ? "ON" : "OFF");
        
        // Draw new
        oldStepperEnabled = enabled;
        tft.setTextColor(TFT_GREENISH_TINT, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 80);
        tft.printf("%s", enabled ? "ON" : "OFF");
    }
    
    // Draw header once (doesn't change)
    static bool headerDrawn = false;
    if (!headerDrawn) {
        tft.setTextColor(TFT_GREENISH_TINT, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y);
        tft.println("=== Stepper ===");
        headerDrawn = true;
    }
}

void ImageRendererEnhanced::drawDualMotorUI(const char* escMode, const char* escDir, 
                                           int escThrottle, long stepperPos, 
                                           long stepperTarget, float stepperSpeed, 
                                           bool stepperRunning, bool stepperEnabled) {
    // Draw ESC status on left side
    drawESCStatus(10, 50, escMode, escDir, escThrottle, 0);
    
    // Draw Stepper status on right side (or below)
    drawStepperStatus(10, 150, stepperPos, stepperTarget, stepperSpeed, 
                     stepperRunning, stepperEnabled);
}