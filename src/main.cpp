#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.begin(115200);
  pinMode(2,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
    
    digitalWrite(2,LOW);
    Serial.println("LOW");
    delay(150);
    digitalWrite(2,HIGH);
    Serial.println("HIGH");
    delay(150);
  }

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}