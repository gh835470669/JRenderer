#include "jrenderer.h"

namespace jre
{
    JRenderer::~JRenderer()
    {
    }

    void JRenderer::Tick(const TickContext &context)
    {
    }

    void JRenderer::Draw(const DrawContext &context)
    {
        this->m_pipeline.Draw();
    }
} // namespace jre
