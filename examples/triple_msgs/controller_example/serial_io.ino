#include <Arduino.h>
#include <Wire.h>

extern CRUMBS crumbsController;
extern void drawDisplay();
extern void setOk();
extern void setError();
extern void pulseActivity();
extern void refreshLeds();
extern void cacheTx(uint8_t, const CRUMBSMessage &);
extern void cacheRx(uint8_t, const CRUMBSMessage &);
extern void sendCrumbs(uint8_t, const CRUMBSMessage &);
extern bool requestCrumbs(uint8_t, CRUMBSMessage &);

void handleSerialInput()
{
    if (!Serial.available())
        return;

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0)
        return;

    // Request syntax: request=<addr or 0x..>
    if (input.startsWith("request="))
    {
        String a = input.substring(strlen("request="));
        a.trim();

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
            for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
            {
                Serial.print(resp.data[i], 3);
                Serial.print(' ');
            }
            Serial.print(F(" crc=0x"));
            Serial.println(resp.crc8, HEX);
        }
        else
        {
            Serial.println(F("Controller: request failed."));
        }
        return;
    }

    // Otherwise parse CSV send
    uint8_t addr;
    CRUMBSMessage m;
    if (parseSerialInput(input, addr, m))
    {
        sendCrumbs(addr, m);
        Serial.println(F("Controller: message sent."));
    }
    else
    {
    Serial.println(F("Controller: parse failed. Use addr,typeID,commandType,data0..data6"));
    }
}

bool parseSerialInput(const String &s, uint8_t &addr, CRUMBSMessage &m)
{
    m.typeID = 0;
    m.commandType = 0;
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
        m.data[i] = 0;
    m.crc8 = 0;

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
            default:
                if (field >= 3 && field < 3 + CRUMBS_DATA_LENGTH)
                {
                    m.data[field - 3] = v.toFloat();
                }
                else if (field == 3 + CRUMBS_DATA_LENGTH)
                {
                    m.crc8 = (uint8_t)strtol(v.c_str(), NULL, 0);
                }
                break;
            }
            field++;
        }
    }
    return (field >= 3 + CRUMBS_DATA_LENGTH);
}
