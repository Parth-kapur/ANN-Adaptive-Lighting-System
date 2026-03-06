// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <Wire.h>
// #include <BH1750.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // ================= WIFI =================
// const char* ssid     = "Parth's S24 Ultra";
// const char* password = "Parth910";
// const char* SERVER   = "10.207.209.37";   // Flask server IP

// // ================= OLED =================
// Adafruit_SSD1306 display(128, 64, &Wire, -1);

// // ================= PINS =================
// #define MOSFET_PIN 26
// #define PIR_PIN    33

// // ================= PWM =================
// #define PWM_CHANNEL 0
// #define PWM_FREQ    5000
// #define PWM_RES     8

// // ================= SENSOR =================
// BH1750 lightMeter;

// // ================= STATE =================
// float lux = 0;
// int brightness = 40;

// bool manualMode = true;          // 🔥 DEFAULT SAFE
// int manualBrightness = 40;
// bool manualPower = true;

// bool motionDetected = false;
// unsigned long lastMotionTime = 0;
// #define NO_MOTION_TIMEOUT 30000

// unsigned long lastControl = 0;
// unsigned long lastHTTP = 0;
// unsigned long lastOLED = 0;

// // ================= PWM GAMMA =================
// int gammaPWM(int b) {
//   b = constrain(b, 0, 90);
//   if (b == 0) return 0;

//   float x = (float)b / 90.0;
//   float corrected = pow(x, 2.2);

//   const int PWM_MIN = 8;
//   const int PWM_MAX = 255;

//   return constrain(PWM_MIN + corrected * (PWM_MAX - PWM_MIN), 0, 255);
// }

// // ================= HTTP GET =================
// String httpGET(String url) {
//   HTTPClient http;
//   http.setTimeout(300);
//   http.begin(url);
//   int code = http.GET();
//   String res = (code == 200) ? http.getString() : "";
//   http.end();
//   res.trim();
//   return res;
// }

// // ================= ANN SERVER CALL =================
// int getANNBrightness(float lux, bool motion, int hour) {
//   if (WiFi.status() != WL_CONNECTED) {
//     return brightness;
//   }

//   Serial.println(">>> CALLING ANN SERVER <<<");

//   HTTPClient http;
//   http.setTimeout(800);
//   http.begin("http://" + String(SERVER) + ":5000/predict");
//   http.addHeader("Content-Type", "application/json");

//   String payload = "{";
//   payload += "\"lux\":" + String(lux) + ",";
//   payload += "\"motion\":" + String(motion ? 1 : 0) + ",";
//   payload += "\"hour\":" + String(hour);
//   payload += "}";

//   int code = http.POST(payload);

//   int b = brightness;   // fallback
//   if (code == 200) {
//     String res = http.getString();
//     Serial.print("ANN RESPONSE: ");
//     Serial.println(res);

//     int idx = res.indexOf(":");
//     if (idx >= 0) {
//       b = res.substring(idx + 1).toInt();
//     }
//   } else {
//     Serial.print("ANN HTTP ERROR: ");
//     Serial.println(code);
//   }

//   http.end();
//   return constrain(b, 0, 90);
// }

// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);

//   pinMode(PIR_PIN, INPUT);

//   ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
//   ledcAttachPin(MOSFET_PIN, PWM_CHANNEL);

//   Wire.begin(21, 22);
//   lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

//   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
//   display.setTextColor(SSD1306_WHITE);
//   display.clearDisplay();
//   display.setCursor(0, 0);
//   display.println("Smart Light");
//   display.println("Booting...");
//   display.display();

//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(300);
//   }

//   display.clearDisplay();
//   display.println("WiFi Connected");
//   display.display();

//   Serial.println("ESP32 READY");
// }

// // ================= LOOP =================
// void loop() {
//   unsigned long now = millis();

//   // -------- SENSOR --------
//   lux = lightMeter.readLightLevel();
//   if (lux < 0 || lux > 100000) lux = 0;

