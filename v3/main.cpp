#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

const float pi = 3.14159265358979323846;

int frame_rate = 30;
#define scaling 3
#define map_size 300
#define map_area map_size * map_size
#define agent_perc 0.15 // <1
float velocity = 0.7;
float decay_rate = 0.02; // <1
float diffusion_rate = 0.5; // >0
float sensor_angle = pi/4;
float rotation_angle = pi/4;
int attraction = 1;
float species_repulsion = 0.1;

int r_sensor_offset = 4;
int g_sensor_offset = 16;
int b_sensor_offset = 32;

std::uniform_real_distribution<float> unif(0, pi*2);
std::uniform_int_distribution<> distr(0, 1);
std::uniform_int_distribution<> spec(0, 2);
std::default_random_engine re;


struct agent {
	std::array<float, 2> position;
	float angle;
	std::array<int, 3> species{};
	agent(float x, float y, float a, int s): position {x, y}, angle(a) {
		species[s]=1;
	}
};

void update_data(sf::Image& data, const std::vector<agent>& agent_vec, const std::vector<agent>& prev_agent_vec) {
	for(const agent& a : prev_agent_vec)
		data.setPixel((int)a.position[0], (int)a.position[1], sf::Color::Black);
	// add  agents
	for(const agent& a : agent_vec)
		data.setPixel((int)a.position[0], (int)a.position[1], sf::Color(a.species[0]*255, a.species[1]*255, a.species[2]*255, 255));
}

void update_trail(sf::Image& trail, const sf::Image& data, const std::vector<agent>& agent_vec) {
	// add new points
	for(const agent& a : agent_vec)
		if(data.getPixel(a.position[0], a.position[1])==sf::Color(a.species[0]*255, a.species[1]*255, a.species[2]*255, 255))
			trail.setPixel(a.position[0], a.position[1], sf::Color(a.species[0]*255, a.species[1]*255, a.species[2]*255, 255));
	sf::Image trail_update = trail;
	// diffusion and let points decay
	for(int c=0; c<3; c++)
		for(int y=0; y<map_size; y++)
			for(int x=0; x<map_size; x++) {
				int surr_sum = 0;
				std::array<float, 2> range_y = {-1, 1};
				std::array<float, 2> range_x = {-1, 1};
				if(y<=0)
					range_y[0]=0;
				else if(y>=map_size-1)
					range_y[1]=0;
				if(x<=0)
					range_x[0]=0;
				else if(x>=map_size-1)
					range_x[1]=0;
	
				for(int ys=range_y[0]; ys<=range_y[1]; ys++)
					for(int xs=range_x[0]; xs<=range_x[1]; xs++) {
						if(c==0)
							surr_sum += trail.getPixel(x+xs, y+ys).r;
						else if(c==1)
							surr_sum += trail.getPixel(x+xs, y+ys).g;
						else
							surr_sum += trail.getPixel(x+xs, y+ys).b;
					}
				// number of neighbour squares
				int num_neigh = (range_x[1]-range_x[0]+1)*(range_y[1]-range_y[0]+1);
				// Linear interpolation
				if(c==0) {
					float lerp = trail.getPixel(x, y).r+diffusion_rate*(surr_sum/num_neigh-trail.getPixel(x, y).r);
					trail_update.setPixel(x, y, sf::Color(fmax(0, (int)(lerp-255*decay_rate)), trail_update.getPixel(x, y).g, trail_update.getPixel(x, y).b, 255));
				} else if(c==1) {
					float lerp = trail.getPixel(x, y).g+diffusion_rate*(surr_sum/num_neigh-trail.getPixel(x, y).g);
					trail_update.setPixel(x, y, sf::Color(trail_update.getPixel(x, y).r, fmax(0, (int)(lerp-255*decay_rate)), trail_update.getPixel(x, y).b, 255));
				} else {
					float lerp = trail.getPixel(x, y).b+diffusion_rate*(surr_sum/num_neigh-trail.getPixel(x, y).b);
					trail_update.setPixel(x, y, sf::Color(trail_update.getPixel(x, y).r, trail_update.getPixel(x, y).g, fmax(0, (int)(lerp-255*decay_rate)), 255));
				}
			}
	trail=trail_update;
}

