#pragma once

#include "Entity.hpp"
#include "ProjectileType.hpp"
#include "Projectile.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"


//Dawood Parhiar D00248313
class Cannon : public Entity
{
public:
	Cannon(const TextureHolder& textures);

    void SetRotationInput(float rotation);
	sf::Vector2f GetMouthPosition() const;
    //void CreateProjectile(SceneNode& node, ProjectileType type,const TextureHolder& textures) const;

    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
    
private:
    sf::Sprite m_sprite;
    float m_rotationSpeed;
    float m_rotationInput;
};

