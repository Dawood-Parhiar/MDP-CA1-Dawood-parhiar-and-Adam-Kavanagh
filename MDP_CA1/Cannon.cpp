#include "Cannon.hpp"
#include "Utility.hpp"

//Dawood Parhiar D00248313
Cannon::Cannon(const TextureHolder& textures)
    : m_sprite(textures.Get(TextureID::kCannon)),
    m_rotationSpeed(100.f),
    m_rotationInput(0.f),
	Entity(1)
{
    Utility::CentreOrigin(m_sprite);
    
}

void Cannon::SetRotationInput(float rotation)
{
    m_rotationInput = rotation;
}


sf::Vector2f Cannon::GetMouthPosition() const
{
    float angleRad = getRotation() * (3.14159265f / 180.f); 
    float offsetX = std::cos(angleRad); 
    float offsetY = std::sin(angleRad);

    return GetWorldPosition() + sf::Vector2f(offsetX, offsetY);
}

//void Cannon::CreateProjectile(SceneNode& node, ProjectileType type, const TextureHolder& textures) const
//{
//    // Create a new projectile
//    std::unique_ptr<Projectile> projectile = std::make_unique<Projectile>(type, textures);
//
//    // Get cannon's world position and rotation
//    float angleRad = Utility::ToRadians(getRotation()); // Convert angle to radians
//
//    float barrelLength = 30.f; // Adjust based on cannon size
//    sf::Vector2f offset(
//        std::cos(angleRad) * barrelLength,  // Move forward along rotation
//        std::sin(angleRad) * barrelLength
//    );
//    sf::Vector2f spawnPosition = GetWorldPosition() + offset;
//    // Calculate velocity based on cannon's rotation
//    sf::Vector2f velocity(
//        std::cos(angleRad) * 100.f, // Forward X direction
//        std::sin(angleRad) * 100.f  // Forward Y direction
//    );
//
//    // Set projectile properties
//    projectile->setPosition(spawnPosition);
//    projectile->SetVelocity(velocity);
//
//    //if (type == ProjectileType::kMissile)
//    {
//        projectile->SetLaunchPosition(spawnPosition);
//        projectile->SetMaxRadius(300.f);
//    }
//
//    // Attach projectile to the scene
//    node.AttachChild(std::move(projectile));
//}

void Cannon::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    
    rotate(m_rotationInput * m_rotationSpeed * dt.asSeconds());
    m_rotationInput = 0;
    GetMouthPosition();
}

void Cannon::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}
