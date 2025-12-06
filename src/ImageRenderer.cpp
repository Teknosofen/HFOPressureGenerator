#include "ImageRenderer.hpp"
#include "Free_Fonts.h"

ImageRenderer::ImageRenderer() 
    : oldESCThrottle(-1),
      oldESCADC(-1),
      oldStepperPos(-1),
      oldStepperTarget(-1),
      oldStepperSpeed(-1.0),
      oldStepperRunning(false),
      oldStepperEnabled(false) {
}

void ImageRenderer::init() {
    tft.init();
    tft.setRotation(1);
    tft.setFreeFont(FSS9); // &FreeSans9pt7b);
    tft.fillScreen(TFT_LOGOBACKGROUND);
    tft.setTextColor(TFT_GREEN, TFT_LOGOBACKGROUND);
    tft.setTextSize(1);
}

void ImageRenderer::drawHeader(const char* title, const char* version) {
    tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
    tft.setTextSize(1);
    tft.setCursor(10, 20);
    tft.printf("%s %s", title, version);
}

void ImageRenderer::drawESCStatus(int x, int y, int throttle, int adc) {
    // Throttle (microseconds)
    if (throttle != oldESCThrottle) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Throttle: %d us   ", oldESCThrottle);
        
        // Draw new
        oldESCThrottle = throttle;
        tft.setTextColor(TFT_BRIGHT_RED, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 20);
        tft.printf("Throttle: %d us", throttle);
    }
    
    // ADC value
    if (adc != oldESCADC) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("ADC: %d    ", oldESCADC);
        
        // Draw new
        oldESCADC = adc;
        tft.setTextColor(TFT_BRIGHT_RED, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 40);
        tft.printf("ADC: %d", adc);
    }
    
    // Draw header once (doesn't change)
    static bool headerDrawn = false;
    if (!headerDrawn) {
        tft.setTextColor(TFT_BRIGHT_RED, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y);
        tft.println("Blower:");
        headerDrawn = true;
    }
}

void ImageRenderer::drawStepperStatus(int x, int y, long pos, long target, 
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
        tft.setTextColor(TFT_BRIGHT_GREEN, TFT_LOGOBACKGROUND);
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
        tft.setTextColor(TFT_BRIGHT_GREEN, TFT_LOGOBACKGROUND);
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
        tft.setTextColor(TFT_BRIGHT_GREEN, TFT_LOGOBACKGROUND);
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
        tft.setTextColor(TFT_BRIGHT_GREEN, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y + 80);
        tft.printf("%s", enabled ? "ON" : "OFF");
    }
    
    // Draw header once (doesn't change)
    static bool headerDrawn = false;
    if (!headerDrawn) {
        tft.setTextColor(TFT_BRIGHT_GREEN, TFT_LOGOBACKGROUND);
        tft.setCursor(x, y);
        tft.println("Stepper Valve");
        headerDrawn = true;
    }
}

void ImageRenderer::drawDualMotorUI(int escThrottle, int escADC,
                                         long stepperPos, long stepperTarget, 
                                         float stepperSpeed, bool stepperRunning, 
                                         bool stepperEnabled) {
    // Draw ESC status
    drawESCStatus(10, 50, escThrottle, escADC);
    
    tft.drawLine(160, 30, 160, 165, TFT_DARKERBLUE);

    // Draw Stepper status
    drawStepperStatus(170, 50, stepperPos, stepperTarget, stepperSpeed, 
                     stepperRunning, stepperEnabled);
}
