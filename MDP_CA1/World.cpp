#include "World.hpp"

#include <iostream>

#include "Obstacle.hpp"
#include "Pickup.hpp"
#include "Projectile.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
	:m_target(output_target)
	,m_camera(output_target.getDefaultView())
	,m_textures()
	,m_fonts(font)
	,m_sounds(sounds)
	,m_scenegraph(ReceiverCategories::kNone)
	,m_scene_layers()
	,m_world_bounds(0.f,0.f, m_camera.getSize().x, 4000.f)
	,m_spawn_position(m_camera.getSize().x/2.f, m_world_bounds.height - m_camera.getSize().y/2.f)//
	,m_scrollspeed(-20.f)
	,m_scrollspeed_compensation(1.f)
	,m_networked_world(networked)
	,m_network_node(nullptr)
	,m_ships()
	
{
	m_scene_texture.create(m_target.getSize().x, m_target.getSize().y);
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);
}

void World::SetWorldScrollCompensation(float compensation)
{
	m_scrollspeed_compensation = compensation;
}

void World::Update(sf::Time dt)
{//Code changes from Dawood Parhiar D00248313
	
	//Scroll the world  
	m_camera.move(0, m_scrollspeed * dt.asSeconds());

	for (auto& ship : m_ships) {
		ship->SetVelocity(0.f, 0.f);
	}

	DestroyEntitiesOutsideView();
	GuideMissiles();

	//Forward commands to the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}

	
	AdaptPlayerVelocity();
	HandleCollisions();


	auto first_to_remove = std::remove_if(m_ships.begin(), m_ships.end(), std::mem_fn(&Ship::IsMarkedForRemoval));
	m_ships.erase(first_to_remove, m_ships.end());





	m_scenegraph.RemoveWrecks();
	//SpawnEnemies();
	m_scenegraph.Update(dt, m_command_queue);
	AdaptPlayerPosition();
	UpdateSounds();
}

void World::Draw()
{//Code changes from Dawood Parhiar D00248313
	if (PostEffect::IsSupported())
	{
		//Apply Water Effect to Background ---
		m_scene_texture.clear();
		m_scene_texture.draw(m_scenegraph);
		m_target.setView(m_camera);
		for (int i = static_cast<int>(SceneLayers::kBackground); i < static_cast<int>(SceneLayers::kLayerCount); ++i)
		{
			if (m_scene_layers[i] != nullptr)
			{
				m_target.draw(*m_scene_layers[i]);
			}
		}
		m_scene_texture.display();
	}
	else
	{
		// No shader support, render everything normally
		m_target.setView(m_camera);
		m_target.draw(m_scenegraph);
	}
}


CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

Ship* World::AddShip(int id)
{
	auto new_ship = std::make_unique<Ship>(ShipType::kPirateShip, m_textures, m_fonts);
	new_ship->setPosition(m_camera.getCenter());
	new_ship->SetId(id);


	m_ships.emplace_back(new_ship.get());
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(new_ship));
	
	return m_ships.back();
}


void World::RemoveShip(int id)
{
	Ship* ship = GetShip(id);
	if (ship)
	{
		ship->Destroy();
		m_ships.erase(std::find(m_ships.begin(), m_ships.end(), ship));
	}
}


Ship* World::GetShip(int id) const
{//Code changes from Dawood Parhiar D00248313
	for (Ship* s : m_ships)
	{
		if (s->GetIdentifier() == id)
		{
			return s;
		}
	}
	return nullptr;
}


void World::CreatePickup(sf::Vector2f position, PickupType type)
{
	std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
	pickup->setPosition(position);
	pickup->SetVelocity(0.f, 1.f);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(pickup));
}

bool World::PollGameAction(GameActions::Action& out)
{
	return m_network_node->PollGameAction(out);
}

std::vector<Ship*>& World::GetShips()
{
	return m_ships;
}

bool World::HasShip(sf::Int8 ship_id) const
{
	if (GetShip(ship_id))
	{
		return true;
	}
	return false;
}


void World::SetCurrentBattleFieldPosition(float lineY)
{
	m_camera.setCenter(m_camera.getCenter().x, lineY - m_camera.getSize().y / 2);
	m_spawn_position.y = m_world_bounds.height;
}

void World::SetWorldHeight(float height)
{
	m_world_bounds.height = height;
}

bool World::HasAlivePlayer() const
{// Code changes from Dawood Parhiar D00248313
	if (!m_ships.empty())
	{
		return true;
	}
	return false;
}


