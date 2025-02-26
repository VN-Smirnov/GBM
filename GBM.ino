//version 0.0.2 for ESP32 devkit

#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <microDS3231.h>
#include <DHTStable.h>

#include "credintails.h"   //Password for wi-fi and mqtt broker and brocker IP address
#include "configuration.h" //Values and Calib Const for sensors and etc.

#define   CH_BUFF_SIZE      6     //Size of buffer for converting  from float to string values
#define   MQTT_CH_BUFF_SIZE 32

/* -----= Begin Structures =-----*/
typedef struct {
  float fValue;
  char sValue[CH_BUFF_SIZE];
  char mqttBrokerTopic[MQTT_CH_BUFF_SIZE];
} mqttMonitoringParameter_t;

typedef struct {
  int unixTime;
  String Time;  
  String Date;
  String mqttBrokerTopic;
} amazingTimeDate_t;


typedef struct {
  amazingTimeDate_t amazingTimeDate;
  mqttMonitoringParameter_t inCoolantTemperature;
  mqttMonitoringParameter_t outCoolantTemperature;
  mqttMonitoringParameter_t internalTemperature;
  mqttMonitoringParameter_t internalHumidity;
  mqttMonitoringParameter_t coolantPressure;
} MonitorParms_t; 
/* -----= End Structures =-----*/

/*------=Begin functions prototypes=------*/
void  pollDateTime(void);
void  pollDS18B20Temperatures(void);
void  pollPressures(void);
void  pollDHTValues(void);
void  showValuesOnLCD(const MonitorParms_t *);
void  showValuesOnConsole(const MonitorParms_t *);
void  sendValuesInMQTT(const MonitorParms_t *);
void  initWifi(void);
void  reconnectToMQTTBrocker(void);
/*------=End functions prototypes=------*/

/*------=Begin Global variables=------*/
uint32_t  currentTimeTicks      = 0;
uint32_t  currentTimeMQTTSend   = 0;
float     pressure = 71.0;

MonitorParms_t GBMParms = {
                          .amazingTimeDate = {.unixTime = 12, .Time = "InitTime", .Date = "InitDate", .mqttBrokerTopic = "Garage/Time"},
                          .inCoolantTemperature = { .fValue = -127, .sValue = "-127",  .mqttBrokerTopic = "Garage/inCoolantTemperature"},
                          .outCoolantTemperature = { .fValue = -127, .sValue = "-127", .mqttBrokerTopic = "Garage/outCoolantTemperature"},
                          .internalTemperature = { .fValue = - 127, .sValue = "-127", .mqttBrokerTopic = "Garage/internalTemperature"},
                          .internalHumidity = {.fValue = - 127, .sValue = "-127", .mqttBrokerTopic = "Garage/internalHumidity"},
                          .coolantPressure = {.fValue = - 127, .sValue = "-127", .mqttBrokerTopic = "Garage/coolantPressure"},
 };
/*------=End Global variables=------*/

/*------=Begin Obj Init=------*/
#if (ENABLE_LCD == 1)
  LiquidCrystal_I2C mainLCD(LCD_ADDRESS, LCD_COLLUMS, LCD_LINES);
#endif

OneWire mainWire(OWP_PIN);
DallasTemperature DS18B20Sens(&mainWire);

DHTStable mainDHT();

MicroDS3231 amazingClock();

uint8_t   addresDS18B20SensRed[] = {BS18B20_RED};
uint8_t   addresDS18B20SensBlack[] = {BS18B20_BLACK};

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqttServer = MQTT_BROCKER_ADDR;

WiFiClient GBMWiFiClient;
PubSubClient GBMMQTTClient(mqttServer, MQTT_PORT_ADDR, GBMWiFiClient);

/*------=End Obj Init=------*/

