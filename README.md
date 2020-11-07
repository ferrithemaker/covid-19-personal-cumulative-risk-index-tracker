# covid-19-personal-accumulative-risk-index-tracker

Inspired by the classic radiation dosimeters that we have seen in many documentaries and films, the idea of ​​the project is to create a wearable device that allow us to measure the accumulated daily risk of exposure to potentially dangerous environments in relation to the transmission of covid-19.

As we know, one of the main transmission factors of covid-19 is the proximity to other people who may be infected by the virus and therefore, one of the objectives to improve prevention is to be in environments where there is a safe distance between people and control the flow of movements around you.

The covid-19 personal cumulative risk index tracker is a wireless device that is configured in promiscuous mode, thus acting as an electromagnetic spectrum analyzer at the frequencies used by wireless devices.

Actually, almost everyone wears a device with a wireless connection on them, and every device with this type of connection, even if it is not connected to any network or in use, regularly emits a series of beacon signals to identify itself in a unique way.

The covid-19 personal cumulative risk index tracker recognizes these signals and their intensity, density and variability, which allow us to know approximately the flow of people around you and based on that information, plus a real-time data of local infection cases, the device creates a reliable daily cumulative index of exposure risk, like the radiation dosimeters.

The central point of the project is not the hardware itself, quite simple indeed, but the algorithm that calculates this index based on the electromagnetic frequency readings. The idea is not simply to detect the number of devices around you, but to analyze the variations of the signals intensity,  and above all, analyze the flow of devices around you. 

These data allow us to calculate what kind of environment we are in, generating various types of profiles:

- Very dense environment and low variability: (moderate / high risk)
- Very dense environment and high variability: (high / very high risk)
- Low dense environment and low variability: (low risk)
- Low dense environment and high variability: (moderate risk)

Based on these profiles we can calculate a daily accumulated index that will tell us not only if at a specific moment we are in a potentially dangerous environment (since this is generally quite obvious), but it will also show us an accumulated value throughout the day, which will give us a global assessment of our daily behaviour (low risk / medium risk / high risk) and will allow us to be aware in order to adjust it and minimize further risks.

This index can be weighted with real time data of local infections rates from online oficial sources like covid-19 API to create a more reliable index.

The device consists of a wireless connectivity microcontroller with the ability to work in promiscuous mode (in our case an ESP32) and a battery (a small power bank or a LiPo battery). To display the information you can use an OLED screen (this is the case in the demo example) or simply a color coding system using LEDs to indicate the different levels of accumulated risk.

## Build and Flashing

First of all, install PlatformIO with your favourite IDE (i.e. VSCode). Follow [this](https://platformio.org/platformio-ide) instructions.

Clone the repo or download it and then:

Connect wristband via USB with the supplied daughter board. In Windows 10, drivers are installed automatically. I guess with other OS will be automatically installed too.

After plugging wristband, please select `env` variant **esp32dev** on VisualCode and build it, `PlatformIO icon->env:esp32dev->Build`:

![Upload Button](https://raw.githubusercontent.com/hpsaturn/TTGO-T-Wristband/av/pio_config_envs/resources/vcode_env_usb_build.jpg)

then, in the same menu, plase select `upload`.

or build and upload it with the `pio` command line:

```bash
pio run -e esp32dev --target upload
```

PlatformIO will build and upload the binaries to the TTGO T-Wristband via USB.



