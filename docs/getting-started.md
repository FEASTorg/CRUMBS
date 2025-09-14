# Getting Started

## Installation

1. Download the CRUMBS library
2. Place it in your Arduino `libraries` folder
3. Include it: `#include <CRUMBS.h>`

## Basic Concepts

- **Controller**: Initiates communication and sends commands
- **Peripheral**: Receives commands and responds to requests
- **Message**: 27-byte structure with typeID, commandType, data[6], errorFlags

## Quick Examples

### Controller

```cpp
#include <CRUMBS.h>

CRUMBS controller(true);  // Controller mode

void setup() {
    controller.begin();

    CRUMBSMessage msg;
    msg.typeID = 1;
    msg.commandType = 1;
    msg.data[0] = 25.5;
    msg.errorFlags = 0;

    controller.sendMessage(msg, 0x08);  // Send to address 0x08
}
```

### Peripheral

```cpp
#include <CRUMBS.h>

CRUMBS peripheral(false, 0x08);  // Peripheral at address 0x08

void onMessage(CRUMBSMessage &msg) {
    // Handle received message
}

void onRequest() {
    // Send response data
    CRUMBSMessage response;
    response.data[0] = 42.0;

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t size = peripheral.encodeMessage(response, buffer, sizeof(buffer));
    Wire.write(buffer, size);
}

void setup() {
    peripheral.begin();
    peripheral.onReceive(onMessage);
    peripheral.onRequest(onRequest);
}
```

## Hardware Setup

1. Connect I2C (SDA/SCL lines)
2. Add 4.7kΩ pull-up resistors on SDA and SCL
3. Assign unique addresses to peripherals (0x08-0x77)

## Common Patterns

### Send Command

```cpp
CRUMBSMessage cmd;
cmd.typeID = 1;           // Sensor module
cmd.commandType = 1;      // Set parameters
cmd.data[0] = 5.0;        // Sample rate
controller.sendMessage(cmd, deviceAddress);
```

### Request Data

```cpp
Wire.requestFrom(deviceAddress, CRUMBS_MESSAGE_SIZE);
delay(50);
CRUMBSMessage response;
controller.receiveMessage(response);
```

### Error Handling

```cpp
if (message.errorFlags != 0) {
    Serial.print("Error: ");
    Serial.println(message.errorFlags);
}
```

## Debug & Troubleshooting

Enable debug output:

```cpp
#define CRUMBS_DEBUG
#include <CRUMBS.h>
```

Common issues:

- **No communication**: Check wiring, pull-ups, addresses
- **Garbled data**: Verify clock speed (100kHz), timing
- **Address conflicts**: Use I2C scanner, avoid 0x00-0x07, 0x78-0x7F
