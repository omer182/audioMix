const dgram = require('dgram');
const cmd = require('node-cmd');
const server = dgram.createSocket('udp4');

const PORT = '16991';
const HOST = '0.0.0.0';

const DEVICES = {
  Headphones: "{0.0.0.00000000}.{a3541ca9-97e2-4203-8572-6eb8980e70a6}",
  Microphone: "{0.0.1.00000000}.{b876065e-da28-4f14-a041-cac6a38e6e6b}",
  Speakers: "{0.0.0.00000000}.{a3ce8b3f-a28c-4cb8-b22c-af914782737e}",
  Default: "DefaultRenderDevice"
};

const souneVolumeCmd = "c:\\nircmd\\soundvolumeview\\SoundVolumeView.exe";

const nirCmd = 'c:\\nircmd\\nircmd.exe';

const PROCESSES = [
  'master',
  'Spotify'
]

function toggleAudioDevices() {
  const command = `${souneVolumeCmd} /SwitchDefault "${DEVICES.Speakers}" "${DEVICES.Headphones}"`
  cmd.run(command);
  console.log('Toggled audio devices');
}

function toggleMute(deviceName, mute) {
  let targetDevice = deviceName  === 'master' ? DEVICES.Default : DEVICES.Microphone
  let state  = mute ? 'Mute' : 'UnMute'
  let command = `${souneVolumeCmd} /${state} ${targetDevice}`
  cmd.run(command);
  console.log(`${mute ? 'Muted' : 'Unmuted'} ${deviceName}`);
}

function setAudioDevice(deviceName) {
  let command = `${souneVolumeCmd} /SetDefault ${deviceName}`
  cmd.run(command);
}

async function adjustSystemSound(slidersArray) { 
  for (let index = 0; index < slidersArray.length; index++) {
    const deviceName = index === 0 ? "DefaultRenderDevice" : `${PROCESSES[index]}.exe`;
    const command = `${souneVolumeCmd} /SetVolume "${deviceName}" ${slidersArray[index]}`;
    console.log(command);
    cmd.run(command);  
  }
}

server.on('message', (message, rinfo) => {
  console.log(`Server received: ${message} from ${rinfo.address}:${rinfo.port}`);

  const command = message.toString().trim();
  
  if (command.startsWith('sliders')) {
    const slidersArray = command.replace("sliders ", "").split("|").map(Number);
    adjustSystemSound(slidersArray)
  } else if (command.startsWith('switch')) {
    toggleAudioDevices();
  } else if (command.startsWith('mute')) {
    const deviceName = command.split(" ")[1];
    toggleMute(deviceName, true);
  } else if (command.startsWith('unmute')) {
    const deviceName = command.split(" ")[1];
    toggleMute(deviceName, false);
  } else if (command.startsWith('set')) {
    const deviceName = command.split(" ")[1];
    setAudioDevice(deviceName);
  } else {
    console.log('Unknown command');
  }
});

server.on('error', (err) => {
  console.log(`Server error:\n${err.stack}`);
  server.close();
});

server.on('listening', () => {
  const address = server.address();
  console.log(`Server listening on ${address.address}:${address.port}`);
});

server.bind(PORT, HOST);