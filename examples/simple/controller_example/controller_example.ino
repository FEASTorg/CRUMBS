/**
 * @file crumbs_controller_test.ino
 * @brief CRUMBS Controller example sketch to send messages to a CRUMBS Slice.
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>

// Instantiate CRUMBS as Controller, set to true for Controller mode
CRUMBS crumbsController(true); // Controller mode

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

    crumbsController.begin(); /**< Initialize CRUMBS communication */

    Serial.println(F("Controller ready. Enter messages in the format:"));
    Serial.println(F("address,typeID,commandType,data0,data1,data2,data3,data4,data5,errorFlags"));
    Serial.println(F("Example Serial Commands:"));
    Serial.println(F("   8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,0")); // Example: address 8, typeID 1, commandType 1, data..., errorFlags 0}
}

// Main loop that listens for serial input, parses commands, and sends CRUMBSMessages to the specified Slice.
void loop()
{
    // Listen for serial input to send commands or request data
    handleSerialInput();
}
