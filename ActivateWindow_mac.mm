#include <Foundation/NSAppleScript.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSString.h>
#include <string>

static std::string previous;

namespace PreviousProcess {

void clear()
{
    previous.clear();
}

void record()
{
    previous.clear();

    static NSAppleScript* script = 0;
    if (!script) {
        script = [[NSAppleScript alloc] initWithSource:
            @"tell application \"System Events\"\n"
            @"  get name of the first process whose frontmost is true\n"
            @"end tell\n"];
    }

    NSDictionary* error = [[[NSDictionary alloc] init] autorelease];
    NSAppleEventDescriptor* desc = [script executeAndReturnError:&error];

    NSString* processName = [desc stringValue];
    if (!processName || [processName length] == 0)
        return;
    previous = std::string([processName UTF8String]);
}

void activate()
{
    if (previous.length() == 0)
        return;

    NSString* stringscript =
        @"tell application \"System Events\"\n"
        @"  set frontmost of process \"";
    stringscript = [stringscript stringByAppendingString:[[NSString stringWithUTF8String:previous.c_str()] autorelease]];
    stringscript = [stringscript stringByAppendingString:
        @"\" to true\n"
        @"end tell\n"];

    NSAppleScript* script = [[[NSAppleScript alloc] initWithSource:stringscript] autorelease];

    NSDictionary* error = [[[NSDictionary alloc] init] autorelease];
    [script executeAndReturnError:&error];

    previous.clear();
}

} // namespace
