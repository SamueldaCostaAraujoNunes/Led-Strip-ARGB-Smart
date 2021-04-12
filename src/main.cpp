
#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif

#include <WS2812FX.h>
#include <SinricPro.h>
#include "LightsWithEffect.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <sstream>
#define RECV_PIN D5

#define APP_KEY ""
#define APP_SECRET ""
#define DEVICE_ID ""
#define INSTANCE_EFFECTS "Effects"

#define SSID ""
#define PASS ""

#define LED_COUNT 125
#define LED_PIN D3

#define BAUD_RATE 9600

LightsWithEffect &lightsWithEffect = SinricPro[DEVICE_ID];
WS2812FX fita = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
IRrecv irrecv(RECV_PIN);
decode_results results;

/*************
 * Variables *
 ***********************************************
 * Global variables to store the device states *
 ***********************************************/

std::map<String, int> modes = {
    {"Static", 0},
    {"Blink", 1},
    {"Breath", 2},
    {"Color Wipe", 3},
    {"Color Wipe Inverse", 4},
    {"Color Wipe Reverse", 5},
    {"Color Wipe Reverse Inverse", 6},
    {"Color Wipe Random", 7},
    {"Random Color", 8},
    {"Single Dynamic", 9},
    {"Multi Dynamic", 10},
    {"Rainbow", 11},
    {"Rainbow Cycle", 12},
    {"Scan", 13},
    {"Dual Scan", 14},
    {"Fade", 15},
    {"Theater Chase", 16},
    {"Theater Chase Rainbow", 17},
    {"Running Lights", 18},
    {"Twinkle", 19},
    {"Twinkle Random", 20},
    {"Twinkle Fade", 21},
    {"Twinkle Fade Random", 22},
    {"Sparkle", 23},
    {"Flash Sparkle", 24},
    {"Hyper Sparkle", 25},
    {"Strobe", 26},
    {"Strobe Rainbow", 27},
    {"Multi Strobe", 28},
    {"Blink Rainbow", 29},
    {"Chase White", 30},
    {"Chase Color", 31},
    {"Chase Random", 32},
    {"Chase Rainbow", 33},
    {"Chase Flash", 34},
    {"Chase Flash Random", 35},
    {"Chase Rainbow White", 36},
    {"Chase Blackout", 37},
    {"Chase Blackout Rainbow", 38},
    {"Color Sweep Random", 39},
    {"Running Color", 40},
    {"Running Red Blue", 41},
    {"Running Random", 42},
    {"Larson Scanner", 43},
    {"Comet", 44},
    {"Fireworks", 45},
    {"Fireworks Random", 46},
    {"Merry Christmas", 47},
    {"Fire Flicker", 48},
    {"Fire Flicker (soft)", 49},
    {"Fire Flicker (intense)", 50},
    {"Circus Combustus", 51},
    {"Halloween", 52},
    {"Bicolor Chase", 53},
    {"Tricolor Chase", 54}};

struct Device
{
  bool powerState = true;
  int brightness = 0;
  int speed = 50;
  struct
  {
    String name = "Static";
    int id = 0;
  } mode;
  struct
  {
    byte r = 0;
    byte g = 0;
    byte b = 0;
  } color;
};
Device device;

int lastChangeCloud = 0;
// int lastTimeIsConnected = 0;

bool thereIsChange = false;
bool thereIsChangePowerState = false;
bool thereIsChangeMode = false;
bool thereIsChangeBrightness = false;
bool thereIsChangeColor = false;
bool thereIsChangeSpeed = false;

/*********************
 * Actions In Device *
 *********************/
void deviceSetPowerState(bool state)
{
  Serial.printf("Powerstate changed to %s\r\n", state ? "on" : "off");
  device.powerState = state;
  if (state)
  {
    fita.start();
  }
  else
  {
    fita.stop();
  }
}

void deviceSetMode(int id)
{
  String nameMode = fita.getModeName(id);
  Serial.printf("Modesetting for \"%s\" set to mode %s\r\n", INSTANCE_EFFECTS, nameMode.c_str());
  device.mode.name = nameMode;
  device.mode.id = id;
  fita.setMode(id);
}

void deviceSetMode(String nameMode)
{
  int id = modes[nameMode];
  deviceSetMode(id);
}

void deviceSetBrightness(int brightness)
{
  Serial.printf("Brightness set to %d\r\n", brightness);
  device.brightness = brightness;
  fita.setBrightness(map(brightness, 0, 100, 0, 255));
}

void deviceSetColor(byte r, byte g, byte b)
{
  Serial.printf("Color set to red=%d, green=%d, blue=%d\r\n", r, g, b);
  device.color.r = r;
  device.color.g = g;
  device.color.b = b;
  fita.setColor(g, r, b);
}

void deviceSetSpeed(int speed)
{
  Serial.printf("Speed Percentage level set to %d\r\n", speed);
  fita.setSpeed(speed * 130);
}

/*************
 * Callbacks *
 *************/

// PowerStateController
bool onPowerState(const String &deviceId, bool &state)
{
  deviceSetPowerState(state);
  return true;
}

