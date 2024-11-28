#include "PirateShip.hpp"

#include "ResourceHolder.hpp"
#include "Utility.hpp"

TextureID ToTextureID(ShipType type)
{
    switch (type)
    {
    case ShipType::kPirateShip:
        return TextureID::KPirate;
        break;
    case ShipType::KEnemyShip:
        return TextureID::kEnemy;
        break;
    }
    return TextureID::KPirate;
}

PirateShip::PirateShip(ShipType type, const TextureHolder& textures, const FontHolder& fonts)
:
m_type(type),
m_sprite(textures.Get(ToTextureID(type)))
, m_health_display(nullptr)
    , m_missile_display(nullptr)
    , m_distance_travelled(0.f)
    , m_directions_index(0)
    , m_spread_level(1)
    , m_is_launching_missile(false)
    , m_fire_countdown(sf::Time::Zero)
    , m_missile_ammo(2)
    , m_is_marked_for_removal(false)
{
    Utility::CentreOrigin(m_sprite);

    
}

void PirateShip::CollectMissile()
{
}

void PirateShip::UpdateTexts()
{
}

void PirateShip::UpdateMovementPattern()
{
}

float PirateShip::GetMaxSpeed()
{
}

void PirateShip::LaunchMissile()
{
}

void PirateShip::CreateProjectile(SceneNode& node, ProjectileType type, float x_float, float y_offset,
    const TextureHolder& textures) const
{
}

sf::FloatRect PirateShip::GetBoundingRect() const
{
    return Entity::GetBoundingRect();
}

bool PirateShip::IsMarkedForRemoval() const
{
}

void PirateShip::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
}

void PirateShip::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    Entity::UpdateCurrent(dt, commands);
}

void PirateShip::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
}

void PirateShip::CreatePickup(SceneNode& node, const TextureHolder& textures) const
{
}

void PirateShip::CheckPickupDrop(CommandQueue& commands)
{
}
