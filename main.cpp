#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

const float pi = 3.14159265358979323846;

#define frame_rate 60
#define scaling 4
#define map_size 200
#define map_area map_size * map_size
#define agent_perc 0.15 // <1
#define velocity 1.0
#define decay_rate 0.01 // <1
#define diffusion_rate 0.6 // >0
#define sensor_angle pi/4
#define sensor_offset 9
#define rotation_angle pi/4

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
}


// map -> scaled pixel array -> draw(pixel array)



int main() {
	std::vector<agent> agent_vec;

	std::array<float, map_area> data{};
	std::array<float, map_area> trail{};
	
	for(int i=0; i<agent_perc*map_area; i++) 
		agent_vec.push_back(agent((int)(unif(re)/(pi*2)*map_size),(int)(unif(re)/(pi*2)*map_size),unif(re)));
	
	sf::RenderWindow window(sf::VideoMode(map_size*scaling, map_size*scaling), "Physarum");
	window.setFramerateLimit(frame_rate);
	
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
		
		for(int i=0; i<map_area; i++) {
			if(trail.at(i)!=0) {
				square.setFillColor(sf::Color(trail.at(i)*255, trail.at(i)*255, trail.at(i)*255, 255));
				square.setPosition((i%map_size)*scaling, (i/map_size)*scaling);
				window.draw(square);
			} 
		}
		/*
		square.setFillColor(sf::Color::Blue);
		for(const agent& a : agent_vec) {
			square.setPosition((int)a.position[0]*scaling, (int)a.position[1]*scaling);
			window.draw(square);
		}
		*/
		window.display();
		draw_window += clock.getElapsedTime();
		
//----------------------------------------------------------------------------------------
        if(i==59) {
			std::cout << draw_window.asMilliseconds()+calculate.asMilliseconds() << "\t"
					  << calculate.asMilliseconds()/60 << "\t"
					  << draw_window.asMilliseconds()/60 << std::endl;
			i=0;
			calculate=sf::seconds(0);
			draw_window=sf::seconds(0);
		}
		i++;
//----------------------------------------------------------------------------------------
    }
    
	return 0;
}

