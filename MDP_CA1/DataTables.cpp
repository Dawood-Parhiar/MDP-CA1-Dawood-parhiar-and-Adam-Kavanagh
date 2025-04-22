#include "DataTables.hpp"
#include "ShipType.hpp"
#include "ProjectileType.hpp"
#include "PickupType.hpp"
#include "Ship.hpp"
#include "ParticleType.hpp"

std::vector<ShipData> InitializeShipData() //Code updated by Adam Kavanagh 
{
    std::vector<ShipData> data(static_cast<int>(ShipType::kShipCount));

	// Pirate Ship
    data[static_cast<int>(ShipType::kPirateShip)].m_hitpoints = 200;
    data[static_cast<int>(ShipType::kPirateShip)].m_speed = 100.f;
    data[static_cast<int>(ShipType::kPirateShip)].m_fire_interval = sf::seconds(3);
    data[static_cast<int>(ShipType::kPirateShip)].m_texture = TextureID::kPirateShip;

    // Stationary Ship
    data[static_cast<int>(ShipType::kStationaryShip)].m_hitpoints = 200;
    data[static_cast<int>(ShipType::kStationaryShip)].m_speed = 0.f;
    data[static_cast<int>(ShipType::kStationaryShip)].m_fire_interval = sf::seconds(3);
    data[static_cast<int>(ShipType::kStationaryShip)].m_texture = TextureID::kPirateShip;
    data[static_cast<int>(ShipType::kStationaryShip)].m_has_roll_animation = false;

    // Rammming Ship

    data[static_cast<int>(ShipType::kRammingShip)].m_hitpoints = 20;
    data[static_cast<int>(ShipType::kRammingShip)].m_speed = 100.f;
    data[static_cast<int>(ShipType::kRammingShip)].m_fire_interval = sf::seconds(0);
    data[static_cast<int>(ShipType::kRammingShip)].m_texture = TextureID::kPirateShip;
    data[static_cast<int>(ShipType::kRammingShip)].m_has_roll_animation = false;

    // Fortress

    data[static_cast<int>(ShipType::kFortress)].m_hitpoints = 300;
    data[static_cast<int>(ShipType::kFortress)].m_speed = 0.f; // No movement
    data[static_cast<int>(ShipType::kFortress)].m_texture = TextureID::kFortress;
    data[static_cast<int>(ShipType::kFortress)].m_fire_interval = sf::seconds(1); // Fires at the player
    data[static_cast<int>(ShipType::kFortress)].m_has_roll_animation = false;

	return data;
}


std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

    

    //cannon balls from enemy hitting to the player
    data[static_cast<int>(ProjectileType::kEnemyCannonBall)].m_damage = 10;
    data[static_cast<int>(ProjectileType::kEnemyCannonBall)].m_speed = 50;
    data[static_cast<int>(ProjectileType::kEnemyCannonBall)].m_texture = TextureID::kEnemyCannonBall;
    

    //Cannon Ball from player's ship
    data[static_cast<int>(ProjectileType::kMissile)].m_damage = 50;
    data[static_cast<int>(ProjectileType::kMissile)].m_speed = 100;
    data[static_cast<int>(ProjectileType::kMissile)].m_texture = TextureID::kMissile;
  

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


    data[static_cast<int>(PickupType::kCoins)].m_texture = TextureID::kCoin;
    data[static_cast<int>(PickupType::kCoins)].m_texture_rect = sf::IntRect(0, 0, 50, 50);
    data[static_cast<int>(PickupType::kCoins)].m_action = std::bind(&Ship::IncreaseCoins, std::placeholders::_1);

    
    return data;
}

std::vector<ParticleData> InitializeParticleData()
{
    std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

    data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
    data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.5f);

    data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
    data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(2.5f);

    data[static_cast<int>(ParticleType::kWaterMist)].m_color = sf::Color(200, 200, 255, 150);
    data[static_cast<int>(ParticleType::kWaterMist)].m_lifetime = sf::seconds(1.5f);

    data[static_cast<int>(ParticleType::kWaterSplashes)].m_color = sf::Color(235, 235, 235, 50);
    data[static_cast<int>(ParticleType::kWaterSplashes)].m_lifetime = sf::seconds(1.5f);    // Short lifespan for quick splashes


    return data;
}
