#pragma once
#include "SceneNode.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"

class Cannon : public SceneNode
{
public:
    explicit Cannon(const TextureHolder& textures);

    void RotateLeft();
    void RotateRight();

    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::Sprite m_sprite;
    float m_rotationSpeed;
};

