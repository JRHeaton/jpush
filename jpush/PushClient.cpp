//
//  PushClient.cpp
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include "PushClient.h"
#include <sstream>

// | start --|  |allsens| |off_thres|  |on_thres-|  |gain-----------------|  |curve1---------------|  |curve2---------------|  EOX
//
// change pad threshold
// F0 47 7F 15  5D 00 20  00 00 07 06  00 00 08 02  00 00 00 01 08 06 0A 00  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7
// F0 47 7F 15  5D 00 20  00 00 06 0D  00 00 07 08  00 00 00 01 08 06 0A 00  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7
// F0 47 7F 15  5D 00 20  00 00 06 04  00 00 06 0E  00 00 00 01 08 06 0A 00  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7
//
// change vel curve
// F0 47 7F 15  5D 00 20  00 00 06 04  00 00 06 0E  00 00 00 01 04 0C 00 08  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7
// F0 47 7F 15  5D 00 20  00 00 06 04  00 00 06 0E  00 00 00 01 08 06 0A 00  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7
// F0 47 7F 15  5D 00 20  00 00 06 04  00 00 06 0E  00 00 00 01 04 0C 00 08  00 00 00 01 0D 04 0C 00  00 00 00 00 0C 03 05 00  F7

#define _c(func, exp) { if(debuggingEnabled) { printf("%s: 0x%0lx\n", #func, (unsigned long)exp); } }
#define _v if(debuggingEnabled)

const UInt8 PushClient::kLCDColumnWidth = 17;
const UInt8 PushClient::kPushSysexPrefix[4] = { static_cast<UInt8>(240), 71, 127, 21 };
const UInt8 PushClient::kPushSysexLCDLineClearCmds[4][3] = {
    { 28, 0, 0 },  // line 1
    { 29, 0, 0 },  // line 2
    { 30, 0, 0 },  // line 3
    { 31, 0, 0 }   // line 4
};
const UInt8 PushClient::kPushSysexLCDLineWriteCmds[4][4] = {
    { 24, 0, 69, 0 },
    { 25, 0, 69, 0 },
    { 26, 0, 69, 0 },
    { 27, 0, 69, 0 }
};
const UInt8 PushClient::kPushSysexSetModeCmd[3] = { 98, 0, 1 };
std::map<std::string, UInt8> *PushClient::ButtonTitleCCMap = nullptr;

PushClient::PushClient(CFStringRef name) : MIDIClient(name) {
    if(!PushClient::ButtonTitleCCMap) {
        PushClient::ButtonTitleCCMap = new std::map<std::string, UInt8>;
        
        auto m = PushClient::ButtonTitleCCMap;
#define b(x, y) (*m)[x] = y
        b("tap tempo",      0x03);
        b("metronome",      0x09);
        b("stop",           0x1D);
        b("master",         0x1C);
        b("1/4",            0x24);
        b("1/4t",           0x25);
        b("1/8",            0x26);
        b("1/8t",           0x27);
        b("1/16",           0x28);
        b("1/16t",          0x29);
        b("1/32",           0x2A);
        b("1/32t",          0x2B);
        b("left",           0x2C);
        b("right",          0x2D);
        b("up",             0x2E);
        b("down",           0x2F);
        b("select",         0x30);
        b("shift",          0x31);
        b("note",           0x32);
        b("session",        0x33);
        b("add effect",     0x34);
        b("add track",      0x35);
        b("octave down",    0x36);
        b("octave up",      0x37);
        b("repeat",         0x38);
        b("accent",         0x39);
        b("scales",         0x3A);
        b("user",           0x3B);
        b("mute",           0x3C);
        b("solo",           0x3D);
        b("in",             0x3E);
        b("out",            0x3F);
        b("play",           0x55);
        b("record",         0x56);
        b("new",            0x57);
        b("duplicate",      0x58);
        b("automation",     0x59);
        b("fixed length",   0x5A);
        b("device",         0x6e);
        b("browse",         0x6F);
        b("track",          0x70);
        b("clip",           0x71);
        b("volume",         0x72);
        b("pan & send",     0x73);
        b("quantize",       0x74);
        b("double",         0x75);
        b("delete",         0x76);
        b("undo",           0x77);
    }
    
    for(int i=0;i < MIDIGetNumberOfDevices(); i++) {
        MIDIDeviceRef d = MIDIGetDevice(i);
        
        CFStringRef n = getName(d);
        if(CFStringCompare(n, CFSTR("Ableton Push"), 0) == kCFCompareEqualTo) {
            MIDIEntityRef e = MIDIDeviceGetEntity(d, 1);
            
            userPortDest = MIDIEntityGetDestination(e, 0);
            userPortSrc = MIDIEntityGetSource(e, 0);
            
            CFRelease(n);
            
            break;
        }
        CFRelease(n);
    }
    
    // setting true just makes it so it still calls the original handlers
    customHandlerStatusBytes[0x90] = true;
    customHandlerStatusBytes[0x80] = true;
    customHandlerStatusBytes[0xb0] = true;
    customHandlerStatusBytes[0xf0] = true;
    
    livePortDest = getDestination(CFSTR("Live Port"));
    destEndpoint = userPortDest;
    connectSource(userPortSrc);
}

