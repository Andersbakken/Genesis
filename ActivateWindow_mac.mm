#include "ActivateWindow.h"
#include <Foundation/NSAppleScript.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSString.h>
#include <Foundation/NSAutoreleasePool.h>

ScriptWrapper ScriptCompiler::platformRun()
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSAppleScript* script = [[NSAppleScript alloc] initWithSource:
        @"tell application \"System Events\"\n"
        @"  set frontmost of the first process whose frontmost is true to true\n"
        @"end tell\n"];
    NSDictionary* error = [[[NSDictionary alloc] init] autorelease];
    [script compileAndReturnError:&error];

    ScriptWrapper wrapper;
    wrapper.script = script;

    [pool drain];

    return wrapper;
}

void PreviousProcessPrivate::platformDestructor()
{
    if (script) {
        NSAppleScript* nsscript = reinterpret_cast<NSAppleScript*>(script);
        [nsscript release];
    }
}

void PreviousProcessPrivate::platformSetScript(const ScriptWrapper &wrapper)
{
    if (script) {
        NSAppleScript* nsscript = reinterpret_cast<NSAppleScript*>(script);
        [nsscript release];
    }
    script = wrapper.script;
}

void PreviousProcess::activate()
{
    NSAppleScript* script;

    if (!priv->script) {
        script = [[NSAppleScript alloc] initWithSource:
            @"tell application \"System Events\"\n"
            @"  set frontmost of the first process whose frontmost is true to true\n"
            @"end tell\n"];
        priv->script = script;
    } else
        script = reinterpret_cast<NSAppleScript*>(priv->script);

    NSDictionary* error = [[[NSDictionary alloc] init] autorelease];
    [script executeAndReturnError:&error];
}
