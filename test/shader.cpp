#include <iostream>
#include <SFML/Graphics.hpp>

int main() {
	sf::RenderWindow window(sf::VideoMode(512, 512), "shader");
// 	window.setFramerateLimit(30);
	
	sf::Shader shader;
	shader.loadFromFile("shader.frag", sf::Shader::Fragment);
	
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
				case sf::Event::Closed:
					window.close();
					break;
				
				default:
					break;
			}
		}
		
		sf::RectangleShape square(sf::Vector2f(100, 100));
		square.setPosition(0, 0);
		
		shader.setUniform("blink_alpha", 0.5f);
		
		window.clear(sf::Color::Black);
		window.draw(square, &shader);
        window.display();
    }
    
	return 0;
}
