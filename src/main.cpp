#include <Arduino.h>

// esp32 sdk imports
#include "esp_heap_caps.h"
#include "esp_log.h"

// epd
#include "epd_driver.h"

// battery
#include <driver/adc.h>
#include "esp_adc_cal.h"

// deepsleep
#include "esp_sleep.h"

// font
#include "opensans8b.h"
#include "opensans9b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

// wifi
#include <WiFi.h>

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "configurations.h"
#include "homeassistantapi.h"
#include "epd_drawing.h"

// Icons for Home Assistant
#include "icons/waterheateron.h"
#include "icons/waterheateroff.h"
#include "icons/lightbulbon.h"
#include "icons/lightbulboff.h"
#include "icons/exhaustfanon.h"
#include "icons/exhaustfanoff.h"
#include "icons/fanoff.h"
#include "icons/fanon.h"
#include "icons/airpurifieron.h"
#include "icons/airpurifieroff.h"
#include "icons/plugon.h"
#include "icons/plugoff.h"
#include "icons/switchon.h"
#include "icons/switchoff.h"
#include "icons/airconditioneron.h"
#include "icons/airconditioneroff.h"
#include "icons/warning.h"
#include "icons/plantWateringlow.h"
#include "icons/plantWateringok.h"

// sensor icons
#include "icons/dooropen.h"
#include "icons/doorclosed.h"
#include "icons/motionsensoron.h"
#include "icons/motionsensoroff.h"
#include "icons/sensorerror.h"

#define White 0xFF
#define LightGrey 0xBB
#define Grey 0x88
#define DarkGrey 0x44
#define Black 0x00

#define BATT_PIN            36

#define TILE_IMG_WIDTH  100
#define TILE_IMG_HEIGHT 100
#define TILE_WIDTH      160
#define TILE_HEIGHT     160
#define TILE_GAP        6
#define SENSOR_TILE_WIDTH      120
#define SENSOR_TILE_HEIGHT     110
#define SENSOR_TILE_IMG_WIDTH  64
#define SENSOR_TILE_IMG_HEIGHT 64
#define BOTTOM_TILE_WIDTH      320
#define BOTTOM_TILE_HEIGHT     90


long StartTime       = 0;
long SleepTimer      = 0;
int  CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;

int vref = 1100; // default battery vref
int wifi_signal = 0;

// required for NTP time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;


