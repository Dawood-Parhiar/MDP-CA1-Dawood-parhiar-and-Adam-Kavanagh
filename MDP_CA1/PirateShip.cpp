#include "PirateShip.hpp"

#include "DataTables.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"

namespace 
{
    const std::vector<PirateShipData> Table = InitializePirateShipData();
}

TextureID ToTextureID(ShipType type)
{
    switch (type)
    {
    case ShipType::kPirateShip:
        return TextureID::KPirate;
        break;
    case ShipType::KEnemyShipOne:
        return TextureID::kEnemy;
        break;
    case ShipType::KEnemyShipTwo:
        return TextureID::kEnemyTwo;
        break;
    }
    return TextureID::KPirate;
}

PirateShip::PirateShip(ShipType type, const TextureHolder& textures, const FontHolder& fonts)
:Entity(GetHitPoints()),
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

    m_missile_command.category = static_cast<int>(ReceiverCategories::kScene);
    m_missile_command.action = [this, &textures](SceneNode& node, sf::Time dt)
    {
        CreateProjectile(node, ProjectileType::kMissile, 0.f, 0.5f, textures);
    };

    m_drop_pickup_command.category = static_cast<int>(ReceiverCategories::kScene);
    m_drop_pickup_command.action = [this, &textures](SceneNode& node, sf::Time dt)
    {
        CreatePickup(node, textures);
    };

    std::string* health = new std::string("");
    std::unique_ptr<TextNode> health_display(new TextNode(fonts, *health));
    m_health_display = health_display.get();
    AttachChild(std::move(health_display));

    if (PirateShip::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerShip))
    {
        std::string* missile_ammo = new std::string("");
        std::unique_ptr<TextNode> missile_display(new TextNode(fonts, *missile_ammo));
        m_missile_display = missile_display.get();
        AttachChild(std::move(missile_display));
    }

}

unsigned int PirateShip::GetCategory() const
{
    return Entity::GetCategory();
}

void PirateShip::CollectMissile(unsigned int count)
{
    m_missile_ammo += count;
}

void PirateShip::UpdateTexts()
{
    m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
    m_health_display->setPosition(0.f, 50.f);
    m_health_display->setRotation(-getRotation());

    if (m_missile_display)
    {
        m_missile_display->setPosition(0.f, 70.f);
        if (m_missile_ammo == 0)
        {
            m_missile_display->SetString("");
        }
        else
        {
            m_missile_display->SetString("M: " + std::to_string(m_missile_ammo));
        }
    }
}

void PirateShip::UpdateMovementPattern(sf::Time dt)
{
    //Ship AI
    const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
    if (!directions.empty())
    {
        // Move along the current direction, then change direction
        if (m_distance_travelled > directions[m_directions_index].m_distance)
        {
            m_directions_index = (m_directions_index + 1) % directions.size();
            m_distance_travelled = 0.f;
        }

        // Compute velocity
        // Add 90 to move down the screen, 0 is right

        double radians = Utility::ToRadians(directions[m_directions_index].m_angle);
        float vx = GetMaxSpeed() * std::cos(radians);
        float vy = GetMaxSpeed() * std::sin(radians);

        SetVelocity(vx, vy);
        m_distance_travelled += GetMaxSpeed() * dt.asSeconds();
    }
}

float PirateShip::GetMaxSpeed()
{
    return Table[static_cast<int>(m_type)].m_speed;


}

void PirateShip::LaunchMissile()
{
    if (m_missile_ammo > 0)
    {
        m_is_launching_missile = true;
        --m_missile_ammo;
    }
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
    return IsDestroyed(); //&& (m_explosion.IsFinished() || !m_show_explosion);

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
