#include <Arduino.h>
#include <vector>
#include "heltec.h"
#include "string.h"
#include "WiFi.h"
#include "esp_wifi.h"

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


using namespace std;

unsigned int channel = 1;

const wifi_promiscuous_filter_t filt={
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

//vector<String> macArray;

vector<MACPool> listOfMAC;

int riskIndex = 0;
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

    if (signal > -70) {
      // Prevent duplicates
      /*for (int i = 0; i < macArray.size(); i++) {
          if (sourceMac == macArray[i]) {
              return;
          }
      }*/

      for (int i = 0; i < listOfMAC.size(); i++) {
          if (sourceMac == listOfMAC[i].getMAC()) {
              listOfMAC[i].updateTime(millis()); // update the last time MAC found
              listOfMAC[i].updateNewMAC(false);
              return;
          }
      }


      //macArray.push_back(sourceMac);

      // new MAC

      listOfMAC.push_back(MACPool(sourceMac,signal,millis(),true));

      //Serial.print("M1: ");
      //Serial.println(sourceMac); // WORKING!!!!
      //Serial.print("M2: ");
      Serial.println(listOfMAC[listOfMAC.size()-1].getMAC());
      //Serial.print("Size: ");
      //Serial.println(listOfMAC.size());

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

      riskIndex = riskIndex + recentLowSingal + (2*recentHighSingal);

      // initial estimation -provisional- : activities with moderated risk: 800-1300 / hour
      // activities with low risk: < 800 / hour
      // aftivities with high risk: > 1300 / hour
      // if you are using the device 12 hour / day: the risk is low if < 9600, medium if >= 9600 <= 15600, high > 15600
      if (riskIndex<9600) { riskValue="low";}
      if (riskIndex>=9600 && riskIndex<=15600) { riskValue="medium";}
      if (riskIndex>15600) { riskValue="high, go home, please!"; }

    }
}

void setup() {
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*Heltec.LoRa Disable*/, false /*Serial Enable*/);

  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0,"Starting...");
  Heltec.display->display();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  //esp_wifi_set_mode(WIFI_MODE_STA);            // Promiscuous works only with station mode
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);   // Set up promiscuous callback
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void loop() {
  channel++;
  if(channel == 11) channel = 1;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0,"Covid-19 accumulated daily");
  Heltec.display->drawString(0, 10,"personal risk index:");
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 24,riskValue);
  Heltec.display->drawString(0, 44,(String)riskIndex);
  Heltec.display->display();
  delay(1000);
}
