/********************************************************
  Version 1.2 (06.10.2019)
  Config must be configured by the user
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H  

/********************************************************
   Config
******************************************************/
#define OFFLINEMODUS 0       // 0 = Blynk and WIFI are used; 1 = offline mode (only preconfigured values in code are used!)
#define DISPLAY 2            // 0 = deactivated; 2 = External 128x64 Display
#define ONLYPID 1            // 1 = Only PID, no preinfusion; 0 = PID and preinfusion
#define TEMPSENSOR 2         // 1 = DS19B20; 2 = TSIC306
#define BREWDETECTION 1      // 0 = off; 1 = Software; 2 = Hardware
#define FALLBACK 1           // 1 = fallback to values stored in eeprom, if blynk is not working; 0 = deactivated
#define TRIGGERTYPE HIGH     // LOW = low trigger, HIGH = high trigger relay
#define OTA true             // true = activate update via OTA
#define PONE 1               // 1 = P_ON_E (default), 0 = P_ON_M (special PID mode, other PID-parameter are needed)
#define GRAFANA 0            // 1 = grafana visualisation. Access required, 0 = off (default)
#define WIFICINNECTIONDELAY 10000 // delay between reconnects in ms
#define MAXWIFIRECONNECTS 5  // maximum number of reconnects; use -1 to set to maximum ("deactivated")

// Wifi
#define AUTH "blynkauthcode"
#define D_SSID "wlanname"
#define PASS "wlanpass"

// OTA
#define OTAHOST "Rancilio"   // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"    // Password for OTA updtates

#define BLYNKADDRESS "blynk.remoteapp.de"         // IP-Address of used blynk server
// define BLYNKADDRESS "raspberrypi.local"


//PID - Werte für Brüherkennung Offline
#define AGGBKP 50    // Kp
#define AGGBTN 0   // Tn 
#define AGGBTV 20    // Tv

//PID - Werte für Regelung Offline
#define SETPOINT 95  // Temperatur Sollwert
#define AGGKP 69     // Kp
#define AGGTN 399    // Tn
#define AGGTV 0      // Tv
#define STARTKP 100   // Start Kp während Kaltstart
#define STARTTN 0  // Start Tn während Kaltstart
#define STARTTEMP 80 // Temperaturschwelle für deaktivieren des Start Kp


#endif // _userConfig_H
