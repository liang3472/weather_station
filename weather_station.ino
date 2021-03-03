#include <WiFiManager.h>
#include <JsonListener.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <Astronomy.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include "logo.h"
#include "weathericons.h"
#include "settings.h"

#define INIT_WIFI 0
#define UPDATE_DATA 1
#define UPDATE_VIEW 2
long lastDownloadUpdate = millis();
time_t dstOffset = 0;
long timerPress;
long lastDraw = 0;
uint16_t screen = INIT_WIFI;
String moonAgeImage = "";
uint8_t moonAge = 0;
TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h
simpleDSTadjust dstAdjusted(StartRule, EndRule);
OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
Astronomy::MoonData moonData;

// 连接wifi
void connectWifi()
{
  WiFiManager wifiManager;
  updateWifiStatus(40, "Try connect Wifi");
  if (WiFi.SSID() == "")
  {
    updateWifiStatus(60, "Please configure Wifi");
  }
  wifiManager.setBreakAfterConfig(true);
  // 测试清除密码
  // wifiManager.resetSettings();

  if (!wifiManager.autoConnect("Weather_Station"))
  {
    updateWifiStatus(70, "Failed to connect");
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
    updateWifiStatus(80, "Please retry");
  }

  updateWifiStatus(90, "Connected:" + WiFi.localIP());
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

// 初始化显示屏
void initDisplay()
{

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(0x6E8A);
  TJpgDec.setCallback(tft_output);
}

// 绘制logo
void drawLogo()
{
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.drawJpg(74, 20, logo, sizeof(logo));
}

// 更新进度
void drawProgress(uint8_t percentage, String text)
{
  tft.setTextDatum(CC_DATUM);
  tft.setFreeFont(FONT_STYLE9);
  tft.setTextColor(TEXT_COLOR, 0x6E8A);
  tft.drawString(text, 120, 160, 1);
  tft.drawRect(20, 130, 240 - 40, 6, TEXT_COLOR);
  tft.fillRect(22, 132, (240 - 40 - 4) * percentage / 100, 2, TFT_WHITE);
}

// 更新wifi状态
void updateWifiStatus(uint8_t percentage, String text)
{
  tft.fillScreen(0x6E8A);
  drawProgress(percentage, text);
  drawLogo();
}

// 加载数据状态
void updateDataState(uint8_t percentage, String text)
{
  tft.fillScreen(0x6E8A);
  drawProgress(percentage, text);
  drawLogo();
}

// 初始化wifi
void initWifi()
{
  updateWifiStatus(20, "Please wait...");
  connectWifi();
  updateWifiStatus(100, "Hello Aliao~");
}

// 加载数据
void loadData()
{
  screen = UPDATE_DATA;
  Serial.println("loading data...");
  updateDataState(10, "Updating time...");
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  while (!time(nullptr))
  {
    Serial.print("#");
    delay(100);
  }
  // calculate for time calculation how much the dst class adds.
  dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - time(nullptr);
  Serial.printf("Time difference for DST: %d", dstOffset);

  updateDataState(50, "Updating conditions");
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent();
  currentWeatherClient->setMetric(true);
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient->updateCurrent(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION);
  delete currentWeatherClient;
  currentWeatherClient = nullptr;

  updateDataState(70, "Updating forecasts...");
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast();
  forecastClient->setMetric(true);
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12, 0};
  forecastClient->setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient->updateForecasts(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION, MAX_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;

  updateDataState(80, "Updating astronomy...");
  Astronomy *astronomy = new Astronomy();
  moonData = astronomy->calculateMoonData(time(nullptr));
  float lunarMonth = 29.53;
  moonAge = moonData.phase <= 4 ? lunarMonth * moonData.illumination / 2 : lunarMonth - moonData.illumination * lunarMonth / 2;
  moonAgeImage = String((char)(65 + ((uint8_t)((26 * moonAge / 30) % 26))));
  delete astronomy;
  astronomy = nullptr;

  updateDataState(100, "Data ready~!");
  updateView();
}

void updateDate()
{
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&now);
  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(FONT_STYLE9_BOLD);
  tft.setTextColor(TEXT_COLOR, TFT_BLACK);
  String date = ctime(&now);
  date = String(1900 + timeinfo->tm_year) + " " + date.substring(0, 11);
  tft.drawString(date, 230, 5, 1);
}

void updateTime()
{
  char time_str[11];
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&now);
  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(FONT_STYLE12_BOLD);
  tft.setTextColor(TEXT_COLOR, TFT_BLACK);
  tft.setFreeFont(&FreeSansBold18pt7b);
  if (IS_STYLE_12HR)
  {
    int hour = (timeinfo->tm_hour + 11) % 12 + 1; // take care of noon and midnight
    sprintf(time_str, "%2d:%02d:%02d\n", hour, timeinfo->tm_min, timeinfo->tm_sec);
    tft.drawString(time_str, 230, 33, 1);
  }
  else
  {
    sprintf(time_str, "%02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    tft.drawString(time_str, 230, 33, 1);
  }
}

void updateView()
{
  screen = UPDATE_VIEW;
  tft.fillScreen(TFT_BLACK);
  drawCurrentWeather();
  updateDate();
  updateTime();
  int x = 0, y = 0;
  drawForecastDetail(x + 10, y + 180, 0);
  drawForecastDetail(x + 95, y + 180, 1);
  drawForecastDetail(x + 180, y + 180, 2);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  if (y >= tft.height())
    return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality()
{
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100)
  {
    return 0;
  }
  else if (dbm >= -50)
  {
    return 100;
  }
  else
  {
    return 2 * (dbm + 100);
  }
}