//   int hour = (millis() / 60000) % 24;   // 🔥 DEMO HOUR FIX

//   // -------- MOTION --------
//   if (digitalRead(PIR_PIN)) {
//     motionDetected = true;
//     lastMotionTime = now;
//   }
//   if (now - lastMotionTime > NO_MOTION_TIMEOUT) {
//     motionDetected = false;
//   }

//   // -------- CONTROL --------
//   if (now - lastControl > 500) {
//     lastControl = now;

//     Serial.print("manualMode = ");
//     Serial.println(manualMode);

//     if (!manualMode) {
//       Serial.print("AUTO | Lux=");
//       Serial.print(lux);
//       Serial.print(" Motion=");
//       Serial.print(motionDetected);
//       Serial.print(" Hour=");
//       Serial.println(hour);

//       brightness = getANNBrightness(lux, motionDetected, hour);
//     } else {
//       brightness = manualPower ? manualBrightness : 0;
//     }

//     ledcWrite(PWM_CHANNEL, gammaPWM(brightness));
//   }

//   // -------- MODE SYNC (CRITICAL FIX) --------
//   if (now - lastHTTP > 1000) {
//     lastHTTP = now;

//     String mode = httpGET("http://" + String(SERVER) + ":5000/get-mode");
//     mode.toLowerCase();                      // 🔥 FIX
//     manualMode = (mode == "manual");

//     if (manualMode) {
//       String p = httpGET("http://" + String(SERVER) + ":5000/get-power");
//       manualPower = (p == "1");

//       String b = httpGET("http://" + String(SERVER) + ":5000/get-brightness");
//       if (b.length() > 0) manualBrightness = b.toInt();
//     }
//   }

//   // -------- OLED --------
//   if (now - lastOLED > 500) {
//     lastOLED = now;

//     display.clearDisplay();
//     display.setCursor(0, 0);

//     display.print("Lux: ");
//     display.println(lux);

//     display.print("Brightness: ");
//     display.println(brightness);

//     display.print("Mode: ");
//     display.println(manualMode ? "MANUAL" : "AUTO (ANN)");

//     display.print("Hour: ");
//     display.println(hour);

//     display.println(motionDetected ? "Motion detected" : "No motion");
//     display.display();
//   }
// }
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= WIFI =================
const char *ssid = "Parth iPhone";
const char *password = "Parth 910";
const char *SERVER = "172.20.10.3";

// ================= OLED =================
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ================= PINS =================
#define MOSFET_PIN 26
#define PIR_PIN 33

// ================= PWM =================
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RES 8

// ================= SENSOR =================
BH1750 lightMeter;

// ================= STATE =================
float lux = 0;
int brightness = 40;

bool manualMode = true;
int manualBrightness = 40;
bool manualPower = true;

bool motionDetected = false;
unsigned long lastMotionTime = 0;

unsigned long lastControl = 0;
unsigned long lastHTTP = 0;
unsigned long lastOLED = 0;

// ================= NO MOTION TIMER =================
bool noMotionCountdown = false;
int countdownValue = 10;
unsigned long countdownStart = 0;

// ================= PWM GAMMA =================
int gammaPWM(int b)
{
  b = constrain(b, 0, 90);
  if (b == 0)
    return 0;

  float x = (float)b / 90.0;
  float corrected = pow(x, 2.2);

  return constrain(8 + corrected * (255 - 8), 0, 255);
}

// ================= HTTP GET =================
String httpGET(String url)
{
  HTTPClient http;
  http.setTimeout(300);
  http.begin(url);
  int code = http.GET();
  String res = (code == 200) ? http.getString() : "";
  http.end();
  res.trim();
  return res;
}

