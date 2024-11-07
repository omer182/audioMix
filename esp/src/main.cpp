#include <WiFi.h>
#include <WiFiUdp.h>
#include "configuration.h"

#define CHANGE_THRESHOLD 3
const int MUTE_THRESHOLD = 25;

WiFiUDP Udp;
IPAddress destIp;

struct MuteButton {
  String name;
  int buttonPin;
  int ledPin;
  bool isPressed;
  bool isMuted;
};

// MUTE
const int NUM_BUTTONS = 2;
MuteButton muteButtons[NUM_BUTTONS] = {
  {"master", 14, 12, false, false},
  {"mic", 14, 12, false, false}
};

// SLIDERS
const int NUM_SLIDERS = 2;
byte pins[NUM_SLIDERS] = {34,35};
uint previousReadings[NUM_SLIDERS];

// SWITCH
const int switchPin = 5;
const int ledSpeakers = 19;
const int ledHeadphones = 18;
bool isSpeakers = true;
bool switchPressed = false; 

// WiFi connection and initialization
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

void sendUDPMessage(const String &message, int port = UDP_PORT) {
    Udp.beginPacket(destIp, port);
    Udp.print(message);
    Udp.endPacket();
    Serial.println("Sent UDP message: " + message);
}

// Send switch message
void sendMuteMessage(String device, bool isMuted) {
  // Prepare the message
  String message = isMuted ? "mute " : "unmute ";
  message += device;
  sendUDPMessage(message);
}

// Set the active audio device
void setDevice(String device) {
  sendUDPMessage("set " + device);
}

// Switch audio deviced
void sendSwitchMessage() {
  sendUDPMessage("switch");
}

// Send sliders data over UDP to Deej App
void sendSlidersData(bool forceData = false) {
  uint readings[NUM_SLIDERS];
  bool dataChanged = false;

  for (int i = 0; i < NUM_SLIDERS; i++) {
      readings[i] = analogRead(pins[i]);
      float percentChange = map(abs((float)readings[i] - previousReadings[i]), 0, 1023, 0, 100);
      if (percentChange >= CHANGE_THRESHOLD) {
          dataChanged = true;
          previousReadings[i] = readings[i]; 
      }
  }


  if (dataChanged || forceData) {
    String builtString = String();
    for (int i = 0; i < NUM_SLIDERS; i++) {
        builtString += String((int)readings[i]);

        if (i < NUM_SLIDERS - 1) {
            builtString += String("|");
        }
    }

    sendUDPMessage(builtString, DEEJ_PORT);

    int activeSliderIndex = isSpeakers ? 0 : 1;
    String activeOutput = isSpeakers ? "speakers" : "headphones";

    if (readings[activeSliderIndex] < 20 && !muteButtons[0].isMuted) { 
        sendMuteMessage(activeOutput, true);
        muteButtons[0].isMuted = true;
        digitalWrite(muteButtons[0].ledPin, LOW); // Turn LED on
    } else if (readings[activeSliderIndex] > 0 && muteButtons[0].isMuted) {
        sendMuteMessage(activeOutput, false);
        muteButtons[0].isMuted = false;
        digitalWrite(muteButtons[0].ledPin, HIGH); // Turn LED off
    }
  }
  delay(10);
}

// Check mute button states and send message if changed
void checkMuteButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentButtonState = digitalRead(muteButtons[i].buttonPin) == LOW;

    if (currentButtonState && !muteButtons[i].isPressed) {
      muteButtons[i].isPressed = true;  
      muteButtons[i].isMuted = !muteButtons[i].isMuted; 
      digitalWrite(muteButtons[i].ledPin, muteButtons[i].isMuted ? LOW : HIGH);  

      sendMuteMessage(muteButtons[i].name, muteButtons[i].isMuted);
    } else if (!currentButtonState) {
      muteButtons[i].isPressed = false;
    }
  }
}

// Check and toggle switch state
void checkSwitch() {
  bool currentSwitchState = digitalRead(switchPin) == LOW;
  if (currentSwitchState && !switchPressed) {
    switchPressed = true;  
    isSpeakers = !isSpeakers;  
    const int currentLedPin = isSpeakers ? ledSpeakers : ledHeadphones; 

    digitalWrite(ledSpeakers, LOW); 
    digitalWrite(ledHeadphones, LOW);
    digitalWrite(currentLedPin, HIGH); 
    sendSwitchMessage();
    sendSlidersData(true);
  } else if (!currentSwitchState) {
    switchPressed = false;  
  }
} 

void setup() {
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

  // Initialize device
  setDevice("speakers");
  digitalWrite(ledSpeakers, HIGH);
  delay(1000);
  sendSlidersData(true);
}

void loop() {
  checkMuteButtons();
  sendSlidersData();
  checkSwitch();
}
