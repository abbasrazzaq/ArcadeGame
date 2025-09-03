//
//  AppDelegate.m
//  macosx_twosome
//
//  Created by admin on 21/06/2018.
//  Copyright © 2018 Abbas Razzaq. All rights reserved.
//

#import "AppDelegate.h"
#import "../game_view.h"

@interface AppDelegate ()

@property IBOutlet NSWindow *window;
//@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
    // Insert code here to initialize your application
    
    _window.acceptsMouseMovedEvents = YES;
}

- (void)applicationDidBecomeActive:(NSNotification *)notification 
{
    Game_View *view = (Game_View *)_window.firstResponder;
    
    [view app_activeness_changed:true];
    
    // TODO: Log
    
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    Game_View *view = (Game_View *)_window.firstResponder;
    [view app_activeness_changed:false];
    
    // TODO: Log
}

- (void)applicationWillTerminate:(NSNotification *)aNotification 
{
    // Insert code here to tear down your application
    // TODO:
}

@end
