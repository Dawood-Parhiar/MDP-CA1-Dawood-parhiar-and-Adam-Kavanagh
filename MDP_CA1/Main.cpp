#include <SFML/Graphics.hpp>
#include "Game.hpp"
#include "ResourceIdentifiers.hpp"
#include <iostream>
#include "Application.hpp"

int main()
{
	//TextureHolder game_textures;
	try
	{
		Application app;
		app.Run();
	}
	catch(std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}
}
//
// #include <cmath>
//
// // Utility function to convert degrees to radians
// namespace Utility2 {
//     float ToRadians(float degrees) {
//         return degrees * 3.14159265358979323846f / 180.f;
//     }
// }
//
// int main() {
//     // Create the main window
//     sf::RenderWindow window(sf::VideoMode(800, 600), "Aim Arc Simulation");
//
//     // Constants for the aiming arc
//     const float radius = 300.f;
//     const float arcAngle = 90.f;
//     const int numSegments = 50;
//     const float transparency = 128;
//
//     // Base position and rotation of the ship
//     sf::Vector2f shipPosition(400.f, 300.f); // Center of the window
//     float shipRotation = 0.f; // Initial rotation angle in degrees
//
//     // Create the aiming arc
//     sf::VertexArray aimingArc(sf::TriangleStrip, (numSegments + 1) * 2);
//
//     // Flag to track if the right shift key is pressed
//     bool isRightShiftPressed = false;
//
//     while (window.isOpen()) {
//         // Process events
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed) {
//                 window.close();
//             } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::RShift) {
//                 isRightShiftPressed = true;
//             } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::RShift) {
//                 isRightShiftPressed = false;
//             }
//         }
//
//         // Update the aiming arc based on the current rotation
//         for (int i = 0; i <= numSegments; ++i) {
//             // Calculate the angle for this segment
//             float angle = shipRotation - arcAngle / 2.f + (arcAngle * i / numSegments);
//             float angleRadians = Utility2::ToRadians(angle);
//
//             // Outer and inner points for the thick line
//             sf::Vector2f outerPoint = shipPosition + sf::Vector2f(radius * std::cos(angleRadians), radius * std::sin(angleRadians));
//             sf::Vector2f innerPoint = shipPosition + sf::Vector2f((radius - 20.f) * std::cos(angleRadians), (radius - 20.f) * std::sin(angleRadians)); // Adjust thickness
//
//             // Add points to the vertex array
//             aimingArc[i * 2].position = outerPoint;
//             aimingArc[i * 2].color = sf::Color(255, 255, 255, transparency); // Transparent white
//             aimingArc[i * 2 + 1].position = innerPoint;
//             aimingArc[i * 2 + 1].color = sf::Color(255, 255, 255, transparency);
//         }
//
//         // Clear the window
//         window.clear();
//
//         // Draw the aiming arc if the right shift key is pressed
//         if (isRightShiftPressed) {
//             window.draw(aimingArc);
//         }
//
//         // Display the contents of the window
//         window.display();
//     }
//
//     return 0;
// }

// #include <SFML/Graphics.hpp>
// #include <cmath>
//
// int main() {
//     // Create the main window
//     sf::RenderWindow window(sf::VideoMode(800, 600), "Square Movement and Rotation");
//
//     // Create a square shape
//     sf::RectangleShape square(sf::Vector2f(100.f, 100.f));
//     square.setFillColor(sf::Color::Green);
//     square.setOrigin(50.f, 50.f); // Set origin to the center of the square
//     square.setPosition(400.f, 300.f); // Center of the window
//
//     // Movement and rotation parameters
//     const float moveSpeed = 200.f; // pixels per second
//     const float rotationSpeed = 90.f; // degrees per second
//
//     // Clock to track time
//     sf::Clock clock;
//
//     while (window.isOpen()) {
//         // Process events
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed) {
//                 window.close();
//             }
//         }
//
//         // Get the elapsed time
//         sf::Time deltaTime = clock.restart();
//         float dt = deltaTime.asSeconds();
//
//         // Handle rotation
//         if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
//             square.rotate(-rotationSpeed * dt);
//         }
//         if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
//             square.rotate(rotationSpeed * dt);
//         }
//
//         // Handle movement
//         if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
//             float angle = square.getRotation();
//             float angleRadians = angle * 3.14159265358979323846f / 180.f;
//             square.move(std::cos(angleRadians) * moveSpeed * dt, std::sin(angleRadians) * moveSpeed * dt);
//         }
//         if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
//             float angle = square.getRotation();
//             float angleRadians = angle * 3.14159265358979323846f / 180.f;
//             square.move(-std::cos(angleRadians) * moveSpeed * dt, -std::sin(angleRadians) * moveSpeed * dt);
//         }
//
//         // Clear the window
//         window.clear();
//
//         // Draw the square
//         window.draw(square);
//
//         // Display the contents of the window
//         window.display();
//     }
//
//     return 0;
// }