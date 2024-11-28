#pragma once
#include "TextureID.hpp"
#include "FontID.hpp"

namespace sf
{
	class Texture;
	class Font;
}

template<typename Identifier, typename Resource>
class ResourceHolder;

typedef ResourceHolder<TextureID, sf::Texture> TextureHolder;
typedef ResourceHolder <FontID, sf::Font > FontHolder;