void drawWifiQuality()
{
  int8_t quality = getWifiQuality();
  uint16_t color = TFT_YELLOW;
  if (quality > 70)
  {
    color = TFT_GREEN;
  }
  else if (quality < 40)
  {
    color = TFT_RED;
  }
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(String(quality) + "%", 20, 5);
  for (int8_t i = 0; i < 4; i++)
  {
    for (int8_t j = 0; j < 2 * (i + 1); j++)
    {
      if (quality > i * 25 || j == 0)
      {

        tft.drawRect(4 * i, 0 + 5, 1, 18, TFT_BLACK);
        tft.fillRect(4 * i, j + 5, 1, 18 - j, color);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  initDisplay();
  initWifi();
  delay(800);
  loadData();
  timerPress = millis();
}

void drawWeatherText(String value)
{
  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(value, 230, 108);
}

void drawTemp(float value)
{
  String degreeSign = "'C";
  // 根据温度设置颜色
  uint16_t color = 0xEE60;
  if (currentWeather.temp > 25)
  {
    color = 0xF2A7;
  }
  else if (currentWeather.temp < 8)
  {
    color = 0xBEDD;
  }
  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(&FreeSansBold18pt7b);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(value + degreeSign, 230, 72);
}

// 绘制当前天气
void drawCurrentWeather()
{
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);

  String iconText = currentWeather.icon;
  if (iconText == "01d" || iconText == "01n")
    TJpgDec.drawJpg(-10, 22, sunny, sizeof(sunny));
  else if (iconText == "02d" || iconText == "02n")
    TJpgDec.drawJpg(-10, 22, partlysunny, sizeof(partlysunny));
  else if (iconText == "03d" || iconText == "03n")
    TJpgDec.drawJpg(-10, 22, partlycloudy, sizeof(partlycloudy));
  else if (iconText == "04d" || iconText == "04n")
    TJpgDec.drawJpg(-10, 22, mostlycloudy, sizeof(mostlycloudy));
  else if (iconText == "09d" || iconText == "09n" || iconText == "10d" || iconText == "10n")
    TJpgDec.drawJpg(-10, 22, rain, sizeof(rain));
  else if (iconText == "11d" || iconText == "11n")
    TJpgDec.drawJpg(-10, 22, tstorms, sizeof(tstorms));
  else if (iconText == "13d" || iconText == "13n")
    TJpgDec.drawJpg(-10, 22, snow, sizeof(snow));
  else if (iconText == "50d" || iconText == "50n")
    TJpgDec.drawJpg(-10, 22, fog, sizeof(fog));
  else
    TJpgDec.drawJpg(-10, 22, sunny, sizeof(sunny));

  drawWeatherText(currentWeather.description);
  drawTemp(currentWeather.temp);
}

void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex)
{
  tft.setTextColor(TEXT_COLOR, TFT_BLACK);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextDatum(CC_DATUM);
  time_t time = forecasts[dayIndex].observationTime + dstOffset;
  struct tm *timeinfo = localtime(&time);
  tft.drawString(WDAY_NAMES[timeinfo->tm_wday], x + 25, y - 27);
  tft.drawString(String(timeinfo->tm_hour) + ":00", x + 25, y - 8);

  uint16_t color = 0xEE60;
  if (forecasts[dayIndex].temp > 25)
  {
    color = 0xF2A7;
  }
  else if (forecasts[dayIndex].temp < 8)
  {
    color = 0xBEDD;
  }
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(String(forecasts[dayIndex].temp, 1) + "'C", x + 25, y + 13);

  TJpgDec.setJpgScale(4);
  TJpgDec.setSwapBytes(true);

  String iconText = forecasts[dayIndex].icon;
  if (iconText == "01d" || iconText == "01n")
    TJpgDec.drawJpg(x + 8, y + 23, sunny, sizeof(sunny));
  else if (iconText == "02d" || iconText == "02n")
    TJpgDec.drawJpg(x + 8, y + 23, partlysunny, sizeof(partlysunny));
  else if (iconText == "03d" || iconText == "03n")
    TJpgDec.drawJpg(x + 8, y + 23, partlycloudy, sizeof(partlycloudy));
  else if (iconText == "04d" || iconText == "04n")
    TJpgDec.drawJpg(x + 8, y + 23, mostlycloudy, sizeof(mostlycloudy));
  else if (iconText == "09d" || iconText == "09n" || iconText == "10d" || iconText == "10n")
    TJpgDec.drawJpg(x + 8, y + 23, rain, sizeof(rain));
  else if (iconText == "11d" || iconText == "11n")
    TJpgDec.drawJpg(x + 8, y + 23, tstorms, sizeof(tstorms));
  else if (iconText == "13d" || iconText == "13n")
    TJpgDec.drawJpg(x + 8, y + 23, snow, sizeof(snow));
  else if (iconText == "50d" || iconText == "50n")
    TJpgDec.drawJpg(x + 8, y + 23, fog, sizeof(fog));
  else
    TJpgDec.drawJpg(x + 8, y + 23, sunny, sizeof(sunny));
}

void loop()
{
  if ((millis() - lastDraw > drawDST) && screen == UPDATE_VIEW)
  {
    lastDraw = millis();
    updateTime();
    drawWifiQuality();
  }
  // Check if we should update weather information
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS)
  {
    loadData();
    lastDownloadUpdate = millis();
  }
}
