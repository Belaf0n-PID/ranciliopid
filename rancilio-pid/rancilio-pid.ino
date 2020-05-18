/********************************************************
   Version 1.9.9 MASTER (12.05.2020)
  Key facts: major revision
  - Check the PIN Ports in the CODE!
  - Find your changerate of the machine, can be wrong, test it!
******************************************************/
         
#include "icon.h"  

// Debug mode is active if #define DEBUGMODE is set
//#define DEBUGMODE

#ifndef DEBUGMODE
#define DEBUG_println(a)
#define DEBUG_print(a)
#define DEBUGSTART(a)
#else
#define DEBUG_println(a) Serial.println(a);
#define DEBUG_print(a) Serial.print(a);
#define DEBUGSTART(a) Serial.begin(a);
#endif

//#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

//Define pins for outputs
#define pinRelayVentil    12
#define pinRelayPumpe     13
#define pinRelayHeater    14

//Libraries for OTA
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#include "userConfig.h" // needs to be configured by the user
//#include "Arduino.h"
#include <EEPROM.h>

const char* sysVersion PROGMEM  = "Version 1.9.9 Master";

/********************************************************
  definitions below must be changed in the userConfig.h file
******************************************************/
int Offlinemodus = OFFLINEMODUS;
const int Display = DISPLAY;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int Brewdetection = BREWDETECTION;
const int fallback = FALLBACK;
const int triggerType = TRIGGERTYPE;
const boolean ota = OTA;
const int grafana = GRAFANA;
const int coldstart_pid = COLDSTART_PID ;

// Wifi
const char* hostname = HOSTNAME;
const char* auth = AUTH;
const char* ssid = D_SSID;
const char* pass = PASS;

unsigned long lastWifiConnectionAttempt = millis();
const unsigned long wifiConnectionDelay = 10000; // try to reconnect every 10 seconds
unsigned int wifiReconnects = 0; //number of reconnects

// OTA
const char* OTAhost = OTAHOST;
const char* OTApass = OTAPASS;

//Blynk
const char* blynkaddress  = BLYNKADDRESS;
const int blynkport = BLYNKPORT;

/********************************************************
   Vorab-Konfig
******************************************************/
int pidON = 1 ;                 // 1 = control loop in closed loop
int relayON, relayOFF;          // used for relay trigger type. Do not change!
boolean kaltstart = true;       // true = Rancilio started for first time
boolean emergencyStop = false;  // Notstop bei zu hoher Temperatur
int bars = 0; //used for getSignalStrength()
boolean brewDetected = 0;

/********************************************************
   moving average - Brüherkennung
*****************************************************/
const int numReadings = 15;             // number of values per Array
float readingstemp[numReadings];        // the readings from Temp
float readingstime[numReadings];        // the readings from time
float readingchangerate[numReadings];

int readIndex = 1;              // the index of the current reading
double total = 0;               // the running
int totaltime = 0 ;             // the running time
double heatrateaverage = 0;     // the average over the numReadings
double changerate = 0;          // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;
int i = 0;
int firstreading = 1 ;          // Ini of the field, also used for sensor check
int inX = 0, inY = 0, inOld = 0, inSum = 0; //used for filter()
char debugline[100];
/********************************************************
   PID - Werte Brüherkennung Offline
*****************************************************/

double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
#if (aggbTn == 0)
double aggbKi = 0;
#else
double aggbKi = aggbKp / aggbTn;
#endif
double aggbKd = aggbTv * aggbKp ;
double brewtimersoftware = 45;    // 20-5 for detection
double brewboarder = 150 ;        // border for the detection,
const int PonE = PONE;
// be carefull: to low: risk of wrong brew detection
// and rising temperature

/********************************************************
   Analog Schalter Read
******************************************************/
const int analogPin = 0; // will be use in case of hardware
int brewcounter = 10;
int brewswitch = 0;
const long analogreadingtimeinterval = 10 ; // ms
unsigned long previousMillistempanalogreading ; // ms for analogreading
long brewtime = 25000;
long aktuelleZeit = 0;
long totalbrewtime = 0;
int preinfusion = 2000;
int preinfusionpause = 5000;
unsigned long bezugsZeit = 0;
unsigned long startZeit = 0;

