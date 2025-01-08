#include "WaterEffects.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

WaterEffects::WaterEffects()
{
    m_shaders.Load(ShaderTypes::kWaterShader, "Media/Shaders/Fullpass.vert", "Media/Shaders/Water.frag");
}

void WaterEffects::Apply(const sf::RenderTexture& input, sf::RenderTarget& output)
{
    sf::Shader& waterShader = m_shaders.Get(ShaderTypes::kWaterShader);

    waterShader.setUniform("time", m_clock.getElapsedTime().asSeconds());
    waterShader.setUniform("resolution", sf::Vector2f(input.getSize()));
    waterShader.setUniform("texture", input.getTexture());

    ApplyShader(waterShader, output);
    
}