#include "simpleApi.h"
#include "unkonwStruct.h"
#include "iTunesApi.h"

void *MyPlistToCF(void *buff, int len)
{
    CFMutableDataRef data = CFDataCreateMutable(NULL, 0);
    CFDataAppendBytes(data, (UInt8 *)buff, len);
    void *pMutablelist = (void *)CFPropertyListCreateWithData(NULL, data, 0, 0, 0);
    return pMutablelist;
}

void *MyCFToPlist(void *buff, CFIndex*len)
{
    CFErrorRef myError;
    CFDataRef xmlData = CFPropertyListCreateData(
        0, buff, kCFPropertyListBinaryFormat_v1_0, 0, &myError);
    *len = CFDataGetLength(xmlData);
    return (void *)CFDataGetBytePtr(xmlData);
}

Boolean MyCFDictIsContainsKey(void *dict, void *cfkey)
{
    return CFDictionaryContainsKey((CFDictionaryRef)dict, (CFStringRef)cfkey);
}
