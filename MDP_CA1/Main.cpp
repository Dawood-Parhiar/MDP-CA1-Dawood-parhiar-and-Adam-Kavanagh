#include <SFML/Graphics.hpp>

#include <iostream>

int main()
{
   
    std::cout << "Program started" << std::endl;
    std::cout << "Hello World!" << std::endl;

    sf::RenderWindow window(sf::VideoMode(800, 600), "Multiplayer Game");
    if (!window.isOpen())
    {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

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

    return 0;
}
