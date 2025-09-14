# Examples

## Controller Example

Send commands via serial interface and request data from peripherals.

### Usage

1. Upload to Arduino
2. Open Serial Monitor (115200 baud)
3. Send: `address,typeID,commandType,data0,data1,data2,data3,data4,data5,errorFlags`
4. Request: `request=address`

### Example Commands

```cpp
8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,0     // Send to address 8
request=8                              // Request data from address 8
```

## Peripheral Example

Respond to controller commands and data requests.

### Key Code Patterns

```cpp
void handleMessage(CRUMBSMessage &message) {
    switch (message.commandType) {
        case 0: // Data request
            break;
        case 1: // Set parameters
            break;
    }
}

void handleRequest() {
    CRUMBSMessage response;
    response.data[0] = 42.0f;

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t size = peripheral.encodeMessage(response, buffer, sizeof(buffer));
    Wire.write(buffer, size);
}

void setup() {
    peripheral.begin();
    peripheral.onReceive(handleMessage);
    peripheral.onRequest(handleRequest);
}
```

## Common Patterns

### Multiple Devices

```cpp
uint8_t addresses[] = {0x08, 0x09, 0x0A};
for (int i = 0; i < 3; i++) {
    controller.sendMessage(msg, addresses[i]);
    delay(10);
}
```

### Data Requests

```cpp
Wire.requestFrom(address, CRUMBS_MESSAGE_SIZE);
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

### I2C Scanning

```cpp
for (uint8_t addr = 8; addr < 120; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
        Serial.print("Found device at 0x");
        Serial.println(addr, HEX);
    }
}
```
