#include <Arduino.h>
#include "crumbs.h"

extern void sendCrumbs(uint8_t address, crumbs_message_t &message);
extern void requestCrumbs(uint8_t address);

// Premade test messages
void sendPremade(uint8_t addr, const char *name)
{
    crumbs_message_t m = {};
    m.type_id = 1;
    m.command_type = 1;

    if (strcmp(name, "MSG4B") == 0)
    {
        m.data_len = 4;
        m.data[0] = 0x12;
        m.data[1] = 0x34;
        m.data[2] = 0x56;
        m.data[3] = 0x78;
    }
    else if (strcmp(name, "MSG8B") == 0)
    {
        m.data_len = 8;
        for (uint8_t i = 0; i < 8; i++)
            m.data[i] = 0x10 + i;
    }
    else if (strcmp(name, "MSG12B") == 0)
    {
        m.data_len = 12;
        for (uint8_t i = 0; i < 12; i++)
            m.data[i] = 0x20 + i;
    }
    else if (strcmp(name, "MSG16B") == 0)
    {
        m.data_len = 16;
        for (uint8_t i = 0; i < 16; i++)
            m.data[i] = 0x30 + i;
    }
    else if (strcmp(name, "MSG27B") == 0)
    {
        m.data_len = 27;
        for (uint8_t i = 0; i < 27; i++)
            m.data[i] = 0x40 + i;
    }
    else if (strcmp(name, "MSG0B") == 0)
    {
        m.data_len = 0;
    }
    else
    {
        Serial.print(F("Unknown premade: "));
        Serial.println(name);
        return;
    }

    Serial.print(F("Sending premade "));
    Serial.print(name);
    Serial.print(F(" ("));
    Serial.print(m.data_len);
    Serial.println(F(" bytes)"));
    sendCrumbs(addr, m);
}

// Accept commands on serial in the following formats (space-separated):
//   SEND <addr> <type_id> <command> <byte0> <byte1> ...
//   SEND <addr> MSG4B|MSG8B|MSG12B|MSG16B|MSG27B|MSG0B
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

    if (toks[0] == "SEND" && tokenCount >= 3)
    {
        uint8_t addr = (uint8_t)strtoul(toks[1].c_str(), NULL, 0);

        // Check for premade message names
        toks[2].toUpperCase();
        if (toks[2].startsWith("MSG"))
        {
            char nameBuf[8];
            toks[2].toCharArray(nameBuf, sizeof(nameBuf));
            sendPremade(addr, nameBuf);
            return;
        }

        // Otherwise parse as: SEND <addr> <type> <cmd> <bytes...>
        if (tokenCount >= 4)
        {
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
    }

    Serial.println(F("Commands:"));
    Serial.println(F("  SEND <addr> <type> <cmd> [bytes...]"));
    Serial.println(F("  SEND <addr> MSG0B|MSG4B|MSG8B|MSG12B|MSG16B|MSG27B"));
    Serial.println(F("  REQUEST <addr>"));
}
