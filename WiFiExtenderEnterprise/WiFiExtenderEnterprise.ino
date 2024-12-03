#include <WiFi.h>

#define VERSION "1.0"

#define EAP_IDENTITY "user@ugr.es" //if connecting from another corporation, use identity@organization.domain in Eduroam
#define EAP_USERNAME "user@ugr.es" //oftentimes just a repeat of the identity
#define EAP_PASSWORD "XXXXXXXX"    //your Eduroam password
const char *ssid = "eduroam";      // Eduroam SSID

int counter = 0;
int noSTAs = 0;
uint32_t ipLastSTA = 0;

#define AP_SSID "AP_SSID"
#define AP_PASS "AP_PASS"

IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_mask(255, 255, 255, 0);
IPAddress ap_leaseStart(192, 168, 4, 2);
//IPAddress ap_dns(8, 8, 4, 4);
IPAddress ap_dns(150, 214, 27, 15);

#define TXPOWER WIFI_POWER_2dBm

#if defined(WIRELESS_STICK_V3) || defined (WIRELESS_STICK)
#define OLED_ENABLED
#endif

#ifdef OLED_ENABLED
// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               
#include "HT_SSD1306Wire.h"
#if defined(WIRELESS_STICK_V3) || defined (WIRELESS_STICK)
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED); // addr , freq , i2c group , resolution , rst
#endif
#endif


/////////////////////////////////////////////
// FUNCTIONS RELATED TO WI-FI CONNECTIVITY //
/////////////////////////////////////////////
/*
// from WiFiGeneric.h:
typedef enum {
WIFI_POWER_19_5dBm = 78,// 19.5dBm
WIFI_POWER_19dBm = 76,// 19dBm
WIFI_POWER_18_5dBm = 74,// 18.5dBm
WIFI_POWER_17dBm = 68,// 17dBm
WIFI_POWER_15dBm = 60,// 15dBm
WIFI_POWER_13dBm = 52,// 13dBm
WIFI_POWER_11dBm = 44,// 11dBm
WIFI_POWER_8_5dBm = 34,// 8.5dBm
WIFI_POWER_7dBm = 28,// 7dBm
WIFI_POWER_5dBm = 20,// 5dBm
WIFI_POWER_2dBm = 8,// 2dBm
WIFI_POWER_MINUS_1dBm = -4// -1dBm
} wifi_power_t;
*/

double convertTxPowerTodBm (int8_t power) {
  if (power >= 78) {
    return 19.5;
  } else if (power >= 76) {
    return 19.0;
  } else if (power >= 74) {
    return 18.5;
  } else if (power >= 68) {
    return 17.0;
  } else if (power >= 60) {
    return 15.0;
  } else if (power >= 52) {
    return 13.0;
  } else if (power >= 44) {
    return 11.0;
  } else if (power >= 34) {
    return 8.5;
  } else if (power >= 28) {
    return 7.0;
  } else if (power >= 20) {
    return 5.0;
  } else if (power >= 8) {
    return 2.0;
  } else if (power >= -4) {
    return -1.0;
  } else {
    return -1.0;
  }
}

#ifdef OLED_ENABLED
////////////////////////////
// FUNCTIONS RELATED TO OLED
////////////////////////////
void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Network.onEvent(onEvent);

#ifdef OLED_ENABLED
    // For OLED
    VextON();
    delay(100);
    // Initialising the UI will init the display too.
    display.init();
    display.setFont(ArialMT_Plain_10);
    // clear the display
    display.clear();
    // Show startup message
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0,  "WPA-enterprise");
    display.drawString(0, 10,  "extender v" + String(VERSION));
    display.drawString(0, 20, "Booting...");
    display.display();
#endif

  WiFi.AP.begin();
  WiFi.AP.config(ap_ip, ap_ip, ap_mask, ap_leaseStart, ap_dns);
  WiFi.AP.create(AP_SSID, AP_PASS);
  if (!WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 1000)) {
    Serial.println("Failed to start AP!");
    return;
  }

  // Example1 (most common): a cert-file-free eduroam with PEAP (or TTLS)
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 60) {  //after 30 seconds timeout - reset board
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("MAC address ");
  Serial.print(WiFi.macAddress());
  Serial.print(", IP address ");
  Serial.print(WiFi.localIP());
  Serial.print(", netmask ");
  Serial.print(WiFi.subnetMask());
  Serial.print(", gateway ");
  Serial.print(WiFi.gatewayIP());
  Serial.print(", DNS ");
  Serial.println(WiFi.dnsIP());

  WiFi.setTxPower(TXPOWER);
}

void loop() {
  delay(20000);

  if (WiFi.status() == WL_CONNECTED) {  //if we are connected to Eduroam network
#ifdef OLED_ENABLED
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0,  "Connected!");
    if (noSTAs <= 0) {
      noSTAs = 0;
      display.drawString(0, 10,  "No STAs");
    } else {
      display.drawString(0, 10,  String(noSTAs) + " STAs");
      display.drawString(0, 20,  IPAddress(ipLastSTA).toString());
    }
    display.display();
#endif

    counter = 0;                        //reset counter
    Serial.println("Wifi is still connected with IP " + WiFi.localIP().toString() + " and transmission power " + String(convertTxPowerTodBm(WiFi.getTxPower())) + " dBm (approx.)");
    Serial.printf("SSID %s, channel %d, RSSI %d dBm\n", WiFi.SSID().c_str(), WiFi.channel(), WiFi.RSSI());
  } else if (WiFi.status() != WL_CONNECTED) {  //if we lost connection, retry
#ifdef OLED_ENABLED
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0,  "Disconnected!");
    display.display();
#endif

    WiFi.begin(ssid);
  }
  while (WiFi.status() != WL_CONNECTED) {  //during lost connection, print dots
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 120) {  //60 seconds timeout - reset board
      ESP.restart();
    }
  }
}

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:     Serial.println("STA Started"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED: Serial.println("STA Connected"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("STA Got IP");
      Serial.println(WiFi.STA);
      WiFi.AP.enableNAPT(true);
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("STA Lost IP");
      WiFi.AP.enableNAPT(false);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("STA Disconnected");
      WiFi.AP.enableNAPT(false);
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP: Serial.println("STA Stopped"); break;

    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("AP Started");
      Serial.println(WiFi.AP);
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("AP STA Connected"); noSTAs++; break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("AP STA Disconnected"); noSTAs--; break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.print("AP STA IP Assigned: ");
      ipLastSTA = info.wifi_ap_staipassigned.ip.addr;
      Serial.println(IPAddress(ipLastSTA));
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED: Serial.println("AP Probe Request Received"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:           Serial.println("AP Stopped"); break;

    default: break;
  }
}
