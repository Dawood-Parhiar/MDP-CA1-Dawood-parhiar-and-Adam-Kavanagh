#include "Cannon.hpp"
#include "Utility.hpp"

Cannon::Cannon(const TextureHolder& textures)
    : m_sprite(textures.Get(TextureID::kCannon)), m_rotationSpeed(100.f)  // Adjust rotation speed if needed
{
    Utility::CentreOrigin(m_sprite);
}

void Cannon::RotateLeft()
{
    rotate(-m_rotationSpeed);  // Rotate counterclockwise
}

void Cannon::RotateRight()
{
    rotate(m_rotationSpeed);  // Rotate clockwise
}

void Cannon::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    // Handle rotation input
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        RotateLeft();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        RotateRight();
}

void Cannon::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}
