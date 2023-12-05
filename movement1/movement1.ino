#include <Arduino.h>
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

#define SERVICE_UUID        "ea911a8f-2276-4411-8a1c-be9af3218acb"
#define CHARACTERISTIC_UUID "8c3d30e5-476e-4fd8-bec9-5dc5b5e44506"

#define L1 27 //ain2
#define L2 26 //ain1


#define LS 32 //stdby left
#define LP 22 //pwma left

#define R1 25 //ain2
#define R2 33 //ain1

#define RS 19 //stdby right
#define RP 21 //pwma right

#define Rsensor 17
#define Lsensor 5
#define Csensor 35

//bluetooth stuff
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

//************************************ phone controlled **********************
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
      analogWrite(LP, abs(in[2]-in[0])/2);
      analogWrite(RP, sqrt((in[2]*in[2])+(in[0]*in[0]))/2);
    //right
    }else{
      move(L1, L2, 1);
      move(R1, R2, in[2]>in[1] ? 1:0);
      Serial.printf("LP = %d", abs(in[2]-in[0]));
      analogWrite(LP, sqrt((in[2]*in[2])+(in[1]*in[1]))/2);
      analogWrite(RP, abs(in[2]-in[1])/2);
    }
  //down
  }else{
    //left
    if(in[0] > in[1]){
      move(L1, L2, 0);
      move(R1, R2, in[3]>in[0] ? 0:1);
      Serial.printf("LP = %d",abs(in[3]-in[0]));
      analogWrite(LP, abs(in[3]-in[0])/2);
      analogWrite(RP, sqrt((in[3]*in[3])+(in[0]*in[0]))/2);
    //right
    }else{
      move(L1, L2, in[3]>in[1] ? 0:1);
      move(R1, R2, 0);
      Serial.printf("LP = %d",abs(in[3]-in[1]));
      analogWrite(LP, sqrt((in[3]*in[3])+(in[1]*in[1]))/2);
      analogWrite(RP, abs(in[3]-in[1])/2);
    }
  }
}

//******************* bluetooth input *****************************************
class MyCallbacks: public BLECharacteristicCallbacks {

  void onWrite (BLECharacteristic *pChar) override {
    checkInput(pChar);
  }

    void checkInput(BLECharacteristic *pChar){
      std::string receivedData = pChar->getValue();
      String input = String(receivedData.c_str());
      Serial.print("Received: " + input);
      String Lturn, Rturn, forward;

      switch(input[0]){
        case 'f': //feet
          input = input.substring(1);
          Serial.println("feet: " + input);
          break;
        case 'i': //inches
          input = input.substring(1);
          Serial.println("inches: " + input);
          break;
        case 's': //seconds
          forward = getSubstringUntil(input, 'f');
          input = input.substring(forward.length()+1);
          Lturn = getSubstringUntil(input, 'L');
          input = input.substring(Lturn.length()+1);
          Rturn = getSubstringUntil(input, 'R');
          Serial.println("forward seconds: " + forward);
          Serial.println("left turn seconds: " + Lturn);
          Serial.println("right turn seconds: " + Rturn);
          automatic(forward.toDouble(), Lturn.toDouble(), Rturn.toDouble());
          break;
        case 'd': //detect ink
          detect();
          break;
        default:
          Serial.println("Other info: " + input);
          phoneControlled(pChar); //default input-> phone controlled
      }
    }

    void automatic(double seconds, double Lturn, double Rturn){
      //forward and left = true
      Serial.println("Running automatic...");

      front_back(seconds, true); //forward
      rotate(Lturn, true); //left
      delay(500);
      rotate(Rturn, false); //right
      front_back(seconds, false); //back

      Serial.println("Automatic done");
    }

    void detect(){
      Serial.println("detecting...");

      while(1){
        int R = digitalRead(Rsensor);
        int L = digitalRead(Lsensor);
        int C = digitalRead(Csensor);
        if(C != 0){
          front_back(.01, true);
        }
        else if((R == 0 && L == 0)){
          front_back(.01, true); //forward
        }
        else if(R != 0 && L == 0){
          rotate(.05, true); //left
        }
        else if(R == 0 && L != 0){
          rotate(.05, false);//right
        }
        else{
          front_back(.01, true); //forward
        }
        //delay(200);
      }    
      Serial.println("Stopping detection...");  
    }

    // void stopMovement() {
    //   digitalWrite(motor1Pin1, LOW);
    //   digitalWrite(motor1Pin2, LOW);
    //   digitalWrite(motor2Pin1, LOW);
    //   digitalWrite(motor2Pin2, LOW);
    // }


    String getSubstringUntil(String input, char delimiter) {
    int delimiterIndex = input.indexOf(delimiter);

    if (delimiterIndex != -1) {
      // If the delimiter is found, extract the substring until that point
      return input.substring(1, delimiterIndex);
    } else {
      // If the delimiter is not found, return the entire string
      return "";
    }
  }

    void phoneControlled(BLECharacteristic *pChar){

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

void front_back(double seconds, bool fb){
    //front = true, back = false
    double miliseconds = seconds*1000;
    Serial.println(fb? "forward" : "back");

    digitalWrite(LS, HIGH); //high = run
    digitalWrite(RS, HIGH);

    digitalWrite(L1, fb? HIGH : LOW);
    digitalWrite(L2, fb? LOW : HIGH);
    digitalWrite(R1, fb? HIGH : LOW);
    digitalWrite(R2, fb? LOW : HIGH);

    analogWrite(LP, 80); // 50% speed
    analogWrite(RP, 80);

    delay(miliseconds); //run the motor for x miliseconds
    analogWrite(LP, 0); //stop
    analogWrite(RP, 0);

  }

  void rotate(double seconds, bool lr){
    //left = true, right = false
    double miliseconds = seconds*2000;
    Serial.println(lr? "left" : "right");

    digitalWrite(LS, HIGH); //high = run
    digitalWrite(RS, HIGH);

    digitalWrite(L1, lr? LOW: HIGH);
    digitalWrite(L2, lr? HIGH : LOW);
    digitalWrite(R1, lr? HIGH : LOW);
    digitalWrite(R2, lr? LOW: HIGH);

    analogWrite(LP, 80); // 50% speed
    analogWrite(RP, 80);

    delay(miliseconds); //run the motor for x seconds
    analogWrite(LP, 0); //stop
    analogWrite(RP, 0);

  }


void setup() {

  //bluetooth stuff
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
  pinMode(Rsensor, INPUT);
  pinMode(Lsensor, INPUT);
  pinMode(Csensor, INPUT);

}

void loop() {
  //more bluetooth stuff
  
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

  //************************************************** movement: **********************************************

  //grab size of board via app, then calculate how many seconds to travel that? = length variable
  //maybe have a counter for how many "down" rotations to have depending on height of board

  

  //move_forward(5000); //travel size of board to the right -->
  //rotate_left(500); //rotate left for half second to gain 45 degree angle down
  //move_back(500); //move back for half second to go down the board
  //rotate_right(500); //rotate right for half second to turn bot straight horizontally again
  //move_back(5000); //travel size of board to the left <--

  // digitalWrite(LS, LOW); 
  // digitalWrite(RS, LOW);

//   int R = digitalRead(Rsensor);
//   int L = digitalRead(Lsensor);
//   if(R == 0 && L == 0){
//     move_forward(.2);
//   }
//   else if(R != 0 && L == 0){
//     rotate_left(.2);
//   }
//   else if(R == 0 && L != 0){
//     rotate_right(.2);
//   }
//   else{
//     move_forward(.2);
//   }
//  delay(200);
}
