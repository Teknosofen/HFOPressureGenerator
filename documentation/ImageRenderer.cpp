#include "ImageRenderer.hpp"

ImageRenderer::ImageRenderer() 
    : oldValue(-1), oldAdcCounts(-1) {
}

void ImageRenderer::init() {
    tft.init();
    tft.setRotation(1);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.fillScreen(TFT_LOGOBACKGROUND);
    tft.setTextColor(TFT_GREEN, TFT_LOGOBACKGROUND);
    tft.setTextSize(1);
}

void ImageRenderer::setPositions(DisplayPos mode, DisplayPos value, DisplayPos dir, DisplayPos adc) {
    modePos = mode;
    valuePos = value;
    dirPos = dir;
    adcPos = adc;
}

void ImageRenderer::updateMode(const char* modeText) {
    if (oldModeText != modeText) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(modePos.x, modePos.y);
        tft.print(oldModeText.c_str());
        
        // Draw new
        oldModeText = modeText;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(modePos.x, modePos.y);
        tft.print(modeText);
    }
}

void ImageRenderer::updateValue(const char* valueLabel, int value) {
    if (value != oldValue || oldValueLabel != valueLabel) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(valuePos.x, valuePos.y);
        tft.printf("%s: %d", oldValueLabel.c_str(), oldValue);
        
        // Draw new
        oldValueLabel = valueLabel;
        oldValue = value;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(valuePos.x, valuePos.y);
        tft.printf("%s: %d", valueLabel, value);
    }
}

void ImageRenderer::updateDirection(const char* dirText) {
    if (oldDirText != dirText) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(dirPos.x, dirPos.y);
        tft.printf("Direction: %s", oldDirText.c_str());
        
        // Draw new
        oldDirText = dirText;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(dirPos.x, dirPos.y);
        tft.printf("Direction: %s", dirText);
    }
}

void ImageRenderer::updateADC(int adcCounts) {
    if (adcCounts != oldAdcCounts) {
        // Erase old
        tft.setTextColor(TFT_LOGOBACKGROUND, TFT_LOGOBACKGROUND);
        tft.setCursor(adcPos.x, adcPos.y);
        tft.printf("ADC counts: %d", oldAdcCounts);
        
        // Draw new
        oldAdcCounts = adcCounts;
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(adcPos.x, adcPos.y);
        tft.printf("ADC counts: %d", adcCounts);
    }
}

void ImageRenderer::drawFullUI(const char* modeText, const char* dirText,
                                const char* valueLabel, int value, int adcCounts) {
    updateMode(modeText);
    updateValue(valueLabel, value);
    updateDirection(dirText);
    updateADC(adcCounts);
}

void ImageRenderer::drawHeader(const char* hfoLabel, const char* hfoVer) {
        tft.setTextColor(TFT_DEEPBLUE, TFT_LOGOBACKGROUND);
        tft.setCursor(10, 20 );
        tft.print(hfoLabel);
        tft.print(hfoVer);
}