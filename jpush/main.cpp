//
//  main.cpp
//  jpush
//
//  Created by John Heaton on 1/1/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include <iostream>
#include "PushClient.h"
#include <sys/random.h>

int main(int argc, const char * argv[]) {
    srand((unsigned int)time(NULL));
    
    int pads = 35;
    __block int highlight = 36;
    __block int off = 50;
    
    PushClient *c = new PushClient(CFSTR("pushstuff"));
    c->modeChangeHandler = ^(bool user) {
        std::cout << "mode: " << (!user ? "live\n" : "user\n");
        
        if(user) {
            c->clearAll();
            c->allGridPads(pads);
            c->writeLCD("jpush", 1);
            c->writeLCD("Ableton Push i/o hax for funsies", 2);
            c->buttonOn("out")->buttonOn("user");
        }
    };
    c->setUserMode();
    c->modeChangeHandler(true);
    c->buttonHandler = ^(std::string name, UInt8 CID, bool on) {
        printf("button: name=%s, CID=%02x, on=%d\n", name.c_str(), CID, on);
    
        if(on && name == "out") {
            highlight = (rand() % 126) + 1;
            off = ((rand() % 126) + 1);
            
            c->allGridPads((rand() % 126) + 1);
        }
    };
    c->noteHandler = ^(bool noteOn, UInt8 key, UInt8 velocity) {
        printf("on=%d, key=%02x, velocity=%d\n", noteOn, key, velocity);
    };
    c->afterTouchHandler = ^(UInt8 key, UInt8 pressure) {
        printf("aftertouch key=%02x, pressure=%d\n", key, pressure);
    };
    c->gridPadHandler = ^(UInt8 xColumn, UInt8 yRow, UInt8 vel, bool on) {
        printf("grid pad (%d, %d): on=%d, velocity=%d\n", xColumn, yRow, on, vel);
        c->gridPadOn(xColumn, yRow, on ? highlight : off);
    };
    
    CFRunLoopRun();
    return 0;
}

