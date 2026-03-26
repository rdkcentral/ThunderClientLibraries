/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"

#include "TextureBounce.h"
#include "IModel.h"
#include "TerminalInput.h"
#include "Renderer.h"

#include <core/core.h>

#include <localtracer/localtracer.h>

#include <cstdio>
#include <unistd.h>
#include <set>

using namespace Thunder;

namespace {
const char Namespace[] = EXPAND_AND_QUOTE(NAMESPACE);
}

class ConsoleOptions : public Thunder::Core::Options {
public:
    ConsoleOptions(int argc, TCHAR* argv[])
        : Thunder::Core::Options(argc, argv, _T("t:n:W:H:h"))
        , Texture("/usr/share/" + std::string(Namespace) + "/ClientCompositorRender/ml-tv-color-small.png")
        , Width(1920)
        , Height(1080)
    {
        Parse();
    }

    std::string Texture;
    uint8_t TextureNumber;
    uint16_t Width;
    uint16_t Height;

private:
    void Option(const TCHAR option, const TCHAR* argument) override
    {
        switch (option) {
        case 't':
            Texture = argument;
            break;
        case 'n':
            TextureNumber = static_cast<uint8_t>(std::stoi(argument));
            break;
        case 'W':
            Width = static_cast<uint16_t>(std::stoi(argument));
            break;
        case 'H':
            Height = static_cast<uint16_t>(std::stoi(argument));
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage: " EXPAND_AND_QUOTE(APPLICATION_NAME) " [-t <Texture.png>] [-n 40] [-W 1280] [-H 720]\n");
            exit(EXIT_FAILURE);
        }
    }
};

class Tracer {
public:
    struct ModuleConfig {
        std::string Module;
        std::set<std::string> EnabledCategories;
    };

    Tracer()
        : _tracer(Messaging::LocalTracer::Open())
        , _printer(true)
        , _modules()
    {
        _tracer.Callback(&_printer);
    }

    ~Tracer()
    {
        _tracer.Close();
    }

    // Simple enable/disable
    void Set(const std::string& module, const std::string& category, bool enable)
    {
        if (enable) {
            _modules[module].EnabledCategories.insert(category);
        } else {
            _modules[module].EnabledCategories.erase(category);
        }

        _tracer.EnableMessage(module, category, enable);
    }

    // Bulk configure
    void Configure(const std::string& module, const std::vector<std::string>& categories)
    {
        for (const auto& category : categories) {
            Set(module, category, true);
        }
    }

    // Enable ALL discovered categories for a module
    void EnableAll(const std::string& module)
    {
        auto categories = DiscoverCategories(module);
        for (const auto& category : categories) {
            Set(module, category, true);
        }
    }

    // Auto-discover available modules
    std::vector<std::string> DiscoverModules()
    {
        std::vector<std::string> modules;
        
        class ModuleHandler : public Core::Messaging::IControl::IHandler {
        public:
            ModuleHandler(std::vector<std::string>& modules) : _modules(modules) {}
            
            void Handle(Core::Messaging::IControl* control) override {
                const string& module = control->Metadata().Module();
                if (std::find(_modules.begin(), _modules.end(), module) == _modules.end()) {
                    _modules.push_back(module);
                }
            }
        private:
            std::vector<std::string>& _modules;
        } handler(modules);
        
        Core::Messaging::IControl::Iterate(handler);
        
        return modules;
    }

    // Auto-discover categories for a module
    std::vector<std::string> DiscoverCategories(const std::string& module)
    {
        std::vector<std::string> categories;
        
        class CategoryHandler : public Core::Messaging::IControl::IHandler {
        public:
            CategoryHandler(const std::string& module, std::vector<std::string>& categories)
                : _module(module), _categories(categories) {}
            
            void Handle(Core::Messaging::IControl* control) override {
                if (control->Metadata().Module() == _module) {
                    const string& category = control->Metadata().Category();
                    if (!category.empty()) {
                        _categories.push_back(category);
                    }
                }
            }
        private:
            const std::string& _module;
            std::vector<std::string>& _categories;
        } handler(module, categories);
        
        Core::Messaging::IControl::Iterate(handler);
        
        return categories;
    }

