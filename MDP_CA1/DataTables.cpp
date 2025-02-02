#include "DataTables.hpp"
#include "ShipType.hpp"
#include "ProjectileType.hpp"
#include "PickupType.hpp"
#include "Ship.hpp"
#include "ParticleType.hpp"

std::vector<ShipData> InitializeShipData()
{
    std::vector<ShipData> data(static_cast<int>(ShipType::kShipCount));

    data[static_cast<int>(ShipType::kPirateShip)].m_hitpoints = 200;
    data[static_cast<int>(ShipType::kPirateShip)].m_speed = 200.f;
    data[static_cast<int>(ShipType::kPirateShip)].m_fire_interval = sf::seconds(1);
    data[static_cast<int>(ShipType::kPirateShip)].m_texture = TextureID::kPirateShip;
    //data[static_cast<int>(ShipType::kPirateShip)].m_texture_rect = sf::IntRect(0, 0, 48, 64);
    data[static_cast<int>(ShipType::kPirateShip)].m_has_roll_animation = true;



    //player 2 ship

    data[static_cast<int>(ShipType::kPlayer2Ship)].m_hitpoints = 200;
    data[static_cast<int>(ShipType::kPlayer2Ship)].m_speed = 200.f;
    data[static_cast<int>(ShipType::kPlayer2Ship)].m_fire_interval = sf::seconds(1);
    data[static_cast<int>(ShipType::kPlayer2Ship)].m_texture = TextureID::kPlayer2Ship;
    data[static_cast<int>(ShipType::kPlayer2Ship)].m_has_roll_animation = true;

    //Enemy Skeleton Ship
    
    //data[static_cast<int>(ShipType::kEnemyShip1)].m_hitpoints = 20;
    //data[static_cast<int>(ShipType::kEnemyShip1)].m_speed = 80.f;
    //data[static_cast<int>(ShipType::kEnemyShip1)].m_fire_interval = sf::Time::Zero;
    //data[static_cast<int>(ShipType::kEnemyShip1)].m_texture = TextureID::kEnemyShip1;
    ////data[static_cast<int>(ShipType::kEnemyShip1)].m_texture_rect = sf::IntRect(144, 0, 84, 64);
    //data[static_cast<int>(ShipType::kEnemyShip1)].m_has_roll_animation = true;

   
    /*data[static_cast<int>(ShipType::kEnemyShip1)].m_directions.emplace_back(Direction(+45.f, 80.f));
    data[static_cast<int>(ShipType::kEnemyShip1)].m_directions.emplace_back(Direction(-45.f, 160.f));
    data[static_cast<int>(ShipType::kEnemyShip1)].m_directions.emplace_back(Direction(+45.f, 80.f));
*/

    ////Enemy Red Ship
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_hitpoints = 40;
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_speed = 50.f;
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_fire_interval = sf::seconds(2);
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_texture = TextureID::kEnemyShip2;
    ////data[static_cast<int>(ShipType::kEnemyShip2)].m_texture_rect = sf::IntRect(228, 0, 60, 59);
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_has_roll_animation = false;


    //data[static_cast<int>(ShipType::kEnemyShip2)].m_directions.emplace_back(Direction(+45.f, 50.f));
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_directions.emplace_back(Direction(0.f, 50.f));
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_directions.emplace_back(Direction(-45.f, 100.f));
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_directions.emplace_back(Direction(0.f, 50.f));
    //data[static_cast<int>(ShipType::kEnemyShip2)].m_directions.emplace_back(Direction(45.f, 50.f));

    return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

    ////No need for this (previously: bullets hitting enemies)
    //data[static_cast<int>(ProjectileType::kAlliedCannonBall)].m_damage = 10;
    //data[static_cast<int>(ProjectileType::kAlliedCannonBall)].m_speed = 300;
    //data[static_cast<int>(ProjectileType::kAlliedCannonBall)].m_texture = TextureID::kEntities;
    //data[static_cast<int>(ProjectileType::kAlliedCannonBall)].m_texture_rect = sf::IntRect(175, 64, 3, 14);

    //cannon balls from enemy hitting to the player
    data[static_cast<int>(ProjectileType::kMissile)].m_damage = 50;
    data[static_cast<int>(ProjectileType::kMissile)].m_speed = 300;
    data[static_cast<int>(ProjectileType::kMissile)].m_texture = TextureID::kEnemyCannonBall;
    //data[static_cast<int>(ProjectileType::kEnemyCannonBall)].m_texture_rect = sf::IntRect(175, 64, 3, 14);

    //Cannon Ball from player's ship
    data[static_cast<int>(ProjectileType::kMissile)].m_damage = 50;
    data[static_cast<int>(ProjectileType::kMissile)].m_speed = 100;
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

    data[static_cast<int>(ParticleType::kWaterMist)].m_color = sf::Color(200, 200, 255, 150);
    data[static_cast<int>(ParticleType::kWaterMist)].m_lifetime = sf::seconds(1.5f);

    data[static_cast<int>(ParticleType::kWaterSplashes)].m_color = sf::Color(100, 150, 255); // Light blue color
    data[static_cast<int>(ParticleType::kWaterSplashes)].m_lifetime = sf::seconds(1.5f);    // Short lifespan for quick splashes


    return data;
}
