#include "Cannon.hpp"
#include "Utility.hpp"

//Dawood Parhiar D00248313
Cannon::Cannon(const TextureHolder& textures)
    : m_sprite(textures.Get(TextureID::kCannon)),
    m_rotationSpeed(100.f),  
    m_rotationInput(0.f) 
{
    Utility::CentreOrigin(m_sprite);
}

void Cannon::SetRotationInput(float rotation)
{
    m_rotationInput = rotation;
}

sf::Vector2f Cannon::GetMouthPosition() const
{
    float angleRad = getRotation() * (3.14159265f / 180.f); // Convert degrees to radians

    // Cannon mouth is at the front (adjust based on your sprite size)
    float offsetX = std::cos(angleRad);  // Move forward
    float offsetY = std::sin(angleRad);

    return GetWorldPosition() + sf::Vector2f(offsetX, offsetY);
}

void Cannon::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    
    rotate(m_rotationInput * m_rotationSpeed * dt.asSeconds());
    m_rotationInput = 0;
}

void Cannon::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}
