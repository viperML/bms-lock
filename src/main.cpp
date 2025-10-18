#include <Arduino.h>

void setup() {
  // Initialize serial communication at 115200 baud
  Serial.begin(115200);

  // Wait for serial port to connect
  delay(1000);

  Serial.println("\n\n================================");
  Serial.println("Hello World from ESP32!");
  Serial.println("================================");
  Serial.println("System initialized successfully");
}

void loop() {
  // Print hello world message every 2 seconds
  Serial.println("Hello World!");
  Serial.print("Millis: ");
  Serial.println(millis());

  delay(2000);
}