    // Fully dynamic menu
    void Menu(Compositor::TerminalInput& keyboard, uint32_t timeoutSeconds = 30)
    {
        auto availableModules = DiscoverModules();

        printf("\n");
        printf("=== Trace Configuration Menu ===\n");
        printf("Discovered %zu modules\n", availableModules.size());
        printf("Timeout: %us\n", timeoutSeconds);
        printf("\n");
        printf("Commands:\n");
        printf("  [1-9] - Select module\n");
        printf("  D - Refresh module list\n");
        printf("  Q - Exit menu\n");
        printf("\n");

        DisplayModuleList(availableModules);

        std::string selectedModule;
        std::vector<std::string> selectedCategories;
        auto lastActivity = std::chrono::steady_clock::now();

        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();

            if (elapsed >= timeoutSeconds) {
                printf("Menu timeout\n");
                break;
            }

            char key = keyboard.Read();
            if (key != 0) {
                lastActivity = std::chrono::steady_clock::now();
                char upper = toupper(key);

                if (upper == 'Q') {
                    break;
                }

                if (upper == 'D') {
                    availableModules = DiscoverModules();
                    DisplayModuleList(availableModules);
                    selectedModule.clear();
                    continue;
                }

                if (selectedModule.empty()) {
                    // Module selection
                    if (key >= '1' && key <= '9') {
                        size_t idx = static_cast<size_t>(key - '1');
                        if (idx < availableModules.size()) {
                            selectedModule = availableModules[idx];
                            selectedCategories = DiscoverCategories(selectedModule);

                            printf("\n");
                            printf("Module: %s\n", selectedModule.c_str());
                            printf("\n");
                            printf("Commands:\n");
                            printf("  [1-9] - Toggle category\n");
                            printf("  A - Enable all\n");
                            printf("  O - Disable all\n");
                            printf("  B - Back to modules\n");
                            printf("\n");

                            DisplayCategoryList(selectedModule, selectedCategories);
                        }
                    }
                } else {
                    // Category toggle mode
                    if (upper == 'B') {
                        selectedModule.clear();
                        DisplayModuleList(availableModules);
                    } else if (upper == 'A') {
                        for (const auto& cat : selectedCategories) {
                            Set(selectedModule, cat, true);
                        }
                        DisplayCategoryList(selectedModule, selectedCategories);
                    } else if (upper == 'O') {
                        for (const auto& cat : selectedCategories) {
                            Set(selectedModule, cat, false);
                        }
                        DisplayCategoryList(selectedModule, selectedCategories);
                    } else if (key >= '1' && key <= '9') {
                        size_t idx = static_cast<size_t>(key - '1');
                        if (idx < selectedCategories.size()) {
                            const auto& category = selectedCategories[idx];
                            bool currentlyEnabled = IsEnabled(selectedModule, category);
                            Set(selectedModule, category, !currentlyEnabled);
                            DisplayCategoryList(selectedModule, selectedCategories);
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    bool IsEnabled(const std::string& module, const std::string& category) const
    {
        auto it = _modules.find(module);
        if (it != _modules.end()) {
            return it->second.EnabledCategories.count(category) > 0;
        }
        return false;
    }

    void DisplayModuleList(const std::vector<std::string>& modules) const
    {
        printf("Available modules:\n");
        for (size_t i = 0; i < modules.size() && i < 9; ++i) {
            bool configured = (_modules.find(modules[i]) != _modules.end());
            printf("  [%zu] %s%s\n", i + 1, modules[i].c_str(), configured ? " *" : "");
        }
    }

    void DisplayCategoryList(const std::string& module,
        const std::vector<std::string>& categories) const
    {
        printf("Categories for %s:\n", module.c_str());
        for (size_t i = 0; i < categories.size() && i < 9; ++i) {
            bool enabled = IsEnabled(module, categories[i]);
            printf("  [%zu] %s: %s\n", i + 1, categories[i].c_str(), enabled ? "ON" : "OFF");
        }
    }

    Messaging::LocalTracer& _tracer;
    Messaging::ConsolePrinter _printer;
    std::map<std::string, ModuleConfig> _modules;
};

int main(int argc, char* argv[])
{
    const char* executableName(Thunder::Core::FileNameOnly(argv[0]));
    int exitCode(0);

    {
        Tracer tracer;
        tracer.Configure("App_CompositionClientRender", { "Information", "Error", "Warning" });
        tracer.Configure("Common_CompositionClientRender", { "Error", "Warning" });

        ConsoleOptions options(argc, argv);
        bool quitApp(false);

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        std::string texturePath = options.Texture;

        if (texturePath.empty()) {
            texturePath = "/usr/share/" + std::string(Namespace) + "/ClientCompositorRender/ml-tv-color-small.png";
        }

        Compositor::TextureBounce::Config config;
        config.Image = texturePath;
        config.ImageCount = options.TextureNumber;

        std::string configStr;
        config.ToString(configStr);

        Compositor::Render renderer;
        Compositor::TextureBounce model;

        if (renderer.Configure(options.Width, options.Height) == false) {
            TRACE_GLOBAL(Trace::Error, ("Failed to initialize renderer"));
            exitCode = 1;
        }

        if (!renderer.Register(&model, configStr)) {
            TRACE_GLOBAL(Trace::Error, ("Failed to initialize model"));
            exitCode = 2;
        }

        if (exitCode == 0) {
            Compositor::TerminalInput keyboard;
            ASSERT(keyboard.IsValid() == true);

            renderer.Start();

            bool result;

            if (keyboard.IsValid() == true) {
                while (!renderer.ShouldExit() && !quitApp) {
                    switch (toupper(keyboard.Read())) {
                    case 'S':
                        if (renderer.ShouldExit() == false) {
                            (renderer.IsRunning() == false) ? renderer.Start() : renderer.Stop();
                        }
                        break;
                    case 'F':
                        result = renderer.ToggleFPS();
                        TRACE_GLOBAL(Trace::Information, ("FPS: %s", result ? "on" : "off"));
                        break;
                    case 'Z':
                        result = renderer.ToggleRequestRender();
                        TRACE_GLOBAL(Trace::Information, ("RequestRender: %s", result ? "off" : "on"));
                        break;
                    case 'R':
                        renderer.TriggerRender();
                        break;
                    case 'M':
                        result = renderer.ToggleModelRender();
                        TRACE_GLOBAL(Trace::Information, ("Model Render: %s", result ? "off" : "on"));
                        break;
                    case 'T':
                        tracer.Menu(keyboard, 30); // 30 second timeout
                        TRACE_GLOBAL(Trace::Information, ("Returning to main menu"));
                        break;
                    case 'Q':
                        quitApp = true;
                        break;
                    case 'H':
                        TRACE_GLOBAL(Trace::Information, ("Available commands:"));
                        TRACE_GLOBAL(Trace::Information, ("  S - Start/Stop rendering loop"));
                        TRACE_GLOBAL(Trace::Information, ("  F - Toggle FPS display overlay"));
                        TRACE_GLOBAL(Trace::Information, ("  Z - Toggle surface RequestRender calls"));
                        TRACE_GLOBAL(Trace::Information, ("  R - Trigger single render request"));
                        TRACE_GLOBAL(Trace::Information, ("  M - Toggle model Draw calls"));
                        TRACE_GLOBAL(Trace::Information, ("  T - Trace configuration menu"));
                        TRACE_GLOBAL(Trace::Information, ("  Q - Quit application"));
                        TRACE_GLOBAL(Trace::Information, ("  H - Show this help"));
                        break;
                    default:
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } else {
                TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to initialize keyboard input"));
            }

            renderer.Stop();
            TRACE_GLOBAL(Thunder::Trace::Information, ("Exiting %s.... ", executableName));
        }
    }

    Core::Singleton::Dispose();

    return exitCode;
}
