#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <iostream>
#include <optional> // Required for SFML 3.0

// --- PHYSICS CONSTANTS ---
const double G = 6.67430e-11;
const double M_EARTH = 5.972e24;
const double R_EARTH = 6371000.0;
const double EARTH_ROTATION_SPEED = 7.2921159e-5; // rad/s

struct Vector3 {
    double x, y, z;
    Vector3 operator+(const Vector3& other) const { return {x+other.x, y+other.y, z+other.z}; }
    Vector3 operator*(double s) const { return {x*s, y*s, z*s}; }
    double magnitude() const { return std::sqrt(x*x + y*y + z*z); }
};

class Satellite {
public:
    Vector3 pos;
    Vector3 vel;

    Satellite(Vector3 p, Vector3 v) : pos(p), vel(v) {}

    void update(double dt) {
        double r = pos.magnitude();
        double accelMag = (G * M_EARTH) / (r * r * r);
        Vector3 accel = pos * (-accelMag);
        vel = vel + (accel * dt);
        pos = pos + (vel * dt);
    }
};

// --- Geography Helper ---
Vector3 getStationPos(double lat, double lon, double time) {
    double latRad = lat * (M_PI / 180.0);
    double lonRad = lon * (M_PI / 180.0);

    // 1. Fixed Earth Position (ECEF)
    double x = R_EARTH * std::cos(latRad) * std::cos(lonRad);
    double y = R_EARTH * std::cos(latRad) * std::sin(lonRad);
    double z = R_EARTH * std::sin(latRad);

    // 2. Rotate with Earth 
    double rotationAngle = EARTH_ROTATION_SPEED * time;

    // Rotation Matrix (Z-axis)
    double x_rot = x * std::cos(rotationAngle) - y * std::sin(rotationAngle);
    double y_rot = x * std::sin(rotationAngle) + y * std::cos(rotationAngle);
    
    return {x_rot, y_rot, z};
}

int main() {
    sf::Texture mapTexture;
    if (!mapTexture.loadFromFile("earth.jpg")) {
        std::cerr << "Error: Could not find earth.jpg" << std::endl;
        return -1;
    }
    
    sf::Sprite mapSprite(mapTexture);
    unsigned int width = mapTexture.getSize().x;
    unsigned int height = mapTexture.getSize().y;
    
    sf::RenderWindow window(sf::VideoMode({width, height}), "Satellite Ground Track");
    window.setFramerateLimit(60);

    // --- SETUP SATELLITE (ISS Orbit) ---
    double altitude = 400000.0; 
    double r_init = R_EARTH + altitude;
    double v_orbit = std::sqrt((G * M_EARTH) / r_init);

    double inclination = 51.6 * (M_PI / 180.0);
    double vx = 0;
    double vy = v_orbit * std::cos(inclination);
    double vz = v_orbit * std::sin(inclination);

    Satellite sat({r_init, 0, 0}, {vx, vy, vz});

    // Trail logic
    std::vector<sf::CircleShape> breadcrumbs;
    double totalTime = 0.0;
    double dt = 1.0; 
    int speedMultiplier = 10;

    // --- DEFINE CITY (Agartala) ---
    // Calculate static screen position once
    double myLat = 23.83;
    double myLon = 91.28;
    float cityScreenX = (myLon * (M_PI/180.0) + M_PI) / (2 * M_PI) * width;
    float cityScreenY = (M_PI/2 - myLat * (M_PI/180.0)) / M_PI * height;

    // Setup City Dot
    sf::CircleShape cityDot(4);
    cityDot.setFillColor(sf::Color(255, 165, 0)); // Orange
    cityDot.setPosition({cityScreenX, cityScreenY});
    cityDot.setOrigin({4, 4}); // Center it

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        // --- 1. PHYSICS UPDATE ---
        for(int i=0; i<speedMultiplier; i++) {
            sat.update(dt);
            totalTime += dt;
        }

        // --- 2. MATH: SATELLITE MAPPING ---
        double lon = std::atan2(sat.pos.y, sat.pos.x);
        lon -= (EARTH_ROTATION_SPEED * totalTime); 
        
        while (lon < -M_PI) lon += 2*M_PI;
        while (lon >  M_PI) lon -= 2*M_PI;

        double lat = std::asin(sat.pos.z / sat.pos.magnitude());

        float screenX = (lon + M_PI) / (2 * M_PI) * width;
        float screenY = (M_PI/2 - lat) / M_PI * height;

        // Add trail
        sf::CircleShape dot(2);
        dot.setFillColor(sf::Color::Red);
        dot.setPosition({screenX, screenY});
        breadcrumbs.push_back(dot);
        if (breadcrumbs.size() > 1000) breadcrumbs.erase(breadcrumbs.begin());

        // --- 3. MATH: VISIBILITY CHECK ---
        Vector3 cityPos3D = getStationPos(myLat, myLon, totalTime);
        Vector3 rangeVec = {sat.pos.x - cityPos3D.x, sat.pos.y - cityPos3D.y, sat.pos.z - cityPos3D.z};
        double dist = rangeVec.magnitude();
        double dotProd = cityPos3D.x * rangeVec.x + cityPos3D.y * rangeVec.y + cityPos3D.z * rangeVec.z;


        // --- 4. RENDER SECTION (Order is Critical!) ---
        window.clear(); // 1. Wipe screen
        
        window.draw(mapSprite); // 2. Draw Background

        window.draw(cityDot);   // 3. Draw City (on top of map)

        // 4. Draw Visibility Line (on top of city)
        if (dotProd > 0) {
            sf::VertexArray line(sf::PrimitiveType::Lines, 2);
            line[0] = sf::Vertex{ sf::Vector2f(cityScreenX, cityScreenY), sf::Color::Green };
            line[1] = sf::Vertex{ sf::Vector2f(screenX, screenY), sf::Color::Green };
            window.draw(line);
            
            window.setTitle("Satellite Ground Track | VISIBLE: " + std::to_string((int)(dist/1000)) + " km");
        } else {
            window.setTitle("Satellite Ground Track | NO SIGNAL");
        }

        // 5. Draw Trails & Satellite (on top of everything)
        for (const auto& crumb : breadcrumbs) window.draw(crumb);
        
        sf::CircleShape head(5);
        head.setFillColor(sf::Color::Cyan);
        head.setPosition({screenX - 2.5f, screenY - 2.5f});
        window.draw(head);

        window.display(); // 6. Show frame
    }
    return 0;
}