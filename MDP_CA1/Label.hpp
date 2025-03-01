#pragma once
#include "Component.hpp"
#include "ResourceIdentifiers.hpp"
#include "State.hpp"

namespace gui
{
	class Label : public Component
	{
	public:
		typedef std::shared_ptr<Label> Ptr;

	public:
		Label(const std::string& text, const FontHolder& fonts, const TextureHolder& textures);
		virtual bool IsSelectable() const override;
		void SetText(const std::string& text);
		void HandleEvent(const sf::Event& event) override;

	private:
		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	private:
		sf::Text m_text;
		sf::Sprite m_sprite;
	};
}

