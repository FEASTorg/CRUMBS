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

        uint8_t addr = 0;
        if (a.startsWith("0x") || a.startsWith("0X"))
            addr = (uint8_t)strtol(a.c_str(), NULL, 16);
        else
            addr = (uint8_t)a.toInt();

        Serial.print(F("Controller: requesting from 0x"));
        Serial.println(addr, HEX);

        CRUMBSMessage resp;
        if (requestCrumbs(addr, resp))
        {
            Serial.println(F("Controller: response decoded:"));
            Serial.print(F("typeID="));
            Serial.print(resp.typeID);
            Serial.print(F(" cmd="));
            Serial.print(resp.commandType);
            Serial.print(F(" data: "));
            for (int i = 0; i < 6; i++)
            {
                Serial.print(resp.data[i], 3);
                Serial.print(' ');
            }
            Serial.print(F(" err="));
            Serial.println(resp.errorFlags);
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
        sendCrumbs(addr, m);
        Serial.println(F("Controller: message sent."));
    }
    else
    {
        Serial.println(F("Controller: parse failed. Use addr,typeID,commandType,data0..data5,errorFlags"));
    }
}

bool parseSerialInput(const String &s, uint8_t &addr, CRUMBSMessage &m)
{
    m.typeID = 0;
    m.commandType = 0;
    for (int i = 0; i < 6; i++)
        m.data[i] = 0;
    m.errorFlags = 0;

    int field = 0, last = 0;
    for (int i = 0; i <= s.length(); i++)
    {
        if (i == s.length() || s[i] == ',')
        {
            String v = s.substring(last, i);
            v.trim();
            last = i + 1;
            switch (field)
            {
            case 0:
                addr = (uint8_t)v.toInt();
                break;
            case 1:
                m.typeID = (uint8_t)v.toInt();
                break;
            case 2:
                m.commandType = (uint8_t)v.toInt();
                break;
            case 3:
                m.data[0] = v.toFloat();
                break;
            case 4:
                m.data[1] = v.toFloat();
                break;
            case 5:
                m.data[2] = v.toFloat();
                break;
            case 6:
                m.data[3] = v.toFloat();
                break;
            case 7:
                m.data[4] = v.toFloat();
                break;
            case 8:
                m.data[5] = v.toFloat();
                break;
            case 9:
                m.errorFlags = (uint8_t)v.toInt();
                break;
            default:
                break;
            }
            field++;
        }
    }
    return (field >= 4);
}
