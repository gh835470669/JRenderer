#pragma once

namespace jre
{
    namespace imgui
    {
        class ImguiWindow
        {
        public:
            virtual void Tick() {};
            virtual ~ImguiWindow() = default;
        };
    }
}
