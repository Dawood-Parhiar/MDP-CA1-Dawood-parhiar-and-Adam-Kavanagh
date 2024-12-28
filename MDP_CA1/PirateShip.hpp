#pragma once
#include "Entity.hpp"
#include "ProjectileType.hpp"
#include "ResourceIdentifiers.hpp"
#include "ShipType.hpp"
#include "TextNode.hpp"

class PirateShip : public Entity
{
public:
    PirateShip(ShipType type,const TextureHolder& textures, const FontHolder& fonts);

	unsigned int GetCategory() const override;
	
    void CollectMissile(unsigned int count);

    void UpdateTexts();
    void UpdateMovementPattern(sf::Time dt);

    float GetMaxSpeed();
    void LaunchMissile();
    void CreateProjectile(SceneNode& node, ProjectileType type, float x_float, float y_offset, const TextureHolder& textures) const;

    sf::FloatRect GetBoundingRect() const override;
    bool IsMarkedForRemoval() const override;

    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
    void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
    void CreatePickup(SceneNode& node, const TextureHolder& textures) const;
    void CheckPickupDrop(CommandQueue& commands);

private:
    ShipType m_type;
    sf::Sprite m_sprite;
    TextNode* m_health_display;
    TextNode* m_missile_display;
    float m_distance_travelled;
    int m_directions_index;

    Command m_fire_command;
    Command m_missile_command;
    Command m_drop_pickup_command;
	
    //unsigned int m_fire_rate;
    unsigned int m_missile_ammo;
    unsigned int m_spread_level;
	
    //bool m_is_firing;
    bool m_is_launching_missile;
    sf::Time m_fire_countdown;

    bool m_is_marked_for_removal;
	
};
