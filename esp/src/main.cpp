#include <WiFi.h>
#include <WiFiUdp.h>
#include "configuration.h"

#define CHANGE_THRESHOLD 0.01

WiFiUDP Udp;
IPAddress destIp;

// SLIDERS
const int NUM_SLIDERS = 2;
byte pins[NUM_SLIDERS] = {34,35};
uint previousReadings[NUM_SLIDERS];
unsigned long lastSampleTime = 0; // For timing 50 ms intervals

// MUTE
struct MuteButton {
  String name;
  int buttonPin;
  int ledPin;
  bool isPressed;
  bool isMuted;
};

const int NUM_BUTTONS = 2;
MuteButton muteButtons[NUM_BUTTONS] = {
  {"master", 14, 12, false, false}
};

// SWITCH
const int switchPin = 5;
const int ledSpeakers = 18;
const int ledHeadphones = 19;
bool isSpeakers = false;
bool switchPressed = false; // Tracks if the button is currently pressed


void connectToWifi() {
    destIp = IPAddress();
    destIp.fromString(DEST_IP);

    Serial.begin(9600);
    Serial.println();
    Serial.println("Configuring access point...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(SSID, PASSWORD);

    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void sendMuteMessage(String device, bool isMuted) {
  // Prepare the message
  String message = isMuted ? "mute " : "unmute ";
  message += device;

  // Start UDP and send the message
  Udp.beginPacket(destIp, 16991);
  Udp.print(message);
  Udp.endPacket();

  // Print the message to serial
  Serial.print("Sent UDP message: ");
  Serial.println(message);
}

void setDevice(String device) {
  String message = "set " + device;
  Udp.beginPacket(destIp, 16991);
  Udp.print(message);
  Udp.endPacket();
  Serial.println("Sent UDP message: " + message);
}

void sendSwitchMessage() {
  String message = "switch";
  Udp.beginPacket(destIp, 16991);
  Udp.print(message);
  Udp.endPacket();
  Serial.println("Sent UDP message: switch");
}

void sendSlidersData(bool forceData = false) {
  uint readings[NUM_SLIDERS];
  bool dataChanged = false;

  unsigned long currentMillis = millis();

// Check if 50 ms has passed
  if (currentMillis - lastSampleTime >= 50) {
    lastSampleTime = currentMillis;
    dataChanged = false; // Reset dataChanged flag

  for (int i = 0; i < NUM_SLIDERS; i++) {
        readings[i] = map(analogRead(pins[i]), 0, 1023, 0, 100);
        // Calculate the percentage change compared to the previous reading
        float percentChange = abs((float)readings[i] - previousReadings[i]) / 100;
        
        if (percentChange >= CHANGE_THRESHOLD) {
            dataChanged = true;
            previousReadings[i] = readings[i];  // Update previous reading
        }
    }
  }

  if (dataChanged || forceData) {
        String builtString = String("sliders ");
        for (int i = 0; i < NUM_SLIDERS; i++) {
            builtString += String((int)readings[i]);

            if (i < NUM_SLIDERS - 1) {
                builtString += String("|");
            }
        }

        Serial.println(builtString + "\n");

        Udp.beginPacket(destIp, 16991);
        Udp.write((uint8_t *)builtString.c_str(), builtString.length());
        Udp.endPacket();

        // Handle mute/unmute based on slider 0
        if (readings[0] < 3 && !muteButtons[0].isMuted) { // Mute when slider is at 0 and not already muted
            sendMuteMessage("master", true);
            muteButtons[0].isMuted = true;
            digitalWrite(muteButtons[0].ledPin, LOW);  // Turn on the LED
        } else if (readings[0] > 0 && muteButtons[0].isMuted) { // Unmute when slider is above 0 and currently muted
            sendMuteMessage("master", false);
            muteButtons[0].isMuted = false;
            digitalWrite(muteButtons[0].ledPin, HIGH);  // Turn off the LED
        }
    }
    delay(10);
}

void checkMuteButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentButtonState = digitalRead(muteButtons[i].buttonPin) == LOW;

    if (currentButtonState && !muteButtons[i].isPressed) {
      muteButtons[i].isPressed = true;  // Mark the button as pressed
      muteButtons[i].isMuted = !muteButtons[i].isMuted;  // Toggle mute state
      digitalWrite(muteButtons[i].ledPin, muteButtons[i].isMuted ? LOW : HIGH);  // Update the LED

      sendMuteMessage(muteButtons[i].name, muteButtons[i].isMuted);  // Send a message for this device
    } else if (!currentButtonState) {
      muteButtons[i].isPressed = false;  // Reset button state when released
    }
  }
}

void checkSwitch() {
  bool currentButtonState = digitalRead(switchPin) == LOW;
  if (currentButtonState && !switchPressed) {
    switchPressed = true;  // Mark the button as pressed
    isSpeakers = !isSpeakers;  // Toggle the state
    const int currentLedPin = isSpeakers ? ledSpeakers : ledHeadphones;  // Switch between LEDs

    digitalWrite(ledSpeakers, LOW);  // Turn off both LEDs first
    digitalWrite(ledHeadphones, LOW);
    digitalWrite(currentLedPin, HIGH);  // Turn on the selected LED
    sendSwitchMessage();
    delay(1500);
    sendSlidersData(true); // also send sliders data to adjust after the switch
  } else if (!currentButtonState) {
    switchPressed = false;  // Reset the button state when released
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // set up mute buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(muteButtons[i].buttonPin, INPUT_PULLUP);  // Set button pins as input with pull-up
    pinMode(muteButtons[i].ledPin, OUTPUT);           // Set LED pins as output
    digitalWrite(muteButtons[i].ledPin, HIGH);        // Turn off LEDs initially
  }

  // set  up switch
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(ledSpeakers, OUTPUT);
  pinMode(ledHeadphones, OUTPUT);

  Serial.println("Ready. Press the button to toggle the LED.");
  connectToWifi();
  analogReadResolution(10);

  // send initial speaker switch
  setDevice("speakers");
  digitalWrite(ledSpeakers, HIGH); 
  digitalWrite(ledHeadphones, LOW); 
  delay(1000);

  // send initial sliders data
  sendSlidersData(true);
}

void loop() {
  checkMuteButtons();
  sendSlidersData();
  checkSwitch();
}