bool World::HasPlayerReachedEnd() const
{//Code changes from Dawood Parhiar D00248313
	if (Ship* ship = GetShip(1))
	{
		return !m_world_bounds.contains(ship->getPosition());
	}
	return false;
}


void World::LoadTextures()
{//Code changes from Dawood Parhiar D00248313
	m_textures.Load(TextureID::kPirateShip, "Media/Textures/ship.png");
	m_textures.Load(TextureID::kEnemyShip1, "Media/EnemyShips/EnemyShip1.png");
	m_textures.Load(TextureID::kMissile, "Media/Textures/cannon_ball.png");
	m_textures.Load(TextureID::kEnemyShip2, "Media/EnemyShips/EnemyShip2.png");
	m_textures.Load(TextureID::kEnemyShip3, "Media/EnemyShips/EnemyShip3.png");
	m_textures.Load(TextureID::kHealthRefill, "Media/Textures/HealthRefill.png");
	m_textures.Load(TextureID::kMissileRefill, "Media/Textures/MissileRefill.png");
	m_textures.Load(TextureID::kFireSpread, "Media/Textures/FireSpread.png");
	m_textures.Load(TextureID::kFireRate, "Media/Textures/FireRate.png");
	m_textures.Load(TextureID::kFinishLine, "Media/Textures/FinishLine.png");
	m_textures.Load(TextureID::kEntities, "Media/Textures/Entities.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
	m_textures.Load(TextureID::kWater, "Media/Textures/Water3.png");
	m_textures.Load(TextureID::kEnemyCannonBall, "Media/Textures/EnemyBall.png");
	m_textures.Load(TextureID::kPlayer2Ship, "Media/EnemyShips/ship13.png");
	m_textures.Load(TextureID::kMountains, "Media/Textures/mountain_area.png");
	m_textures.Load(TextureID::kCoin, "Media/Textures/coin.png");
	m_textures.Load(TextureID::kCannon, "Media/Textures/cannon.png");
}

void World::BuildMountains()
{//Code changes from Dawood Parhiar D00248313
	sf::Texture& mountain_texture = m_textures.Get(TextureID::kMountains);

	// Define fixed positions for mountains
	std::vector<sf::Vector2f> mountain_positions = {
		{100.f, 500.f}, {500.f, 1000.f}, {200.f, 1500.f}, {600.f, 1700.f}, {100.f, 2400.f},
		{500.f, 3000.f}, {200.f, 3500.f}, {600.f, 3800.f}
	};

	for (const auto& pos : mountain_positions)
	{
		std::unique_ptr<Obstacle> mountain = std::make_unique<Obstacle>(mountain_texture);
		mountain->setPosition(pos);
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(mountain));
	}
}

void World::DropCoins()
{//Code changes from Dawood Parhiar D00248313
	//Drop coins when an enemy is destroyed
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kEnemyShip);
	command.action = DerivedAction<Ship>([this](Ship& enemy, sf::Time dt)
		{
			if (!enemy.IsDestroyed())
			{
				return;
			}
			sf::Vector2f enemy_position = enemy.getPosition();
			std::unique_ptr<Pickup> coin(new Pickup(PickupType::kCoins, m_textures));
			coin->setPosition(enemy_position);
			m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(coin));
		});
}

