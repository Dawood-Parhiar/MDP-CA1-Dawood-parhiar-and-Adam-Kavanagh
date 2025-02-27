#include "Ship.hpp"
#include "TextureID.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include "DataTables.hpp"
#include "EmitterNode.hpp"
#include "Projectile.hpp"
#include "PickupType.hpp"
#include "Pickup.hpp"
#include "SoundNode.hpp"

namespace
{
	const std::vector<ShipData> Table = InitializeShipData();
}

TextureID ToTextureID(ShipType type)
{
	switch (type)
	{
	case ShipType::kPirateShip:
		return TextureID::kPirateShip;
		break;
	/*case ShipType::kAlliedShip:
		return TextureID::kAlliedShip;
		break;*/
	case ShipType::kEnemyShip1:
		return TextureID::kEnemyShip1;
		break;
	case ShipType::kEnemyShip2:
		return TextureID::kEnemyShip2;
		break;
	}
	return TextureID::kPirateShip;
}

Ship::Ship(ShipType type, const TextureHolder& textures, const FontHolder& fonts)  
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	//, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_sprite(textures.Get(ToTextureID(type)))//using .png instead of a rect from a sprite
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_health_display(nullptr)
	, m_missile_display(nullptr)
	, m_coins_display(nullptr)
	, m_distance_travelled(0.f)
	, m_directions_index(0)
	, m_coins(0)
	, m_fire_rate(1)
	, m_spread_level(1)
	, m_is_firing(false)
	, m_is_launching_missile(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_missile_ammo(1)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_spawned_pickup(false)
	, m_played_explosion_sound(false)
	, original_x(0.f)
	, original_y(0.f)
	,m_id(0)
	,m_cannon(nullptr)
	,m_cannon_ptr(nullptr)

{
	//positions for animation of the ship
	original_x = m_sprite.getPosition().x;
	original_y = m_sprite.getPosition().y;

	m_cannon = std::make_unique<Cannon>(textures);
	m_cannon->setPosition(getPosition());

	m_cannon_ptr = m_cannon.get();
	AttachChild(std::move(m_cannon));
	
	m_explosion.SetFrameSize(sf::Vector2i(256, 256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	 //No Bullets in the game
	 m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateBullet(node, textures);
		};

	m_missile_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_missile_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateProjectile(node, ProjectileType::kMissile, original_x , 0.f, textures);
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

	

	if (Ship::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerShip))// || Ship::GetCategory() == static_cast<int>(ReceiverCategories::kAlliedShip))
	{
		m_missile_ammo = 20;
		std::string* missile_ammo = new std::string("");
		std::unique_ptr<TextNode> missile_display(new TextNode(fonts, *missile_ammo));
		m_missile_display = missile_display.get();
		AttachChild(std::move(missile_display));

		std::string* coins = new std::string("");
		std::unique_ptr<TextNode> coins_display(new TextNode(fonts, *coins));
		m_coins_display = coins_display.get();
		AttachChild(std::move(coins_display));
	}


	UpdateTexts();

	//Add particle system for ships
	std::unique_ptr<EmitterNode> splashes(new EmitterNode(ParticleType::kWaterSplashes));
	splashes->setPosition(0.f, GetBoundingRect().height/2.f);
	AttachChild(std::move(splashes));

}

unsigned int Ship::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<unsigned int>(ReceiverCategories::kPlayerShip);
	}
	return static_cast<unsigned int>(ReceiverCategories::kEnemyShip);

}

void Ship::IncreaseFireRate()
{
	if (m_fire_rate < 5)
	{
		++m_fire_rate;
	}
}

void Ship::IncreaseCoins()
{
	++m_coins;
}

void Ship::IncreaseFireSpread()
{
	if (m_spread_level < 3)
	{
		++m_spread_level;
	}
}

void Ship::CollectMissile(unsigned int count)
{
	m_missile_ammo += count;
}

