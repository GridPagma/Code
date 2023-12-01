#include <NewPing.h>

#define trigPin 27
#define echoPin 26
#define maxDistance 200
NewPing sonar(trigPin, echoPin, maxDistance);

void setup() {
  Serial.begin(115200);
  delay(50);
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
}

void loop() {
  delay(50);
  int distance = sonar.ping_cm();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(250);
}
