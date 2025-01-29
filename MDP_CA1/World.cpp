#include "World.hpp"

#include "Obstacle.hpp"
#include "Pickup.hpp"
#include "Projectile.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds)
	:m_target(output_target)
	,m_camera(output_target.getDefaultView())
	,m_textures()
	,m_fonts(font)
	,m_sounds(sounds)
	,m_scenegraph(ReceiverCategories::kNone)
	,m_scene_layers()
	,m_world_bounds(0.f,0.f, m_camera.getSize().x, 3000.f)
	,m_spawn_position(m_camera.getSize().x/2.f, m_world_bounds.height - m_camera.getSize().y/2.f)//
	,m_scrollspeed(-50.f)
	,m_player_ships()
	,m_players()
{
	m_scene_texture.create(m_target.getSize().x, m_target.getSize().y);
	LoadTextures();
	//InitializePlayers();
	BuildScene();
	m_camera.setCenter(m_spawn_position);


}

void World::Update(sf::Time dt)
{
	
	//Scroll the world   m_scenegraph.Update(dt, m_command_queue);
	m_camera.move(0, m_scrollspeed * dt.asSeconds());

	for (size_t i = 0; i < m_players.size(); ++i)
	{
		m_players[i].HandleRealTimeInput(m_command_queue);
	}

	for (Ship* player: m_player_ships)
	{
		player->SetVelocity(0.f, 0.f);
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
	m_scenegraph.RemoveWrecks();
	//SpawnEnemies();
	m_scenegraph.Update(dt, m_command_queue);
	AdaptPlayerPosition();
	//UpdateSounds();
}

void World::Draw()
{
	//m_textures.Load(TextureID::kWater, "Media/Textures/water.png");
	
	if (PostEffect::IsSupported())
	{

		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scenegraph);
		m_scene_texture.display(); // Finalize the render texture
		m_bloom_effect.Apply(m_scene_texture, m_target);
		//m_water_effect.Apply(m_scene_texture, m_target);


		////Draw the lowerAir and upperAir layers to the render target
		////m_target.setView(m_camera);
		//for (int i = static_cast<int>(SceneLayers::kBackground); i < static_cast<int>(SceneLayers::kLayerCount); ++i)
		//{
		//	if (m_scene_layers[i] != nullptr)
		//	{
		//		if (i == static_cast<int>(SceneLayers::kBackground))
		//		{
		//			m_scene_texture.clear();
		//			m_target.setView(m_camera);
		//			m_scene_texture.draw(*m_scene_layers[i]);
		//			m_scene_texture.display();

		//			m_water_effect.Apply(m_scene_texture, m_target);
		//		}
		//		else
		//		{
		//			m_target.draw(*m_scene_layers[i]);
		//			m_water_effect.Apply(m_scene_texture, m_target);
		//		}
		//	}
		//}
		
		
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

bool World::HasAlivePlayer() const
{
	for (const Ship* player : m_player_ships)
	{
		if (!player->IsMarkedForRemoval())
			return true;
	}
	return false;
}

bool World::HasPlayerReachedEnd() const
{
	return !m_world_bounds.contains(m_player_ships.at(1)->getPosition());
}


void World::LoadTextures()
{
	m_textures.Load(TextureID::kPirateShip, "Media/Textures/ship.png");
	m_textures.Load(TextureID::kEnemyShip1, "Media/EnemyShips/EnemyShip1.png");
	m_textures.Load(TextureID::kMissile, "Media/Textures/cannon_ball.png");
	m_textures.Load(TextureID::kEnemyShip2, "Media/EnemyShips/EnemyShip2.png");
	m_textures.Load(TextureID::kEnemyShip3, "Media/EnemyShips/EnemyShip3.png");
	/*m_textures.Load(TextureID::kLandscape, "Media/Textures/Desert.png");
	m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");*/

	//changed the texture
	

	m_textures.Load(TextureID::kHealthRefill, "Media/Textures/HealthRefill.png");
	m_textures.Load(TextureID::kMissileRefill, "Media/Textures/MissileRefill.png");
	m_textures.Load(TextureID::kFireSpread, "Media/Textures/FireSpread.png");
	m_textures.Load(TextureID::kFireRate, "Media/Textures/FireRate.png");
	m_textures.Load(TextureID::kFinishLine, "Media/Textures/FinishLine.png");

	m_textures.Load(TextureID::kEntities, "Media/Textures/Entities.png");
	//m_textures.Load(TextureID::kJungle, "Media/Textures/Jungle.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");

	//Textures for the Ship Battle game
	m_textures.Load(TextureID::kWater, "Media/Textures/Water3.jpg");
	m_textures.Load(TextureID::kEnemyCannonBall, "Media/Textures/EnemyBall.png");
	m_textures.Load(TextureID::kPlayer2Ship, "Media/EnemyShips/ship13.png");
	m_textures.Load(TextureID::kMountains, "Media/Textures/mountain_area.png");


}

void World::BuildMountains()
{

	for (int i = 0; i < 10; i++)
	{	//Add The mountains in the scene
		sf::Texture& mountain_texture = m_textures.Get(TextureID::kMountains);
		std::unique_ptr<Obstacle> mountain = std::make_unique<Obstacle>(mountain_texture);
		mountain->setPosition(Utility::RandomInt(500),Utility::RandomInt(2500));
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(mountain));
	}
}

void World::BuildScene()
{
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
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(m_world_bounds.left, m_world_bounds.top);
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Add the finish line
	sf::Texture& finish_texture = m_textures.Get(TextureID::kFinishLine);
	std::unique_ptr<SpriteNode> finish_sprite(new SpriteNode(finish_texture));
	finish_sprite->setPosition(0.f, -76.f);
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(finish_sprite));

	BuildMountains();

	//Add Players here
	InitializePlayers();
	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	std::unique_ptr<ParticleNode> splashesNode(new ParticleNode(ParticleType::kWaterSplashes, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(splashesNode));

	std::unique_ptr<ParticleNode> mistNode(new ParticleNode(ParticleType::kWaterMist, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(mistNode));


	// Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scenegraph.AttachChild(std::move(soundNode));

	//AddEnemies();

	/*std::unique_ptr<Ship> left_escort(new Ship(ShipType::kEnemyShip1, m_textures, m_fonts));
	left_escort->setPosition(-80.f, 50.f);
	m_player_1_ship->AttachChild(std::move(left_escort));

	std::unique_ptr<Ship> right_escort(new Ship(ShipType::kEnemyShip1, m_textures, m_fonts));
	right_escort->setPosition(80.f, 50.f);
	m_player_1_ship->AttachChild(std::move(right_escort));*/
}

void World::AdaptPlayerPosition()
{
	//keep each player on the screen
	sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
	const float border_distance = 40.f;

	for (Ship* player : m_player_ships)
	{
		sf::Vector2f position = player->getPosition();
		position.x = std::max(position.x, view_bounds.left + border_distance);
		position.x = std::min(position.x, view_bounds.left + view_bounds.width - border_distance);
		position.y = std::max(position.y, view_bounds.top + border_distance);
		position.y = std::min(position.y, view_bounds.top + view_bounds.height - border_distance);
		player->setPosition(position);
	}
}

void World::AdaptPlayerVelocity()
{
	for (Ship* player : m_player_ships)
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
{
	//Spawn an enemy when it is relevant i.e when it is in the Battlefieldboudns
	while (!m_enemy_spawn_points.empty() && m_enemy_spawn_points.back().m_y > GetBattleFieldBounds().top)
	{
		SpawnPoint spawn = m_enemy_spawn_points.back();
		std::unique_ptr<Ship> enemy(new Ship(spawn.m_type, m_textures, m_fonts));
		enemy->setPosition(spawn.m_x, spawn.m_y);
		enemy->setRotation(0);
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(enemy));
		m_enemy_spawn_points.pop_back();
	}
}

void World::AddEnemies()
{
	AddEnemy(ShipType::kEnemyShip1, 0.f, 500.f);
	AddEnemy(ShipType::kEnemyShip1, 0.f, 1000.f);
	AddEnemy(ShipType::kEnemyShip1, 100.f, 1100.f);
	AddEnemy(ShipType::kEnemyShip1, -100.f, 1100.f);
	AddEnemy(ShipType::kEnemyShip2, -70.f, 1400.f);
	AddEnemy(ShipType::kEnemyShip2, 70.f, 1400.f);
	AddEnemy(ShipType::kEnemyShip2, 70.f, 1600.f);

	//Sort the enemies according to y-value so that enemies are checked first
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
	command.category = static_cast<int>(ReceiverCategories::kPlayer2Ship) | static_cast<int>(ReceiverCategories::kProjectile);
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


	//Target the closest enemy in the radius
	Command enemyCollector;
	enemyCollector.category = static_cast<int>(ReceiverCategories::kPlayer2Ship);
	enemyCollector.action = DerivedAction<Ship>([this](Ship& enemy, sf::Time)
		{
			if (!enemy.IsDestroyed())
			{
				m_active_enemies.emplace_back(&enemy);
			}
		});

	Command missileGuider;
	missileGuider.category = static_cast<int>(ReceiverCategories::kAlliedProjectile);
	missileGuider.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time dt)
		{
			if (!missile.IsGuided())
			{
				return;
			}

			float min_distance = std::numeric_limits<float>::max();
			Ship* closest_enemy = nullptr;

			//get missile launch position and current position
			sf::Vector2f missile_position = missile.GetWorldPosition();
			sf::Vector2f launch_position = missile.GetLaunchPosition();

			//check if missile has exceeded its range
			float dist_from_launch = std::hypot(
				missile_position.x - launch_position.x,
				missile_position.y - launch_position.y
			);
			if (dist_from_launch > missile.GetMaxRadius())
			{
				//missile.IsDestroyed();
				missile.Destroy();
				return;
			}

			for (Ship* enemy : m_active_enemies)
			{
				float enemy_distance = Distance(missile, *enemy);
				if (enemy_distance < min_distance && enemy_distance <= missile.GetMaxRadius())
				{
					closest_enemy = enemy;
					min_distance = enemy_distance;
				}
			}

			if (closest_enemy)
			{
				missile.GuideTowards(closest_enemy->GetWorldPosition());
			}
		});

	m_command_queue.Push(enemyCollector);
	m_command_queue.Push(missileGuider);
	m_active_enemies.clear();
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
	}
	else
	{
		return false;
	}
}