// ModeController
bool onSetMode(const String &deviceId, const String &instance, String &mode)
{
  deviceSetMode(mode);
  return true;
}

// BrightnessController
bool onBrightness(const String &deviceId, int &brightness)
{
  deviceSetBrightness(brightness);
  return true;
}

// AdjustBrightnessController
bool onAdjustBrightness(const String &deviceId, int &brightnessDelta)
{
  int sum = brightnessDelta + device.brightness;
  deviceSetBrightness(sum);
  return true;
}

// ColorController
bool onColor(const String &deviceId, byte &r, byte &g, byte &b)
{
  deviceSetColor(r, g, b);
  return true;
}

// PercentageController
bool onSetPercentage(const String &deviceId, int &percentage)
{
  deviceSetSpeed(percentage);
  return true; // request handled properly
}

bool onAdjustPercentage(const String &deviceId, int &percentageDelta)
{
  int sum = percentageDelta + device.speed;
  deviceSetSpeed(sum);
  return true;
}

/**********
 * Events *
 *************************************************
 * Examples how to update the server status when *
 * you physically interact with your device or a *
 * sensor reading changes.                       *
 *************************************************/

// PowerStateController
void updatePowerState(bool state)
{
  if (state != device.powerState)
  {
    deviceSetPowerState(state);
    lastChangeCloud = millis();
    thereIsChange = true;
    thereIsChangePowerState = true;
  }
}

// ModeController

void updateMode(int mode)
{
  if (mode != device.mode.id)
  {
    deviceSetMode(mode);
    lastChangeCloud = millis();
    thereIsChange = true;
    thereIsChangeMode = true;
  }
}

void updateMode(String nameMode)
{
  int id = modes[nameMode];
  updateMode(id);
}

// BrightnessController
void updateBrightness(int brightness)
{
  if (brightness != device.brightness)
  {
    deviceSetBrightness(brightness);
    lastChangeCloud = millis();
    thereIsChange = true;
    thereIsChangeBrightness = true;
  }
}

// ColorController
void updateColor(byte r, byte g, byte b)
{
  if (r != device.color.r || g != device.color.g || b != device.color.b)
  {
    deviceSetColor(r, g, b);
    lastChangeCloud = millis();
    thereIsChange = true;
    thereIsChangeColor = true;
  }
}

// PercentageController
void updatePercentageSpeed(int percentageSpeed)
{
  if (percentageSpeed != device.speed)
  {
    deviceSetSpeed(percentageSpeed);
    lastChangeCloud = millis();
    thereIsChange = true;
    thereIsChangeSpeed = true;
  }
}

// Decode Sinal Control
String decodeHex(decode_results *results)
{
  uint16_t count = results->rawlen;
  uint64_t number = results->value;
  unsigned long long1 = (unsigned long)((number & 0xFFFF0000) >> 16);
  unsigned long long2 = (unsigned long)((number & 0x0000FFFF));
  return String(long1, HEX) + String(long2, HEX); // six octets
}

struct CommandsController
{
  void NONE_EXIST()
  {
    return;
  }
  void BRIGHT_MOST()
  {
    int newBrightness = 0;
    if (device.brightness >= 75)
    {
      newBrightness = 100;
    }
    else
    {
      newBrightness = device.brightness + 25;
    }
    updateBrightness(newBrightness);
  }
  void BRIGHT_LESS()
  {
    int newBrightness = 0;
    if (device.brightness <= 25)
    {
      newBrightness = 0;
    }
    else
    {
      newBrightness = device.brightness - 25;
    }
    updateBrightness(newBrightness);
  }
  void OFF()
  {
    updatePowerState(false);
  }
  void ON()
  {
    updatePowerState(true);
  }
  void WHITE_COLOR()
  {
    updateColor(255, 255, 255);
  }
  void RED1()
  {
    updateColor(255, 0, 0);
  }
  void RED2()
  {
    updateColor(255, 64, 0);
  }
  void RED3()
  {
    updateColor(255, 128, 0);
  }
  void RED4()
  {
    updateColor(255, 192, 0);
  }
  void RED5()
  {
    updateColor(255, 255, 0);
  }
  void GREEN1()
  {
    updateColor(0, 255, 0);
  }
  void GREEN2()
  {
    updateColor(0, 255, 64);
  }
  void GREEN3()
  {
    updateColor(0, 255, 128);
  }
  void GREEN4()
  {
    updateColor(0, 255, 192);
  }
  void GREEN5()
  {
    updateColor(0, 255, 255);
  }
  void BLUE1()
  {
    updateColor(0, 0, 255);
  }
  void BLUE2()
  {
    updateColor(64, 0, 255);
  }
  void BLUE3()
  {
    updateColor(128, 0, 255);
  }
  void BLUE4()
  {
    updateColor(192, 0, 255);
  }
  void BLUE5()
  {
    updateColor(255, 0, 255);
  }
  void EFFECT_RUNNING_LIGHTS()
  {
    updateMode("Running Lights");
  }
  void EFFECT_FIRE_FLICKER()
  {
    updateMode("Fire Flicker");
  }
  void EFFECT_RAINBOW_CYCLE()
  {
    updateMode("Rainbow Cycle");
  }
  void EFFECT_STATIC()
  {
    updateMode("Static");
  }
};
CommandsController commandsController;

