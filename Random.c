#include <CoreFoundation/CoreFoundation.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static void logString(CFStringRef format, ...)
{
    va_list args;
    va_start(args, format);
    CFStringRef str = CFStringCreateWithFormat(kCFAllocatorSystemDefault, NULL, format, args);
    if (!str) return;

    CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8);
    char *buf = str ? (char *)malloc(len + 1) : 0;

    if (buf)
    {
        Boolean converted = CFStringGetCString(str, buf, len, kCFStringEncodingUTF8);

        if (converted)
        {
            buf[len] = '\0';
        }
    }

    if (buf) free(buf);
    if (str) CFRelease(str);
    va_end(args);
}

static CFMutableDataRef dataFromFile(const char *fname)
{
    int fd = open(fname, O_RDONLY);
    CFMutableDataRef res = CFDataCreateMutable(kCFAllocatorSystemDefault, 0);
    char buf[4096];
    ssize_t amountRead;
    while ((amountRead = read(fd, buf, 4096)) > 0) CFDataAppendBytes(res, (const UInt8 *)buf, amountRead);
    close(fd);
    return(res);
}

static bool writeDataToFile(CFDataRef data, const char *fname)
{
    int fd = open(fname, O_RDONLY | O_CREAT | O_TRUNC, 0666);

    if (fd < 0)
    {
        logString(CFSTR("There was an error creating the file: %d"), errno);
        return(false);
    }

    CFIndex len = CFDataGetLength(data);
    printf("%s\n", (const char *)CFDataGetBytePtr(data));
    ssize_t read = write(fd, CFDataGetBytePtr(data), len);
    fsync(fd);
    close(fd);
    return(read == len);
}

static int plConvert(const char *cfile, const char *nfile, bool openStep)
{
    CFMutableDataRef plistdata = dataFromFile(cfile);
    bool success = false;

    if (!plistdata) {
        logString(CFSTR("Could not read plist from file: %s"), cfile);
    } else {
        CFPropertyListFormat format;
        CFErrorRef error;
        CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorSystemDefault, (CFDataRef)plistdata, 0, &format, &error);

        if (!plist) {
            logString(CFSTR("Unable to create property list from data: %@"), error);
        } else {
            // logString(CFSTR("Property List Contents:\n%@"), plist);

            if (!openStep) {
                if (format == kCFPropertyListBinaryFormat_v1_0) {
                    logString(CFSTR("Writing XML Property List to %s..."), nfile);
                    format = kCFPropertyListXMLFormat_v1_0;
                } else if (format == kCFPropertyListXMLFormat_v1_0) {
                    logString(CFSTR("Writing Binary Property List to %s..."), nfile);
                    format = kCFPropertyListBinaryFormat_v1_0;
                } else {
                    logString(CFSTR("Unknown Property List Format! Will convert to XML File at %s..."), nfile);
                    format = kCFPropertyListXMLFormat_v1_0;
                }
            } else {
                logString(CFSTR("Converting to OpenStep Property List at %s..."), nfile);
                format = kCFPropertyListOpenStepFormat;
            }

            CFDataRef outputData = CFPropertyListCreateData(kCFAllocatorSystemDefault, plist, format, 0, &error);
            
            if (!outputData) {
                logString(CFSTR("Coult not write property list: %@"), error);
                return(1);
            } else {
                success = writeDataToFile(outputData, nfile);
            }

            CFRelease(outputData);
        }

        CFRelease(plistdata);
    }

    return(success);
}

int main(int argc, const char *argv[])
{
    if (argc == 4) {
        return(plConvert(argv[2], argv[3], true));
    } else if (argc == 3) {
        return(plConvert(argv[1], argv[2], false));
    } else {
        logString(CFSTR("Not Enough Arguments. Usage: convert <plist> <converted>"));
        return(0);
    }

    return(plConvert(argv[1], argv[2], false));
}