float sense(const agent& a, float angle, const sf::Image& trail) {
	float relativ_angle = fmod(angle+a.angle, pi*2);
	int sensor_x;
	int sensor_y;
	if(a.species[0]){
		sensor_x = (int)(a.position[0]+r_sensor_offset*cos(pi*2-relativ_angle));
		sensor_y = (int)(a.position[1]+r_sensor_offset*sin(pi*2-relativ_angle));
	}
	else if(a.species[1]){
		sensor_x = (int)(a.position[0]+g_sensor_offset*cos(pi*2-relativ_angle));
		sensor_y = (int)(a.position[1]+g_sensor_offset*sin(pi*2-relativ_angle));
	}
	else {
		sensor_x = (int)(a.position[0]+b_sensor_offset*cos(pi*2-relativ_angle));
		sensor_y = (int)(a.position[1]+b_sensor_offset*sin(pi*2-relativ_angle));
	}
	if(sensor_x>=0 && sensor_x<map_size && sensor_y>=0 && sensor_y<map_size) {
		if(a.species[0])
			return trail.getPixel(sensor_x, sensor_y).r - species_repulsion*(trail.getPixel(sensor_x, sensor_y).g + trail.getPixel(sensor_x, sensor_y).b);
		else if(a.species[1])
			return trail.getPixel(sensor_x, sensor_y).g - species_repulsion*(trail.getPixel(sensor_x, sensor_y).r + trail.getPixel(sensor_x, sensor_y).b);
		else
			return trail.getPixel(sensor_x, sensor_y).b - species_repulsion*(trail.getPixel(sensor_x, sensor_y).r + trail.getPixel(sensor_x, sensor_y).g);
	}
	return 0;
}

void move_agents(std::vector<agent>& agent_vec, std::vector<agent>& prev_agent_vec, const sf::Image& trail) {
	// move
	prev_agent_vec = agent_vec;
	for(agent& a : agent_vec) {
		a.position[0] += velocity*cos(pi*2-a.angle);
		a.position[1] += velocity*sin(pi*2-a.angle);
		if(a.position[0]<0||a.position[0]>=map_size||
		   a.position[1]<0||a.position[1]>=map_size) {
			a.position[0] = fmin(map_size-1, fmax(0, a.position[0]));
			a.position[1] = fmin(map_size-1, fmax(0, a.position[1]));
			a.angle = unif(re);
		}
	}
	// sense
	// attraction
	if(attraction==1)
		for(agent& a : agent_vec) {
			float attr_forward = sense(a, 0, trail);
			float attr_left = sense(a, -sensor_angle, trail);
			float attr_right = sense(a, sensor_angle, trail);
			if(attr_forward>attr_left && attr_forward>attr_right)
				continue;
			else if(attr_forward<attr_left && attr_forward<attr_right)
				a.angle = fmod(a.angle+((float)distr(re)-0.5)*2*rotation_angle, pi*2);
			else if(attr_left<attr_right)
				a.angle = fmod(a.angle+rotation_angle, pi*2);
			else if(attr_left>attr_right)
				a.angle = fmod(a.angle-rotation_angle, pi*2);
			else continue;
		}
	// repulsion
	if(attraction==-1)
		for(agent& a : agent_vec) {
			float attr_forward = sense(a, 0, trail);
			float attr_left = sense(a, -sensor_angle, trail);
			float attr_right = sense(a, sensor_angle, trail);
			if(attr_forward<attr_left && attr_forward<attr_right)
				continue;
			else if(attr_forward>attr_left && attr_forward>attr_right)
				a.angle = fmod(a.angle+((float)distr(re)-0.5)*2*rotation_angle, pi*2);
			else if(attr_left>attr_right)
				a.angle = fmod(a.angle+rotation_angle, pi*2);
			else if(attr_left<attr_right)
				a.angle = fmod(a.angle-rotation_angle, pi*2);
			else continue;
		}
	
}


int main() {
	std::vector<agent> prev_agent_vec;
	std::vector<agent> agent_vec;

	sf::Image data;
	data.create(map_size, map_size, sf::Color::Black);
	sf::Image trail;
	trail.create(map_size, map_size, sf::Color::Black);
	
	sf::Texture texture;
	texture.create(map_size, map_size);
	
	sf::Sprite sprite;
	sprite.setScale(scaling, scaling);
	
// 	for(int i=0; i<agent_perc*map_area; i++)
// 		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size), (int)(unif(re)/(pi*2)*map_size), unif(re), spec(re)));
	
	for(int i=0; i<agent_perc*map_area*2/6; i++)
		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size), (int)(unif(re)/(pi*2)*map_size), unif(re), 0));
	for(int i=0; i<agent_perc*map_area*2/6; i++)
		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size), (int)(unif(re)/(pi*2)*map_size), unif(re), 1));
	for(int i=0; i<agent_perc*map_area*2/6; i++)
		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size), (int)(unif(re)/(pi*2)*map_size), unif(re), 2));
	
	
	sf::RenderWindow window(sf::VideoMode(map_size*scaling, map_size*scaling), "Physarum");
	window.setFramerateLimit(frame_rate);
	window.setPosition(sf::Vector2i(0, 0));
	
	sf::RectangleShape square(sf::Vector2f(scaling, scaling));
	
