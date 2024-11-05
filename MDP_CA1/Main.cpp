#include <SFML/Graphics.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    //This is going to be our Multiplayer game in c++ using sfml
    std::cout << "Hello World!" << std::endl;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Multiplayer Game");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);
    window.setFramerateLimit(60);
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
        window.draw(shape);
        window.display();
    }
    
}
