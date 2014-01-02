//
//  MIDIClient.cpp
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include "MIDIClient.h"

#define _c(func, exp) { auto ret = exp; if(debuggingEnabled) { printf("%s: %d\n", #func, ret); } }

struct __sysexCompletion {
    bool _freeBuf = false;
    MIDICompletionProc cproc;
    UInt8 *odata; // original ptr not moved by sysex sender
};

void MIDIClient::logData(const char *n, UInt8 *b, size_t len) {
    printf("%s (len=%zu): { \n", n, len);
    for(int i=0;i<len;++i) {
        printf("0x%02x", b[i]);
        if(i+1 < len)
            printf(", ");
        else
            printf(" ");
        
        if((i != 0 && i % 8 == 0) || i+1 >= len)
            printf("\n");
    }
    puts("}");
}

void MIDIClient::MIDISysexCompletion(MIDISysexSendRequest *request) {
    struct __sysexCompletion *com = (struct __sysexCompletion *)request->completionRefCon;
    if(com) {
        if(com->_freeBuf)
            delete [] com->odata;
        if(com->cproc)
            com->cproc(request);
    }
    
    delete com;
    delete request;
}

void MIDIClient::MIDINotifyProc(const MIDINotification *message, void *refCon) {
    
}

void MIDIClient::MIDIReadProc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon) {
    ((MIDIClient *)readProcRefCon)->handlePacketList(pktlist);
}

void MIDIClient::handleMsg(UInt8 *buf, size_t len) {
    MIDIMsgHandler::handleMsg(buf, len);
    
    if(debuggingEnabled)
        logData("midiIn", buf, len);
}

MIDIClient::MIDIClient(CFStringRef name, bool verbose) {
    this->debuggingEnabled = verbose;
    
    _c(MIDIClientCreate, MIDIClientCreate(name, &MIDIClient::MIDINotifyProc, this, &client))
    _c(MIDIInputPortCreate, MIDIInputPortCreate(client, name, &MIDIClient::MIDIReadProc, this, &inPort));
    _c(MIDIOutputPortCreate, MIDIOutputPortCreate(client, name, &outPort));
}

MIDIClient::~MIDIClient() {
    MIDIClientDispose(client);
    MIDIPortDispose(inPort);
    MIDIPortDispose(outPort);
}

ItemCount MIDIClient::numDests() {
    return MIDIGetNumberOfDestinations();
}

ItemCount MIDIClient::numSources() {
    return MIDIGetNumberOfSources();
}

MIDIEndpointRef MIDIClient::getSource(CFStringRef name) {
    MIDIEndpointRef ret(0);
    
    for(int i=0;i<MIDIGetNumberOfSources();++i) {
        MIDIEndpointRef src = MIDIGetSource(i);
        
        CFStringRef n = getName(src);
        if(CFStringFind(name, name, kCFCompareCaseInsensitive).length == CFStringGetLength(name)) {
            CFRelease(n);
            return src;
        }
        CFRelease(n);
    }
    
    return ret;
}

MIDIEndpointRef MIDIClient::getDestination(CFStringRef name) {
    MIDIEndpointRef ret(0);
    
    for(int i=0;i<MIDIGetNumberOfDestinations();++i) {
        MIDIEndpointRef src = MIDIGetDestination(i);
        
        CFStringRef n = getName(src);
        if(CFStringFind(n, name, kCFCompareCaseInsensitive).length == CFStringGetLength(name)) {
            
            CFRelease(n);
            return src;
        }
        
        CFRelease(n);
    }
    
    return ret;
}

MIDIEndpointRef MIDIClient::destAtIndex(CFIndex index) {
    return MIDIGetDestination(index);
}

void MIDIClient::connectSource(MIDIEndpointRef source) {
    _c(MIDIPortConnectSource, MIDIPortConnectSource(inPort, source, this));
}

CFStringRef MIDIClient::getName(MIDIObjectRef obj) {
    CFStringRef name;
    MIDIObjectGetStringProperty(obj, kMIDIPropertyName, &name);
    
    return name;
}

MIDIClient & MIDIClient::wait(double seconds) {
    usleep(1000000 * seconds);
    
    return *this;
}

void MIDIClient::after(double seconds, void (^block)()) {
    double delayInSeconds = seconds;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_main_queue(), block);
}

MIDIClient &MIDIClient::sendMIDI(MIDIEndpointRef dest, UInt8 *buf, size_t len) {
    MIDIPacketList l;
    l.numPackets = 1;
    l.packet[0].timeStamp = 0;
    l.packet[0].length = len;
    memcpy(l.packet[0].data, buf, len > 256 ? 256 : len);
    
    if(debuggingEnabled) {
        logData("MIDISend", buf, len);
    }
    
    _c(MIDISend, MIDISend(outPort, dest, &l));
    
    return *this;
}

MIDIClient &MIDIClient::sendSysex(MIDIEndpointRef dest,
                           UInt8 *buf,
                           size_t len,
                           MIDICompletionProc completionProc,
                           bool applyTerminatingByte)
{
    struct MIDISysexSendRequest *r = (struct MIDISysexSendRequest *)calloc(1, sizeof(struct MIDISysexSendRequest));
    r->destination = dest;
    r->bytesToSend = (UInt32)len + (applyTerminatingByte == true ? 1 : 0);
    
    UInt8 *gbuf = buf;
    if(applyTerminatingByte) {
        gbuf = (UInt8 *)malloc(r->bytesToSend);
        memcpy(gbuf, buf, r->bytesToSend);
        gbuf[r->bytesToSend-1] = 0xf7; // EOX
    }
    
    UInt8 *ptr = gbuf;
    r->data = ptr;
    
    if(logSysex) {
        logData("MIDISysexSend", (UInt8*)r->data, r->bytesToSend);
    }
    
    r->completionProc = &MIDIClient::MIDISysexCompletion;
    
    struct __sysexCompletion *com = new struct __sysexCompletion;
    memset(com, 0, sizeof (struct __sysexCompletion));
    com->_freeBuf = applyTerminatingByte;
    if(completionProc != nullptr)
        com->cproc = completionProc;
    com->odata = gbuf;
    r->completionRefCon = (void *)com;
    
    _c(MIDISendSysex, MIDISendSysex(r));
    
    return *this;
}

void MIDIClient::logObject(MIDIObjectRef obj){
    CFStringRef name = getName(obj);
 
    printf("MIDIObject<%u>: name=\"%s\"\n", (unsigned int)obj, CFStringGetCStringPtr(name, kCFStringEncodingUTF8));
    CFRelease(name);
}

void MIDIClient::logSelf() {
    printf("MIDI client @ %p: client=%d, inPort=%d, outPort=%d ", this, client, inPort, outPort);
    std::string log = this->logString();
    if(log.c_str()) {
        printf(", %s", log.c_str());
    }
    puts("");
}

std::string MIDIClient::logString() {
    return "";
}