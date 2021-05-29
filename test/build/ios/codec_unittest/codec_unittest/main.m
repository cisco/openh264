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


int DoTest(char *pPath, unsigned long *pLen)
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
    NSFileManager* manager = [NSFileManager defaultManager];
    [manager changeCurrentDirectoryPath:[document stringByExpandingTildeInPath]];
    NSString* escapedPath = [document stringByReplacingOccurrencesOfString:@" " withString:@"\\ "];
    unsigned long uDocumentPathLen = [escapedPath length];
    uPathLen= (uDocumentPathLen <= uPathLen) ? uDocumentPathLen : uPathLen;
    memcpy(pPath,[escapedPath UTF8String],uPathLen);
    char path[1024] = "";
    sprintf(path, "%s%s",pPath,"/codec_unittest.xml");
    int argc =2;
    char* argv[]={(char*)"codec_unittest",path};
    CodecUtMain(argc,argv);
    return 0;
}




int main(int argc, char * argv[])
{
    //Call the UT
#ifdef IOS_SIMULATOR
    const char* path="/tmp/codec_unittest.xml";
    argc =2;
    argv[0]=(char*)"codec_unittest";
    argv[1]=path;
    CodecUtMain(argc,argv);
    abort();
#else
    char xmlWritePath[1024] = "";
    unsigned long uPathLen = 1024;
    if(DoTest(xmlWritePath,&uPathLen) == 0)
        NSLog(@"Unit test running sucessfully on devices");
    else
        NSLog(@"Unit test runing failed on devices");
    abort();

#endif

    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
