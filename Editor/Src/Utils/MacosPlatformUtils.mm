//
// Created by johnk on 2026/7/11.
//

#include <Editor/Utils/PlatformUtils.h>

#if PLATFORM_MACOS

#import <Cocoa/Cocoa.h>

namespace Editor {
    std::optional<std::string> PlatformUtils::SelectDirectory(const std::string& inTitle, const std::string& inInitialDirectory)
    {
        @autoreleasepool {
            NSOpenPanel* const panel = [NSOpenPanel openPanel];
            panel.title = [NSString stringWithUTF8String:inTitle.c_str()];
            panel.canChooseFiles = NO;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = NO;
            panel.canCreateDirectories = YES;

            if (!inInitialDirectory.empty()) {
                NSString* const initialPath = [NSString stringWithUTF8String:inInitialDirectory.c_str()];
                panel.directoryURL = [NSURL fileURLWithPath:initialPath isDirectory:YES];
            }

            if ([panel runModal] != NSModalResponseOK) {
                return std::nullopt;
            }
            return std::string(panel.URL.path.fileSystemRepresentation);
        }
    }

    std::vector<std::string> PlatformUtils::SelectFiles(const std::string& inTitle, const std::string& inInitialDirectory)
    {
        @autoreleasepool {
            NSOpenPanel* const panel = [NSOpenPanel openPanel];
            panel.title = [NSString stringWithUTF8String:inTitle.c_str()];
            panel.canChooseFiles = YES;
            panel.canChooseDirectories = NO;
            panel.allowsMultipleSelection = YES;

            if (!inInitialDirectory.empty()) {
                NSString* const initialPath = [NSString stringWithUTF8String:inInitialDirectory.c_str()];
                panel.directoryURL = [NSURL fileURLWithPath:initialPath isDirectory:YES];
            }

            if ([panel runModal] != NSModalResponseOK) {
                return {};
            }

            std::vector<std::string> result;
            for (NSURL* url in panel.URLs) {
                result.emplace_back(url.path.fileSystemRepresentation);
            }
            return result;
        }
    }
}

#endif
