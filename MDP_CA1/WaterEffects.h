#pragma once
#include <SFML/System/Clock.hpp>

#include "PostEffect.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"

class WaterEffects : public PostEffect
{
public:
    WaterEffects();
    virtual void Apply(const sf::RenderTexture& input, sf::RenderTarget& output);

private:
    ShaderHolder m_shaders;
    sf::Clock m_clock;
};
