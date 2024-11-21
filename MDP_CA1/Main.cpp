#include <iostream>
#include <SFML/Graphics.hpp>


void sampleWindowSFML()
{
    // Create a window with a circle
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while(window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        window.clear();
        window.draw(shape);
        window.display();
    }
    
    
}
int main()
{
    //This is going to be our Multiplayer game in c++ using sfml
    std::cout << "Hello World!" << std::endl;
    sampleWindowSFML();
    return 0;
}

