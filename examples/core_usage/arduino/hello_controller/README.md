# Hello Controller

**Pair with hello_peripheral** - Upload peripheral first, then this.

## What it does

- Sends messages with 's' key
- Requests data with 'r' key
- Shows simple command/response pattern

## Upload & Test

1. Wire Arduino to peripheral (SDA/SCL + GND)
2. Upload this sketch
3. Open Serial Monitor (115200 baud)
4. Type 's' to send message
5. Type 'r' to request data

## Key Concepts

- `crumbs_arduino_init_controller()` - Setup controller
- `crumbs_controller_send()` - Send command
- Two-step query: SET_REPLY + read

**Next:** See [basic_controller](../basic_controller/) for multiple commands
