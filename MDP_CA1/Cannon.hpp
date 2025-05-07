#pragma once

#include "Entity.hpp"
#include "ProjectileType.hpp"
#include "Projectile.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"
#include "TextNode.hpp"


//Dawood Parhiar D00248313
class Cannon : public Entity
{
public:
	Cannon(const TextureHolder& textures);

    void SetRotationInput(float rotation);
	sf::Vector2f GetMouthPosition() const;
    sf::Transform GetPosition() const { return m_sprite.getTransform(); }

    float GetRotation() const;
    void  SetRotation(float angle);


    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
    void SetPlayerName(std::string& name, const FontHolder& fonts);


private:
    
    float m_rotationSpeed;
    float m_rotationInput;
    sf::Sprite m_sprite;
    TextNode* m_name_display = nullptr;
};

