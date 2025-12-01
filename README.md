# HTZSafe Custom ESPHome Component

A custom ESPHome component for integrating wireless motion sensor receivers (commonly sold as driveway alarm systems) with Home Assistant.

Fair warning, I only have one receiver and two sensors to test this with.

## Overview

This component allows you to monitor wireless motion sensors through an ESP32 board connected to the receiver unit of a wireless driveway alarm system. Each motion sensor triggers a binary sensor in Home Assistant that automatically resets after 5 seconds.

## Hardware Requirements

### Components

1. **ESP32 D1 Mini** (or similar ESP32 board)

   | Top View | Bottom View |
   |----------|-------------|
   | ![ESP32 D1 Mini Top](top.png) | ![ESP32 D1 Mini Bottom](bottom.png) |

2. **Wireless Driveway Alarm Receiver** - [Driveway Alarm- 1/2 Mile Long Range](https://www.amazon.com/dp/B08NYH9JL4) (non-affiliate link)
   - Description: *"Driveway Alarm- 1/2 Mile Long Range Wireless Driveway Alarm Outdoor Weather Resistant Motion Sensor&Detector-DIY Security Alert-Monitor&Protect Outdoor/Indoor Property"*

### Wiring

Connect the receiver board to the ESP32 as follows:

| Receiver Pin | ESP32 Pin |
|--------------|-----------|
| TX           | GPIO16 (RX) |
| +5V          | VCC       |
| GND          | GND       |

**Receiver Board Pin Layout:**

![HTZSafe Receiver Pins](htzsafe_receiver_top.png)

**Note:** The receiver board has clearly marked pins as shown in the image above.

## Installation

### 1. Copy Component Files

Copy the `components/htzsafe` folder to your ESPHome configuration directory:

```
your-esphome-config/
├── components/
│   └── htzsafe/
│       ├── __init__.py
│       ├── htzsafe.cpp
│       └── htzsafe.h
└── your-device.yaml
```

### 2. Create ESPHome Configuration

Use the `owl-sensor.yaml` as a reference.

### 3. Identify Your Sensors

Before you can use the sensors, you need to determine their unique identifiers:

1. Flash the ESP32 with a basic configuration (without any devices listed)
2. Power on each motion sensor one at a time
3. Watch the ESPHome logs for messages like:

```
[20:15:06.905][W][htzsafe:085]: Unknown device identifier: 0x926a
```

4. Copy the identifier (e.g., `0x926a`) and add it to your configuration:

```yaml
htzsafe:
  id: motion_receiver
  uart_id: htzsafe_uart
  devices:
    - id: driveway_sensor
      name: "Driveway"
      identifier: 0x926a  # Your actual identifier
```

### 4. Flash and Deploy

```bash
esphome run your-device.yaml
```

## Configuration Details

### Power Optimization

Due to power consumption issues, the following optimizations are included, YMMV:

- **Brownout detector disabled** - Prevents resets during power surges
- **CPU frequency reduced to 80MHz** - Lowers power consumption
- **WiFi TX power reduced to 8.5dB** - Reduces startup power surge

⚠️ **Important:** These optimizations require a strong WiFi signal. Place the ESP32 in an area with good WiFi reception to avoid connectivity issues.

### HTZSafe Component Configuration

```yaml
htzsafe:
  id: motion_receiver        # Component ID (required)
  uart_id: htzsafe_uart      # UART component ID (required)
  devices:                   # List of sensors (required)
    - id: sensor_id          # Unique ID for Home Assistant entity
      name: "Sensor Name"    # Friendly name
      identifier: 0xXXXX     # 2-byte hex identifier from logs
```

### Protocol Details

The receiver uses a simple serial protocol:

- **Baud rate:** 9600
- **Header:** `0xebaf05` (3 bytes)
- **Identifier:** 2-byte unique ID for each sensor
- **Total message:** 5 bytes

When a sensor is triggered:
1. The binary sensor turns ON immediately
2. After 5 seconds, it automatically turns OFF
3. This timeout is handled by the component itself

## Troubleshooting

### Device Not Responding

- Check wiring connections
- Verify WiFi signal strength
- Check ESPHome logs for error messages

### Unknown Device Identifier

- This is expected when setting up new sensors
- Copy the identifier from the logs
- Add it to your configuration with a descriptive name

### Frequent Reboots

- Ensure you have good WiFi signal strength
- Consider using a better power supply
- The brownout detector is already disabled

### Sensors Not Triggering

- Verify the sensor batteries are fresh
- Check the receiver LED lights up when sensor is triggered
- Confirm the identifier in your configuration matches the logs

## Example Use Cases

- Monitor driveway or gate entrances
- Mailbox notifications
- Perimeter security
- Path/walkway detection
- Wildlife detection

## Home Assistant Integration

Once configured, the sensors will automatically appear in Home Assistant as binary sensors. You can use them in automations like:

```yaml
automation:
  - alias: "Driveway Alert"
    trigger:
      - platform: state
        entity_id: binary_sensor.driveway
        to: "on"
    action:
      - service: notify.mobile_app
        data:
          message: "Motion detected in driveway"
```

## License

This component is provided as-is for personal use.

## Credits

Component developed for use with commercial wireless driveway alarm systems to integrate with Home Assistant via ESPHome.