void Ship::UpdateTexts()
{
	m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	m_health_display->setPosition(0.f, 50.f);
	m_health_display->setRotation(-getRotation());
	if (m_missile_display)
	{
		m_missile_display->setPosition(0.f, 90.f);
		m_missile_display->SetString("M: " + std::to_string(m_missile_ammo));
		//display coins if missile display is present to a ship
		m_coins_display->SetString("Coins: " + std::to_string(m_coins));
		m_coins_display->setPosition(0.f, 70.f);
		m_coins_display->setRotation(-getRotation());
	}
}

void Ship::UpdateMovementPattern(sf::Time dt)
{
	//Enemy AI
	const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
	if (!directions.empty())
	{
		//Move along the current direction, then change direction
		if (m_distance_travelled > directions[m_directions_index].m_distance)
		{
			m_directions_index = (m_directions_index + 1) % directions.size();
			m_distance_travelled = 0.f;
		}

		//Compute velocity
		//Add 90 to move down the screen, 0 is right

		double radians = Utility::ToRadians(directions[m_directions_index].m_angle + 90.f);
		float vx = GetMaxSpeed() * std::cos(radians);
		float vy = GetMaxSpeed() * std::sin(radians);

		SetVelocity(vx, vy);
		m_distance_travelled += GetMaxSpeed() * dt.asSeconds();
	}
}

float Ship::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Ship::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}


void Ship::LaunchMissile()
{

	//Dawood Parhiar D00248313
	ProjectileType type = IsAllied() ? ProjectileType::kAlliedCannonBall : ProjectileType::kEnemyCannonBall;

	if (m_missile_ammo > 0 && Table[static_cast<int>(type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_launching_missile = true;
		--m_missile_ammo;

	}
}

void Ship::CreateBullet(SceneNode& node, const TextureHolder& textures) const
{
	ProjectileType type = IsAllied() ? ProjectileType::kAlliedCannonBall : ProjectileType::kEnemyCannonBall;
	switch (m_spread_level)
	{
	case 1:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		break;
	case 2:
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	case 3:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	}
}

void Ship::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const
{
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures));

	sf::Vector2f offset(x_offset * Utility::ToRadians(getRotation()), y_offset * Utility::ToRadians(getRotation()));
	//fire the projectile from the center of the ship
	sf::Vector2f velocity(std::cos(getRotation() * 3.14159f / 180.f) * 300.f,  // Speed in x-direction
		std::sin(getRotation() * 3.14159f / 180.f) * 300.f);// fire the projectile horizontally

	float sign = IsAllied() ? -1.f : 1.f;

	projectile->setPosition(GetWorldPosition() + offset);
	projectile->SetVelocity(velocity* sign);

	if (type == ProjectileType::kMissile)
	{
		projectile->SetLaunchPosition(GetWorldPosition() + offset);
		projectile->SetMaxRadius(300.f);
	}
	/*
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures));

	// Get the correct spawn position from the cannon's mouth
	sf::Vector2f spawnPosition = m_cannon->GetMouthPosition();
	projectile->setPosition(spawnPosition);

	// Get the cannon's rotation to fire in the correct direction
	float cannonRotation = m_cannon->getRotation();
	float angleRad = Utility::ToRadians(cannonRotation); // Convert to radians

	// Set velocity based on cannon direction
	sf::Vector2f velocity(std::cos(angleRad) * 300.f, std::sin(angleRad) * 300.f);

	float sign = IsAllied() ? -1.f : 1.f;
	projectile->SetVelocity(velocity * sign);

	if (type == ProjectileType::kMissile)
	{
		projectile->SetLaunchPosition(spawnPosition);
		projectile->SetMaxRadius(300.f);
	}*/

	node.AttachChild(std::move(projectile));
}

sf::FloatRect Ship::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Ship::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Ship::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);
	}
}

