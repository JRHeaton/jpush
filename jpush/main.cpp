//
//  main.cpp
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include <iostream>
#include "PushClient.h"

int main(int argc, const char * argv[]) {
    
    PushClient *c = new PushClient(CFSTR("pushstuff"));
    c->clearAll();
    c->allGridPads(35);
    c->writeLCD("jpush", 1);
    c->writeLCD("Ableton Push i/o hax for funsies", 2);
    c->buttonOn("up")->buttonOn("left")->buttonOn("down")->buttonOn("right");
    c->buttonHandler = ^(std::string name,
                         UInt8 CID,
                         bool on) {
        printf("button: name=%s, CID=%02x, on=%d\n", name.c_str(), CID, on);
        c->buttonOn(CID, !on?0:127);
    };
    c->noteHandler = ^(bool noteOn,
                       UInt8 key,
                       UInt8 velocity) {
        printf("on=%d, key=%02x, velocity=%d\n", noteOn, key, velocity);
    };
    c->afterTouchHandler = ^(UInt8 key,
                             UInt8 pressure) {
        printf("aftertouch key=%02x, pressure=%d\n", key, pressure);
    };
    c->gridPadHandler = ^(UInt8 xColumn, UInt8 yRow, UInt8 vel, bool on) {
        printf("(%d, %d): %d @%d\n", xColumn, yRow, on, vel);
        c->gridPadOn(xColumn, yRow, on ? 36 : 50);
    };
    
    CFRunLoopRun();
    return 0;
}

