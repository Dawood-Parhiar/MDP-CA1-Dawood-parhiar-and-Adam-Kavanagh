#include "Obstacle.hpp"
#include "Utility.hpp"

Obstacle::Obstacle(const sf::Texture& texture)//Code changes from Dawood Parhiar D00248313
    : Entity(1)  // Assign some hitpoints if needed
    , m_sprite(texture)
{
    Utility::CentreOrigin(m_sprite);
}

unsigned int Obstacle::GetCategory() const
{
    return static_cast<int>(ReceiverCategories::kObstacle);
}

sf::FloatRect Obstacle::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

void Obstacle::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}
