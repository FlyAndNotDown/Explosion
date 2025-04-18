//
// Created by johnk on 2024/8/21.
//

#pragma once

#include <string>
#include <unordered_set>

#include <Core/Module.h>
#include <Render/RenderModule.h>
#include <Runtime/Api.h>

namespace Runtime {
    class World;

    struct EngineInitParams {
        bool logToFile;
        std::string gameRoot;
        std::string rhiType;
    };

    class RUNTIME_API Engine { // NOLINT
    public:
        virtual ~Engine();

        virtual bool IsEditor() = 0;

        void MountWorld(World* inWorld);
        void UnmountWorld(World* inWorld);
        Render::RenderModule& GetRenderModule() const;
        void Tick(float inDeltaTimeSeconds);

    protected:
        explicit Engine(const EngineInitParams& inParams);

        void AttachLogFile() const;
        void InitRender(const std::string& inRhiTypeStr);
        void LoadPlugins() const;
        void LoadConfigs() const;

        std::unordered_set<World*> worlds;
        Render::RenderModule* renderModule;
        std::future<void> lastFrameRenderThreadFence;
        std::future<void> last2FrameRenderThreadFence;
    };

    class RUNTIME_API MinEngine final : public Engine {
    public:
        explicit MinEngine(const EngineInitParams& inParams);
        ~MinEngine() override;
        
        bool IsEditor() override;
    };

    struct RUNTIME_API EngineModule : Core::Module { // NOLINT
        virtual Engine* CreateEngine(const EngineInitParams& inParams) = 0;
    };

    class RUNTIME_API EngineHolder {
    public:
        static void Load(const std::string& inModuleName, const EngineInitParams& inInitParams);
        static void Unload();
        static Engine& Get();

    private:
        static Common::UniquePtr<Engine> engine;
    };
}
