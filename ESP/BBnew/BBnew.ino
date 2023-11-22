#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int data[4] = {0, 0, 0, 0};

#define L1 27
#define L2 26


#define LS 32
#define LP 22

#define R1 25
#define R2 33

#define RS 19 
#define RP 21

#define SERVICE_UUID        "ea911a8f-2276-4411-8a1c-be9af3218acb"
#define CHARACTERISTIC_UUID "8c3d30e5-476e-4fd8-bec9-5dc5b5e44506"

void move(int m1, int m2, bool p){
  digitalWrite(m1, p ? HIGH:LOW);
  digitalWrite(m2, p ? LOW:HIGH);
}

void movement(int in[]){
  digitalWrite(LS, HIGH);
  digitalWrite(RS, HIGH);
  //left right up down
  //up
  if(in[2] > in[3]){
    //left
    if(in[0] > in[1]){
      move(L1, L2, in[2]>in[0] ? 1:0);
      move(R1, R2, 1);
      Serial.printf("LP = %d",abs(in[2]-in[0]));
      analogWrite(LP, abs(in[2]-in[0]));
      analogWrite(RP, sqrt((in[2]*in[2])+(in[0]*in[0])));
    //right
    }else{
      move(L1, L2, 1);
      move(R1, R2, in[2]>in[1] ? 1:0);
      Serial.printf("LP = %d", abs(in[2]-in[0]));
      analogWrite(LP, sqrt((in[2]*in[2])+(in[1]*in[1])));
      analogWrite(RP, abs(in[2]-in[1]));
    }
  //down
  }else{
    //left
    if(in[0] > in[1]){
      move(L1, L2, 0);
      move(R1, R2, in[3]>in[0] ? 0:1);
      Serial.printf("LP = %d",abs(in[3]-in[0]));
      analogWrite(LP, abs(in[3]-in[0]));
      analogWrite(RP, sqrt((in[3]*in[3])+(in[0]*in[0])));
    //right
    }else{
      move(L1, L2, in[3]>in[1] ? 0:1);
      move(R1, R2, 0);
      Serial.printf("LP = %d",abs(in[3]-in[1]));
      analogWrite(LP, sqrt((in[3]*in[3])+(in[1]*in[1])));
      analogWrite(RP, abs(in[3]-in[1]));
    }
  }
}


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite (BLECharacteristic *pChar) override {
    std::string pChar2_value_stdstr = pChar->getValue();
    String pChar2_value_string = String(pChar2_value_stdstr.c_str());
    int pChar2_value_int = pChar2_value_string.toInt();
    Serial.print("Bytes Received: ");
    for (int i = 0; i < 4; i++){
      int j = (int)pChar2_value_string[i];
      if(j == 1){
        j = 0;
      }
      Serial.printf("%d, ", j);
      data[i] = j;
    }
    Serial.print("\n");
    movement(data);
  }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("MyESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
                    
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());


  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  pinMode(L1,OUTPUT);
  pinMode(L2,OUTPUT);
  pinMode(LS,OUTPUT);
  pinMode(LP,OUTPUT);
  pinMode(R1,OUTPUT);
  pinMode(R2,OUTPUT);
  pinMode(RS,OUTPUT);
  pinMode(RP,OUTPUT);
}

void loop() {
  // notify changed value
  
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
