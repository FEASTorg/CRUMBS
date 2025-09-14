# API Reference

## CRUMBS Class

### Constructor

```cpp
CRUMBS(bool isController = false, uint8_t address = 0);
```

- `isController`: `true` for controller, `false` for peripheral
- `address`: I2C address for peripheral mode (ignored for controller)

### Methods

```cpp
void begin();                                           // Initialize I2C
void sendMessage(const CRUMBSMessage &msg, uint8_t addr);  // Send message
void receiveMessage(CRUMBSMessage &msg);                // Receive message
void onReceive(void (*callback)(CRUMBSMessage &));      // Set receive callback
void onRequest(void (*callback)());                     // Set request callback
uint8_t getAddress() const;                             // Get device address

// Encoding/Decoding
size_t encodeMessage(const CRUMBSMessage &msg, uint8_t *buffer, size_t size);
bool decodeMessage(const uint8_t *buffer, size_t size, CRUMBSMessage &msg);
```

## CRUMBSMessage Structure

```cpp
struct CRUMBSMessage {
    uint8_t sliceAddress;  // Target identifier (not serialized)
    uint8_t typeID;        // Module type (sensor=1, motor=2, etc.)
    uint8_t commandType;   // Command (read=0, set=1, etc.)
    float data[6];         // Payload data (6 floats = 24 bytes)
    uint8_t errorFlags;    // Error/status flags
};
```

**Serialized size**: 27 bytes (typeID + commandType + data + errorFlags)

## Constants

```cpp
#define CRUMBS_MESSAGE_SIZE 27     // Serialized message size
#define TWI_CLOCK_FREQ 100000      // I2C clock frequency (100kHz)
```

## Debug Macros

```cpp
#define CRUMBS_DEBUG               // Enable debug output
CRUMBS_DEBUG_PRINT(...)           // Print with "CRUMBS: " prefix
CRUMBS_DEBUG_PRINTLN(...)         // Print line with "CRUMBS: " prefix
```

## Error Codes (sendMessage)

- `0`: Success
- `1`: Data too long for transmit buffer
- `2`: NACK on address (device not found)
- `3`: NACK on data transmission
- `4`: Other I2C error
