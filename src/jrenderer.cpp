#include "jrenderer.h"    

JRenderer::~JRenderer()
{

}

void JRenderer::main_loop()
{
    this->tick();
    this->draw();
}

void JRenderer::tick()
{

}

void JRenderer::draw()
{
    this->pipeline.draw();
}
