#include <Arduino.h>
#include <CRUMBS.h>
#include <stdlib.h>

extern void sendCrumbs(uint8_t address, CRUMBSMessage &message);
extern void requestCrumbs(uint8_t address);

namespace
{
    bool parseAddress(const String &token, uint8_t &address)
    {
        if (token.length() == 0)
        {
            return false;
        }

        long value = 0;
        if (token.startsWith("0x") || token.startsWith("0X"))
        {
            value = strtol(token.c_str(), nullptr, 16);
        }
        else
        {
            value = token.toInt();
        }

        if (value < 0 || value > 0x7F)
        {
            return false;
        }

        address = static_cast<uint8_t>(value);
        return true;
    }

    bool collectTokens(const String &source, String *tokens, size_t expected)
    {
        size_t count = 0;
        int index = 0;

        while (count < expected && index < source.length())
        {
            while (index < source.length() && source[index] == ' ')
            {
                index++;
            }
            if (index >= source.length())
            {
                break;
            }

            int nextSpace = source.indexOf(' ', index);
            if (nextSpace < 0)
            {
                nextSpace = source.length();
            }

            tokens[count++] = source.substring(index, nextSpace);
            index = nextSpace + 1;
        }

        return count == expected;
    }
}

void handleSerial()
{
    if (!Serial.available())
    {
        return;
    }

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
    {
        return;
    }

    String upper = line;
    upper.toUpperCase();

    if (upper.startsWith("REQUEST"))
    {
        int space = line.indexOf(' ');
        if (space < 0)
        {
            Serial.println(F("Usage: REQUEST <addr>"));
            return;
        }

        String token = line.substring(space + 1);
        token.trim();

        uint8_t address = 0;
        if (!parseAddress(token, address))
        {
            Serial.println(F("Invalid address."));
            return;
        }

        requestCrumbs(address);
        return;
    }

    if (upper.startsWith("SEND"))
    {
        int space = line.indexOf(' ');
        if (space < 0)
        {
            Serial.println(F("Usage: SEND <addr> <green> <yellow> <red> <period_ms>"));
            return;
        }

        String args = line.substring(space + 1);
        args.trim();

        String tokens[5];
        if (!collectTokens(args, tokens, 5))
        {
            Serial.println(F("Usage: SEND <addr> <green> <yellow> <red> <period_ms>"));
            return;
        }

        uint8_t address = 0;
        if (!parseAddress(tokens[0], address))
        {
            Serial.println(F("Invalid address."));
            return;
        }

        float ratios[3];
        for (int i = 0; i < 3; ++i)
        {
            ratios[i] = tokens[i + 1].toFloat();
            if (!(ratios[i] >= 0.0f && ratios[i] <= 1.0f))
            {
                Serial.println(F("Ratios must be between 0.0 and 1.0."));
                return;
            }
        }

        long periodMs = tokens[4].toInt();
        if (periodMs <= 0)
        {
            Serial.println(F("Period must be positive."));
            return;
        }

        CRUMBSMessage message = {};
        message.typeID = 1;
        message.commandType = 1;
        message.data[0] = ratios[0];
        message.data[1] = ratios[1];
        message.data[2] = ratios[2];
        message.data[3] = static_cast<float>(periodMs);

        sendCrumbs(address, message);
        return;
    }

    Serial.println(F("Unknown command. Use SEND or REQUEST."));
}