PushClient::~PushClient() {
    MIDIPortDisconnectSource(inPort, userPortDest);
    MIDIClient::~MIDIClient();
}

UInt8 PushClient::keyForGridPosition(UInt8 column, UInt8 row) {
    return (0x24 + ((0x40 - 8) - (row * 8))) + column;
}

void PushClient::gridPositionForKey(UInt8 key, UInt8 *column, UInt8 *row) {
    key -= 0x24; // normalize
    UInt8 r;
    r = 7 - (key / 8);

    if(row)
        *row = r;
    if(column)
        *column = 8 - (((8 - r) * 8) - key);
}

PushClient &PushClient::setUserMode(bool userMode, bool autoHighlightUserButton) {
    UInt8 b[4];
    memcpy(b, PushClient::kPushSysexSetModeCmd, sizeof(PushClient::kPushSysexSetModeCmd));
    b[3] = userMode == true;
    sendSysex(b, 4);
    
    if(userMode && autoHighlightUserButton)
        buttonOn("user");
    
    return *this;
}

PushClient &PushClient::clearAll() {
    clearLCD();
    clearButtons();
    clearGrid();
    
    return *this;
}

PushClient &PushClient::sendMIDI(UInt8 *buf, size_t len) {
    MIDIClient::sendMIDI(userPortDest, buf, len);
    
    return *this;
}

PushClient &PushClient::sendSysex(UInt8 *buf,
                                  size_t len,
                                  MIDICompletionProc completionProc,
                                  bool applyTerminatingByte,
                                  bool prependWithPushStartSysex) {
    size_t llen = len + (prependWithPushStartSysex ? 4 : 0);
    
    UInt8 *gbuf = buf;
    if(prependWithPushStartSysex) {
        gbuf = (UInt8 *)malloc(len+4);
        memcpy(gbuf, PushClient::kPushSysexPrefix, 4);
        memcpy(&gbuf[4], buf, len);
    }
    
    MIDIClient::sendSysex(destEndpoint, gbuf, llen, completionProc, applyTerminatingByte);
    free(gbuf);
    
    return *this;
}

PushClient &PushClient::gridPadOnV(UInt8 key, UInt8 velocity) {
    UInt8 buf[3] = { 0x90, key, velocity };
    
    return sendMIDI(buf, 3);
}

PushClient &PushClient::gridPadOn(UInt8 xColumn, UInt8 yRow, UInt8 velocity) {
    UInt8 key = PushClient::keyForGridPosition(xColumn, yRow);
    
    return gridPadOnV(key, velocity);
}

PushClient &PushClient::allGridPads(UInt8 velocity) {
    bool pre = debuggingEnabled;
    debuggingEnabled = false;
    for (int i=0;i<8;++i) {
        for(int x=0;x<8;++x) {
            gridPadOn(i, x, velocity);
        }
    }
    debuggingEnabled = pre;
    
    return *this;
}

