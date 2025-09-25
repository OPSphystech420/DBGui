#pragma once

#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
    #import <Cocoa/Cocoa.h>
#else
    #import <UIKit/UIKit.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui.h"
#include "Fonts.h"

#import <MariaDBKit/MariaDBKit.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <cstring>

extern ImFont* FiraCode;
extern ImFont* TabsFont;