void World::DropCoins(sf::Vector2f position)
{//Code changes from Dawood Parhiar D00248313
	std::unique_ptr<Pickup> coin(new Pickup(PickupType::kCoins, m_textures));
	coin->setPosition(position);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(coin));
}
void World::SpawnInitialCoins()
{//Code changes from Dawood Parhiar D00248313
	for (int i = 0; i < 20; ++i)
	{
		DropCoins(sf::Vector2f(Utility::RandomInt(800), Utility::RandomInt(3000)));
	}
}
void World::BuildScene()
{//Code changes from Dawood Parhiar D00248313
	//Initialize the different layers
	for (std::size_t i = 0; i < static_cast<int>(SceneLayers::kLayerCount); ++i)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kLowerAir)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scenegraph.AttachChild(std::move(layer));
	}
	
	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kWater);
	texture.setRepeated(true);

	float view_height = m_camera.getSize().y;
	sf::IntRect textureRect(m_world_bounds);
	textureRect.height += static_cast<int>(view_height);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(m_world_bounds.left, m_world_bounds.top - view_height);
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Add the finish line
	sf::Texture& finish_texture = m_textures.Get(TextureID::kFinishLine);
	std::unique_ptr<SpriteNode> finish_sprite(new SpriteNode(finish_texture));
	finish_sprite->setPosition(0.f, -76.f);
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(finish_sprite));

	BuildMountains();
	SpawnInitialCoins();

	//Add Players here
	//InitializePlayers();

	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	//m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	std::unique_ptr<ParticleNode> splashesNode(new ParticleNode(ParticleType::kWaterSplashes, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(splashesNode));

	std::unique_ptr<ParticleNode> mistNode(new ParticleNode(ParticleType::kWaterMist, m_textures));
	//m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(mistNode));


	// Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scenegraph.AttachChild(std::move(soundNode));

	if (m_networked_world)
	{
		std::unique_ptr<NetworkNode> network_node(new NetworkNode());
		m_network_node = network_node.get();
		m_scenegraph.AttachChild(std::move(network_node));
	}

	AddEnemies();

	/*std::unique_ptr<Ship> left_escort(new Ship(ShipType::kEnemyShip1, m_textures, m_fonts));
	left_escort->setPosition(-80.f, 50.f);
	m_player_1_ship->AttachChild(std::move(left_escort));

	std::unique_ptr<Ship> right_escort(new Ship(ShipType::kEnemyShip1, m_textures, m_fonts));
	right_escort->setPosition(80.f, 50.f);
	m_player_1_ship->AttachChild(std::move(right_escort));*/
}

void World::AdaptPlayerPosition()
{//Code changes from Dawood Parhiar D00248313
	//keep each player on the screen
	sf::FloatRect view_bounds = GetViewBounds();
	const float border_distance = 40.f;

	for ( auto& ship : m_ships)
	{
		
		sf::Vector2f position = ship->getPosition();
		position.x = std::max(position.x, view_bounds.left + border_distance);
		position.x = std::min(position.x, view_bounds.left + view_bounds.width - border_distance);
		position.y = std::max(position.y, view_bounds.top + border_distance);
		position.y = std::min(position.y, view_bounds.top + view_bounds.height - border_distance);
		ship->setPosition(position);
	}
}

void World::AdaptPlayerVelocity()
{//Code changes from Dawood Parhiar D00248313
	for (Ship* player : m_ships)
	{
		sf::Vector2f velocity = player->GetVelocity();

		//If they are moving diagonally divide by sqrt 2
		if (velocity.x != 0.f && velocity.y != 0.f)
		{
			player->SetVelocity(velocity / std::sqrt(2.f));
		}
		//Add scrolling velocity
		player->Accelerate(0.f, m_scrollspeed);
	}
}

void World::SpawnEnemies()
{//Code changes from Dawood Parhiar D00248313
	//Spawn an enemy when it is relevant i.e when it is in the Battlefieldboudns
	while (!m_enemy_spawn_points.empty() && m_enemy_spawn_points.back().m_y > GetBattleFieldBounds().top)
	{
		SpawnPoint spawn = m_enemy_spawn_points.back();
		std::unique_ptr<Ship> enemy(new Ship(spawn.m_type, m_textures, m_fonts));
		enemy->setPosition(spawn.m_x, spawn.m_y);
		enemy->setRotation(Utility::RandomInt(300));

		if (m_networked_world)
		{
			enemy->DisablePickups();
		}
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(enemy));
		m_enemy_spawn_points.pop_back();
	}
}

void World::AddEnemies()
{//Code changes from Dawood Parhiar D00248313
	AddEnemy(ShipType::kEnemyShip2, 0.f, 500.f);
	AddEnemy(ShipType::kEnemyShip2, 0.f, 1000.f);
	AddEnemy(ShipType::kEnemyShip2, 100.f, 1100.f);
	AddEnemy(ShipType::kEnemyShip2, -100.f, 1100.f);
	AddEnemy(ShipType::kEnemyShip2, -70.f, 1400.f);
	AddEnemy(ShipType::kEnemyShip2, 70.f, 1400.f);
	AddEnemy(ShipType::kEnemyShip2, 70.f, 1600.f);

	
	SortEnemies();
}

void World::SortEnemies()
{
	//Sort all enemies according to their y-value, such that lower enemies are checked first for spawning
	std::sort(m_enemy_spawn_points.begin(), m_enemy_spawn_points.end(), [](SpawnPoint lhs, SpawnPoint rhs)
		{
			return lhs.m_y < rhs.m_y;
		});
}

