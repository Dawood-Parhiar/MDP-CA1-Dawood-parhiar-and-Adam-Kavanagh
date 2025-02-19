#include "TextNode.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"

TextNode::TextNode(const FontHolder& fonts, std::string& text)
	:m_text(text, fonts.Get(Font::kMain), 22)
{
}
//Dawood Parhiar D00248313, changed color to red
void TextNode::SetString(const std::string& text)
{
	m_text.setString(text);
	m_text.setFillColor(sf::Color::Red);
	Utility::CentreOrigin(m_text);
}

void TextNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_text, states);
}
