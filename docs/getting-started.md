# Getting Started

## Installation

1. Download or clone the CRUMBS repository.
2. Place the folder in your Arduino `libraries` directory (or use the platform's library manager).
3. Include the headers your target requires:

- Arduino sketches: `#include <crumbs_arduino.h>` (this pulls in `crumbs.h`)
-- Linux projects: `#include "crumbs.h"` and `#include "crumbs_linux.h"` as needed

1. No external dependencies required.

## Basic Usage

### Controller (Arduino — C API)

```cpp
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void setup() {
    // Initialize the Arduino HAL for controller mode
    crumbs_arduino_init_controller(&ctx);

    crumbs_message_t m = {};
    m.type_id = 1;
    m.command_type = 1;
    m.data[0] = 25.5f;

    // Send to target address 0x08 using the Wire HAL via crumbs_controller_send
    crumbs_controller_send(&ctx, 0x08, &m, crumbs_arduino_wire_write, NULL);
}
```

### Peripheral (Arduino — C API)

```cpp
#include <crumbs_arduino.h>

crumbs_context_t ctx;

// Message receive callback
void on_message(crumbs_context_t *ctx, const crumbs_message_t *m) {
    // process the message (peek m->data[] etc.)
}

// Request callback — fill reply message
void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    reply->type_id = 1;
    reply->command_type = 0;
    reply->data[0] = 42.0f;
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
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