// ================= ANN CALL =================
int getANNBrightness(float lux, bool motion, int hour)
{
  if (WiFi.status() != WL_CONNECTED)
    return brightness;

  HTTPClient http;
  http.setTimeout(800);
  http.begin("http://" + String(SERVER) + ":5000/predict");
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"lux\":" + String(lux) + ",";
  payload += "\"motion\":" + String(motion ? 1 : 0) + ",";
  payload += "\"hour\":" + String(hour);
  payload += "}";

  int code = http.POST(payload);

  int b = brightness;
  if (code == 200)
  {
    String res = http.getString();
    int idx = res.indexOf(":");
    if (idx >= 0)
      b = res.substring(idx + 1).toInt();
  }

  http.end();
  return constrain(b, 0, 90);
}

// ================= PUSH DATA =================
void pushData(float lux, int brightness, bool manualMode, bool motion, int hour)
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  HTTPClient http;
  http.begin("http://" + String(SERVER) + ":5000/push-data");
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"lux\":" + String(lux) + ",";
  payload += "\"brightness\":" + String(brightness) + ",";
  payload += "\"mode\":\"" + String(manualMode ? "manual" : "auto") + "\",";
  payload += "\"motion\":" + String(motion ? 1 : 0) + ",";
  payload += "\"hour\":" + String(hour);
  payload += "}";

  http.POST(payload);
  http.end();
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOSFET_PIN, PWM_CHANNEL);

  Wire.begin(21, 22);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.println("Smart Light");
  display.println("Booting...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(300);

  display.clearDisplay();
  display.println("WiFi Connected");
  display.display();
}

// ================= LOOP =================
void loop()
{
  unsigned long now = millis();

  lux = lightMeter.readLightLevel();
  if (lux < 0 || lux > 100000)
    lux = 0;

  int hour = (millis() / 3600000) % 24;

  // -------- MOTION --------
  if (digitalRead(PIR_PIN))
  {
    motionDetected = true;
    lastMotionTime = now;
    noMotionCountdown = false;
  }

  unsigned long noMotionDuration = now - lastMotionTime;

  // -------- COUNTDOWN (10s → 20s) --------
  if (noMotionDuration >= 10000 && noMotionDuration < 20000)
  {
    if (!noMotionCountdown)
    {
      noMotionCountdown = true;
      countdownValue = 10;
      countdownStart = now;
    }

    if (now - countdownStart >= 1000 && countdownValue > 0)
    {
      countdownValue--;
      countdownStart = now;
    }
  }

  // -------- FORCE OFF AT 20s --------
  if (noMotionDuration >= 20000)
  {
    brightness = 0;
    ledcWrite(PWM_CHANNEL, 0);
  }

  // -------- CONTROL --------
  if (now - lastControl > 500 && noMotionDuration < 20000)
  {
    lastControl = now;

    if (!manualMode)
    {
      brightness = getANNBrightness(lux, motionDetected, hour);
    }
    else
    {
      brightness = manualPower ? manualBrightness : 0;
    }

    ledcWrite(PWM_CHANNEL, gammaPWM(brightness));
  }

  // -------- MODE SYNC --------
  if (now - lastHTTP > 1000)
  {
    lastHTTP = now;

    String mode = httpGET("http://" + String(SERVER) + ":5000/get-mode");
    mode.toLowerCase();
    manualMode = (mode == "manual");

    if (manualMode)
    {
      manualPower = httpGET("http://" + String(SERVER) + ":5000/get-power") == "1";
      String b = httpGET("http://" + String(SERVER) + ":5000/get-brightness");
      if (b.length())
        manualBrightness = b.toInt();
    }

    pushData(lux, brightness, manualMode, motionDetected, hour);
  }

  // -------- OLED --------
  if (now - lastOLED > 500)
  {
    lastOLED = now;

    display.clearDisplay();
    display.setCursor(0, 0);

    display.print("Lux: ");
    display.println(lux);

    display.print("Brightness: ");
    display.println(brightness);

    display.print("Mode: ");
    display.println(manualMode ? "MANUAL" : "AUTO");

    display.println(motionDetected ? "Motion" : "No Motion");

    if (noMotionCountdown && countdownValue > 0)
    {
      display.print("Off in: ");
      display.println(countdownValue);
    }

    display.display();
  }
}