uint8_t StartWiFi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection) {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000) { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    timeClient.begin();
    timeClient.setTimeOffset(gmtOffset_sec);
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else {
    wifi_signal = 0;
    Serial.println("WiFi connection *** FAILED ***"); 
  } 
  return connectionStatus;
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

// this will place a tile on screen that includes icon, staus and name of the HA entity
void DrawTile(int x, int y, int width, int height, const uint8_t *image_data, String state, String label)
{
  // this assumes images are 100x100px size. make sure images are cropped to 100x100 before converting
  int image_x = int((width - TILE_IMG_WIDTH)/2) + x; 
  int image_y = int((height - TILE_IMG_HEIGHT)/2) + y;
  int top_txt_cursor_x = width / 2 + x;
  int top_txt_cursor_y = image_y + TILE_IMG_HEIGHT + 10 + 12;
  int bottom_txt_cursor_x = int(width / 2) + x;
  int bottom_txt_cursor_y = y + 21;
  drawRect(x, y, width, height, Black);
  drawRect(x + 1, y + 1, width - 2, height - 2, Black);
  drawImage(image_x, image_y, TILE_IMG_WIDTH, TILE_IMG_HEIGHT, image_data);
  drawString(top_txt_cursor_x, top_txt_cursor_y, label, CENTER);
  drawString(bottom_txt_cursor_x, bottom_txt_cursor_y, state, CENTER);
}

void DrawSensorTile(int x, int y, int width, int height, const uint8_t* image_data, String label)
{
  // this assumes images are 128x128px size. make sure images are cropped to 128x128 before converting
  int image_x = int((width - SENSOR_TILE_IMG_WIDTH)/2) + x; 
  int image_y = y + 10;  
  int txt_cursor_x = int(width/2) + x;
  int txt_cursor_y = image_y + SENSOR_TILE_IMG_HEIGHT + 10 + 12;
  drawRect(x, y, width, height, Black);
  drawRect(x+1, y+1, width-2, height-2, Black);
  drawImage(image_x, image_y, SENSOR_TILE_IMG_WIDTH, SENSOR_TILE_IMG_HEIGHT, image_data);
  drawString(txt_cursor_x, txt_cursor_y, label, CENTER);
}

void DrawTile(int x, int y, int state, int type, String name, String value)
{
    int tile_width = TILE_WIDTH - TILE_GAP; 
    int tile_height = TILE_HEIGHT - TILE_GAP;
    String state_txt = "OFF";
    if (state == entity_state::ON) state_txt = "ON"; 
    else if (state == entity_state::UNAVAILABLE) state_txt = "UNAVAILABLE"; 
    switch (type)
    {
    case entity_type::SWITCH:
        if (state == entity_state::ON) DrawTile(x,y,tile_width,tile_height,switchon_data, name, state_txt); 
        else if (state == entity_state::OFF) DrawTile(x,y,tile_width,tile_height,switchoff_data, name, state_txt); 
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "SWITCH"); 
        break;
    case entity_type::LIGHT:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,lightbulbon_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,lightbulboff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "LIGHT"); 
        break;
    case entity_type::FAN:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,fanon_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,fanoff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "FAN"); 
        break;
    case entity_type::EXFAN:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,exhaustfanon_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,exhaustfanoff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "EXHAUST FAN"); 
        break;
    case entity_type::AIRPURIFIER:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,airpurifieron_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,airpurifieroff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "AIR PURIFIER"); 
        break;
    case entity_type::WATERHEATER:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,waterheateron_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,waterheateroff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "WATER HEATER"); 
        break;
    case entity_type::PLUG:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,plugon_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,plugoff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "PLUG"); 
        break;
    case entity_type::AIRCONDITIONER:
        if (state == entity_state::ON)  DrawTile(x,y,tile_width,tile_height,airconditioneron_data,name, state_txt);
        else if (state == entity_state::OFF)  DrawTile(x,y,tile_width,tile_height,airconditioneroff_data,name, state_txt);
        else DrawTile(x,y,tile_width,tile_height,warning_data, name, "AIR CONDITIONER"); 
    break;
  case entity_type::PLANT:
    if (value.toInt() >= 80)
      DrawTile(x, y, tile_width, tile_height, plantwateringok_data, name, value + "%");
    else if (value.toInt() > 0)
      DrawTile(x, y, tile_width, tile_height, plantwateringlow_data, name, value + "%");
    else
      DrawTile(x, y, tile_width, tile_height, warning_data, name, "PLANT");
        break;
    default:
        break;
    }
}

void DrawSensorTile(int x, int y, int state, int type, String name)
{
    int tile_width = SENSOR_TILE_WIDTH - TILE_GAP; 
    int tile_height = SENSOR_TILE_HEIGHT - TILE_GAP;
    switch (type)
    {
    case sensor_type::DOOR:
        if (state == entity_state::ON) DrawSensorTile(x,y,tile_width,tile_height,dooropen_data, name); 
        else if (state == entity_state::OFF) DrawSensorTile(x,y,tile_width,tile_height,doorclosed_data, name); 
        else DrawSensorTile(x,y,tile_width,tile_height,sensorerror_data, name); 
        break;
    case sensor_type::MOTION:
        if (state == entity_state::ON) DrawSensorTile(x,y,tile_width,tile_height,motionsensoron_data, name); 
        else if (state == entity_state::OFF) DrawSensorTile(x,y,tile_width,tile_height,motionsensoroff_data, name); 
        else DrawSensorTile(x,y,tile_width,tile_height,sensorerror_data, name); 
        break;
    default:
        break;
    }
}

