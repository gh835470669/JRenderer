#pragma once

#include <list>
#include <memory>
#include "jrenderer/tick_draw.h"

namespace jre
{
    class Scene;
    struct SceneTickContext
    {
        float delta_time;
        uint32_t cur_frame;
        Scene &scene;
    };

    class ISceneTicker
    {
    public:
        virtual ~ISceneTicker() = default;
        virtual void tick(SceneTickContext context) = 0;
    };

    class SceneTicker
    {
    public:
        std::list<std::shared_ptr<ISceneTicker>> tickers;
    };

    class SceneUBOTicker : public ISceneTicker
    {
    public:
        void tick(SceneTickContext context) override;
    };
}