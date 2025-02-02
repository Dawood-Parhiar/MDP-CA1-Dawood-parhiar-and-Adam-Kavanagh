#include "WaterEffects.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

WaterEffects::WaterEffects()
{
    //shader frpm Chatgpt to simulate water waves on the screen
    m_shaders.Load(ShaderTypes::kWaterShader, "Media/Shaders/Fullpass.vert", "Media/Shaders/Water.frag");
}

void WaterEffects::Apply(const sf::RenderTexture& input, sf::RenderTarget& output)
{
    PrepareTexture(input.getSize());
    sf::Shader& waterShader = m_shaders.Get(ShaderTypes::kWaterShader);

    waterShader.setUniform("time", m_clock.getElapsedTime().asSeconds());
    waterShader.setUniform("resolution", sf::Vector2f(input.getSize()));
    waterShader.setUniform("texture", input.getTexture());

	ApplyShader(waterShader, output);
    
    
}

void WaterEffects::PrepareTexture(sf::Vector2u size)
{
    if (m_water_texture.getSize() != size)
    {
	    	m_water_texture.create(size.x, size.y);
			m_water_texture.setSmooth(true);

            m_firstpass_textures[0].create(size.x / 2, size.y / 2);
            m_firstpass_textures[0].setSmooth(true);
            m_firstpass_textures[1].create(size.x / 2, size.y / 2);
            m_firstpass_textures[1].setSmooth(true);

            m_secondpass_textures[0].create(size.x / 4, size.y / 4);
            m_secondpass_textures[0].setSmooth(true);
            m_secondpass_textures[1].create(size.x / 4, size.y / 4);
            m_secondpass_textures[1].setSmooth(true);
    }
}



