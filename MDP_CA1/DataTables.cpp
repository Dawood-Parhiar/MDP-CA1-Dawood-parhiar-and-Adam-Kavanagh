#include "DataTables.hpp"
#include "ShipType.hpp"
#include "ProjectileType.hpp"
#include "PickupType.hpp"
#include "Ship.hpp"
#include "ParticleType.hpp"

std::vector<ShipData> InitializeShipData()
{
    std::vector<ShipData> data(static_cast<int>(ShipType::kAircraftCount));

    data[static_cast<int>(ShipType::kPirateShip)].m_hitpoints = 100;
    data[static_cast<int>(ShipType::kPirateShip)].m_speed = 200.f;
    data[static_cast<int>(ShipType::kPirateShip)].m_fire_interval = sf::seconds(1);
    data[static_cast<int>(ShipType::kPirateShip)].m_texture = TextureID::kPirateShip;
    //data[static_cast<int>(ShipType::kPirateShip)].m_texture_rect = sf::IntRect(0, 0, 48, 64);
    data[static_cast<int>(ShipType::kPirateShip)].m_has_roll_animation = false;

    //Raptor
    
    data[static_cast<int>(ShipType::kRaptor)].m_hitpoints = 20;
    data[static_cast<int>(ShipType::kRaptor)].m_speed = 80.f;
    data[static_cast<int>(ShipType::kRaptor)].m_fire_interval = sf::Time::Zero;
    data[static_cast<int>(ShipType::kRaptor)].m_texture = TextureID::kRaptor;
    //data[static_cast<int>(ShipType::kRaptor)].m_texture_rect = sf::IntRect(144, 0, 84, 64);
    data[static_cast<int>(ShipType::kRaptor)].m_has_roll_animation = false;

    //AI for Raptor
    data[static_cast<int>(ShipType::kRaptor)].m_directions.emplace_back(Direction(+45.f, 80.f));
    data[static_cast<int>(ShipType::kRaptor)].m_directions.emplace_back(Direction(-45.f, 160.f));
    data[static_cast<int>(ShipType::kRaptor)].m_directions.emplace_back(Direction(+45.f, 80.f));


    data[static_cast<int>(ShipType::kAvenger)].m_hitpoints = 40;
    data[static_cast<int>(ShipType::kAvenger)].m_speed = 50.f;
    data[static_cast<int>(ShipType::kAvenger)].m_fire_interval = sf::seconds(2);
    data[static_cast<int>(ShipType::kAvenger)].m_texture = TextureID::kAvenger;
    //data[static_cast<int>(ShipType::kAvenger)].m_texture_rect = sf::IntRect(228, 0, 60, 59);
    data[static_cast<int>(ShipType::kAvenger)].m_has_roll_animation = false;

    //AI for Raptor
    data[static_cast<int>(ShipType::kAvenger)].m_directions.emplace_back(Direction(+45.f, 50.f));
    data[static_cast<int>(ShipType::kAvenger)].m_directions.emplace_back(Direction(0.f, 50.f));
    data[static_cast<int>(ShipType::kAvenger)].m_directions.emplace_back(Direction(-45.f, 100.f));
    data[static_cast<int>(ShipType::kAvenger)].m_directions.emplace_back(Direction(0.f, 50.f));
    data[static_cast<int>(ShipType::kAvenger)].m_directions.emplace_back(Direction(45.f, 50.f));

    return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_damage = 10;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_speed = 300;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture_rect = sf::IntRect(175, 64, 3, 14);

    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_damage = 10;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_speed = 300;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture_rect = sf::IntRect(175, 64, 3, 14);

    //Cannon Ball
    data[static_cast<int>(ProjectileType::kMissile)].m_damage = 200;
    data[static_cast<int>(ProjectileType::kMissile)].m_speed = 150;
    data[static_cast<int>(ProjectileType::kMissile)].m_texture = TextureID::kMissile;
    //data[static_cast<int>(ProjectileType::kMissile)].m_texture_rect = sf::IntRect(160, 64, 15, 32);

    return data;
}

std::vector<PickupData> InitializePickupData()
{
    std::vector<PickupData> data(static_cast<int>(PickupType::kPickupCount));
    data[static_cast<int>(PickupType::kHealthRefill)].m_texture = TextureID::kEntities;
    data[static_cast<int>(PickupType::kHealthRefill)].m_texture_rect = sf::IntRect(0, 64, 40, 40);
    data[static_cast<int>(PickupType::kHealthRefill)].m_action = [](Ship& a)
        {
            a.Repair(25);
        };

    data[static_cast<int>(PickupType::kMissileRefill)].m_texture = TextureID::kEntities;
    data[static_cast<int>(PickupType::kMissileRefill)].m_texture_rect = sf::IntRect(40, 64, 40, 40);
    
    data[static_cast<int>(PickupType::kMissileRefill)].m_action = std::bind(&Ship::CollectMissile, std::placeholders::_1, 3);

    data[static_cast<int>(PickupType::kFireSpread)].m_texture = TextureID::kEntities;
    data[static_cast<int>(PickupType::kFireSpread)].m_texture_rect = sf::IntRect(80, 64, 40, 40);
    data[static_cast<int>(PickupType::kFireSpread)].m_action = std::bind(&Ship::IncreaseFireSpread, std::placeholders::_1);

    data[static_cast<int>(PickupType::kFireRate)].m_texture = TextureID::kEntities;
    data[static_cast<int>(PickupType::kFireRate)].m_texture_rect = sf::IntRect(120, 64, 40, 40);
    data[static_cast<int>(PickupType::kFireRate)].m_action = std::bind(&Ship::IncreaseFireRate, std::placeholders::_1);
    
    return data;
}

std::vector<ParticleData> InitializeParticleData()
{
    std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

    data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
    data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.5f);

    data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
    data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(2.5f);

    return data;
}
