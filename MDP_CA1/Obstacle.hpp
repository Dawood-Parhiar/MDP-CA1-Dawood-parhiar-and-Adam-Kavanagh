#pragma once
#include "Entity.hpp"
#include "SpriteNode.hpp"
#include "TextureID.hpp"

class Obstacle: public Entity
{
public:
	explicit Obstacle(const sf::Texture& texture);
	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	sf::Sprite m_sprite;
	

};

