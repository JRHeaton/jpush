//
//  PushClient.h
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#ifndef PUSHCLIENT_H
#define PUSHCLIENT_H

#include "MIDIClient.h"
#include <map>

#define VELOCITY_MAX 127

class PushClient : public MIDIClient {
protected:
    void            handleMsg(UInt8 *buf, size_t len);
    MIDIEndpointRef userPortSrc;
    
public:
    PushClient(CFStringRef name=CFSTR("PushClient"));
    ~PushClient();
    
    MIDIEndpointRef userPortDest, livePortDest;
    MIDIEndpointRef destEndpoint = 0;
    
    static const UInt8 kLCDColumnWidth;
    static const UInt8 kPushSysexPrefix[4];
    static const UInt8 kPushSysexSetModeCmd[3];
    static const UInt8 kPushSysexLCDLineClearCmds[4][3];
    static const UInt8 kPushSysexLCDLineWriteCmds[4][4];
    
//    static bool AbletonOpen() {
//        
//    };
    
    static std::map<std::string, UInt8> *ButtonTitleCCMap;
    
    void            (^modeChangeHandler)(bool userMode)                     = 0;
    void            (^gridPadHandler)(UInt8 xColumn,
                                      UInt8 yRow,
                                      UInt8 velocity,
                                      bool on)              = 0;
    void            (^buttonHandler)(std::string name,
                                     UInt8 CID,
                                     bool on)               = 0;
    
    static UInt8    keyForGridPosition(UInt8 column, UInt8 row);
    static void     gridPositionForKey(UInt8 key, UInt8 *column, UInt8 *row);
    
    PushClient  &sendMIDI(UInt8 *buf, size_t len);
    PushClient  &sendSysex(UInt8 *buf,
                           size_t len,
                           MIDICompletionProc completionProc=nullptr,
                           bool applyTerminatingByte=true,
                           bool prependWithPushStartSysex=true);
    
    PushClient  &setUserMode(bool userMode=true, bool autoHighlightUserButton=true);
    PushClient  &clearAll();
    
    PushClient  &allGridPads(UInt8 velocity=VELOCITY_MAX);
    PushClient  &clearGrid() { return allGridPads(0); };
    PushClient  &gridPadOnV(UInt8 key, UInt8 velocity=VELOCITY_MAX);
    PushClient  &gridPadOn(UInt8 xColumn, UInt8 yRow, UInt8 velocity=VELOCITY_MAX);
    PushClient  &gridPadOff(UInt8 xColumn, UInt8 yRow) { return gridPadOn(xColumn, yRow, 0); };
    
    PushClient  &buttonOn(std::string title, UInt8 velocity=VELOCITY_MAX);
    PushClient  &buttonOn(UInt8 buttonCC, UInt8 velocity=VELOCITY_MAX);
    PushClient  &buttonOff(UInt8 buttonCC) { return buttonOn(buttonCC, 0); };
    PushClient  &buttonOff(std::string title) { return buttonOn(title, 0); };
    PushClient  &allButtons(UInt8 velocity=VELOCITY_MAX);
    PushClient  &clearButtons();
    
    PushClient  &clearLCD(SInt8 line=-1);
    PushClient  &writeLCD(std::string text, SInt8 line=0, UInt8 charInset=0);
    
    virtual     std::string logString();
};

#endif /* PUSHCLIENT_H */