typedef void (CommandsController::*DoFunc)(void);

// typedef std::pair<std::string, DoFunc> Signals;
std::map<String, DoFunc> decodeComander = {
    {"f7c03f", &CommandsController::ON},
    {"f740bf", &CommandsController::OFF},
    {"f7807f", &CommandsController::BRIGHT_LESS},
    {"f7ff", &CommandsController::BRIGHT_MOST},
    {"f720df", &CommandsController::RED1},
    {"f7a05f", &CommandsController::GREEN1},
    {"f7609f", &CommandsController::BLUE1},
    {"f7e01f", &CommandsController::WHITE_COLOR},
    {"f710ef", &CommandsController::RED2},
    {"f7906f", &CommandsController::GREEN2},
    {"f750af", &CommandsController::BLUE2},
    {"f730cf", &CommandsController::RED3},
    {"f7b04f", &CommandsController::GREEN3},
    {"f7708f", &CommandsController::BLUE3},
    {"f78f7", &CommandsController::RED4},
    {"f78877", &CommandsController::GREEN4},
    {"f748b7", &CommandsController::BLUE4},
    {"f728d7", &CommandsController::RED5},
    {"f7a857", &CommandsController::GREEN5},
    {"f76897", &CommandsController::BLUE5},
    {"f7d02f", &CommandsController::EFFECT_RUNNING_LIGHTS},
    {"f7f00f", &CommandsController::EFFECT_FIRE_FLICKER},
    {"f7c837", &CommandsController::EFFECT_RAINBOW_CYCLE},
    {"f7e817", &CommandsController::EFFECT_STATIC}};

void loopIrRecv()
{
  String result = "";
  if (irrecv.decode(&results))
  {
    result = decodeHex(&results);
    irrecv.resume();
  }
  if (result != "")
  {
    Serial.println(result);
    DoFunc value = decodeComander[result];
    if (value)
    {
      (commandsController.*(value))();
    }
  }
}

void notifyPossibleChanges()
{
  if (thereIsChange)
  {
    if (millis() - 10000 > lastChangeCloud)
    {
      if (thereIsChangePowerState)
      {
        Serial.println("Atualizando SinricPro: PowerState");
        lightsWithEffect.sendPowerStateEvent(device.powerState);
        thereIsChangePowerState = false;
        return;
      }
      if (thereIsChangeMode)
      {
        Serial.println("Atualizando SinricPro: Mode");
        lightsWithEffect.sendModeEvent(INSTANCE_EFFECTS, device.mode.name, "PHYSICAL_INTERACTION");
        thereIsChangeMode = false;
        return;
      }
      if (thereIsChangeBrightness)
      {
        Serial.println("Atualizando SinricPro: Brightness");
        lightsWithEffect.sendBrightnessEvent(device.brightness);
        thereIsChangeBrightness = false;
        return;
      }
      if (thereIsChangeColor)
      {
        Serial.println("Atualizando SinricPro: Color");
        lightsWithEffect.sendColorEvent(device.color.r, device.color.g, device.color.b);
        thereIsChangeColor = false;
        return;
      }
      if (thereIsChangeSpeed)
      {
        Serial.println("Atualizando SinricPro: Speed");
        lightsWithEffect.sendSetPercentageEvent(device.speed);
        thereIsChangeSpeed = false;
        return;
      }
      thereIsChange = false;
    }
  }
}

/********* 
 * Setup *
 *********/

void setupSinricPro()
{

  // PowerStateController
  lightsWithEffect.onPowerState(onPowerState);

  // ModeController
  lightsWithEffect.onSetMode(INSTANCE_EFFECTS, onSetMode);

  // BrightnessController
  lightsWithEffect.onBrightness(onBrightness);
  lightsWithEffect.onAdjustBrightness(onAdjustBrightness);

  // ColorController
  lightsWithEffect.onColor(onColor);

  // PercentageController
  lightsWithEffect.onSetPercentage(onSetPercentage);
  lightsWithEffect.onAdjustPercentage(onAdjustPercentage);

  SinricPro.onConnected([] { Serial.printf("[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([] { Serial.printf("[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
};

void setupWS2812FX()
{
  fita.init();
  fita.setColor(PURPLE, BLUE);
  fita.setMode(FX_MODE_STATIC);
  fita.setBrightness(255);
  fita.setSpeed(3000);
  fita.start();
}

void setupWiFi()
{
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected\r\n");
}

/*********
 * Setup *
 *********/

void setup()
{
  Serial.begin(BAUD_RATE);
  // irrecv.enableIRIn();
  setupWiFi();
  setupWS2812FX();
  setupSinricPro();
}

/********
 * Loop *
 ********/

void loop()
{
  fita.service();
  SinricPro.handle();
  // loopIrRecv(); // O receptor infravermelho não funciona muito bem, quando algum efeito está ativo. Caso deseje controlar a fita apenas pela alexa e Sinric Pro, mantenha comentado.
  // notifyPossibleChanges();
}