/********************************************************
   Sensor check
******************************************************/
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10 ; //define maximum number of consecutive polls (of intervaltempmes* duration) to have errors


/********************************************************
   DISPLAY
******************************************************/
#include <U8x8lib.h>
            

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ 16);   //Display initalisieren
                             

// Display 128x64
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <ACROBOTIC_SSD1306.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels  
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define XPOS 0
#define YPOS 1
#define DELTAY 2

/********************************************************
   PID
******************************************************/
#include "PID_v1.h"

unsigned long previousMillistemp;  // initialisation at the end of init()
const long intervaltempmestsic = 400 ;
const long intervaltempmesds18b20 = 400  ;
int pidMode = 1; //1 = Automatic, 0 = Manual

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
double Input, Output, setPointTemp;  //
double previousInput = 0;

double setPoint = SETPOINT;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double startKp = STARTKP;
double startTn = STARTTN;
#if (startTn == 0)
double startKi = 0;
#else
double startKi = startKp / startTn;
#endif

double starttemp = STARTTEMP;
#if (aggTn == 0)
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif
double aggKd = aggTv * aggKp ;


PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT);

/********************************************************
   DALLAS TEMP
******************************************************/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress sensorDeviceAddress;

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"       // include the library
TSIC Sensor1(2);    // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

//Zeitserver
/*
   NOT KNOWN WHAT THIS IS USED FOR
  #include <TimeLib.h>
  #include <WidgetRTC.h>
  BlynkTimer timer;
  WidgetRTC rtc;
  WidgetBridge bridge1(V1);
*/

//Update Intervall zur App
unsigned long previousMillisBlynk;  // initialisation at the end of init()
const long intervalBlynk = 1000;
int blynksendcounter = 1;

//Update für Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const long intervalDisplay = 500;

/********************************************************
   BLYNK WERTE EINLESEN und Definition der PINS
******************************************************/



  BLYNK_CONNECTED() {
  if (Offlinemodus == 0) {
    Blynk.syncAll();
    //rtc.begin();
  }
  }

BLYNK_WRITE(V4) {
  aggKp = param.asDouble();
}

BLYNK_WRITE(V5) {
  aggTn = param.asDouble();
}
BLYNK_WRITE(V6) {
  aggTv =  param.asDouble();
}

BLYNK_WRITE(V7) {
  setPoint = param.asDouble();
}

BLYNK_WRITE(V8) {
  brewtime = param.asDouble() * 1000;
}

BLYNK_WRITE(V9) {
  preinfusion = param.asDouble() * 1000;
}

BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble() * 1000;
}
BLYNK_WRITE(V11) {
  startKp = param.asDouble();
}
BLYNK_WRITE(V12) {
  starttemp = param.asDouble();
}
BLYNK_WRITE(V13)
{
  pidON = param.asInt();
}
BLYNK_WRITE(V14)
{
  startTn = param.asDouble();
}
BLYNK_WRITE(V30)
{
  aggbKp = param.asDouble();//
}

BLYNK_WRITE(V31) {
  aggbTn = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggbTv =  param.asDouble();
}
BLYNK_WRITE(V33) {
  brewtimersoftware =  param.asDouble();
}
BLYNK_WRITE(V34) {
  brewboarder =  param.asDouble();
}


/********************************************************
  Notstop wenn Temp zu hoch
*****************************************************/
void testEmergencyStop(){
  if (Input > 120){
    emergencyStop = true;
  } else if (Input < 100) {
    emergencyStop = false;
  }
}

/****
  after ~28 cycles the input is set to 99,66% if the real input value - analog pin 
*/
int filter(int input) {
  inX = input * 0.3;
  inY = inOld * 0.7;
  inSum = inX + inY;
  inOld = inSum;
  return inSum;
}

/********************************************************
  Displayausgabe
*****************************************************/

