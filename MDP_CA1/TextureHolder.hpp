#pragma once
#include "TextureID.hpp"
#include <string>
#include <SFML/Graphics.hpp>
#include <map>
#include <memory>

class TextureHolder
{
public:
    void Load(TextureID id, const std::string& filename);
    sf::Texture& Get(TextureID id);
    const sf::Texture& Get(TextureID id) const;
    void Unload(TextureID id); // Function to unload a texturesf::Texture& TextureHolder::Get(TextureID id)
{
    auto found = m_texture_map.find(id);
    if (found == m_texture_map.end())
    {
        throw std::runtime_error("TextureHolder::Get - Texture not found for the given ID");
    }
    return *found->second;
}

const sf::Texture& TextureHolder::Get(TextureID id) const
{
    auto found = m_texture_map.find(id);
    if (found == m_texture_map.end())
    {
        throw std::runtime_error("TextureHolder::Get - Texture not found for the given ID");
    }
    return *found->second;
}


private:
    std::map<TextureID, std::unique_ptr<sf::Texture>> m_texture_map;
};