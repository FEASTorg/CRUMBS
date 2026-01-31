# Future Ideas

- Maybe in the future we could have "classes" of devices (similar to USB device classes) that have pre-defined typeIDs and opcodes to make it easier for others to develop compatible hardware/firmware, but this is not a requirement of the library itself.

- reserved opcodes
  - CAPABILITIES bitmap (multi-frame support, low-power modes, streaming)
  - CONFIG_GET/CONFIG_SET (device configuration)
  - STATS (performance/diagnostic counters)
  - Other protocol extensions as needs emerge

- use the 32nd byte for the family type if its a registered family? like 0x01 for reference family, 0x02 for slice family, etc. then for custom families it can be 0x00 or 0xFF or something. maybe 0x00 for reference and 0xFF for custom, and 0x01-0xFE for (official) registered families? This would require more abstraction in the controller code but would allow it to handle multiple families on the same bus

- have clear option for linear O(1) lookup vs binary O(log n) lookup for handlers
