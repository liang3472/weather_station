#include <simpleDSTadjust.h>

const int drawDST = 1000; // 绘制间隔，防止频繁刷新 单位ms

#define BG_COLOR 0x000//0x6E8A // 背景色 // #7CDC5C
#define TEXT_COLOR 0xFFFF // 文字颜色

#define FONT_STYLE12 &FreeMono12pt7b // 12pt文字样式
#define FONT_STYLE9 &FreeMono9pt7b // 9pt文字样式
#define FONT_STYLE12_BOLD &FreeMonoBold12pt7b // 12pt加粗文字样式
#define FONT_STYLE9_BOLD &FreeMonoBold9pt7b // 9pt加粗文字样式


const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// 时钟校准
#define NTP_SERVERS "0.ch.pool.ntp.org", "1.ch.pool.ntp.org", "2.ch.pool.ntp.org"

// 时区设置
#define UTC_OFFSET +8
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour

// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://home.openweathermap.org/users/sign_up
String OPEN_WEATHER_MAP_APP_ID = "c622aebce5269c828f701bdc9b43dbcd";
String OPEN_WEATHER_MAP_LOCATION = "Beijing,China";

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "en";
const uint8_t MAX_FORECASTS = 10;

// Change for 12 Hour/ 24 hour style clock
bool IS_STYLE_12HR = false;

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MOON_PHASES[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
                              "Full Moon", "Waning Gibbous", "Third quarter", "Waning Crescent"};