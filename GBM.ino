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

#define   CH_BUFF_SIZE            6     //Size of buffer for converting  from float to string values
#define   MQTT_CH_BUFF_SIZE       32
#define   MQTT_CONNECT_ATTEMPT    5
#define   PRESSURE_CONST          71.0

/* -----= Begin Types and Structures =-----*/
typedef enum {
  connected,
  disconnected,
  connecting,
} connectionStatus_t;

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
  connectionStatus_t        mqttStatus;
  connectionStatus_t        wifiStatus;
  amazingTimeDate_t         amazingTimeDate;
  mqttMonitoringParameter_t inCoolantTemperature;
  mqttMonitoringParameter_t outCoolantTemperature;
  mqttMonitoringParameter_t internalTemperature;
  mqttMonitoringParameter_t internalHumidity;
  mqttMonitoringParameter_t coolantPressure;
} MonitorParms_t; 
/* -----= End Types and Structures =-----*/

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

MonitorParms_t GBMParms = {
                          .mqttStatus = disconnected,
                          .wifiStatus = disconnected,
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
  
  analogReadResolution(10);

  pinMode(PRESS_SENS_PIN,  INPUT_PULLUP);
  
  initWifi();

  reconnectToMQTTBrocker();
  
}

void loop() {
    
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
  #if(DS3231_FAKE_SENSORS == 0)
    GBMParms.amazingTimeDate.unixTime = amazingClock.getUnix(TIME_ZONE);
    GBMParms.amazingTimeDate.Time = amazingClock.getTimeString();
    GBMParms.amazingTimeDate.Date = amazingClock.getDateString();
  #else
    GBMParms.amazingTimeDate.unixTime = 1458;
    GBMParms.amazingTimeDate.Time = "12:45";
    GBMParms.amazingTimeDate.Date = "21:02:2025";
  #endif
}

void pollDS18B20Temperatures(void){
  #if(DS18B20_FAKE_SENSORS == 0)
    DS18B20Sens.requestTemperatures();
    delay(100);
    GBMParms.inCoolantTemperature.fValue = DS18B20Sens.getTempC(addresDS18B20SensRed);          //
    GBMParms.inCoolantTemperature.fValue = DS18B20Sens.getTempC(addresDS18B20SensBlack);        //
  #else
    GBMParms.inCoolantTemperature.fValue = static_cast<float>(random(0, 70));
    GBMParms.outCoolantTemperature.fValue = static_cast<float>(random(0, 50));
  #endif
  
  snprintf(GBMParms.inCoolantTemperature.sValue, sizeof(GBMParms.inCoolantTemperature.sValue), "%.1f", GBMParms.inCoolantTemperature.fValue);
  snprintf(GBMParms.outCoolantTemperature.sValue, sizeof(GBMParms.outCoolantTemperature.sValue), "%.1f", GBMParms.outCoolantTemperature.fValue);
}

void pollPressures(void){
  #if(PRESSURE_FAKE_SENSORS == 0)
    uint16_t  pressure;
    pressure = constain(analogRead(PRESS_SENS_PIN), MIN_PRESSURE_VALUE, MAX_PRESSURE_VALUE);
    GBMParms.coolantPressure.fValue = (pressure-MIN_PRESSURE_VALUE)/PRESSURE_CONST;
  #else
    GBMParms.coolantPressure.fValue = static_cast<float>(random(0, 20)/10);
  #endif
  
  snprintf(GBMParms.coolantPressure.sValue, sizeof(GBMParms.coolantPressure.sValue), "%.1f", GBMParms.coolantPressure.fValue);
}

void pollDHTValues(void) {
  #if(DHT11_FAKE_SENSORS == 0)
    mainDHT.read11(DHT11_PIN);
    delay(100);
    GBMParms.internalTemperature.fValue = mainDHT.getTemperature()-DHT11_TEMPERATURE_CORRECTION;
    GBMParms.internalHumidity.fValue = mainDHT.getHumidity();
  #else
    GBMParms.internalTemperature.fValue = static_cast<float>(random(-15, 60));
    GBMParms.internalHumidity.fValue = static_cast<float>(random(0, 101));
  #endif
  snprintf(GBMParms.internalTemperature.sValue, sizeof(GBMParms.internalTemperature.sValue), "%.1f", GBMParms.internalTemperature.fValue);
  snprintf(GBMParms.internalHumidity.sValue, sizeof(GBMParms.internalHumidity.sValue), "%.1f", GBMParms.internalHumidity.fValue);
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

   
    
      while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
         #if (DEBUG == 1)
          Serial.print(".");
        #endif
      }
    #if (DEBUG == 1)
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    #endif
}

void reconnectToMQTTBrocker() {
  uint8_t attemptMqttConnectCounter = 0;
  do {
      #if (DEBUG == 1)
      Serial.print("Attempting MQTT connection...");
      #endif

      // Attempt to connect
      if (GBMMQTTClient.connect(MQTT_NAME_CLIENT, MQTT_USER, MQTT_PASSWORD)) {
        #if (DEBUG == 1)
          Serial.println("connected");
        #endif
        //Once connected, publish an announcement...
        GBMMQTTClient.publish("temp/message","hello world");
        break;
        // ... and resubscribe
        
      } else {
        #if (DEBUG == 1)
          Serial.print("failed, rc=");
          Serial.print(GBMMQTTClient.state());
          Serial.println(" try again in 3 seconds");
        #endif
        // Wait 3 seconds before retrying
        delay(3000);
      }
    }   while(attemptMqttConnectCounter < MQTT_CONNECT_ATTEMPT);
}
