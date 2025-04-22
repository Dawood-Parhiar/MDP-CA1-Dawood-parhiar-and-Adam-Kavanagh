#include "TextureHolder.hpp"  // For TextureHolder class
#include "TextureID.hpp"      // For TextureID enumeration
#include <stdexcept>          // For std::runtime_error
#include <memory>             // For std::unique_ptr
#include <iostream>           // For std::cerr
#include "ResourceIdentifiers.hpp"

void TextureHolder::Load(TextureID id, const std::string& filename)
{
    // Create a new texture
    auto texture = std::make_unique<sf::Texture>();

    // Attempt to load the texture from the file
    if (!texture->loadFromFile(filename))
    {
        throw std::runtime_error("TextureHolder::Load - Failed to load " + filename);
    }

    // Insert the texture into the map
    m_texture_map[id] = std::move(texture);
}

sf::Texture& TextureHolder::Get(TextureID id) const
{
    // Find the texture in the map
    auto found = m_texture_map.find(id);
    if (found == m_texture_map.end())
    {
        throw std::runtime_error("TextureHolder::Get - Texture not found for the given ID");
    }

    // Return a reference to the texture
    return *found->second;
}