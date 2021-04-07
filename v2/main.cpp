#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

const float pi = 3.14159265358979323846;

int frame_rate = 120;
#define scaling 1
#define map_size 720
#define map_area map_size * map_size
#define agent_perc 0.1 // <1
float velocity = 0.7;
float decay_rate = 0.02; // <1
float diffusion_rate = 0.7; // >0
float sensor_angle = pi/2;
int sensor_offset = 9;
float rotation_angle = pi/2;
int attraction = 1;

std::uniform_real_distribution<float> unif(0, pi*2);
std::uniform_int_distribution<> distr(0, 1);
std::default_random_engine re;


struct agent {
	std::array<float, 2> position;
	float angle;
	agent(float x, float y, float a): position {x, y}, angle(a) {}
	
};

void update_data(std::array<float, map_area>& data, const std::vector<agent>& agent_vec) {
	std::array<float, map_area> data_out{};
	// add  agents
	for(const agent& a : agent_vec) 
		data_out.at((int)a.position[0]+((int)a.position[1])*map_size)=1;
	data=data_out;
}

void update_trail(std::array<float, map_area>& trail, const std::array<float, map_area>& data) {
	// add new points
	for(int i=0; i<map_area; i++)
		if(data.at(i)==1)
			trail.at(i)=1;
	std::array<float, map_area> trail_update = trail;
	// diffusion and let points decay
	for(int i=0; i<map_area; i++) {
		float sur_sum=0;
		std::array<float, 2> range_x = {-1, 1};
		std::array<float, 2> range_y = {-1, 1};
		if(i<map_size)
			range_y[0]=0;
		else if(i>=map_area-map_size)
			range_y[1]=0;
		if(i%map_size==0)
			range_x[0]=0;
		else if(i%map_size==map_size-1)
			range_x[1]=0;
		
		for(int y=range_y[0]; y<=range_y[1]; y++)
			for(int x=range_x[0]; x<=range_x[1]; x++)
				sur_sum += trail.at(i+x+y*map_size);
		// number of neighbour squares
		int num_neigh = (range_x[1]-range_x[0]+1)*(range_y[1]-range_y[0]+1);
		// Linear interpolation
		float lerp = trail.at(i)+(1-diffusion_rate)*(sur_sum/num_neigh-trail.at(i));
		trail_update.at(i) = fmax(0, lerp-decay_rate);
	}
	trail=trail_update;
}

float sense(const agent& a, float angle, const std::array<float, map_area>& trail) {
	float relativ_angle = fmod(angle+a.angle, pi*2);
	int sensor_x = (int)(a.position[0]+sensor_offset*cos(pi*2-relativ_angle));
	int sensor_y = (int)(a.position[1]+sensor_offset*sin(pi*2-relativ_angle));
	if(sensor_x>=0 && sensor_x<map_size && sensor_y>=0 && sensor_y<map_size)
		return trail.at(sensor_x+sensor_y*map_size);
	return 0;
}

