//
//  MIDIMsgHandler.cpp
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include "MIDIMsgHandler.h"

#define ST_SYSEX    0xf0
#define ST_CC       0xb0
#define ST_ATOUCH   0xa0
#define ST_NOTEOFF  0x80
#define ST_NOTEON   0x90

const struct MIDIMsgHandler::MsgType
MIDIMsgHandler::MsgTypes [5] = {
    { ST_NOTEON,    3 },    // note on
    { ST_NOTEOFF,   3 },    // note off
    { ST_ATOUCH,    3 },    // aftertouch
    { ST_CC,        3 },    // control change
    { ST_SYSEX,     0 }     // sysex
};

void MIDIMsgHandler::handlePacketList(const MIDIPacketList *list) {
    MIDIPacket *packet = (MIDIPacket *)&list->packet[0];
    for (int i = 0; i < list->numPackets; ++i) {
        handlePacket(packet);
        packet = MIDIPacketNext(packet);
    }
}

void MIDIMsgHandler::handlePacket(MIDIPacket *pkt) {
    UInt8 *buf = (UInt8 *)pkt->data;
    
    while(buf < (pkt->data + pkt->length)) {
        bool found = false;
        for(int i=0;i<sizeof(MsgTypes)/2;++i) {
            const struct MIDIMsgHandler::MsgType &t = MIDIMsgHandler::MsgTypes[i];
            
            if((buf[0] & 0xf0) == t.status) {
                handleMsg(buf, t.length);
                
                buf += t.length;
                found = true;
            }
        }
        if(found)
            found = false;
        else
            buf++;
    }
}

void MIDIMsgHandler::handleMsg(UInt8 *buf, size_t len) {
    for(auto it = customHandlerStatusBytes.begin(); it != customHandlerStatusBytes.end(); ++it) {
        if(it->first == buf[0] && it->second == false) {
            return; // let subclasses do their own processing if they want
        }
    }
    
    switch(buf[0]) {
        case ST_NOTEON: {
            if(noteHandler)
                noteHandler(true, buf[1], buf[2]);
        } break;
        case ST_NOTEOFF: {
            if(noteHandler)
                noteHandler(false, buf[1], buf[2]);
        } break;
        case ST_SYSEX: {
            if(sysexHandler)
                sysexHandler(buf, len);
        } break;
        case ST_CC: {
            if(controlChangeHandler)
                controlChangeHandler(buf[1], buf[2]);
        } break;
        case ST_ATOUCH: {
            if(afterTouchHandler) {
                afterTouchHandler(buf[1], buf[2]);
            }
        } break;
            
        default: {
            if(rawMsgHandler)
                rawMsgHandler(buf, len);
        }
    }
}