void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scenegraph.CheckSceneCollision(m_scenegraph, collision_pairs);
	for (SceneNode::Pair pair : collision_pairs)
	{
		if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kPlayer2Ship))
		{
			auto& player = static_cast<Ship&>(*pair.first);
			auto& enemy = static_cast<Ship&>(*pair.second);
			//Collision response
			player.Damage(enemy.GetHitPoints());
			enemy.Damage(10);
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
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kEnemyProjectile) || MatchesCategories(pair, ReceiverCategories::kPlayer2Ship, ReceiverCategories::kAlliedProjectile))
		{
			auto& aircraft = static_cast<Ship&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			aircraft.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		//if hits the mountain
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerShip, ReceiverCategories::kObstacle) || MatchesCategories(pair, ReceiverCategories::kPlayer2Ship, ReceiverCategories::kObstacle))
		{
			auto& aircraft = static_cast<Ship&>(*pair.first);
			auto& mountain = static_cast<Obstacle&>(*pair.second);
			//Collision response
			aircraft.Destroy();
		}
	}
}

void World::UpdateSounds()
{
	if (!m_player_ships.empty())
	{
		// Set listener's position to player position
		m_sounds.SetListenerPosition(m_player_ships[0]->GetWorldPosition());

		 //calculate the average position of all players
		
		/*sf::Vector2f average_position(0.f, 0.f);
		for (const Ship* player : m_player_ships)
		{
			average_position += player->GetWorldPosition();
		}
		average_position /= static_cast<float>(m_player_ships.size());
		m_sounds.SetListenerPosition(average_position);*/
		
	}

	// Remove unused sounds
	m_sounds.RemoveStoppedSounds();
}

void World::InitializePlayers()
{

	//player 1's ship
	std::unique_ptr<Ship> ship1(new Ship(ShipType::kPirateShip, m_textures, m_fonts));
	ship1->setPosition(m_spawn_position);
	ship1->SetVelocity(50.f, m_scrollspeed);
	m_player_ships.push_back(ship1.get());

	Player player1(1);
	m_players.emplace_back(player1);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(ship1));


	// Add player 2's ship
	std::unique_ptr<Ship> ship2(new Ship(ShipType::kPlayer2Ship, m_textures, m_fonts));
	ship2->setRotation(180);
	ship2->setPosition(130.f, 130.f);
	ship2->SetVelocity(50.f, 40.f);
	m_player_ships.emplace_back(ship2.get());

	Player player2(2);
	m_players.emplace_back(player2);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(ship2));

}