void setup() {
  #if(DEBUG == 1)
    Serial.begin(9600);
  #endif
  DS18B20Sens.begin();
  
  #if(ENABLE_LCD == 1)
    mainLCD.init();
    mainLCD.backlight();
  #endif
  
  pinMode(PRESS_SENS_PIN,  INPUT_PULLUP);
  
  initWifi();

}

void loop() {
  if (!GBMMQTTClient.connected()) 
    {
      reconnectToMQTTBrocker();
    }
  
  if(millis()-currentTimeTicks > POLLING_SENSOR_TIME_INTERVAL) {
    currentTimeTicks = millis();
    
    pollDateTime();
    pollDS18B20Temperatures();
    pollPressures();
    pollDHTValues();

    #if(LCD_ENABLED == 1)
      showValuesOnLCD(&GBMParms);
    #endif

    #if(DEBUG == 1)
      showValuesOnConsole(&GBMParms);
    #endif
  }

  if(millis()-currentTimeMQTTSend > SENT_TO_MQTT_INTERVAL){
    currentTimeMQTTSend = millis();

    sendValuesInMQTT(&GBMParms);
  }
  GBMMQTTClient.loop();

}

void pollDateTime(void){
  #if(FAKE_SENSORS == 0)

  #else
    GBMParms.amazingTimeDate.unixTime = 1458;
    GBMParms.amazingTimeDate.Time = "12:45";
    GBMParms.amazingTimeDate.Date = "21:02:2025";
  #endif
}

void pollDS18B20Temperatures(void){
  #if(FAKE_SENSORS == 0)

  #else
    GBMParms.inCoolantTemperature.fValue = static_cast<float>(random(0, 70));
    snprintf(GBMParms.inCoolantTemperature.sValue, sizeof(GBMParms.inCoolantTemperature.sValue), "%.1f", GBMParms.inCoolantTemperature.fValue);
    GBMParms.outCoolantTemperature.fValue = static_cast<float>(random(0, 50));
    snprintf(GBMParms.outCoolantTemperature.sValue, sizeof(GBMParms.outCoolantTemperature.sValue), "%.1f", GBMParms.outCoolantTemperature.fValue);
  #endif
}

void pollPressures(void){
  #if(FAKE_SENSORS == 0)

  #else
    GBMParms.coolantPressure.fValue = static_cast<float>(random(0, 20)/10);
    snprintf(GBMParms.coolantPressure.sValue, sizeof(GBMParms.coolantPressure.sValue), "%.1f", GBMParms.coolantPressure.fValue);
  #endif
}

void pollDHTValues(void) {
  #if(FAKE_SENSORS == 0)

  #else
    GBMParms.internalTemperature.fValue = static_cast<float>(random(-15, 60));
    snprintf(GBMParms.internalTemperature.sValue, sizeof(GBMParms.internalTemperature.sValue), "%.1f", GBMParms.internalTemperature.fValue);
    GBMParms.internalHumidity.fValue = static_cast<float>(random(0, 101));
    snprintf(GBMParms.internalHumidity.sValue, sizeof(GBMParms.internalHumidity.sValue), "%.1f", GBMParms.internalHumidity.fValue);
  #endif
}

void showValuesOnLCD(const MonitorParms_t* values){
  //print on LCD Time and Date
  mainLCD.setCursor(0, 0);
  mainLCD.print(values->amazingTimeDate.Date);
  mainLCD.print(" ");
  mainLCD.print(values->amazingTimeDate.Time);

  //print on LCD Coolant Temperatures
  //input Temperature
  mainLCD.setCursor(0, 1);
  mainLCD.print("Ti:");
  mainLCD.setCursor(0, 4);
  mainLCD.print(values->inCoolantTemperature.sValue);
  mainLCD.write(223);               
  mainLCD.print(" ");

  //output Temperature
  mainLCD.setCursor(10, 1);
  mainLCD.print("To:");
  mainLCD.setCursor(14, 1);
  mainLCD.print(values->outCoolantTemperature.sValue);
  mainLCD.write(223);
  
  //print DHT11 Temperature and Humidity
  //Temperature
  mainLCD.setCursor(0,2);
  mainLCD.print("Tg");
  mainLCD.setCursor(4,2);
  mainLCD.print(values->internalTemperature.sValue);
  mainLCD.write(223);

  //Humidity
  mainLCD.setCursor(10, 2);
  mainLCD.print("Hi:");
  mainLCD.setCursor(14, 2);
  mainLCD.print(values->internalHumidity.sValue);
  mainLCD.print("% ");

  //Print on LCD Coolant Pressure
  mainLCD.setCursor(0, 3);
  mainLCD.print("Pressure:");
  mainLCD.setCursor(10, 3);
  mainLCD.print(values->coolantPressure.sValue);
  mainLCD.print(" Bar ");

}

