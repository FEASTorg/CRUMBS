# CRUMBS Docs

This library, named CRUMBS, provides a streamlined framework for I2C communication on Arduino platforms. It is centered around a fixed-size message structure and a set of functions that abstract away the complexities of sending and receiving data over I2C.

**Key Components:**

- **Message Structure:**  
  The library defines a fixed-size message (28 bytes) that includes identifiers (such as a slice or unit ID, type ID, and command type), a payload of six floating-point values, and an error flag byte. This design ensures a consistent message format for communication between devices.

- **I2C Communication Modes:**  
  CRUMBS supports two modes of operation:

  - **Controller Mode:** In this mode, the device initiates communication by sending messages to target devices (or "slices").
  - **Peripheral (Slice) Mode:** Here, the device listens on a specified I2C address, receives messages from the controller, and can send responses back when requested.

- **Core Functionalities:**

  - **Initialization:** The library sets up I2C communication and registers event handlers for receiving data and handling data requests.
  - **Message Encoding/Decoding:** It includes functions to serialize (encode) a CRUMBSMessage into a byte buffer for transmission and to reconstruct (decode) a message from a received buffer.
  - **Event Callbacks:** Developers can register custom callback functions to handle incoming messages or to process data requests, allowing flexible responses to different commands.

- **Debugging Support:**  
  When debugging is enabled, the library prints detailed status messages to the serial monitor. These messages provide insights into the internal processing (such as message encoding, decoding, and transmission statuses) which can help during development and troubleshooting.

- **Examples:**  
  Two example sketches are provided: one demonstrating how a controller device sends commands and another showing how a peripheral device (slice) receives messages and responds. These examples illustrate how to set up the library in both modes and how to use the provided functions to achieve robust I2C communication.
