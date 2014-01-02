//
//  MIDIClient.h
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#ifndef MIDICLIENT_H
#define MIDICLIENT_H

#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>

#include "MIDIMsgHandler.h"

// i/o and similar methods will return 'this' on success, nullptr on failure

#define VELOCITY_MAX 127

class MIDIClient : public MIDIMsgHandler {
protected:
    MIDIClientRef   client;
    MIDIPortRef     inPort, outPort;
    
    static void     MIDISysexCompletion(MIDISysexSendRequest *request);
    static void     MIDINotifyProc(const MIDINotification *message, void *refCon);
    static void     MIDIReadProc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon);
    
    virtual void    handleMsg(UInt8 *buf, size_t len);
    
public:
    MIDIClient(CFStringRef name=CFSTR("MIDIClient"), bool verbose=false);
    virtual ~MIDIClient();
    
    ItemCount           numDests();
    ItemCount           numSources();
    
    MIDIEndpointRef     getSource(CFStringRef name);
    MIDIEndpointRef     getDestination(CFStringRef name);
    MIDIEndpointRef     destAtIndex(CFIndex index);
    
    void                connectSource(MIDIEndpointRef source);
    virtual CFStringRef getName(MIDIObjectRef obj);
    
    MIDIClient          &wait(double seconds);
    void                after(double seconds, void (^block)());
    
    virtual MIDIClient  &sendMIDI(MIDIEndpointRef dest, UInt8 *buf, size_t len);
    virtual MIDIClient  &sendSysex(MIDIEndpointRef dest,
                                   UInt8 *buf,
                                   size_t len,
                                   MIDICompletionProc completionProc=nullptr,
                                   bool applyTerminatingByte=true);
        
    virtual void        logObject(MIDIObjectRef obj);
    void                logSelf();
    virtual std::string logString();
    
    bool                debuggingEnabled = false, logSysex = false;
    void                logData(const char *n, UInt8 *b, size_t len); // convenience
};

#endif /* MIDICLIENT_H */
