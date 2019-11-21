/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include <Cocoa/Cocoa.h>

#import "android/skin/qt/mac-native-window.h"

void* getNSWindow(void* ns_view) {
    NSView *view = (NSView *)ns_view;
    if (!view) {
        return NULL;
    }
    Class viewClass = [view class];
    Class nsviewClass = [NSView class];
    if ([viewClass isSubclassOfClass:nsviewClass ]) {
        return [view window];
    } else {
        return view;
    }

}

void nsWindowHideWindowButtons(void* ns_window) {
    NSWindow *window = (NSWindow *)ns_window;
    if (!window) {
        return;
    }
    [[window standardWindowButton:NSWindowCloseButton] setHidden:true];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:true];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:true];
    [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
}

int numHeldMouseButtons() {
    return (int) [NSEvent pressedMouseButtons];
}

void nsWindowAdopt(void *ns_parent, void *ns_child) {
    NSWindow *parent = (NSWindow*)ns_parent, *child = (NSWindow*)ns_child;
    [parent addChildWindow:child ordered:NSWindowAbove];
}
