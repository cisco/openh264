//
//  main.m
//  codec_unittest
//
//  Created by openh264 on 14-6-5.
//  Copyright (c) 2014年 com.cisco. All rights reserved.
//


#import <UIKit/UIKit.h>

#import "AppDelegate.h"
extern int CodecUtMain(int argc, char** argv);


int GetDocumentPath(char *pPath, unsigned long *pLen)
{
    if (!pLen) return 1;
    unsigned long uPathLen = *pLen;
    if(NULL == pPath || 0 == uPathLen)
    {
        return 1;
    }
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if([paths count] == 0)
    {
        return 2;
    }
    NSString* document = [paths objectAtIndex:0];
    NSString* escapedPath = [document stringByReplacingOccurrencesOfString:@" " withString:@"\\ "];
    unsigned long uDocumentPathLen = [escapedPath length];
    uPathLen= (uDocumentPathLen <= uPathLen) ? uDocumentPathLen : uPathLen;
    memcpy(pPath,[escapedPath UTF8String],uPathLen);
    return 0;
}




int main(int argc, char * argv[])
{
    //Call the UT
#ifdef IOS_SIMULATOR
    const char* path="/tmp/codec_unittest.xml";
#else
    char xmlWritePath[1024] = "";
    unsigned long uPathLen = 1024;
    char path[1024] = "";
    GetDocumentPath(xmlWritePath,&uPathLen);
    sprintf(path, "%s%s",xmlWritePath,"/codec_unittest.xml");
#endif
    argc =2;
    argv[0]="codec_unittest";
    argv[1]=path;
    CodecUtMain(argc,argv);
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
