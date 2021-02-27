#include <WiFiManager.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include "logo.h"
#include "settings.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// 连接wifi
void connectWifi() {
  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);
  //清除密码
  //wifiManager.resetSettings();

  if (!wifiManager.autoConnect("Weather_Station")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

// 初始化显示屏
void initDisplay() {
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(BG_COLOR);
}

void drawLogo() {
  tft.setSwapBytes(true);
  tft.pushImage(74, 20, 92, 92, logo);

  // tft.setTextDatum(BC_DATUM);
  // tft.setFreeFont(FONT_STYLE12);
  // tft.setTextColor(TEXT_COLOR, BG_COLOR);
  // tft.drawString("Please wait...", 80, 100, 1);// Print the string name of the font
}

void drawProgress(uint8_t percentage, String text) {
  tft.setTextDatum(BC_DATUM);
  tft.setFreeFont(FONT_STYLE9);
  tft.setTextColor(TEXT_COLOR, BG_COLOR);
  tft.drawString(text, 120, 160, 1);
  tft.drawRect(20, 130, 240 - 40, 6, TEXT_COLOR);
  tft.fillRect(22, 132, (240 - 40 - 4) * percentage / 100, 2, TFT_BLUE);
}

void updateData() {
  tft.fillScreen(BG_COLOR);
  drawLogo();
  drawProgress(10, "Please wait...");
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  updateData();
  connectWifi();
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.println(WiFi.localIP());
}
