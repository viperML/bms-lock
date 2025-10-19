#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEClient.h>

// BLE is always available on ESP32, no need for CONFIG checks for BLE

// BMS Bluetooth MAC address (BLE format - CHANGE THIS TO YOUR BMS MAC)
// Format: "XX:XX:XX:XX:XX:XX"
// Original example: "a4:c1:38:46:08:56"
// Testing with phone: "40:de:24:53:34:30"
const char* targetBTAddress = "40:de:24:53:34:30";

// BLE objects
BLEClient* pClient = nullptr;
BLEAddress* pServerAddress = nullptr;

TFT_eSPI tft = TFT_eSPI();

// Connection state
bool isConnected = false;
bool isConnecting = false;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_RETRY_INTERVAL = 10000; // 10 seconds
int connectionAttempts = 0;

// BLE Callbacks
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("BLE Connected!");
    isConnected = true;
    isConnecting = false;
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("BLE Disconnected!");
    isConnected = false;
    isConnecting = false;
  }
};

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
  tft.println("BMS Lock");

  // Target device
  tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("BMS MAC: ");
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
  tft.println("Ensure:");
  tft.setCursor(10, 135);
  tft.println("1. BMS is ON");
  tft.setCursor(10, 150);
  tft.println("2. BMS in range");
  tft.setCursor(10, 165);
  tft.println("3. MAC is correct");

  // Visual indicator (circle)
  tft.fillCircle(160, 205, 30, statusColor);
}

void connectToBLEDevice() {
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

  Serial.print("Attempting BLE connection to: ");
  Serial.println(targetBTAddress);
  Serial.print("Attempt #");
  Serial.println(connectionAttempts);

  updateDisplay("CONNECTING...", COLOR_CONNECTING);

  // Disconnect first if needed
  if (pClient != nullptr && pClient->isConnected()) {
    pClient->disconnect();
    delay(1000);
  }

  // Create client if not exists
  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    Serial.println("BLE Client created");
  }

  // Attempt connection
  Serial.println("Initiating BLE connection...");
  pServerAddress = new BLEAddress(targetBTAddress);

  if (pClient->connect(*pServerAddress)) {
    Serial.println("BLE Connection successful!");
    isConnected = true;
    isConnecting = false;
    updateDisplay("CONNECTED!", COLOR_CONNECTED);
  } else {
    Serial.println("BLE Connection failed!");
    Serial.println("Please ensure:");
    Serial.println("1. BMS is powered ON");
    Serial.println("2. BMS is in range");
    Serial.println("3. MAC address is correct");
    isConnecting = false;
    updateDisplay("FAILED - RETRY", COLOR_DISCONNECTED);
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n================================");
  Serial.println("ESP32 BLE BMS Lock Display");
  Serial.println("================================");
  Serial.println("\nIMPORTANT INSTRUCTIONS:");
  Serial.println("1. Ensure BMS is powered ON");
  Serial.println("2. BMS must be within range");
  Serial.println("3. This will lock the BMS by");
  Serial.println("   maintaining a BLE connection");
  Serial.println("================================\n");

  // Initialize display
  initDisplay();
  updateDisplay("INITIALIZING...", COLOR_CONNECTING);

  // Initialize BLE
  Serial.println("Initializing BLE...");
  BLEDevice::init("ESP32_BMS_Lock");

  Serial.println("BLE initialized");
  Serial.print("Target BMS MAC: ");
  Serial.println(targetBTAddress);

  updateDisplay("READY", COLOR_CONNECTING);
  delay(1000);

  // Initial connection attempt
  connectToBLEDevice();
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
    connectToBLEDevice();
  }

  delay(100);
}
