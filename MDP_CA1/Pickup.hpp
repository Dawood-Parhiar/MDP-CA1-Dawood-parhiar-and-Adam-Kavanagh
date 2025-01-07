#pragma once
#include "Entity.hpp"
#include "PickupType.hpp"
#include "ResourceIdentifiers.hpp"

class Ship;

class Pickup : public Entity
{
public:
	Pickup(PickupType type, const TextureHolder& textures);
	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const;
	void Apply(Ship& player) const;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	PickupType m_type;
	sf::Sprite m_sprite;
};

