#include <NewPing.h>

int trigPin = 6;
int echoPin = 5;
int maxDistance = 15;
NewPing sonar(trigPin, echoPin, maxDistance);

void setup() {
  Serial.begin(115200);
  delay(50);
}

void loop() {
  delay(50);
  int distance = sonar.ping_cm();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(250);
}