#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <esp_now.h>

#define SERVICE_UUID "0b5ff426-c3a6-46c0-825d-d43fb58fee28"
#define CHARACTERISTIC_UUID "f0ad7ee7-0d84-455c-9dda-7fca1f7f3fe6"


// Relay MAC address
uint8_t receiverMacAddr1[] = {0x58, 0xBF, 0x25, 0xDC, 0x54, 0x8E};
esp_now_peer_info_t peer1;

uint8_t receiverMacAddr2[] = {0x58, 0xBF, 0x25, 0xDC, 0x52, 0x18};
esp_now_peer_info_t peer2;

// Relay state data structure
struct __attribute__((packed)) datapacket {
  bool relayState;
};

datapacket Relay1State;
datapacket Relay2State;

class MyCallbacks: public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0){
      Serial.println("New Value");
      for(int i = 0; i < value.length(); i++){
        Serial.print(value[i]);
      }
      Serial.printf("\n***************\n");
    }

    if(value == "H1"){
      Relay1State.relayState = 1;
      esp_now_send(peer1.peer_addr, (uint8_t *)&Relay1State, sizeof(Relay1State));
    } else if (value =="H2"){
      Relay2State.relayState = 1;
      esp_now_send(peer2.peer_addr, (uint8_t *)&Relay2State, sizeof(Relay2State));
    } else if (value == "L1"){
      Relay1State.relayState = 0;
      esp_now_send(peer1.peer_addr, (uint8_t *)&Relay1State, sizeof(Relay1State));
    } else if (value == "L2"){
      Relay2State.relayState = 0;
      esp_now_send(peer2.peer_addr, (uint8_t *)&Relay2State, sizeof(Relay2State));
    }
  }
};

esp_now_peer_info_t addPeer(uint8_t *receiverMacAddr, int channel, bool encrypt){
  esp_now_peer_info_t peer;
  memcpy(peer.peer_addr, receiverMacAddr, 6);
  peer.channel = channel;
  peer.encrypt = encrypt;
  if (esp_now_add_peer(&peer) == ESP_OK){
    Serial.printf("Peered: %02x:%02x:%02x:%02x:%02x:%02x\n", peer.peer_addr[0], peer.peer_addr[1], peer.peer_addr[2], peer.peer_addr[3], peer.peer_addr[4], peer.peer_addr[5]);
  }
  return peer;
}

void transmissionComplete(uint8_t *receiver_mac, uint8_t transmissionStatus){
  if (transmissionStatus == 0){
    Serial.println("Data sent");
  } else {
    Serial.print("Error Code: ");
    Serial.println(transmissionStatus);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAP("ESP-phantom", "");
  Serial.println("softap");
  Serial.println();
  Serial.println(WiFi.softAPIP());

  if(esp_now_init() != ESP_OK ){
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  peer1 = addPeer(receiverMacAddr1, 0, false);
  peer2 = addPeer(receiverMacAddr2, 0, false);

  esp_now_register_send_cb((esp_now_send_cb_t)transmissionComplete);

  Serial.println("BLE STARTING");

  BLEDevice::init("ESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,
                                                                      BLECharacteristic::PROPERTY_READ |
                                                                      BLECharacteristic::PROPERTY_WRITE );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
}