void Ship::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		CheckPickupDrop(commands);
		m_explosion.Update(dt);
		// Play explosion sound only once
		if (!m_played_explosion_sound)
		{
			if (m_type == ShipType::kPirateShip)
			{
				SoundEffect soundEffect = SoundEffect::kExplosion2;
				PlayLocalSound(commands, soundEffect);
			}
			else
			{
				SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
				PlayLocalSound(commands, soundEffect);
			}

			m_played_explosion_sound = true;
		}
		return;
	}
	if (m_cannon)
	{
		m_cannon->UpdateCurrent(dt, commands);
	}
	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();

	UpdateMovementPattern(dt);

	UpdateRollAnimation(dt);

	//Check if bullets or misiles are fired
	CheckProjectileLaunch(dt, commands);
}

void Ship::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	
	if (!IsAllied())
	{
		
			LaunchMissile();
		
	}

	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		PlayLocalSound(commands, IsAllied() ? SoundEffect::kEnemyGunfire : SoundEffect::kAlliedGunfire);
		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
		m_is_firing = false;
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		//Wait, can't fire
		m_fire_countdown -= dt;
		m_is_firing = false;
	}

	//Missile launch
	if (m_is_launching_missile)
	{
		PlayLocalSound(commands, SoundEffect::kLaunchMissile);
		commands.Push(m_missile_command);
		m_is_launching_missile = false;
	}
}

bool Ship::IsAllied() const
{
	return m_type == ShipType::kPirateShip;
}

void Ship::CreatePickup(SceneNode& node, const TextureHolder& textures) const
{
	auto type = static_cast<PickupType>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));
	std::unique_ptr<Pickup> pickup(new Pickup(type, textures));
	pickup->setPosition(GetWorldPosition());
	pickup->SetVelocity(0.f, 0.f);
	node.AttachChild(std::move(pickup));
}

void Ship::CheckPickupDrop(CommandQueue& commands)
{
	//TODO Get rid of the magic number 3 here 
	if (!IsAllied() && Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)) == 0 && !m_spawned_pickup)
	{
		commands.Push(m_drop_pickup_command);
	}
	m_spawned_pickup = true;
}

void Ship::UpdateRollAnimation(sf::Time dt)
{

	// Check if the ship type has roll animation enabled 
	if (Table[static_cast<int>(m_type)].m_has_roll_animation)
	{
		// Time-based animation using sine waves from Chatgpt

		static float timeAccumulator = 0.0f; // Accumulate elapsed time for smooth animation
		timeAccumulator += dt.asSeconds();
	
		// Bobbing effect (up and down movement)
		const float bobAmplitude = 0.2f;  // Adjust for how high/low the boat moves
		const float bobFrequency = 0.2f;  // Speed of bobbing
		float bobOffset = bobAmplitude * sin(timeAccumulator * bobFrequency);
	
		// Rotational tilt (left-right tilting)
		const float tiltAmplitude = 4.0f;  // Maximum tilt in degrees
		const float tiltFrequency = 1.5f;  // Speed of tilting
		float tiltAngle = tiltAmplitude * sin(timeAccumulator * tiltFrequency + 2.0f); // Offset phase for natural motion
	
		// Apply transformations
		m_sprite.setPosition(original_x, original_y + bobOffset); // Adjust Y position for bobbing
		m_sprite.setRotation(tiltAngle);                       // Apply tilt to the sprite
	}

}

Cannon* Ship::GetCannon() const
{
	return m_cannon_ptr;// ? m_cannon.get() : nullptr;
}

void Ship::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});

	commands.Push(command);
}

void Ship::SetRenderTargets(sf::RenderTarget& target)
{
	m_render_target = &target;
}


void Ship::MoveShip(sf::Time dt, float speed)
{
	float rotation = getRotation() +90;
	float angle = Utility::ToRadians(rotation);
	float vx = speed * std::cos(angle);
	float vy = speed * std::sin(angle);
	Accelerate(vx, vy);
}

void Ship::SetId(int id)
{
	m_id = id;
}

int Ship::GetId()
{
	return m_id;
}



