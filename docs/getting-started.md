# Getting Started

## Installation

1. Download the CRUMBS library
2. Place in Arduino `libraries` folder
3. Include: `#include <CRUMBS.h>`

## Basic Usage

### Controller

```cpp
#include <CRUMBS.h>

CRUMBS controller(true);

void setup() {
    controller.begin();

    CRUMBSMessage msg;
    msg.typeID = 1;
    msg.commandType = 1;
    msg.data[0] = 25.5;
    msg.errorFlags = 0;

    controller.sendMessage(msg, 0x08);
}
```

### Peripheral

```cpp
#include <CRUMBS.h>

CRUMBS peripheral(false, 0x08);

void onMessage(CRUMBSMessage &msg) {
    // Process received message
}

void onRequest() {
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

- Connect I2C (SDA/SCL) with 4.7kΩ pull-ups
- Use addresses 0x08-0x77 for peripherals
- Set I2C clock to 100kHz (default)

## Debug & Troubleshooting

Enable debug: `#define CRUMBS_DEBUG` before include

**Note**: Only one CRUMBS instance allowed per Arduino (singleton pattern)

Common issues:

- **No response**: Check wiring, addresses, pull-ups
- **Data corruption**: Verify timing, use delays between operations
- **Address conflicts**: Use I2C scanner to verify addresses