void DrawBottomTile(int x, int y, String value, String name)
{
    int tile_width = BOTTOM_TILE_WIDTH - TILE_GAP; 
    int tile_height = BOTTOM_TILE_HEIGHT - TILE_GAP;
    drawRect(x, y, tile_width, tile_height, Black);
    drawRect(x+1, y+1, tile_width-2, tile_height-2, Black);
    setFont(OpenSans24B);
    drawString(int(tile_width/2) + x, 508, value, CENTER);
    setFont(OpenSans9B);
    drawString(int(tile_width/2) + x, 532, name, CENTER);
}

void DrawBottomBar()
{
    int tiles = 3;
    float totalEnergy = 0;
    float totalPower  = 0;
    String totalEnergyName;
    String totaPowerName;
    for (int i = 0; i < (sizeof(haFloatSensors) / sizeof(HAEntities)); i++){
        if (haFloatSensors[i].entityType == sensor_type::ENERGYMETER)
        {
            totalEnergy = totalEnergy + getSensorFloatValue(haFloatSensors[i].entityID);
            totalEnergyName = haFloatSensors[i].entityName;
        }
        else if (haFloatSensors[i].entityType == sensor_type::ENERGYMETERPWR)
        {
            totalPower = totalPower + getSensorFloatValue(haFloatSensors[i].entityID);
            totaPowerName = haFloatSensors[i].entityName;
        }
    }
    int x = 3;
    int y = 456;
    // first one 
    if (totalEnergy != 0)
    {
        DrawBottomTile(x, y, String(totalEnergy) + " kWh", totalEnergyName);
        x = x + BOTTOM_TILE_WIDTH;
        tiles--;
    }
    if (totalPower != 0)
    {
        DrawBottomTile(x, y, String((int)totalPower) + " W", totaPowerName);
        x = x + BOTTOM_TILE_WIDTH;
        tiles--;
    }
    
    for (int i = 0; i < (sizeof(haFloatSensors) / sizeof(HAEntities)); i++){
        if (haFloatSensors[i].entityType == sensor_type::TEMP && tiles >= 1)
        {
            float temp = getSensorAttributeValue(haFloatSensors[i].entityID, "temperature").toFloat();
            if (temp == 0)
                getSensorValue(haFloatSensors[i].entityID).toFloat();
            DrawBottomTile(x, y, String(temp, 1) + "° C", haFloatSensors[i].entityName);
            x = x + BOTTOM_TILE_WIDTH;
            tiles--;
        }
    }
}

void DrawSwitchBar()
{
    setFont(OpenSans9B);
    int x = 3;
    int y = 23;
    for (int i = 0; i < (sizeof(haEntities) / sizeof(HAEntities)); i++){
        if (haEntities[i].entityName != "") {
          if (haEntities[i].entityType == entity_type::SWITCH ||
              haEntities[i].entityType == entity_type::LIGHT ||
              haEntities[i].entityType == entity_type::PLUG ||
              haEntities[i].entityType == entity_type::EXFAN ||
              haEntities[i].entityType == entity_type::FAN ||
              haEntities[i].entityType == entity_type::AIRPURIFIER ||
              haEntities[i].entityType == entity_type::WATERHEATER ||
              haEntities[i].entityType == entity_type::AIRCONDITIONER)
          {            
              DrawTile(x, y, checkOnOffState(haEntities[i].entityID), haEntities[i].entityType, haEntities[i].entityName, "");
          }
          else 
          {
              String val = getSensorValue(haEntities[i].entityID);
              DrawTile(x, y, 0, haEntities[i].entityType, haEntities[i].entityName, val);
          }
        }
      
        x = x + TILE_WIDTH; // move column right
        if (i == 5) { // move row down
            x = 3;
            y = y + TILE_HEIGHT;
        }
    }
}

void DrawSensorBar()
{
    setFont(OpenSans9B);
    int x = 3;
    int y = 345;
    for (int i = 0; i < (sizeof(haSensors) / sizeof(HAEntities)); i++){
        if (haSensors[i].entityType == sensor_type::DOOR ||
            haSensors[i].entityType == sensor_type::MOTION )
        {
      if (haSensors[i].entityName != "")
            DrawSensorTile(x,y,checkOnOffState(haSensors[i].entityID),haSensors[i].entityType, haSensors[i].entityName);
        }
        x = x + SENSOR_TILE_WIDTH;
    }
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 20; //            <-20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 16; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 12; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 8; //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 4;  // -100dbm to  -81dbm displays 1-bar
    
    if (rssi != 0) 
      fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    else // draw empty bars
      drawRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
  if (rssi == 0) 
    drawString(x , y, "x", LEFT);
}