void displaymessage(String displaymessagetext, String displaymessagetext2) {
  if (Display == 2) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    display.setTextSize(1);


    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 47);
    display.println(displaymessagetext);
    display.print(displaymessagetext2);
    //Rancilio startup logo
    display.drawBitmap(41,2, startLogo_bits,startLogo_width, startLogo_height, WHITE);
    //draw circle
    //display.drawCircle(63, 24, 20, WHITE);
    display.display();
   // display.fadeout();
   // display.fadein();
  }
  if (Display == 1) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.setCursor(0, 0);
    u8x8.println(displaymessagetext);
    u8x8.print(displaymessagetext2);
  }

}
/********************************************************
  Read analog input pin
*****************************************************/
void readAnalogInput() {
  unsigned long currentMillistemp = millis();
  if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval)
  {
    previousMillistempanalogreading = currentMillistemp;
    brewswitch = filter(analogRead(analogPin));
    //DEBUG_print("Analog_reading:");
    //DEBUG_println(brewswitch);
  }
}

/********************************************************
  Moving average - brewdetection (SW)
*****************************************************/
void movAvg() {
  if (firstreading == 1) {
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readingstemp[thisReading] = Input;
      readingstime[thisReading] = 0;
      readingchangerate[thisReading] = 0;
    }
    firstreading = 0 ;
  }

  readingstime[readIndex] = millis() ;
  readingstemp[readIndex] = Input ;

  if (readIndex == numReadings - 1) {
    changerate = (readingstemp[numReadings - 1] - readingstemp[0]) / (readingstime[numReadings - 1] - readingstime[0]) * 10000;
  } else {
    changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) / (readingstime[readIndex] - readingstime[readIndex + 1]) * 10000;
  }

  readingchangerate[readIndex] = changerate ;
  total = 0 ;
  for (i = 0; i < numReadings; i++)
  {
    total += readingchangerate[i];
  }

  heatrateaverage = total / numReadings * 100 ;
  if (heatrateaveragemin > heatrateaverage) {
    heatrateaveragemin = heatrateaverage ;
  }

  if (readIndex >= numReadings - 1) {
    // ...wrap around to the beginning:
    readIndex = 0;
  } else {
    readIndex++;
  }
}


/********************************************************
  check sensor value. If < 0 or difference between old and new >25, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
boolean checkSensor(float tempInput) {
  boolean sensorOK = false;
  /********************************************************
    sensor error
  ******************************************************/
  if ( ( tempInput < 0 || tempInput > 150 || abs(tempInput - previousInput) > 25) && !sensorError) {
    error++;
    sensorOK = false;
    DEBUG_print("Error counter: ");
    DEBUG_println(error);
    DEBUG_print("temp delta: ");
    DEBUG_println(tempInput);
    sprintf(debugline, "WARN: temperature sensor reading: consec_errors=%d, temp_current=%f, temp_prev=%f", error, tempInput, previousInput);
     DEBUG_println(debugline);
  } else if (tempInput > 0) {
    error = 0;
    sensorOK = true;
  }
  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    sprintf(debugline, "ERROR: temperature sensor malfunction: temp_current=%f, temp_prev=%f", tempInput, previousInput);
    DEBUG_println(debugline);
  } else if (error == 0) {
    sensorError = false ;
  }

  return sensorOK;
}

/********************************************************
  Refresh temperature.
  Each time checkSensor() is called to verify the value.
  If the value is not valid, new data is not stored.
*****************************************************/
void refreshTemp() {
  /********************************************************
    Temp. Request
  ******************************************************/
  unsigned long currentMillistemp = millis();
  previousInput = Input ;
  if (TempSensor == 1)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20)
    {
      previousMillistemp = currentMillistemp;
      sensors.requestTemperatures();
      if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0) return;  //if sensor data is not valid, abort function
      Input = sensors.getTempCByIndex(0);
      if (Brewdetection == 1) {
        movAvg();
      } else {
        firstreading = 0;
      }
    }
  }
  if (TempSensor == 2)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmestsic)
    {
      previousMillistemp = currentMillistemp;
      /*  variable "temperature" must be set to zero, before reading new data
            getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
      */
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
      //Temperatur_C = random(50,52);
      if (!checkSensor(Temperatur_C) && firstreading == 0) return;  //if sensor data is not valid, abort function
      Input = Temperatur_C;
      //Input = random(50,70) ;// test value
      if (Brewdetection == 1)
      {
        movAvg();
      } else {
        firstreading = 0;
      }
    }
  }
}