void World::AddEnemy(ShipType type, float relx, float rely)
{
	SpawnPoint spawn(type, m_spawn_position.x + relx, m_spawn_position.y - rely);
	m_enemy_spawn_points.emplace_back(spawn);
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize()/2.f, m_camera.getSize());
}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area at the top where enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.top -= 100.f;
	bounds.height += 100.f;

	return bounds;

}

void World::DestroyEntitiesOutsideView()
{
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kEnemyShip) | static_cast<int>(ReceiverCategories::kProjectile) | static_cast<int>(ReceiverCategories::kCannon);
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
		{
			//Does the object intersect with the battlefield
			if (!GetBattleFieldBounds().intersects(e.GetBoundingRect()))
			{
				e.Destroy();
			}
		});
	m_command_queue.Push(command);
}

void World::GuideMissiles()
{
	// Only destroy missiles when they exceed their max radius —
	// don’t change their velocity.
	Command missileGuider;
	missileGuider.category = static_cast<int>(ReceiverCategories::kAlliedProjectile);
	missileGuider.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time)
		{
			// If it’s not a guided missile, do nothing anyway.
			if (!missile.IsGuided())
				return;

			// Range check
			sf::Vector2f pos = missile.GetWorldPosition();
			sf::Vector2f launch = missile.GetLaunchPosition();
			float traveled = std::hypot(pos.x - launch.x,
				pos.y - launch.y);

			if (traveled > missile.GetMaxRadius())
				missile.Destroy();

		});

	m_command_queue.Push(missileGuider);
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if (static_cast<int>(type1) & category1 && static_cast<int>(type2) & category2)
	{
		return true;
	}
	else if (static_cast<int>(type1) & category2 && static_cast<int>(type2) & category1)
	{ 
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}


void World::HandleCollisions()
{//Code changes from Dawood Parhiar D00248313
	std::set<SceneNode::Pair> collision_pairs;
	m_scenegraph.CheckSceneCollision(m_scenegraph, collision_pairs);
	for (SceneNode::Pair pair : collision_pairs)
	{
		if (MatchesCategories(pair, ReceiverCategories::kObstacle, ReceiverCategories::kAlliedProjectile))
		{
			auto& obst = static_cast<Obstacle&>(*pair.first);
			auto& missile = static_cast<Projectile&>(*pair.second);
			//Collision response
			missile.Destroy();
		}
		
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kPlayerShip))
		{
			auto& player = static_cast<Ship&>(*pair.first);
			auto& enemy = static_cast<Ship&>(*pair.second);
			//Collision response
			player.Damage(enemy.GetHitPoints());
			enemy.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kPickup))
		{
			auto& player = static_cast<Ship&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Collision response
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kAlliedProjectile))
		{
			auto& ship = static_cast<Ship&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			if (projectile.GetOwnerId() == ship.GetIdentifier())
				continue;
			sf::Int32 ownerId = projectile.GetOwnerId();
			//Collision response
			ship.Damage(projectile.GetDamage());
			projectile.Destroy();
			projectile.SetOwnerId(ownerId);
		}

		else if (MatchesCategories(pair, ReceiverCategories::kEnemyShip, ReceiverCategories::kAlliedProjectile))
		{
			auto& ship = static_cast<Ship&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			if (projectile.GetOwnerId() == ship.GetIdentifier())
				continue;

			sf::Int32 ownerId = projectile.GetOwnerId();

			//Collision response
			ship.Damage(projectile.GetDamage());
			projectile.Destroy();
			projectile.SetOwnerId(ownerId);
		}

		//if hits the mountain
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kObstacle))
		{
			auto& ship = static_cast<Ship&>(*pair.first);
			auto& mountain = static_cast<Obstacle&>(*pair.second);
			//Collision response
			ship.Damage(10);
			
		}
		//coins collision
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kCoin))
		{
			auto& ship = static_cast<Ship&>(*pair.first);
			auto& coin = static_cast<Pickup&>(*pair.second);
			//Collision response
			coin.Apply(ship);
			coin.Destroy();
		}
	}
}

void World::UpdateSounds()
{
	if (m_ships.empty())
	{
		return;
	}

	sf::Vector2f listener_position(0.f, 0.f);

	for ( auto& it : m_ships)
	{
		listener_position += it->GetWorldPosition();
	}

	listener_position /= static_cast<float>(m_ships.size());
	m_sounds.SetListenerPosition(listener_position);
	// Remove unused sounds
	m_sounds.RemoveStoppedSounds();
}



