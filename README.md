# ESP32 Windows Audio Mixer Control via UDP

This project connects an ESP32 to a UDP server on a backend, allowing for remote control of the audio mixer on a Windows machine. The ESP32 can send commands to adjust the system and application volumes, toggle between audio devices, mute/unmute specific devices, and set default devices. The backend server uses Node.js to receive and execute these commands on the Windows machine using the NirCmd and SoundVolumeView tools.

## Features

- Adjust system and application volumes
- Toggle between audio output devices (e.g., speakers and headphones)
- Mute/unmute specific audio devices
- Set the default audio device

## Requirements

1. **ESP32**: Microcontroller for sending UDP commands.
2. **Node.js and `node-cmd`**: Server-side processing.
3. [**SoundVolumeView**](https://www.nirsoft.net/utils/sound_volume_view.html) Tool for controlling audio on Windows.
4. **Network Setup**: ESP32 and server must be on the same network to communicate via UDP.

## Hardware

- **ESP32**: Microcontroller that sends UDP messages based on button inputs and sliders.

## Installation

### 1. ESP32 Setup

The ESP32 should be programmed to send commands in a specific format to the server IP on port `16991`. Each command triggers an action in the Node.js server.

### 2. Backend Setup

```js
npm i
npm start
```
