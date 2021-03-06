#include <Arduino.h>

//#include "WiFi.h"
#include "esp_wifi.h"
#include <HTTPClient.h>
#include <vector>
#include "string.h"
#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library
// VERY IMPORTANT!!! 
// You need to modify the library file User_Setup_Select.h
// comment the line #include <User_Setup.h>
// uncomment the line #include <User_Setups/Setup26_TTGO_T_Wristband.h>



TFT_eSPI tft = TFT_eSPI();       // Invoke custom library




class MACPool {
private:
 String mac;
 int signal;
 unsigned long time;
 bool newMAC;
 
 
public:
MACPool(String,int,unsigned long,bool);
String getMAC();
int getSignal();
unsigned long getTime();
void updateTime(unsigned long);
bool getNewMAC();
void updateNewMAC(bool);
};

MACPool::MACPool(String mac,int signal,unsigned long time,bool newMAC)
{
 this->mac = mac;
 this->signal = signal;
 this->time = time;
 this->newMAC = newMAC;
}

String MACPool::getMAC() {
  return this->mac;
}

int MACPool::getSignal() {
  return this->signal;
}

unsigned long MACPool::getTime() {
  return this->time;
}

void MACPool::updateTime(unsigned long time) {
  this->time=time;
}

void MACPool::updateNewMAC(bool nm) {
  this->newMAC=nm;
}

bool MACPool::getNewMAC() {
  return this->newMAC;
}

int screen = 1;

using namespace std;

const char* ssid     = "xxxx";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "xxxx";         // network password

unsigned int channel = 1;

float weight = 1.0; // weigh the index with real time data


const wifi_promiscuous_filter_t filt={
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};


vector<MACPool> listOfMAC;

int riskIndex = 0;
int oldIndex = 0;
String riskValue = "low";

typedef struct {
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  
    wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
    WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
    int signal = p->rx_ctrl.rssi;

    MacAddr mac_add = (MacAddr)wh->sa;
    String sourceMac;
    for (int i = 0; i < sizeof(mac_add.mac); i++) {
        String macDigit = String(mac_add.mac[i], HEX);
        if (macDigit.length() == 1) {
            macDigit = "0" + macDigit;
        }
        
        sourceMac += macDigit;
        if (i != sizeof(mac_add.mac) - 1) {
          sourceMac += ":";
        }
    }

    sourceMac.toUpperCase();

    if (signal > -70) { // signal level threshold

      // Prevent duplicates
      for (int i = 0; i < listOfMAC.size(); i++) {
          if (sourceMac == listOfMAC[i].getMAC()) {
              listOfMAC[i].updateTime(millis()); // update the last time MAC found
              listOfMAC[i].updateNewMAC(false);
              return;
          }
      }

      // new MAC

      listOfMAC.push_back(MACPool(sourceMac,signal,millis(),true));

      //Serial.println(listOfMAC[listOfMAC.size()-1].getMAC());

      // purge outdated MACs

      for (auto it = listOfMAC.begin(); it != listOfMAC.end(); ) {
          if (millis()-it->getTime() > 300000) { // remove if older than 5min
              it = listOfMAC.erase(it);
          } else {
              ++it;
          }
      }

      // update the risk index

      int recentLowSingal = 0;
      int recentHighSingal = 0;
      for (int i = 0; i < listOfMAC.size(); i++) {
          if (millis()-listOfMAC[i].getTime() < 30000 && listOfMAC[i].getNewMAC()==true) {
              if (listOfMAC[i].getSignal() < -60) { recentLowSingal++; } else { recentHighSingal++; }
          }
          // new low and high signals from last 30sec
      }

      riskIndex = int(riskIndex + recentLowSingal + (2*recentHighSingal) * weight);

      // initial estimation -provisional- : activities with moderated risk: 800-1300 / hour
      // activities with low risk: < 800 / hour
      // aftivities with high risk: > 1300 / hour
      // if you are using the device 12 hour / day: the risk is low if < 9600, medium if >= 9600 <= 15600, high > 15600
      if (riskIndex<9600) { riskValue="low";}
      if (riskIndex>=9600 && riskIndex<=15600) { riskValue="medium";}
      if (riskIndex>15600) { riskValue="high, go home, please!"; }

    }
}


void setup(void) {
  //Serial.begin(9600);
  pinMode(25, PULLUP);
  pinMode(27, OUTPUT);
  pinMode(33,PULLUP);
  //digitalWrite(25, HIGH);
  //digitalWrite(4, HIGH);
  tft.init();
  tft.setRotation(2);

  // get real time data from online sources disabled
  /*
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); 
  Serial.println(" ...");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 50) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); 
    Serial.print(' ');
  }
  
  if (i < 50) { // connection OK
    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    HTTPClient client;
    client.begin("http://api.ferranfabregas.me/covid19"); // not the definitive API, for testing only
    int httpCode = client.GET();
    if (httpCode > 0) {
      // only for testing
      String payload = client.getString(); // get the data
      //Serial.println(payload);
      int inf_pos = payload.indexOf("infected");
      String infected = payload.substring(inf_pos+11,payload.indexOf(",",inf_pos));
      int inf_inh = payload.indexOf("inhabitants");
      String inhabitants = payload.substring(inf_inh+14,payload.indexOf(",",inf_inh));
      //Serial.println(inhabitants);
      weight = (infected.toFloat() / inhabitants.toFloat()) * 100;
      Serial.println(weight);
    } else {
      Serial.println("Error on HTTP request");
    }
  } else { 
    Serial.println("failed!");
    delay(2000);
  }
  WiFi.disconnect();
  WiFi.enableSTA(false);
  WiFi.softAPdisconnect(true);
  delay(5000);
  */

  //Serial.println("Setting promiscuous mode...");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  // set WiFi in promiscuous mode
  //esp_wifi_set_mode(WIFI_MODE_STA);            // Promiscuous works only with station mode
  esp_wifi_set_mode(WIFI_MODE_NULL);
  // power save options
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_start();
  esp_wifi_set_max_tx_power(-4);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);   // Set up promiscuous callback
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void loop() {

  if (oldIndex!=riskIndex) {
    tft.fillScreen(0x000000);
    tft.setCursor(0, 0, 2);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);  
    tft.setTextSize(1);
    // We can now plot text on screen using the "print" class
    tft.println("Covid-19");
    tft.println("daily");
    tft.println("accumulated");
    tft.println("risk index");
    tft.println("");
    tft.setTextFont(4);
    tft.println(riskValue);
    tft.println((String)riskIndex);
    oldIndex = riskIndex;
  }
  if (digitalRead(33)) {
    if (screen==0) {
      digitalWrite(27,HIGH);
      screen = 1;
    } else {
      digitalWrite(27,LOW);
      screen = 0;
    }
  }
  //Serial.println();
  channel++;
  if(channel == 11) channel = 1;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(2000);
}