void move_agents(std::vector<agent>& agent_vec, const std::array<float, map_area>& trail, const std::array<float, map_area>& data) {
	// move
	for(agent& a : agent_vec) {
		float dx = velocity*cos(pi*2-a.angle);
		float dy = velocity*sin(pi*2-a.angle);
		if(a.position[0]+dx<0||a.position[0]+dx>=map_size||
		   a.position[1]+dy<0||a.position[1]+dy>=map_size) {
			a.position[0] = fmin(map_size-1, fmax(0, a.position[0]+dx));
			a.position[1] = fmin(map_size-1, fmax(0, a.position[1]+dy));
			a.angle = unif(re);
		}
		else if(((int)a.position[0]!=(int)(a.position[0]+dx) ||
		   (int)a.position[1]!=(int)(a.position[1]+dy)) &&
		   data.at((int)(a.position[0]+dx)+(int)(a.position[1]+dy)*map_size)!=0) {
			a.angle = unif(re);
		}
		else {
			a.position[0] += dx;
			a.position[1] += dy;
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


// map -> scaled pixel array -> draw(pixel array)

void update_image(sf::Image& image, const std::array<float, map_area>& trail, const std::vector<agent>& agent_vec) {
	for(int i=0; i<map_area; i++) {
			if(trail.at(i)!=0) {
				image.setPixel(i%map_size, (int)(i/map_size), sf::Color(trail.at(i)*255, trail.at(i)*255, trail.at(i)*255, 255));
// 				square.setPosition((i%map_size)*scaling, (i/map_size)*scaling);
// 				window.draw(square);
			} 
		}
		for(const agent& a : agent_vec) {
			sf::Color col = image.getPixel(a.position[0], a.position[1]);
			image.setPixel(a.position[0], a.position[1], col+sf::Color(0, 0, 255, 100));
// 			window.draw(square);
		}
}


int main() {
	std::vector<agent> agent_vec;

	std::array<float, map_area> data{};
	std::array<float, map_area> trail{};
	
	sf::Texture texture;
	texture.create(map_size, map_size);
	
	sf::Sprite sprite;
	
	sf::Image image;
	image.create(map_size, map_size, sf::Color::Black);
	
	for(int i=0; i<agent_perc*map_area; i++) 
		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size),(int)(unif(re)/(pi*2)*map_size),unif(re)));
	
	sf::RenderWindow window(sf::VideoMode(map_size*scaling, map_size*scaling), "Physarum");
	window.setFramerateLimit(frame_rate);
	window.setPosition(sf::Vector2i(0, 0));
	
	sf::RectangleShape square(sf::Vector2f(scaling, scaling));
	
//----------------------------------------------------------------------------------------
	sf::Clock clock;
	sf::Time calculate;
	sf::Time draw_window;
	int i=0;
	std::cout << "Number of agents: " << agent_perc*map_area << std::endl;
	std::cout << "total\t" << "calc/f\t" << "draw/f" << std::endl;
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
					// sensor offset o, p
					if (static_cast<char>(event.text.unicode)=='o') {
						if(sensor_offset>1)
							sensor_offset -= 1;
						std::cout << "sensor_offset: " << sensor_offset << std::endl;
					}
					if (static_cast<char>(event.text.unicode)=='p') {
						sensor_offset += 1;
						std::cout << "sensor_offset: " << sensor_offset << std::endl;
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
					break;
				
				default:
					break;
			}
		}
		
		clock.restart();
		move_agents(agent_vec, trail, data);
		update_data(data, agent_vec);
		update_trail(trail, data);
		calculate += clock.getElapsedTime();
		
		clock.restart();
		window.clear(sf::Color::Black);
		
// 		for(int i=0; i<map_area; i++) {
// 			if(trail.at(i)!=0) {
// 				square.setFillColor(sf::Color(trail.at(i)*255, trail.at(i)*255, trail.at(i)*255, 255));
// 				square.setPosition((i%map_size)*scaling, (i/map_size)*scaling);
// 				window.draw(square);
// 			} 
// 		}
// 		square.setFillColor(sf::Color(0, 0, 255, 100));
// 		for(const agent& a : agent_vec) {
// 			square.setPosition((int)a.position[0]*scaling, (int)a.position[1]*scaling);
// 			window.draw(square);
// 		}
		
		update_image(image, trail, agent_vec);
		
		texture.update(image);
		
		sprite.setTexture(texture);
		
		window.draw(sprite);
		
		window.display();
		draw_window += clock.getElapsedTime();
		
//----------------------------------------------------------------------------------------
        if(i>frame_rate) {
			std::cout << (draw_window.asMilliseconds()+calculate.asMilliseconds())/frame_rate*60 << "\t"
					  << calculate.asMicroseconds()/frame_rate << "\t"
					  << draw_window.asMicroseconds()/frame_rate << std::endl;
			i=0;
			calculate=sf::seconds(0);
			draw_window=sf::seconds(0);
		}
		i++;
//----------------------------------------------------------------------------------------
    }
    
	return 0;
}

