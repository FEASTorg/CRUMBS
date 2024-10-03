#ifndef CRUMBS_H
#define CRUMBS_H

#define CRUMBS_DEBUG

#include <Arduino.h>
#include <Wire.h>
#include <tinycbor.h>
#include "CRUMBSMessage.h"

#ifdef CRUMBS_DEBUG
    #define CRUMBS_DEBUG_PRINT(...) Serial.print("CRUMBS: "); Serial.print(__VA_ARGS__)
    #define CRUMBS_DEBUG_PRINTLN(...) Serial.print("CRUMBS: "); Serial.println(__VA_ARGS__)
#else
    #define CRUMBS_DEBUG_PRINT(...)
    #define CRUMBS_DEBUG_PRINTLN(...)
#endif

class CRUMBS {
public:
    CRUMBS(bool isMaster = false, uint8_t address = 0);

    void begin();
    void sendMessage(const CRUMBSMessage& message, uint8_t targetAddress);
    void receiveMessage(CRUMBSMessage& message);
    
    void onReceive(void (*callback)(CRUMBSMessage&));
    void onRequest(void (*callback)());

    uint8_t getAddress() const;

    size_t encodeMessage(const CRUMBSMessage& message, uint8_t* buffer, size_t bufferSize);

    bool decodeMessage(const uint8_t* buffer, size_t bufferSize, CRUMBSMessage& message);

private:
    uint8_t i2cAddress;
    bool masterMode;
    void (*receiveCallback)(CRUMBSMessage&) = nullptr;
    void (*requestCallback)() = nullptr;

    static void receiveEvent(int bytes);
    static void requestEvent();
};

#endif // CRUMBS_H
