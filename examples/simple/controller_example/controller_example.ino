/**
 * @file crumbs_controller_test.ino
 * @brief CRUMBS Controller example sketch to send messages to a CRUMBS Slice.
 */

#define CRUMBS_DEBUG

#include <crumbs.h>
#include <crumbs_arduino.h>

#include <Wire.h>

// Instantiate CRUMBS as Controller, set to true for Controller mode
static crumbs_context_t crumbsController;   // C struct

//  Maximum expected input length for serial commands.
#define MAX_INPUT_LENGTH 60

// Initializes the Controller device, sets up serial communication, and provides usage instructions.
void setup()
{
    Serial.begin(115200); /**< Initialize serial communication at 115200 baud rate */

    while (!Serial)
    {
        delay(10); // Wait for Serial Monitor to open
    }

    crumbs_arduino_init_controller(&crumbsController);
    crumbs_reset_crc_stats(&crumbsController);

    Serial.println(F("Controller: CRC diagnostics reset."));

    Serial.println(F("Controller ready. Enter messages in the format:"));
    Serial.println(F("address,type_id,command_type,data0,data1,data2,data3,data4,data5,data6"));
    Serial.println(F("Example Serial Commands:"));
    Serial.println(F("   8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,3.14")); // Example: address 8, type_id 1, command_type 1, data...
}

// Main loop that listens for serial input, parses commands, and sends crumbs_message_ts to the specified Slice.
void loop()
{
    // Listen for serial input to send commands or request data
    handleSerialInput();
}
