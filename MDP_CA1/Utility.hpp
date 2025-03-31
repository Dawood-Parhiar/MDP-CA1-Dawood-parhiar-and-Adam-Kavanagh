#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include "Animation.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "State.hpp"

class Utility
{
private:
	static void CreateButton(State::Context& context, std::shared_ptr<gui::Button>& play_button, int x,
		int y, const std::string& label, const gui::Button::Callback& callback,
		bool toggle, const std::function<bool()>& predicate);

	public:
		static sf::Vector2f UnitVector(const sf::Vector2f& source);
		static void CentreOrigin(sf::Sprite& sprite);
		static void LeftOrigin(sf::Sprite& sprite);
		static void CentreOrigin(sf::Text& text);
		static void CentreOrigin(Animation& animation);
		static std::string toString(sf::Keyboard::Key key);
		static double ToRadians(int degrees);
		static double ToDegrees(double angle);
		static int RandomInt(int exclusive_max);
		static int Length(sf::Vector2f vector);
	    static void CreateButton(State::Context& context, std::shared_ptr<gui::Button>& button, int x, int y,
	                  const std::string& label,
	                  const gui::Button::Callback& callback, const std::function<bool()>& predicate);
	    static void CreateButton(State::Context& context, std::shared_ptr<gui::Button>& button, int x, int y,
	                         const std::string& label, bool toggle);

		static void CreateLabel(const State::Context& context, ::std::shared_ptr<gui::Label>& label, int x, int y,
		                 const std::string& label_text, int text_size);
		static void Debug(const std::string& message);
};