/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() {
  if (OnlyPID == 0) {
    readAnalogInput();
    unsigned long currentMillistemp = millis();

    if (brewswitch < 1000 && brewcounter >= 11) {   //abort function for state machine from every state
      brewcounter = 43;
    }


    if (brewcounter > 10) {
      bezugsZeit = currentMillistemp - startZeit;
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
      case 10:    // waiting step for brew switch turning on
        if (brewswitch > 1000) {
          startZeit = millis();
          brewcounter = 20;
        }
        break;
      case 20:    //preinfusioon
        DEBUG_println("Preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 21;
        break;
      case 21:    //waiting time preinfusion
        if (bezugsZeit > preinfusion) {
          brewcounter = 30;
        }
        break;
      case 30:    //preinfusion pause
        DEBUG_println("preinfusion pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 31;
        break;
      case 31:    //waiting time preinfusion pause
        if (bezugsZeit > preinfusion + preinfusionpause) {
          brewcounter = 40;
        }
        break;
      case 40:    //brew running
        DEBUG_println("Brew started");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 41;
        break;
      case 41:    //waiting time brew
        if (bezugsZeit > totalbrewtime) {
          brewcounter = 42;
        }
        break;
      case 42:    //brew finished
        DEBUG_println("Brew stopped");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 43;
        break;
      case 43:    // waiting for brewswitch off position
        if (brewswitch < 1000) {
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
          brewcounter = 10;
          currentMillistemp = 0;
          bezugsZeit = 0;
          brewDetected = 0;
        }
        break;
    }
  }
}

/********************************************************
   Check if Wifi is connected, if not reconnect
*****************************************************/
 void checkWifi(){
   if (Offlinemodus == 1 || brewcounter > 11) return;
   int statusTemp = WiFi.status();
   DEBUG_println("wlanstatuscheck");
   // check WiFi connection:
   if (statusTemp != WL_CONNECTED) {
     // (optional) "offline" part of code
    DEBUG_println("not connected");
      // check delay:
     if (millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) {
      DEBUG_println("innerwificonnectloop");
       lastWifiConnectionAttempt = millis();      
       // attempt to connect to Wifi network:
       WiFi.disconnect();
       WiFi.begin(ssid, pass); 
       delay(1000);    //will not work without delay
       yield();
       wifiReconnects++;    
     }

    }
 }


/********************************************************
    send data to display
******************************************************/
void printScreen() {
  if (Display == 1 && !sensorError) {
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.setCursor(0, 0);
    u8x8.print("               ");
    u8x8.setCursor(0, 1);
    u8x8.print("               ");
    u8x8.setCursor(0, 2);
    u8x8.print("               ");
    u8x8.setCursor(0, 0);
    u8x8.setCursor(1, 0);
    u8x8.print(bPID.GetKp());
    u8x8.setCursor(6, 0);
    u8x8.print(",");
    u8x8.setCursor(7, 0);
    if (bPID.GetKi() != 0){
    u8x8.print(bPID.GetKp() / bPID.GetKi());}
    else
    {u8x8.print("0");}
    u8x8.setCursor(11, 0);
    u8x8.print(",");
    u8x8.setCursor(12, 0);
    u8x8.print(bPID.GetKd() / bPID.GetKp());
    u8x8.setCursor(0, 1);
    u8x8.print("Input:");
    u8x8.setCursor(9, 1);
    u8x8.print("   ");
    u8x8.setCursor(9, 1);
    u8x8.print(Input);
    u8x8.setCursor(0, 2);
    u8x8.print("SetPoint:");
    u8x8.setCursor(10, 2);
    u8x8.print("   ");
    u8x8.setCursor(10, 2);
    u8x8.print(setPoint);
    u8x8.setCursor(0, 3);
    u8x8.print(round((Input * 100) / setPoint));
    u8x8.setCursor(4, 3);
    u8x8.print("%");
    u8x8.setCursor(6, 3);
    u8x8.print(Output);
  }
  if (Display == 2 && !sensorError)
  {
    display.clearDisplay();
    display.drawBitmap(0,0, logo_bits,logo_width, logo_height, WHITE);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(32, 14); 
    display.print("Ist :  ");
    display.print(Input, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");
    display.setCursor(32, 24); 
    display.print("Soll:  ");
    display.print(setPoint, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");
   // display.print("Heizen: ");
    
   // display.println(" %");
   
// Draw heat bar
   display.drawLine(15, 58, 117, 58, WHITE);
   display.drawLine(15, 58, 15, 61, WHITE); 
   display.drawLine(117, 58, 117, 61, WHITE);

   display.drawLine(16, 59, (Output / 10) + 16, 59, WHITE);
   display.drawLine(16, 60, (Output / 10) + 16, 60, WHITE);
   display.drawLine(15, 61, 117, 61, WHITE);
   
//draw current temp in icon
   display.drawLine(9, 48, 9, 58 - (Input / 2), WHITE); 
   display.drawLine(10, 48, 10, 58 - (Input / 2), WHITE);  
   display.drawLine(11, 48, 11, 58 - (Input / 2), WHITE); 
   display.drawLine(12, 48, 12, 58 - (Input / 2), WHITE); 
   display.drawLine(13, 48, 13, 58 - (Input / 2), WHITE);
   
//draw setPoint line
   display.drawLine(18, 58 - (setPoint / 2), 23, 58 - (setPoint / 2), WHITE); 
 
// PID Werte ueber heatbar
    display.setCursor(40, 50);  

    display.print(bPID.GetKp(), 0); // P 
    display.print("|");
    if (bPID.GetKi() != 0){      
    display.print(bPID.GetKp() / bPID.GetKi(), 0);;} // I 
    else
    { 
      display.print("0");
    }
    display.print("|");
    display.println(bPID.GetKd() / bPID.GetKp(), 0); // D
    display.setCursor(98,50);
    display.print(Output / 10, 0);
    display.print("%");

// Brew
    display.setCursor(32, 35); 
    display.print("Brew:  ");
    display.setTextSize(1);
    display.print(bezugsZeit / 1000);
    display.print("/");
    if (ONLYPID == 1){
    display.println(brewtimersoftware, 0);             // deaktivieren wenn Preinfusion ( // voransetzen )
    }
    else 
    {
    display.println(totalbrewtime / 1000);            // aktivieren wenn Preinfusion
    }
//draw box
   display.drawRoundRect(0, 0, 128, 64, 1, WHITE);
   
// Für Statusinfos
   display.drawRoundRect(32, 0, 84, 12, 1, WHITE); 
   if (Offlinemodus == 0) {
      getSignalStrength();
      if (WiFi.status() == WL_CONNECTED){
        display.drawBitmap(40,2,antenna_OK,8,8,WHITE);
        for (int b=0; b <= bars; b++) {
          display.drawFastVLine(45 + (b*2),10 - (b*2),b*2,WHITE);
        }
      } else {
        display.drawBitmap(40,2,antenna_NOK,8,8,WHITE);
        display.setCursor(88, 0);
        display.print("RC: ");
        display.print(wifiReconnects);
      }
      if (Blynk.connected()){
        display.drawBitmap(60,2,blynk_OK,11,8,WHITE);
      } else {
        display.drawBitmap(60,2,blynk_NOK,8,8,WHITE);
      }
    } else {
      display.setCursor(40, 2); 
      display.print("Offlinemodus");
    }    
   display.display();
  }
}

/********************************************************
  send data to Blynk server
*****************************************************/

void sendToBlynk() {
  if (Offlinemodus != 0) return;
  unsigned long currentMillisBlynk = millis();
  if (currentMillisBlynk - previousMillisBlynk >= intervalBlynk) {
    previousMillisBlynk = currentMillisBlynk;
    if (Blynk.connected()) {
      if (grafana == 1) {
        Blynk.virtualWrite(V60, Input, Output,bPID.GetKp(),bPID.GetKi(),bPID.GetKd(),setPoint );
        }
      if (blynksendcounter == 1) {
        Blynk.virtualWrite(V2, Input);
        Blynk.syncVirtual(V2);
      }
      if (blynksendcounter == 2) {
        Blynk.virtualWrite(V23, Output);
        Blynk.syncVirtual(V23);
      }
      if (blynksendcounter == 3) {
        Blynk.virtualWrite(V3, setPoint);
        Blynk.syncVirtual(V3);
      }
      if (blynksendcounter == 4) {
        Blynk.virtualWrite(V35, heatrateaverage);
        Blynk.syncVirtual(V35);
      }
      if (blynksendcounter >= 5) {
        Blynk.virtualWrite(V36, heatrateaveragemin);
        Blynk.syncVirtual(V36);
        blynksendcounter = 0;
      }
      blynksendcounter++;
    }
  }
}

/********************************************************
    Brewdetection
******************************************************/
void brewdetection() {
  if (brewboarder == 0) return; //abort brewdetection if deactivated

  // Brew detecion == 1 software solution , == 2 hardware
  if (Brewdetection == 1) {
    if (millis() - timeBrewdetection > brewtimersoftware * 1000) {
      timerBrewdetection = 0 ;    //rearm brewdetection
      if (OnlyPID == 1) {
        bezugsZeit = 0 ;    // brewdetection is used in OnlyPID mode to detect a start of brew, and set the bezugsZeit
      }
    }
  } else if (Brewdetection == 2) {
    if (millis() - timeBrewdetection > brewtimersoftware * 1000) {
      timerBrewdetection = 0 ;   //rearm brewdetection
    }
  }

  if (Brewdetection == 1) {
    if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 ) {
      DEBUG_println("SW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
    }
  } else if (Brewdetection == 2) {
    if (brewcounter > 10 && brewDetected == 0 && brewboarder != 0) {
      DEBUG_println("HW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
      brewDetected = 1;
    }
  }
}

/********************************************************
  Get Wifi signal strength and set bars for display
*****************************************************/
void getSignalStrength(){
  if (Offlinemodus == 1) return;
  
  long rssi;
  if (WiFi.status() == WL_CONNECTED) {
    rssi = WiFi.RSSI();  
  } else {
    rssi = -100;
  }

  if (rssi >= -50) { 
    bars = 4;
  } else if (rssi < -50 & rssi >= -65) {
    bars = 3;
  } else if (rssi < -65 & rssi >= -75) {
    bars = 2;
  } else if (rssi < -75 & rssi >= -80) {
    bars = 1;
  } else {
    bars = 0;
  }
}

/********************************************************
    Timer 1 - ISR für PID Berechnung und Heizrelais-Ausgabe
******************************************************/
void ICACHE_RAM_ATTR onTimer1ISR() {
  timer1_write(50000); // set interrupt time to 10ms

  if (Output <= isrCounter) {
    digitalWrite(pinRelayHeater, LOW);
    //DEBUG_println("Power off!");
  } else {
    digitalWrite(pinRelayHeater, HIGH);
    //DEBUG_println("Power on!");
  }

  isrCounter += 10; // += 10 because one tick = 10ms
  //set PID output as relais commands
  if (isrCounter > windowSize) {
    isrCounter = 0;
  }

  //run PID calculation
  bPID.Compute();
}

void setup() {
  DEBUGSTART(115200);
  /********************************************************
    Define trigger type
  ******************************************************/
  if (triggerType)
  {
    relayON = HIGH;
    relayOFF = LOW;
  } else {
    relayON = LOW;
    relayOFF = HIGH;
  }

  /********************************************************
    Ini Pins
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);
  pinMode(pinRelayHeater, OUTPUT);
  digitalWrite(pinRelayVentil, relayOFF);
  digitalWrite(pinRelayPumpe, relayOFF);
  digitalWrite(pinRelayHeater, LOW);


  if (Display == 1) {
    /********************************************************
      DISPLAY Intern
    ******************************************************/
    u8x8.begin();
    u8x8.setPowerSave(0);
  }
  if (Display == 2) {
    /********************************************************
      DISPLAY 128x64
    ******************************************************/
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)  for AZ Deliv. Display
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();
  }
  displaymessage(sysVersion, "");
  delay(2000);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) {
  WiFi.hostname(hostname);
    if (fallback == 0) {

      displaymessage("Connect to Blynk", "no Fallback");
      Blynk.begin(auth, ssid, pass, blynkaddress, blynkport);
    }

    if (fallback == 1) {
      unsigned long started = millis();
      displaymessage("1: Connect Wifi to:", ssid);
      // wait 10 seconds for connection:
      /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
        would try to act as both a client and an access-point and could cause
        network-issues with your other WiFi-devices on your WiFi-network. */
      WiFi.mode(WIFI_STA);
      WiFi.persistent(false);
      WiFi.begin(ssid, pass);
      DEBUG_print("Connecting to ");
      DEBUG_print(ssid);
      DEBUG_println(" ...");

      while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
      {
        yield();    //Prevent Watchdog trigger
      }

      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_println("WiFi connected");
        DEBUG_println("IP address: ");
        DEBUG_println(WiFi.localIP());
        displaymessage("2: Wifi connected, ", "try Blynk   ");
        DEBUG_println("Wifi works, now try Blynk connection");
        delay(2000);
        Blynk.config(auth, blynkaddress, blynkport) ;
        Blynk.connect(30000);

        // Blnky works:
        if (Blynk.connected() == true) {
          displaymessage("3: Blynk connected", "sync all variables...");
          DEBUG_println("Blynk is online, new values to eeprom");
         // Blynk.run() ; 
          Blynk.syncVirtual(V4);
          Blynk.syncVirtual(V5);
          Blynk.syncVirtual(V6);
          Blynk.syncVirtual(V7);
          Blynk.syncVirtual(V8);
          Blynk.syncVirtual(V9);
          Blynk.syncVirtual(V10);
          Blynk.syncVirtual(V11);
          Blynk.syncVirtual(V12);
          Blynk.syncVirtual(V13);
          Blynk.syncVirtual(V14);
          Blynk.syncVirtual(V30);
          Blynk.syncVirtual(V31);
          Blynk.syncVirtual(V32);
          Blynk.syncVirtual(V33);
          Blynk.syncVirtual(V34);
         // Blynk.syncAll();  //sync all values from Blynk server
          // Werte in den eeprom schreiben
          // ini eeprom mit begin
          EEPROM.begin(1024);
          EEPROM.put(0, aggKp);
          EEPROM.put(10, aggTn);
          EEPROM.put(20, aggTv);
          EEPROM.put(30, setPoint);
          EEPROM.put(40, brewtime);
          EEPROM.put(50, preinfusion);
          EEPROM.put(60, preinfusionpause);
          EEPROM.put(70, startKp);
          EEPROM.put(80, starttemp);
          EEPROM.put(90, aggbKp);
          EEPROM.put(100, aggbTn);
          EEPROM.put(110, aggbTv);
          EEPROM.put(120, brewtimersoftware);
          EEPROM.put(130, brewboarder);
          // eeprom schließen
          EEPROM.commit();
        }
      } else {
        displaymessage("Begin Fallback,", "No Wifi");
        delay(2000);
        DEBUG_println("Start offline with eeprom values, no wifi :(");        
        // eeprom öffnen
        EEPROM.begin(1024);
        // eeprom werte prüfen, ob numerisch
        double dummy;
        EEPROM.get(0, dummy);
        DEBUG_print("check eeprom 0x00 in dummy: ");
        DEBUG_println(dummy);
        if (!isnan(dummy)) {
          EEPROM.get(0, aggKp);
          EEPROM.get(10, aggTn);
          EEPROM.get(20, aggTv);
          EEPROM.get(30, setPoint);
          EEPROM.get(40, brewtime);
          EEPROM.get(50, preinfusion);
          EEPROM.get(60, preinfusionpause);
          EEPROM.get(70, startKp);
          EEPROM.get(80, starttemp);
          EEPROM.get(90, aggbKp);
          EEPROM.get(100, aggbTn);
          EEPROM.get(110, aggbTv);
          EEPROM.get(120, brewtimersoftware);
          EEPROM.get(130, brewboarder);
        }
        else
        {
          displaymessage("No eeprom,", "Value");
          DEBUG_println("No working eeprom value, I am sorry, but use default offline value  :)");
          Offlinemodus = 1 ;
          delay(2000);
        }
        // eeeprom schließen
        EEPROM.commit();
      }
    }
  }

  /********************************************************
     OTA
  ******************************************************/
  if (ota && Offlinemodus == 0 ) {
    //wifi connection is done during blynk connection

    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
    ArduinoOTA.begin();
  }


  /********************************************************
     Ini PID
  ******************************************************/

  setPointTemp = setPoint;
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);


  /********************************************************
     TEMP SENSOR
  ******************************************************/
  if (TempSensor == 1) {
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, 10) ;
    sensors.requestTemperatures();
    Input = sensors.getTempCByIndex(0);
  }
  
  if (TempSensor == 2) {
     temperature = 0;
     Sensor1.getTemperature(&temperature);
     Input = Sensor1.calc_Celsius(&temperature);
   }
  /********************************************************
    movingaverage ini array
  ******************************************************/
  if (Brewdetection == 1) {
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readingstemp[thisReading] = 0;
      readingstime[thisReading] = 0;
      readingchangerate[thisReading] = 0;
    }
  }

  //Initialisation MUST be at the very end of the init(), otherwise the time comparision in loop() will have a big offset
  unsigned long currentTime = millis();
  previousMillistemp = currentTime;
  windowStartTime = currentTime;
  previousMillisDisplay = currentTime;
  previousMillisBlynk = currentTime;
  previousMillistempanalogreading = currentTime; 
  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  timer1_isr_init();
  timer1_attachInterrupt(onTimer1ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(50000); // set interrupt time to 10ms

}

