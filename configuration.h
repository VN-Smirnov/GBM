#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/*---=Begin Debug config=---*/
#define         DEBUG         1                   //0 - releas,       1 - debug
#define         FAKE_SENSORS  1                   //0 - real sensors, 1 - fake sensors
#define         ENABLE_LCD    0                   //0 - without LCD,  1 - LCD enabled     
/*---=End Debug config=---*/

/*---=Begin pin number config=---*/
#define     OWP_PIN                       4       //OneWire pin for DS18B20 sensors
#define     PRESS_SENS_PIN                15      //ADC pin for Pressure Sensor
#define     DHT11_PIN                     18      //DHT11 Sensor Pin
/*---=End pin number config=---*/

/*---=Begin valeu and kof param=---*/

#define     POLLING_SENSOR_TIME_INTERVAL  3000    //Poling Sensor Time Interval in MilliSec
#define     SENT_TO_MQTT_INTERVAL         15000  //Send Interval to MQTT Brocker   
#define     DHT11_TEMPERATURE_CORRECTION  0.8     //Correction for Temperature
#define     MIN_PRESSURE_VALUE            108     //Min Press Value, for sensor calibration  
#define     MAX_PRESSURE_VALUE            1023    //Max Press Value, for sensor calibration  

/*---=End valeus and koef param=---*/

/*---=Begin LCD config=---*/

#define     LCD_ADDRESS                  0x27
#define     LCD_COLLUMS                   20
#define     LCD_LINES                     4

/*---=End LCD config=----*/

/*---=Begin DS18B20 config Addresses=---*/


#define     BS18B20_RED                  0x28, 0xDF, 0x12, 0x94, 0x97, 0x12, 0x03, 0x23
#define     BS18B20_BLACK                0x28, 0x61, 0x65, 0x94, 0x97, 0x02, 0x03, 0xE7
#define     BS18B20_TABLE1               0x28, 0x20, 0xA9, 0x49, 0xF6, 0xBC, 0x3C, 0x15
#define     BS18B20_TABLE2               0x28, 0xB2, 0xDE, 0x49, 0xF6, 0x06, 0x3C, 0x13

/*---=End DS18B20 config Addresses=----*/

#define     MQTT_NAME_CLIENT  "GBM-04"

#endif