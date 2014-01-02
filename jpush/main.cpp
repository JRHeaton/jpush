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
    c->sysexHandler = ^(UInt8 *buf, size_t len) {
        c->logData("sysex input", buf, len);
    };
    
    c->setUserMode(true, true);
    c->allButtons();
    
    CFRunLoopRun();
    return 0;
}