void loop() {
if (WiFi.status() == WL_CONNECTED && Offlinemodus == 0) { 
  ArduinoOTA.handle();  // For OTA
  // Disable interrupt it OTA is starting, otherwise it will not work
  ArduinoOTA.onStart([](){
    timer1_disable();
    digitalWrite(pinRelayHeater, LOW); //Stop heating
  });
  ArduinoOTA.onError([](ota_error_t error) {
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  });
  // Enable interrupts if OTA is finished
  ArduinoOTA.onEnd([](){
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  });
     
      Blynk.run(); //Do Blynk magic stuff     
    
   wifiReconnects = 0;
   } else {
     checkWifi();
   }
  unsigned long startT;
  unsigned long stopT;

  refreshTemp();   //read new temperature values
  testEmergencyStop();  // test if Temp is to high
  //analogreading() ; // reading analog pin
  brew();   //start brewing if button pressed

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  } else if (pidON == 1 && pidMode == 0) {
    pidMode = 1;
    bPID.SetMode(pidMode);
  }
  //Sicherheitsabfrage
  if (!sensorError && Input > 0 && !emergencyStop) {  
      if (coldstart_pid == 1 ) {  // default starttemp
      starttemp = setPoint  ;
      startKp = 50 ;
      startTn = 130 ;
      } 
   //Set PID if first start of machine detected
     if (Input < starttemp && kaltstart) {
        //default PID Mode 
      if (startTn != 0) {
        startKi = startKp / startTn;
      } else {
        startKi = 0 ;
      }
      bPID.SetTunings(startKp, startKi, 0, P_ON_M);
      }    
     else if (timerBrewdetection == 0) {    //Prevent overwriting of brewdetection values
      // calc ki, kd
      if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKd = aggTv * aggKp ;
      bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
      kaltstart = false;
    }
    //if brew detected, set PID values
    brewdetection();
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1) {
      // calc ki, kd
      if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn ;
      } else {
        aggbKi = 0 ;
      }
      aggbKd = aggbTv * aggbKp ;
      bPID.SetTunings(aggbKp, aggbKi, aggbKd) ;
      if(OnlyPID == 1){
      bezugsZeit= millis() - timeBrewdetection ;
      }
    }

    sendToBlynk();

    //update display if time interval xpired
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
      previousMillisDisplay = currentMillisDisplay;
      printScreen();
    }

  } else if (sensorError) {

    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }

    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    if (Display == 2) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Error: Temp = ");
      display.println(Input);
      display.print("Check Temp. Sensor!");
      display.display();
    }
    if (Display == 1) {
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Error: Temp = ");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
    }
  } else if (emergencyStop){

    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }
        
    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    if (Display == 2) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Emergency Stop!");
      display.println("");
      display.println("Temp > 120");
      display.print("Temp: ");
      display.println(Input);
      display.print("Resume if Temp < 100");
      display.display();
    }
    if (Display == 1) {
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Emergency Stop! T>120");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
    }
  }
}
