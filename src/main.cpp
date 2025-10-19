#include <Arduino.h>
#include <TFT_eSPI.h>
#include "BluetoothSerial.h"

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

// Bluetooth MAC address to connect to (CHANGE THIS TO YOUR TARGET DEVICE)
// Format: "XX:XX:XX:XX:XX:XX"
const char* targetBTAddress = "40:DE:24:53:34:30";

BluetoothSerial SerialBT;
TFT_eSPI tft = TFT_eSPI();

// Connection state
bool isConnected = false;
bool isConnecting = false;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_RETRY_INTERVAL = 10000; // 10 seconds
int connectionAttempts = 0;

// Display colors
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_CONNECTED TFT_GREEN
#define COLOR_DISCONNECTED TFT_RED
#define COLOR_CONNECTING TFT_YELLOW

void initDisplay() {
  // Initialize the display
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(COLOR_BACKGROUND);

  // Turn on backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  Serial.println("Display initialized");
}

void updateDisplay(const char* status, uint16_t statusColor) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Title
  tft.setTextColor(TFT_CYAN, COLOR_BACKGROUND);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("BT Monitor");

  // Target device
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Target: ");
  tft.println(targetBTAddress);

  // Connection attempts
  tft.setCursor(10, 55);
  tft.print("Attempts: ");
  tft.println(connectionAttempts);

  // Status
  tft.setTextColor(statusColor, COLOR_BACKGROUND);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.println(status);

  // Instructions
  tft.setTextColor(TFT_MAGENTA, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setCursor(10, 120);
  tft.println("Make sure:");
  tft.setCursor(10, 135);
  tft.println("1. Phone BT is ON");
  tft.setCursor(10, 150);
  tft.println("2. Phone is paired");
  tft.setCursor(10, 165);
  tft.println("   to ESP32");

  // Visual indicator (circle)
  tft.fillCircle(160, 205, 30, statusColor);
}

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Bluetooth connected!");
    isConnected = true;
    isConnecting = false;
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Bluetooth disconnected!");
    isConnected = false;
    isConnecting = false;
  }
}

void connectToBTDevice() {
  if (isConnecting || isConnected) {
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastConnectionAttempt < CONNECTION_RETRY_INTERVAL) {
    return;
  }

  lastConnectionAttempt = currentTime;
  isConnecting = true;
  connectionAttempts++;

  Serial.print("Attempting to connect to: ");
  Serial.println(targetBTAddress);
  Serial.print("Attempt #");
  Serial.println(connectionAttempts);

  updateDisplay("CONNECTING...", COLOR_CONNECTING);

  // Disconnect first if needed
  if (SerialBT.connected()) {
    SerialBT.disconnect();
    delay(1000);
  }

  // Convert MAC address string to uint8_t array
  uint8_t addr[6];
  sscanf(targetBTAddress, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
         &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);

  // Attempt connection with timeout
  Serial.println("Initiating connection...");
  bool connected = SerialBT.connect(addr);

  if (connected) {
    Serial.println("Connection successful!");
    isConnected = true;
    isConnecting = false;
    updateDisplay("CONNECTED!", COLOR_CONNECTED);
  } else {
    Serial.println("Connection failed!");
    Serial.println("Please ensure:");
    Serial.println("1. Phone Bluetooth is ON");
    Serial.println("2. Phone is discoverable OR already paired");
    Serial.println("3. Check the MAC address is correct");
    isConnecting = false;
    updateDisplay("FAILED - RETRY", COLOR_DISCONNECTED);
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n================================");
  Serial.println("ESP32 Bluetooth Display Monitor");
  Serial.println("================================");
  Serial.println("\nIMPORTANT INSTRUCTIONS:");
  Serial.println("1. Turn ON Bluetooth on your phone");
  Serial.println("2. Make your phone DISCOVERABLE or");
  Serial.println("3. Pair your phone with this ESP32:");
  Serial.println("   - Look for 'ESP32_BT_Client' in");
  Serial.println("     your phone's Bluetooth settings");
  Serial.println("   - Pair with it first");
  Serial.println("================================\n");

  // Initialize display
  initDisplay();
  updateDisplay("INITIALIZING...", COLOR_CONNECTING);

  // Initialize Bluetooth in Master mode
  if (!SerialBT.begin("ESP32_BT_Client", true)) {
    Serial.println("Bluetooth initialization failed!");
    updateDisplay("BT INIT FAILED", COLOR_DISCONNECTED);
    while(1); // Halt
  }

  SerialBT.register_callback(btCallback);

  Serial.println("Bluetooth initialized");
  Serial.print("Target device: ");
  Serial.println(targetBTAddress);

  updateDisplay("READY", COLOR_CONNECTING);
  delay(1000);

  // Initial connection attempt
  connectToBTDevice();
}

void loop() {
  // Check connection status and update display
  static bool lastConnectionState = false;

  if (isConnected != lastConnectionState) {
    lastConnectionState = isConnected;
    if (isConnected) {
      updateDisplay("CONNECTED", COLOR_CONNECTED);
    } else {
      updateDisplay("DISCONNECTED", COLOR_DISCONNECTED);
    }
  }

  // If not connected, try to reconnect
  if (!isConnected && !isConnecting) {
    connectToBTDevice();
  }

  delay(100);
}
