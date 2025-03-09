//#pragma once
//#include <SFML/Graphics/RenderTexture.hpp>
//#include <SFML/System/Clock.hpp>
//
//#include "PostEffect.hpp"
//#include "ResourceHolder.hpp"
//#include "ResourceIdentifiers.hpp"
//
//#include <array>
//
//#include "SceneLayers.hpp"
//
//class WaterEffects : public PostEffect
//{
//public:
//    WaterEffects();
//    virtual void Apply(const sf::RenderTexture& input, sf::RenderTarget& output) override;
//    void PrepareTexture(sf::Vector2u size);
//   
//
//private:
//    typedef std::array<sf::RenderTexture, 2> RenderTextureArray;
//
//private:
//    ShaderHolder m_shaders;
//    sf::Clock m_clock;
//    sf::RenderTexture	m_water_texture;
//    RenderTextureArray	m_firstpass_textures;
//    RenderTextureArray	m_secondpass_textures;
//};
