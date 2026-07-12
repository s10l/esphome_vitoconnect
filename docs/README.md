# ESP32-C3 Super Mini Optolink Build

This folder contains the hardware documentation for an ESP32-C3 Super Mini based Optolink adapter running `esphome_vitoconnect`.

Repository: <https://github.com/MichaelSp/esphome_vitoconnect>

## Overview

The build combines:

- ESP32-C3 Super Mini
- Viessmann Optolink optical interface
- ESPHome firmware using the `vitoconnect` external component
- Home Assistant integration through ESPHome
- 3D printed forms for holding the ESP32-C3 board and the Viessmann V optical connector

The matching ESPHome package entry point is [vitoconnect.yaml](vitoconnect.yaml).

## Images

- [schaltplan.png](schaltplan.png): wiring schematic
- [ESP32-C3-Super-Mini-Pinout.png](ESP32-C3-Super-Mini-Pinout.png): ESP32-C3 Super Mini pinout
- [esphome.png](esphome.png): ESPHome view
- [home-assistant-view.png](home-assistant-view.png): Home Assistant view
- [img/](img/): build photos

## 3D Print Files

The printable forms are in [forms/](forms/):

- [ESP32-C3+Super+Mini-Viessmann-V.stl](forms/ESP32-C3+Super+Mini-Viessmann-V.stl)
- [ESP32-C3+Super+Mini-Viessmann-V.blend](forms/ESP32-C3+Super+Mini-Viessmann-V.blend)

## Firmware

For a standalone example, see:

- [../example-esp32.yaml](../example-esp32.yaml)
- [../example-esp8266.yaml](../example-esp8266.yaml)

For the documented ESP32-C3 package setup, start with:

```yaml
substitutions:
  name: optolink

packages:
  - !include preset/basic.yaml
  - !include esp32-c3/board.yaml
  - !include esp32-c3/uart.yaml
  - !include esp32-c3/vitoconnect.yaml
  - !include esp32-c3/vitocal-300g.yaml
```

## OpenV Wiki Placement

This build belongs in the OpenV wiki as:

- a short software link under `Microcontroller -> esphome_vitoconnect`
- a hardware build page near the Optolink `Bauanleitung` pages, especially the ESP32 S3/C6/C3 and 3D housing pages

The wiki page should stay short and link back to this folder as the maintained source for files and images.
