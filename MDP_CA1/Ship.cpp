#include "Ship.hpp"
#include "TextureID.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include "DataTables.hpp"
#include "EmitterNode.hpp"
#include "NetworkNode.hpp"
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
	,m_identifier(0)
	,m_cannon(nullptr)
	,m_cannon_ptr(nullptr)
	,m_explosion_began(false)
	,m_pickups_enabled(true)
	, m_player_cannon_cooldown(sf::Time::Zero) // Adam


{//Code changes from Dawood Parhiar D00248313 in Ship class
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

	
	 m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			LaunchEnemyCannon(node, textures);
		};
	
	m_missile_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_missile_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateProjectile(node, ProjectileType::kMissile,original_x,0, textures);
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

	

	if (Ship::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerShip))
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
	m_health_display->setPosition(0.f, 70.f);
	
	if (m_missile_display)
	{
		m_missile_display->setPosition(0.f, 90.f);
		m_missile_display->SetString("M: " + std::to_string(m_missile_ammo));
		
		//display coins if missile display is present to a ship
		m_coins_display->SetString("Coins: " + std::to_string(m_coins));
		m_coins_display->setPosition(0.f, 110.f);
		
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


void Ship::LaunchPlayerCannon() //adam
{
	if (m_player_cannon_cooldown <= sf::Time::Zero)
	{
		//Dawood Parhiar D00248313
		ProjectileType type = IsAllied() ? ProjectileType::kAlliedCannonBall : ProjectileType::kEnemyCannonBall;

		if (m_missile_ammo > 0 && Table[static_cast<int>(type)].m_fire_interval != sf::Time::Zero)
		{
			m_is_launching_missile = true;
			--m_missile_ammo;

			m_player_cannon_cooldown = sf::seconds(1.5); //Adam
		}
	}
}

void Ship::LaunchEnemyCannon(SceneNode& node, const TextureHolder& textures) const
{
	ProjectileType type =  ProjectileType::kEnemyCannonBall;
	CreateProjectile(node, type,0,0.5, textures);
}

void Ship::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const
{
	
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures));
	
	if (!IsAllied())
	{

		sf::Vector2f enemyOffset(
			x_offset * std::cos(Utility::ToRadians(getRotation())),
			y_offset * std::sin(Utility::ToRadians(getRotation()))
		);

		// Reverse the direction by adding 180 degrees so they fire from cannon
		float reversedAngleRad = Utility::ToRadians(getRotation() + 180.f);

		// Compute the new velocity in the opposite direction
		sf::Vector2f velocity(
			std::cos(reversedAngleRad) * 300.f,  // Reverse X-direction
			std::sin(reversedAngleRad) * 300.f   // Reverse Y-direction
		);
		float sign = 1.f;

		projectile->setPosition(GetWorldPosition() + enemyOffset);
		projectile->SetVelocity(velocity * sign);

		if (type == ProjectileType::kMissile)
		{
			projectile->SetLaunchPosition(GetWorldPosition() + enemyOffset);
			projectile->SetMaxRadius(300.f);
		}
		projectile->SetOwnerId(this->GetIdentifier());
	}
	else
	{
		//spawn point
		sf::Vector2f spawn = m_cannon_ptr->GetMouthPosition();
		projectile->setPosition(spawn);

		// build the full transform chain:
		//    ship world × cannon local × sprite local
		sf::Transform full =
			m_cannon_ptr->GetWorldTransform()
			* m_cannon_ptr->GetPosition();  // 

		// measure forward direction in world‐space
		sf::Vector2f tipWorld = full.transformPoint({ 0.f, 0.f });
		sf::Vector2f aheadWorld = full.transformPoint({ 1.f, 0.f });
		sf::Vector2f dir = aheadWorld - tipWorld;

		// normalize
		float len = std::hypot(dir.x, dir.y);
		if (len > 0.f) dir /= len;

		// apply your speed
		const float speed = 300.f;
		projectile->SetVelocity(-dir * speed);
		projectile->SetLaunchPosition(spawn);
		projectile->SetMaxRadius(300.f);
		projectile->SetOwnerId(this->GetIdentifier());
	}
	node.AttachChild(std::move(projectile));

	//if (m_cannon_ptr) // Ensure cannon exists
	//{
	//	m_cannon_ptr->CreateProjectile(node, type, textures);
	//}
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
		if (!m_explosion_began)
		{
			if (m_type == ShipType::kPirateShip)
			{
				SoundEffect soundEffect = SoundEffect::kExplosion2;
				PlayLocalSound(commands, soundEffect);
			}
			else
			{
				/*SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
				PlayLocalSound(commands, soundEffect);*/

				sf::Vector2f position = GetWorldPosition();

				Command command;
				command.category = static_cast<int>(ReceiverCategories::kNetwork);
				command.action = DerivedAction<NetworkNode>([position](NetworkNode& node, sf::Time)
					{
						node.NotifyGameAction(GameActions::kEnemyExplode, position);
					});

				commands.Push(command);
			}

			m_explosion_began = true;
		}
		return;
	}
	if (m_cannon)
	{
		m_cannon->UpdateCurrent(dt, commands);
		//m_cannon_ptr->UpdateCurrent(dt, commands);
	}
	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();

	UpdateMovementPattern(dt);

	UpdateRollAnimation(dt);

	//Check if bullets or misiles are fired
	CheckProjectileLaunch(dt, commands);
	
	if (m_player_cannon_cooldown > sf::Time::Zero) // Adam
	{
		m_player_cannon_cooldown -= dt;
	}

}

void Ship::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	
	if (!IsAllied())
	{
		Fire();
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
	return m_cannon_ptr;
}

int Ship::GetMissileAmmo()
{
	return m_missile_ammo;
}

void Ship::DisablePickups()
{
	m_pickups_enabled = false;
}



void Ship::SetMissileAmmo(sf::Int32 ammo)
{
	m_missile_ammo = ammo;
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

void Ship::Remove()
{
	Entity::Remove();
	m_show_explosion = false;
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
	m_identifier = id;
}

int Ship::GetIdentifier() const
{
	return m_identifier;
}



