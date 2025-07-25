#import <Cocoa/Cocoa.h>

// Declare your C++ function
extern "C" void runmain(const char* imagePath);

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    runmain(nullptr);  // If no file is passed, just start the app
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
    const char* path = [filename UTF8String];
    runmain(path);  // Call your C++ icon maker logic
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        return NSApplicationMain(argc, argv);
    }
}
