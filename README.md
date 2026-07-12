# Vitoconnect for ESPHome

Integration of Viessmann Optolink protocols [GWG], KW and P300 into [ESPHome]. This component is named after the cloud reading unit of Viessmann: Vitoconnect.

## Usage

For usage, simply add the following to your config file. Example: V200WO1.
Address, length and post processing can be retrieved from <https://github.com/openv/openv/wiki/Adressen>. Length of 2 bytes is by default interpreted as int16, 4 bytes as uint32. Both values are then converted to float.

```yaml
external_components:
  - source: github://MichaelSp/esphome_vitoconnect

esphome:
  name: viessmann-reader
  friendly_name: vitoconnect

esp32:
  board: esp32doit-devkit-v1

# esp8266:
  # board: nodemcuv2

uart:
  - id: uart_vitoconnect
    rx_pin: GPIO16              # ESP32 RX2
    tx_pin: GPIO17              # ESP32 TX2
    #rx_pin: GPIO03             # ESP8266 RX
    #tx_pin: GPIO01             # ESP8266 TX
    baud_rate: 4800
    data_bits: 8
    parity: EVEN
    stop_bits: 2
    # debug:
    #   direction: BOTH
    #   dummy_receiver: false
    #   after:
    #     delimiter: [0x06]
    #   sequence:
    #     - lambda: UARTDebug::log_hex(direction, bytes, ':');

vitoconnect:
  uart_id: uart_vitoconnect
  protocol: P300                # set protocol to GWG, KW or P300
  update_interval: 30s          # default: 60s
  queue_size: 40                # max pending Optolink requests, default: 20, range: 1-100

sensor:
  - platform: vitoconnect
    name: "Außentemperatur"
    address: 0x01C1             # vitoconnect: address of the value
    length: 2                   # vitoconnect: length of the value
    unit_of_measurement: "°C"
    accuracy_decimals: 1
    filters:
      - multiply: 0.1
  - platform: vitoconnect
    name: "Betriebsstunden Verdichter"
    address: 0x0580
    length: 4
    check_once: true            # read once at startup, then skip regular polling
    unit_of_measurement: "h"
    accuracy_decimals: 1
    filters:
      # - multiply: 0.000277777777777778 # use multiply filter for ESP8266
      - lambda: return x / 3600.0;
  - platform: vitoconnect
    name: "Brennerleistung"
    address: 0xA38F
    length: 1
    unit_of_measurement: "%"
    accuracy_decimals: 1
    filters:
      - multiply: 0.5
binary_sensor:
  - platform: vitoconnect
    name: "Status Verdichter"
    address: 0x0400
    check_once: false           # default: false
```

### `queue_size`

`queue_size` controls how many Optolink read/write requests can wait in the internal queue at once.
It is useful when one update cycle contains more datapoints than the default queue can hold, or when many datapoints use `check_once: true` during startup.

Default is `20`. Valid range is `1` to `100`.

`queue_size` does not make the boiler answer faster and does not change `update_interval`. If the queue is too small, extra datapoints may fail to queue and can be skipped for that cycle.

### `check_once`

`check_once` is available on `sensor`, `binary_sensor` and `text_sensor`.

```yaml
sensor:
  - platform: vitoconnect
    name: "Betriebsstunden Verdichter"
    address: 0x0580
    length: 4
    check_once: true
```

When `check_once: true`, the datapoint is read during startup only. Regular update cycles skip it after the initial read.

Use it for values that rarely change, such as static configuration values or counters you only need once after boot. Leave it unset, or set it to `false`, for values that should update every `update_interval`.

Tested with OptoLink ESP32 adapter from here:
<https://github.com/openv/openv/wiki/Bauanleitung-ESP32-Adafruit-Feather-Huzzah32-and-Proto-Wing>

## ESP32-C3 Super Mini build

Hardware notes, wiring images, Home Assistant screenshots and 3D print files for an ESP32-C3 Super Mini Optolink build are collected in [docs/README.md](docs/README.md).

## Credits

Built based on [VitoWifi] by [Bert Melis] and inspired by [vitowifi_esphome] by [Philipp Hack].

## License

MIT License
Copyright (c) 2025 Philipp Danner

## Support Development

### Paypal

[![](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=848P2G8EA68PJ)

[ESPHome]: https://esphome.io/
[VitoWifi]: https://github.com/bertmelis/VitoWifi
[vitowifi_esphome]: https://github.com/phha/vitowifi_esphome
[Bert Melis]: https://github.com/bertmelis
[Philipp Hack]: https://github.com/phha
[GWG]: README_GWG.md
