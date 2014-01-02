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
    MIDIRestart();

    PushClient *c = new PushClient(CFSTR("pushstuff"));
    c->clearAll();
    c->writeLCD("Test 1", 1, PushClient::kLCDColumnWidth);
    c->writeLCD("Test 2", 2, PushClient::kLCDColumnWidth * 2);
    c->gridPadHandler = ^(UInt8 xColumn, UInt8 yRow, bool on) {
        printf("(%d, %d): %d\n", xColumn, yRow, on);
        c->gridPadOn(xColumn, yRow, !on ? 0 : 127);
    };
    
    CFRunLoopRun();
    return 0;
}

