#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kPlayerShip = 1 << 1,
	kPlayer2Ship = 1 << 2,
	kEnemyShip = 1 << 3,
	kAlliedProjectile = 1 << 4,
	kEnemyProjectile = 1 << 5,
	kPickup = 1 << 6,
	kParticleSystem = 1 << 7,
	kSoundEffect = 1 << 8,
	kObstacle = 1 << 9,
	kCoin = 1 << 10,

	kShip = kPlayerShip | kPlayer2Ship | kEnemyShip,
	kProjectile = kAlliedProjectile | kEnemyProjectile
};

// A message would be sent to all aircraft
//unsigned int all_aircraft = ReceiverCategories::kPlayerShip | ReceiverCategories::kAlliedShip | ReceiverCategories::kEnemyShip