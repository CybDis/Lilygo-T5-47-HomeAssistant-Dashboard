#include "configurations_security.h"

// Start of reserved configurations. Do not change if you dont know what you are doing
enum entity_state {ON, OFF, ERROR, UNAVAILABLE};
enum entity_type {SWITCH, LIGHT, EXFAN, FAN, AIRPURIFIER, WATERHEATER, PLUG, AIRCONDITIONER, PLANT};
enum entity_state_type {ONOFF, VALUE};
enum sensor_type {DOOR, MOTION, ENERGYMETER, TEMP, ENERGYMETERPWR};
struct HAEntities{
    String entityName;
    String entityID;
    int entityType;
    int entityStateType;
};

struct HAConfigurations{
    String timeZone;
    String version;
    String haStatus;
};
// End of reserved configurations

// NOTE: Create configurations_security.h with these lines:
// Start of user security configurations. Change as instructed in the comments
// Change to your WiFi credentials
// const char* ssid        = "...";
// const char* password    = "...";
// create a long lived access token and put it here. ref: https://www.home-assistant.io/docs/authentication/
// const String ha_token   = "...";


// url to HA server. Only http is supported for now!
const String ha_server  = "http://192.168.1.88:8123";

// GMT Offset in seconds. UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
int   gmtOffset_sec     = -28800;

// deep sleep configurations
long SleepDuration   = 60; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour      = 8;  // Wakeup after 06:00 to save battery power
int  SleepHour       = 23; // Sleep  after 23:00 to save battery power

/**
 *  Entities are shown in top two rows. Supported types are in entity_type and different icons are used for easy recognition
 *  Only 12 different entities are supported 6 cols x 2 rows. 
 *  Entities follow the format of { <Name that should be displayed>, <entity_id in HA>, <entity_type>, <entity_state_type>}
 *  User a short entity name so it can fit nicely in 160px width in 9px font. 
**/
HAEntities haEntities [] {
    {"Plant 1", "sensor.plant_1_moisture", PLANT, VALUE},
    {"Plant 2 Tall", "sensor.plant_2_tall_moisture", PLANT, VALUE},
};

/**
 *  Sensors are shown in 3rd row. Supported types are DOOR and MOTION currently. Different icons are used for easy recognition
 *  Only 8 different entities are supported - 8 cols. 
 *  Entities follow the format of { <Name that should be displayed>, <entity_id in HA>, <DOOR|MOTION>, <ONOFF>}
 *  User a short entity name so it can fit nicely in 120px width in 9px font. 
**/
HAEntities haSensors[] {
    {"Kitchen Door", "binary_sensor.lumi_lumi_sensor_magnet_aq2_1d3d0b07_on_off", DOOR, ONOFF},
};

/**
 *  Sensors are shown in last row. Supported types are ENERGYMETER, ENERGYMETERPWR and  and TEMP currently. Different icons are used for easy recognition
 *  Only 3 different entities are supported - 3 cols. ENERGYMETER and ENERGYMETERPWR are grouped together and shown as total
 *  Entities follow the format of { <Name that should be displayed>, <entity_id in HA>, <ENERGYMETER|ENERGYMETERPWR|TEMP>, <VALUE>}
 *  User a short entity name so it can fit nicely in 120px width in 9px font. 
 *  You can have only one ENERGYMETER type and ENERGYMETERPWR type tile in the display. ENERGYMETER and ENERGYMETERPWR are grouped together and shown as total
 *  However you can have multiple temrature tiles (up to 3 if you are not using ENERGYMETER and ENERGYMETERPWR in you HA instances) 
 *  Or you can customize the code the way you see fit (advanced)
**/
HAEntities haFloatSensors[] {
    {"Outside", "weather.2875_home", TEMP, VALUE},
};
