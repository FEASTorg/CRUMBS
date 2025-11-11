# CRC Variant Selection for CRUMBS Protocol (ATmega328P Target)

| Variant        | Flash (bytes) | RAM (bytes) | Speed (μs/kiB) | Suitability                             |
| -------------- | ------------- | ----------- | -------------- | --------------------------------------- |
| `crc8_bit`     | ~80           | 0           | 7800–18000     | Too slow                                |
| `crc8_nibble`  | ~130          | 0           | 5300–7600      | _**Best balance**_                      |
| `crc8_nibblem` | ~130          | 16          | 4900–7200      | Slightly faster, costs RAM              |
| `crc8_byte`    | ~300          | 0           | 900–2200       | Fastest, wasteful on flash-limited MCUs |

**Choice:** `ace_crc::crc8_nibble`  
**Reason:** Optimal tradeoff between flash size, speed, and zero RAM use for AVR-class MCUs (e.g., ATmega328P).  
**CRC field:** 1 byte, calculated over first 31 bytes of message payload.
