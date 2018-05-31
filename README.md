# AudioPort Device

## Description

This is a REDHAWK project that contains the source and build script for an AudioPort Device that implements JTNC Interfaces. It requires the `alsa` library and the following IDL Projects to be installed:

* JTRS (jtrsInterfaces)
* Packet (packetInterfaces)
* DevMsgCtl (devmsgcltInterfaces)
* Audio (audioInterfaces)

## Installation Instructions

To build from source, run the `build.sh` script found at the top level directory. To install to $SDRROOT, run `build.sh install`. 
Note: root privileges (`sudo`) may be required to install.

## Properties Notes

- **ptt_device** is the input event device that should represent a keyboard. It has `/dev/input/event2` as its default value. The Left Ctrl key is set to be the PTT Button. You might need to add the user to the input group to be able to access this device:
```
$ usermod -a -G input <userName>
```
- **capture_card** is the name of the alsa device set to stream capture audio samples. You can list the Capture Devices with the command: 
```
	$ arecord -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
```
- **playback_card** is the name of the alsa device set to stream playback audio samples. You can list the Playback Devices with the command:
```
$ aplay -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
```
- **capture_mixer_control** is the name of alsa mixer control configure by the card driver to adjust volume for the capture device
- **playback_mixer_control** is the name of alsa mixer control configure by the card driver to adjust volume for the playback device

You can list all the alsa mixer control with the commands:

```
$ amixer controls
$ amixer scontrols
$ amixer -Dhw:<card_name> controls
$ amixer -Dhw:<card_name> scontrols
$ amixer -c<card_name> controls
$ amixer -c<card_name> scontrols
```