PushClient &PushClient::buttonOn(std::string title, UInt8 velocity) {
    if(!title.length()) {
        _v std::cout << "invalid button title\n";
        return *this;
    }
    
    CFMutableStringRef str = CFStringCreateMutable(NULL, title.length());
    CFStringAppendCString(str, title.c_str(), kCFStringEncodingUTF8);
    CFStringLowercase(str, CFLocaleGetSystem());
    
    PushClient& c = buttonOn(PushClient::ButtonTitleCCMap->at(CFStringGetCStringPtr(str, kCFStringEncodingUTF8)), velocity);
    CFRelease(str);
    
    return c;
}

PushClient &PushClient::buttonOn(UInt8 buttonCC, UInt8 velocity) {
    UInt8 b[3] = { 0xb0, buttonCC, velocity };
    return sendMIDI(b, 3);
}

PushClient &PushClient::clearLCD(SInt8 line) {
    if(line >= 0) {
        if(line >= 4)
            return *this;
        
        return sendSysex((UInt8 *)PushClient::kPushSysexLCDLineClearCmds[line], 3);
    }

    for(int i=0;i<4;++i) {
        sendSysex((UInt8 *)PushClient::kPushSysexLCDLineClearCmds[i], 3);
    }
    
    return *this;
}

PushClient &PushClient::writeLCD(std::string text, SInt8 line, UInt8 charInset) {
    if(line < 0)
        line = 0;
    else if (line > 3)
        line = 3;
    if(charInset < 0)
        charInset = 0;
    else if(charInset > 67)
        charInset = 67;
    
#define SIZE 72
    
    int space = 68 - charInset;
    if(space <= 0)
        return *this;
    
    UInt8 *buf = new UInt8 [SIZE];
    memset(buf, 0x20, SIZE);
    memcpy(buf, PushClient::kPushSysexLCDLineWriteCmds[line], 4);
    memcpy(&buf[4 + charInset], text.c_str(), text.length() < space ? text.length() : space);
    
    sendSysex(buf, SIZE);
    delete [] buf;
    
    return *this;
}

std::string PushClient::logString() {
    std::ostringstream str;
    str << "userPortDest=" << userPortDest;
    
    return str.str();
}

PushClient &PushClient::allButtons(UInt8 velocity) {
    for(auto it = PushClient::ButtonTitleCCMap->begin(); it != PushClient::ButtonTitleCCMap->end(); ++it) {
        UInt8 key = it->second;
        buttonOn(key, velocity);
    }
    
    return *this;
}

PushClient &PushClient::clearButtons() {
    allButtons(0);
    
    return *this;
}

void PushClient::handleMsg(UInt8 *buf, size_t len) {
    MIDIClient::handleMsg(buf, len);
    
    if((buf[0] & 0xf0) == 0x90) {
        if(gridPadHandler) {
            UInt8 c, r;
            PushClient::gridPositionForKey(buf[1], &c, &r);
            gridPadHandler(c, r, buf[2], true);
        }
    } else if((buf[0] & 0xf0) == 0x80) {
        if(gridPadHandler) {
            UInt8 c, r;
            PushClient::gridPositionForKey(buf[1], &c, &r);
            gridPadHandler(c, r, buf[2], false);
        }
    } else if((buf[0] & 0xf0) == 0xb0) {
        if(buttonHandler) {
            std::string name;
            for(auto it = ButtonTitleCCMap->begin(); it != ButtonTitleCCMap->end(); ++it) {
                if(it->second == buf[1]) {
                    name = it->first;
                    break;
                }
            }
            
            buttonHandler(name, buf[1], buf[2] != 0);
        }
    } else if((buf[0] & 0xf0) == 0xf0) {
        off_t offset = sizeof(PushClient::kPushSysexPrefix);
        if(!memcmp(&buf[offset], kPushSysexSetModeCmd, sizeof(kPushSysexSetModeCmd))) {
            if(modeChangeHandler)
                modeChangeHandler(buf[offset + sizeof(kPushSysexSetModeCmd)] == 1);
        }
    }
}