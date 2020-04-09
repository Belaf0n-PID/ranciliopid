/********************************************************
  Version 1.4 (06.04.2020)
  Config must be configured by the user
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H  

/********************************************************
   Vorab-Konfig
******************************************************/
#define OFFLINEMODUS 0       // 0 = Blynk and WIFI are used; 1 = offline mode (only preconfigured values in code are used!)
#define ONLYPID 0            // 1 = Only PID, no preinfusion; 0 = PID and preinfusion
#define TEMPSENSOR 2         // 2 = TSIC306
#define BREWDETECTION 1      // 0 = off; 1 = Software; 2 = Hardware
#define FALLBACK 1           // 1 = fallback to values stored in eeprom, if blynk is not working; 0 = deactivated
#define TRIGGERTYPE HIGH     // LOW = low trigger, HIGH = high trigger relay
#define OTA true             // true = activate update via OTA
#define PONE 1               // 1 = P_ON_E (default), 0 = P_ON_M (special PID mode, other PID-parameter are needed)
#define GRAFANA 1            // 1 = grafana visualisation. Access required, 0 = off (default)
#define WIFICINNECTIONDELAY 10000 // delay between reconnects in ms
#define MAXWIFIRECONNECTS 5  // maximum number of reconnects; use -1 to set to maximum ("deactivated")
#define MACHINELOGO 1        // 1 = Rancilio, 2 = Gaggia

// Wifi & Blynk 
#define HOSTNAME "rancilio"
#define AUTH "blynkauthcode"
#define D_SSID "wlanname"
#define PASS "wlanpass"
#define BLYNKPORT 8080

// OTA
#define OTAHOST "Rancilio"   // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"    // Password for OTA updates

#define BLYNKADDRESS "blynk.remoteapp.de"         // IP-Address of used blynk server
#define BLYNKPORT 8080  //Port for blynk server
// define BLYNKADDRESS "raspberrypi.local"


//PID - values for offline brewdetection
#define AGGBKP 50    // Kp
#define AGGBTN 0   // Tn 
#define AGGBTV 20    // Tv

//PID - offline values
#define SETPOINT 95  // Temperatur setpoint
#define AGGKP 69     // Kp
#define AGGTN 399    // Tn
#define AGGTV 0      // Tv
#define STARTKP 50   // Start Kp during coldstart
#define STARTTN 130  // Start Tn during cold start
#define STARTTEMP 85 // temperature setpoint to deactivate startup values

//PID - Werte für Kaltstart der Maschine
#define PONMKP 50    // Kp
#define PONMTN 150   // Tn 
#define PONMTV 0    // Tv

#endif // _userConfig_H