//----------------------------------------------------------------------------------------
	sf::Clock clock;
	sf::Clock clock2;
	sf::Time total;
	sf::Time calculate;
	sf::Time draw_window;
	sf::Time probe;
	sf::Time tmp;
	int i=0;
	std::cout << "Number of agents: " << agent_perc*map_area << std::endl;
	std::cout << "total\t" << "calc/f\t" << "draw/f\t" << "probe/f" << std::endl;
//----------------------------------------------------------------------------------------
	
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
				case sf::Event::Closed:
					window.close();
					break;
				
				case sf::Event::TextEntered:
					// frame rate q, w
					if (static_cast<char>(event.text.unicode)=='q') {
						if(frame_rate>10) frame_rate -= 10;
						window.setFramerateLimit(frame_rate);
						std::cout << "frame_rate: " << frame_rate << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='w') {
						frame_rate += 10;
						window.setFramerateLimit(frame_rate);
						std::cout << "frame_rate: " << frame_rate << std::endl;
					}
					// toggle attraction
					if (static_cast<char>(event.text.unicode)=='a') {
						attraction *= -1;
						std::cout << "attraction: " << attraction << std::endl;
					}
					// velocity e, r
					if (static_cast<char>(event.text.unicode)=='e') {
						velocity -= 0.1;
						std::cout << "velocity: " << velocity << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='r') {
						velocity += 0.1;
						std::cout << "velocity: " << velocity << std::endl;
					}
					// diffusion t, z
					if (static_cast<char>(event.text.unicode)=='t') {
						if(diffusion_rate>0.1)
							diffusion_rate -= 0.1;
						std::cout << "diffusion_rate: " << diffusion_rate << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='z') {
						if(diffusion_rate<1)
							diffusion_rate += 0.1;
						std::cout << "diffusion_rate: " << diffusion_rate << std::endl;
					}
					// decay u, i
					if (static_cast<char>(event.text.unicode)=='u') {
						if(decay_rate>0.01)
							decay_rate -= 0.01;
						std::cout << "decay_rate: " << decay_rate << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='i') {
						if(decay_rate<1)
							decay_rate += 0.01;
						std::cout << "decay_rate: " << decay_rate << std::endl;
					}
					// sensor angle s, d
					if (static_cast<char>(event.text.unicode)=='s') {
						sensor_angle -= pi/8;
						std::cout << "sensor_angle: " << sensor_angle/pi << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='d') {
						sensor_angle += pi/8;
						std::cout << "sensor_angle: " << sensor_angle/pi << std::endl;
					}
					// rotation angle y, x
					if (static_cast<char>(event.text.unicode)=='y') {
						rotation_angle -= pi/8;
						std::cout << "rotation_angle: " << rotation_angle/pi << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='x') {
						rotation_angle += pi/8;
						std::cout << "rotation_angle: " << rotation_angle/pi << std::endl;
					}
					// species repulsion f, g
					if (static_cast<char>(event.text.unicode)=='f') {
						species_repulsion -= 0.1;
						std::cout << "species_repulsion: " << species_repulsion << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='g') {
						species_repulsion += 0.1;
						std::cout << "species_repulsion: " << species_repulsion << std::endl;
					}
					break;
				
				default:
					break;
			}
		}
		
		clock.restart();
		move_agents(agent_vec, prev_agent_vec, trail);
		update_data(data, agent_vec, prev_agent_vec);
		
		tmp += clock.getElapsedTime();
		
		update_trail(trail, data, agent_vec);
		
		probe += clock.getElapsedTime()-tmp;
		calculate += clock.getElapsedTime();
		tmp=sf::seconds(0);
		
		clock.restart();
		
		window.clear(sf::Color::Black);
		texture.update(trail);
		sprite.setTexture(texture);
		window.draw(sprite);
		window.display();
		
		draw_window += clock.getElapsedTime();
		
		total = clock2.getElapsedTime();
		
//----------------------------------------------------------------------------------------
        if(i>frame_rate-1) {
			std::cout << clock2.getElapsedTime().asSeconds() << "\t"
					  << calculate.asMicroseconds()/frame_rate << "\t"
					  << draw_window.asMicroseconds()/frame_rate << "\t"
					  << probe.asMicroseconds()/frame_rate << std::endl;
			i=0;
			clock2.restart();
			calculate=sf::seconds(0);
			draw_window=sf::seconds(0);
			probe=sf::seconds(0);
		}
		i++;
//----------------------------------------------------------------------------------------
    }
    
	return 0;
}

