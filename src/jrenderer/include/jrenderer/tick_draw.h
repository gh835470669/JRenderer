#pragma once

#include <utility>

namespace jre
{

    struct TickContext
    {
        float delta_time;
    };

    class ITickable
    {
    public:
        virtual ~ITickable() = default;
        virtual void tick(TickContext context) = 0;
    };

    class Tickable : public ITickable
    {
    public:
        bool enabled = true;
        virtual ~Tickable() = default;
        virtual void tick(TickContext context) override
        {
            std::exchange(enabled, preset_enabled(enabled));
            if (enabled)
            {
                on_tick(context);
            }
        }

        virtual inline bool preset_enabled(bool old_enabled) { return old_enabled; }
        virtual void on_tick(TickContext context) = 0;
    };

    class IDrawable
    {
    public:
        virtual ~IDrawable() = default;
        virtual void draw() = 0;
    };

    class Drawable : public IDrawable
    {
    public:
        bool visible = true;
        virtual ~Drawable() = default;
        virtual void draw() override
        {
            std::exchange(visible, preset_visible(visible));
            if (visible)
            {
                on_draw();
            }
        }
        virtual inline bool preset_visible(bool old_visible) { return old_visible; }
        virtual void on_draw() = 0;
    };

}