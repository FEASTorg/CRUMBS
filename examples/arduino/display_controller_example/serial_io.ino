#include <Arduino.h>
#include "crumbs.h"

extern void sendCrumbs(uint8_t address, crumbs_message_t &message);
extern void requestCrumbs(uint8_t address);

// Accept commands on serial in the following formats (space-separated):
//   SEND <addr> <type_id> <command> <byte0> <byte1> ...
//   REQUEST <addr>
// Bytes can be decimal or hex (e.g., 0x12).

void handleSerial()
{
    if (!Serial.available())
        return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
        return;

    // Split tokens by whitespace
    const size_t maxTokens = 32;
    String toks[maxTokens];
    size_t tokenCount = 0;
    int idx = 0;
    while (tokenCount < maxTokens && idx < (int)line.length())
    {
        while (idx < (int)line.length() && isspace(line[idx]))
            idx++;
        if (idx >= (int)line.length())
            break;
        int next = line.indexOf(' ', idx);
        if (next < 0)
            next = line.length();
        toks[tokenCount++] = line.substring(idx, next);
        idx = next + 1;
    }

    if (tokenCount == 0)
        return;

    toks[0].toUpperCase();
    if (toks[0] == "REQUEST" && tokenCount >= 2)
    {
        uint8_t addr = (uint8_t)strtoul(toks[1].c_str(), NULL, 0);
        requestCrumbs(addr);
        return;
    }

    if (toks[0] == "SEND" && tokenCount >= 4)
    {
        uint8_t addr = (uint8_t)strtoul(toks[1].c_str(), NULL, 0);
        crumbs_message_t m = {};
        m.type_id = (uint8_t)strtoul(toks[2].c_str(), NULL, 0);
        m.command_type = (uint8_t)strtoul(toks[3].c_str(), NULL, 0);
        m.data_len = 0;
        for (size_t i = 0; i < CRUMBS_MAX_PAYLOAD && (4 + i) < tokenCount; ++i)
        {
            m.data[i] = (uint8_t)strtoul(toks[4 + i].c_str(), NULL, 0);
            m.data_len++;
        }
        sendCrumbs(addr, m);
        return;
    }

    Serial.println(F("Invalid command. Use: REQUEST <addr> or SEND <addr> <type> <cmd> <byte0>.."));
}
