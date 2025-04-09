#import "ViewController.h"

// Forward declarations for plugin functions - these should match the exported functions
// from the PluginGfxe.cpp file
extern int luaopen_plugin_gfxe(void *L);

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Demo code to show plugin is loaded
    NSLog(@"Graphics Extensions plugin is ready to be used");
}

@end
