#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kPlayerShip = 1 << 1,
	kEnemyShip = 1 << 2,
	kEnemyProjectile = 1 << 3,
	kPickup = 1 << 4,

	kShip = kPlayerShip  | kEnemyShip,
	
};

// A message would be sent to all aircraft
//unsigned int all_aircraft = ReceiverCategories::kPlayerAircraft | ReceiverCategories::kAlliedAircraft | ReceiverCategories::kEnemyAircraft