// void DrawBattery(int x, int y) {
//   uint8_t percentage = 100;
//   float voltage = analogRead(35) / 4096.0 * 7.46;
//   if (voltage > 1 ) { // Only display if there is a valid reading
//     Serial.println("Voltage = " + String(voltage));
//     percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
//     if (voltage >= 4.20) percentage = 100;
//     if (voltage <= 3.20) percentage = 0;  // orig 3.5
//     drawRect(x + 55, y - 15 , 40, 15, Black);
//     fillRect(x + 95, y - 9, 4, 6, Black);
//     fillRect(x + 57, y - 13, 36 * percentage / 100.0, 11, Black);
//     drawString(x, y, String(percentage) + "%", LEFT);
//     //drawString(x + 13, y + 5,  String(voltage, 2) + "v", CENTER);
//   }
// }

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  float voltage = analogRead(36) / 4096.0 * 6.566 * (vref / 1000.0);
  if (voltage > 1 ) { // Only display if there is a valid reading
    Serial.println("\nVoltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.20) percentage = 0;  // orig 3.5
     drawRect(x + 55, y - 15 , 40, 15, Black);
     fillRect(x + 95, y - 9, 4, 6, Black);
     fillRect(x + 57, y - 13, 36 * percentage / 100.0, 11, Black);
     drawString(x, y, String(percentage) + "%", LEFT);
     drawString(x + 130, y,  String(voltage, 2) + "v", CENTER);
  }
}

void SetupTime()
{
    Serial.println("Getting time...");

    while(!timeClient.update()) {
        timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);

    CurrentHour = timeClient.getHours();
    CurrentMin  = timeClient.getMinutes();
    CurrentSec  = timeClient.getSeconds();
}

void DisplayGeneralInfoSection()
{
    setFont(OpenSans8B);
    Serial.println("Getting haStatus...");
    HAConfigurations haConfigs = getHaStatus();
    Serial.println("drawing status line...");    
    drawString(EPD_WIDTH/2, 18, dayStamp + " - " +  timeStamp + " (HA Ver:" + haConfigs.version + "/" + haConfigs.haStatus + ", TZ:" + haConfigs.timeZone + ")", CENTER);
}

void DisplayStatusSection() {
  setFont(OpenSans8B);
  DrawBattery(5, 18);
  DrawRSSI(900, 18, wifi_signal);
}

void DrawWifiErrorScreen()
{
    epd_clear();
    DisplayStatusSection();
    epd_update();
}

void DrawHAScreen()
{
    epd_clear();
    
    DisplayStatusSection();
    DisplayGeneralInfoSection();
    Serial.println("Drawing switchBar...");
    DrawSwitchBar();
    Serial.println("Drawing sensorBar...");
    DrawSensorBar();
    Serial.println("Drawing bottomBar...");
    DrawBottomBar();

    epd_update();
}

void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  while (!Serial);
  Serial.println(String(__FILE__) + "\nStarting...");
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) Serial.println("Memory alloc failed!");
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  setFont(OpenSans9B);
}

void BeginSleep() {
  epd_poweroff_all();
  SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec));
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL); // in Secs, 1000000LL converts to Secs as unit = 1uSec
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Entering " + String(SleepTimer) + " (secs) of sleep time");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

void setup() {
  InitialiseSystem();

  if (StartWiFi() == WL_CONNECTED) {
      SetupTime();

      bool WakeUp = false;
      if (WakeupHour > SleepHour)
        WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
      else
        WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
      if (WakeUp) {
          DrawHAScreen();
      }
  }
  else {
    DrawWifiErrorScreen();
  }
  BeginSleep();
}

void loop() {    
  // nothing to do here
}

