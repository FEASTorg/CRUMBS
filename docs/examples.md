# Examples

## Controller Example

Demonstrates controller setup with serial interface for sending commands and requesting data.

### Usage

1. Upload to Arduino
2. Open Serial Monitor (115200 baud)
3. Send commands: `address,typeID,commandType,data0,data1,data2,data3,data4,data5,errorFlags`
4. Request data: `request=address`

### Example Commands

```cpp
8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,0
```

Sends to address 8: typeID=1, commandType=1, data values, no errors.

```cpp
request=8
```

Requests data from address 8.

### Key Code Patterns

#### Serial Parsing

```cpp
bool parseSerialInput(const String &input, uint8_t &targetAddress, CRUMBSMessage &message) {
    // Tokenize comma-separated values
    // Convert strings to appropriate data types
    // Populate message structure
}
```

#### Data Requests

```cpp
Wire.requestFrom(address, CRUMBS_MESSAGE_SIZE);
delay(50);  // Wait for response
CRUMBSMessage response;
controller.receiveMessage(response);
```

## Peripheral Example

Demonstrates peripheral setup with message handling and data response.

### Key Code Patterns

#### Message Handler

```cpp
void handleMessage(CRUMBSMessage &message) {
    switch (message.commandType) {
        case 0: // Data request
            break;
        case 1: // Set parameters
            break;
        default:
            // Unknown command
            break;
    }
}
```

#### Request Handler

```cpp
void handleRequest() {
    CRUMBSMessage response;
    response.typeID = 1;
    response.data[0] = 42.0f;  // Example data

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t size = peripheral.encodeMessage(response, buffer, sizeof(buffer));
    Wire.write(buffer, size);
}
```

#### Setup

```cpp
void setup() {
    peripheral.begin();
    peripheral.onReceive(handleMessage);
    peripheral.onRequest(handleRequest);
}
```

## Common Patterns

### Multiple Device Communication

```cpp
uint8_t addresses[] = {0x08, 0x09, 0x0A};
for (int i = 0; i < 3; i++) {
    controller.sendMessage(msg, addresses[i]);
    delay(10);
}
```

### Command Processing

```cpp
switch (message.typeID) {
    case 1: // Sensor
        handleSensorCommand(message);
        break;
    case 2: // Motor
        handleMotorCommand(message);
        break;
}
```

### Error Validation

```cpp
if (message.errorFlags != 0) {
    Serial.print("Error: ");
    Serial.println(message.errorFlags);
    return;
}
```

### I2C Device Scanning

```cpp
void scanI2CDevices() {
    Serial.println("Scanning I2C bus...");
    int count = 0;
    for (uint8_t addr = 8; addr < 120; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device found at 0x");
            Serial.println(addr, HEX);
            count++;
        }
    }
}
```

## Best Practices from Examples

1. **Use callbacks** for event-driven peripheral handling
2. **Add delays** between I2C operations (10ms typical)
3. **Validate messages** before processing
4. **Handle errors gracefully** with appropriate responses
5. **Use debug output** during development (`#define CRUMBS_DEBUG`)
