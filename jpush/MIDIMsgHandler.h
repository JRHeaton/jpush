//
//  MIDIMsgHandler.h
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#ifndef MIDIMSGHANDLER_H
#define MIDIMSGHANDLER_H

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <iostream>
#include <map>

class MIDIMsgHandler {
private:
    static const struct MsgType {
        UInt8 status;
        UInt8 length;
     } MsgTypes[5];
    
public:
    std::map<UInt8, bool /*keep super */> customHandlerStatusBytes;
    
    void    (^rawMsgHandler)(UInt8 *buf, size_t len)    = 0;
    void    (^sysexHandler)(UInt8 *buf, size_t len)     = 0;
    void    (^controlChangeHandler)(UInt8 CID,
                                    UInt8 val)          = 0;
    void    (^noteHandler)(bool noteOn,
                           UInt8 key,
                           UInt8 velocity)              = 0;
    void    (^afterTouchHandler)(UInt8 key,
                                 UInt8 pressure)        = 0;
    
    void            handlePacketList(const MIDIPacketList *list);
    void            handlePacket(MIDIPacket *pkt);
    virtual void    handleMsg(UInt8 *buf, size_t len);
};

#endif /* MIDIMSGHANDLER_H */