void showValuesOnConsole(const MonitorParms_t* values){
  Serial.println();
  
  Serial.print(F("Time: "));
  Serial.println(values->amazingTimeDate.Time);
  
  Serial.print(F("Date: "));
  Serial.println(values->amazingTimeDate.Date);
  
  Serial.print(F("Input Temp Coolant(float): ")); Serial.println(values->inCoolantTemperature.fValue);
  Serial.print(F("Input Temp Coolant (char): ")); Serial.println(values->inCoolantTemperature.sValue);
  
  Serial.print(F("Output Temp Coolant(float): ")); Serial.println(values->outCoolantTemperature.fValue);
  Serial.print(F("Output Temp Coolant (char): ")); Serial.println(values->outCoolantTemperature.sValue);

  Serial.print(F("Garage Temp(float): ")); Serial.println(values->internalTemperature.fValue);
  Serial.print(F("Garage Temp (char): ")); Serial.println(values->internalTemperature.sValue);

  Serial.print(F("Garage Humidity(float): ")); Serial.println(values->internalHumidity.fValue);
  Serial.print(F("Garage Humidity (char): ")); Serial.println(values->internalHumidity.sValue);

  Serial.print(F("Coolant Pressure(float): ")); Serial.println(values->coolantPressure.fValue);
  Serial.print(F("Coolant Pressure (char): ")); Serial.println(values->coolantPressure.sValue);

  Serial.println();
}

void sendValuesInMQTT(const MonitorParms_t* values){
  #if(DEBUG == 1)
    Serial.println("MQTT Send Function is Work!");
  #endif
    //Написат условия коннекта
    //GBMMQTTClient.publish(values->amazingTimeDate.mqttBrokerTopic, values->amazingTimeDate.Time);
    //delay(50);
    GBMMQTTClient.publish(values->inCoolantTemperature.mqttBrokerTopic, values->inCoolantTemperature.sValue);
    delay(50);
    GBMMQTTClient.publish(values->outCoolantTemperature.mqttBrokerTopic, values->outCoolantTemperature.sValue);       //Publish inCoolantTemperature
    delay(50);
    GBMMQTTClient.publish(values->internalTemperature.mqttBrokerTopic, values->internalTemperature.sValue);
    delay(50);
    GBMMQTTClient.publish(values->internalHumidity.mqttBrokerTopic, values->internalHumidity.sValue);
    delay(50);
    GBMMQTTClient.publish(values->coolantPressure.mqttBrokerTopic, values->coolantPressure.sValue);
}

void initWifi(void) {

    delay(10);
  // We start by connecting to a WiFi network

    #if(DEBUG == 1)
    
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ssid);
    
    #endif
    

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    #if(DEBUG == 1)
    
      while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
        Serial.print(".");
      }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    #endif
}

void reconnectToMQTTBrocker() {
  // Loop until we're reconnected
  while (!GBMMQTTClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (GBMMQTTClient.connect(MQTT_NAME_CLIENT, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      GBMMQTTClient.publish("temp/message","hello world");
      // ... and resubscribe
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(GBMMQTTClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
