#pragma once
#include <SFML/Graphics.hpp>
#include "ResourceIdentifiers.hpp"
#include "ResourceHolder.hpp"
#include "SceneNode.hpp"
#include "SceneLayers.hpp"
#include "Ship.hpp"
#include "TextureID.hpp"
#include "SpriteNode.hpp"
#include "CommandQueue.hpp"
#include "BloomEffect.hpp"
#include "SoundPlayer.hpp"

#include <array>

#include "Player.hpp"
#include "WaterEffects.h"

class World : private sf::NonCopyable
{
public:
	explicit World(sf::RenderTarget& target, FontHolder& font, SoundPlayer& sounds);
	void Update(sf::Time dt);
	void Draw();

	CommandQueue& GetCommandQueue();

	Ship* AddShip(int id);
	void RemoveShip(int id);

	Ship* GetShip(int id) const;

	bool HasAlivePlayer() const;
	bool HasPlayerReachedEnd() const;


private:
	void LoadTextures();
	void BuildMountains();
	void DropCoins();
	void DropCoins(sf::Vector2f position);
	void SpawnInitialCoins();
	void BuildScene();
	void AdaptPlayerPosition();
	void AdaptPlayerVelocity();

	void SpawnEnemies();
	void AddEnemies();
	void AddEnemy(ShipType type, float relx, float rely);
	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;

	void DestroyEntitiesOutsideView();
	void GuideMissiles();

	void HandleCollisions();
	void UpdateSounds();



private:
	struct SpawnPoint
	{
		SpawnPoint(ShipType type, float x, float y) :m_type(type), m_x(x), m_y(y)
		{

		}
		ShipType m_type;
		float m_x;
		float m_y;
	};

private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds;
	SceneNode m_scenegraph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;

	float m_scrollspeed;

	std::vector<Ship*> m_player_ships;
	CommandQueue m_command_queue;

	std::vector<SpawnPoint> m_enemy_spawn_points;
	std::vector<Ship*> m_active_enemies;

	BloomEffect m_bloom_effect;
	WaterEffects m_water_effect;
};

