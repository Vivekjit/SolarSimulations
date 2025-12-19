#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream> // For building UI strings
#include <optional> // Required for SFML 3.0

// --- 1. CONSTANTS ---
const double G = 6.67430e-11;
const double M_EARTH = 5.972e24;
const double R_EARTH_REAL = 6371000.0; 
const double R_EARTH_VISUAL = 200.0;   
const double SCALE = R_EARTH_VISUAL / R_EARTH_REAL;
const double EARTH_ROTATION_SPEED = 7.2921159e-5; // rad/s (Real physics)

// --- 2. 3D MATH HELPERS ---
struct Vector3 {
    double x, y, z;
};

// Rotation Functions
Vector3 rotateX(Vector3 p, double angle) {
    return {p.x, p.y * cos(angle) - p.z * sin(angle), p.y * sin(angle) + p.z * cos(angle)};
}
Vector3 rotateY(Vector3 p, double angle) {
    return {p.x * cos(angle) + p.z * sin(angle), p.y, -p.x * sin(angle) + p.z * cos(angle)};
}

// --- 3. ORBITAL STRUCTS ---
struct OrbitalElements {
    std::string name;
    sf::Color color;
    double inclination; double raan; double ecc; double argPerigee; double meanAnomaly; double meanMotion;
    std::vector<Vector3> trail; // Visual Trail
};

Vector3 getSatellitePosition(OrbitalElements oe, double t) {
    double M = fmod(oe.meanAnomaly + oe.meanMotion * t, 2 * M_PI);
    double E = M;
    for (int i = 0; i < 5; i++) E = E - (E - oe.ecc * sin(E) - M) / (1 - oe.ecc * cos(E));

    double a = pow(G * M_EARTH / (oe.meanMotion * oe.meanMotion), 1.0/3.0);
    double P = a * (cos(E) - oe.ecc);
    double Q = a * sqrt(1 - oe.ecc * oe.ecc) * sin(E);

    double cO = cos(oe.raan), sO = sin(oe.raan), ci = cos(oe.inclination), si = sin(oe.inclination), cw = cos(oe.argPerigee), sw = sin(oe.argPerigee);
    double x = P * (cO*cw - sO*si*sw) - Q * (cO*sw + sO*si*cw);
    double y = P * (sO*cw + cO*si*sw) - Q * (sO*sw - cO*si*cw);
    double z = P * (si*sw)           + Q * (si*cw);
    return {x, y, z};
}

// --- 4. GEOGRAPHY HELPER (Agartala) ---
Vector3 getCityPos(double lat, double lon, double time) {
    // 1. Earth Spin (Physics Time)
    double theta = (lon * M_PI / 180.0) + (EARTH_ROTATION_SPEED * time); 
    double phi   = lat * M_PI / 180.0;

    // 2. Spherical -> Cartesian (Y is UP in our visual engine)
    double r = R_EARTH_VISUAL;
    double x = r * cos(phi) * cos(theta);
    double y = r * sin(phi); 
    double z = r * cos(phi) * sin(theta);
    
    return {x, y, z};
}


int main() {
    sf::RenderWindow window(sf::VideoMode({1200, 900}), "OrbitView 3D | Agartala Station");
    window.setFramerateLimit(60);
    
    // --- UI FONT SETUP ---
    sf::Font font;
    // Mac Path
    if (!font.openFromFile("/System/Library/Fonts/Helvetica.ttc")) { 
        // Windows Fallback
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
             // Linux / Manual Fallback
             // Ensure you have a font file if these fail!
             std::cerr << "WARNING: Font not found. UI text may not appear." << std::endl;
        }
    }
    sf::Text hudText(font); 
    hudText.setCharacterSize(14); 
    hudText.setFillColor(sf::Color::White);
    hudText.setPosition({10, 10});
    hudText.setLineSpacing(1.2f);

    // --- SATELLITES ---
    std::vector<OrbitalElements> sats;
    sats.push_back({"ISS", sf::Color::Cyan, 51.64*(M_PI/180), 247.46*(M_PI/180), 0.0006, 1.0, 0.0, 15.49*(2*M_PI/86400)}); 
    sats.push_back({"Hubble", sf::Color::Magenta, 28.47*(M_PI/180), 100.0*(M_PI/180), 0.0003, 0.0, 0.0, 14.8*(2*M_PI/86400)}); 
    sats.push_back({"GPS", sf::Color::Red, 55.0*(M_PI/180), 45.0*(M_PI/180), 0.01, 0.0, 0.0, 2.0*(2*M_PI/86400)}); 

    // --- EARTH MESH ---
    std::vector<Vector3> earthPoints;
    for (int lat = -90; lat <= 90; lat += 5) {
        for (int lon = 0; lon < 360; lon += 5) {
            double r = R_EARTH_VISUAL;
            double latRad = lat * M_PI / 180.0, lonRad = lon * M_PI / 180.0;
            earthPoints.push_back({r * cos(latRad) * cos(lonRad), r * sin(latRad), r * cos(latRad) * sin(lonRad)});
        }
    }

    // --- THE SUN (Fixed Visual Position) ---
    Vector3 sunPos = {-800.0, 0.0, 0.0}; 

    double camAngleX = 0.3, camAngleY = 0.0, zoom = 1.0;
    double time = 0.0, timeSpeed = 100.0;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left))  camAngleY -= 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) camAngleY += 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up))    camAngleX -= 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down))  camAngleX += 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Z)) zoom *= 1.02; 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::X)) zoom *= 0.98;

        time += (1.0 / 60.0) * timeSpeed;
        window.clear(sf::Color::Black);

        // --- 1. RENDER SUN ---
        Vector3 sRot = rotateY(sunPos, camAngleY); sRot = rotateX(sRot, camAngleX);
        double sScale = zoom * 600.0 / (1000.0 - sRot.z);
        sf::CircleShape sunShape(30 * sScale / 1.0); 
        sunShape.setFillColor(sf::Color(255, 179, 26));
        sunShape.setPosition({(float)(sRot.x * sScale + 600), (float)(sRot.y * sScale + 450)});
        sunShape.setOrigin({15, 15});
        window.draw(sunShape);

        // --- 2. RENDER EARTH ---
        for (auto& p : earthPoints) {
            // Earth Spin Logic
            double theta = EARTH_ROTATION_SPEED * time;
            double x_spin = p.x * cos(theta) - p.z * sin(theta);
            double z_spin = p.x * sin(theta) + p.z * cos(theta);
            Vector3 pSpin = {x_spin, p.y, z_spin};

            Vector3 r = rotateY(pSpin, camAngleY); r = rotateX(r, camAngleX);
            
            // Lighting
            double mag = sqrt(pSpin.x*pSpin.x + pSpin.y*pSpin.y + pSpin.z*pSpin.z);
            Vector3 norm = {pSpin.x/mag, pSpin.y/mag, pSpin.z/mag};
            double light = norm.x * -1.0; // Sun is at -X
            
            sf::Color c = (light > 0) ? sf::Color(230, 230, 60) : sf::Color(26, 26, 255);

            double perspective = zoom * 600.0 / (1000.0 - r.z);
            if (r.z < 500) {
                sf::RectangleShape dot({2, 2});
                dot.setPosition({(float)(r.x * perspective + 600), (float)(r.y * perspective + 450)});
                dot.setFillColor(c);
                window.draw(dot);
            }
        }

        // --- 3. AGARTALA ---
        Vector3 city3D = getCityPos(23.83, 91.28, time);
        Vector3 cRot = rotateY(city3D, camAngleY); cRot = rotateX(cRot, camAngleX);
        double cPersp = zoom * 600.0 / (1000.0 - cRot.z);
        float cx = cRot.x * cPersp + 600;
        float cy = cRot.y * cPersp + 450;
        
        if (cRot.z < 500) {
            sf::CircleShape cityDot(5);
            cityDot.setFillColor(sf::Color(255, 165, 0)); // Orange
            cityDot.setPosition({cx, cy});
            cityDot.setOrigin({2.5, 2.5});
            window.draw(cityDot);
        }

        // --- 4. SATELLITES & LINES ---
        for (auto& sat : sats) {
            Vector3 posM = getSatellitePosition(sat, time);
            Vector3 posV = { posM.x * SCALE, posM.z * SCALE, posM.y * SCALE };
            
            // Trails
            static int fc = 0;
            if (fc++ % 5 == 0) {
                sat.trail.push_back(posV);
                if (sat.trail.size() > 150) sat.trail.erase(sat.trail.begin());
            }
            for (auto& tp : sat.trail) {
                 Vector3 tr = rotateY(tp, camAngleY); tr = rotateX(tr, camAngleX);
                 double tpP = zoom * 600.0 / (1000.0 - tr.z);
                 sf::RectangleShape td({1, 1});
                 td.setPosition({(float)(tr.x*tpP + 600), (float)(tr.y*tpP + 450)});
                 td.setFillColor(sat.color);
                 window.draw(td);
            }

            // Draw Sat
            Vector3 r = rotateY(posV, camAngleY); r = rotateX(r, camAngleX);
            double sP = zoom * 600.0 / (1000.0 - r.z);
            float sx = r.x * sP + 600;
            float sy = r.y * sP + 450;
            
            if (r.z < 500) {
                sf::CircleShape sDot(5); sDot.setFillColor(sat.color);
                sDot.setPosition({sx, sy}); sDot.setOrigin({2.5, 2.5});
                window.draw(sDot);

                // --- LASER LINES (Agartala to Sat) ---
                Vector3 vecToSat = {posV.x - city3D.x, posV.y - city3D.y, posV.z - city3D.z};
                double cMag = sqrt(city3D.x*city3D.x + city3D.y*city3D.y + city3D.z*city3D.z);
                double dot = (city3D.x/cMag * vecToSat.x) + (city3D.y/cMag * vecToSat.y) + (city3D.z/cMag * vecToSat.z);

                if (dot > 0 && cRot.z < 500) { 
                    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                    line[0] = sf::Vertex{sf::Vector2f(cx, cy), sf::Color::White};
                    line[1] = sf::Vertex{sf::Vector2f(sx, sy), sf::Color::White};
                    window.draw(line);
                } else if (cRot.z < 500) {
                     sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                     line[0] = sf::Vertex{sf::Vector2f(cx, cy), sf::Color(100, 0, 0, 50)}; 
                     line[1] = sf::Vertex{sf::Vector2f(sx, sy), sf::Color(100, 0, 0, 50)};
                     window.draw(line);
                }
            }
        }

        // --- 5. RENDER UI (HUD) ---
        std::stringstream ss;
        ss << "=== ORBITVIEW 3D SYSTEM ===\n";
        ss << "Location: Agartala (23.83 N, 91.28 E)\n";
        ss << "Simulation Speed: " << (int)timeSpeed << "x\n";
        ss << "Zoom Level: " << std::fixed << std::setprecision(2) << zoom << "x\n\n";
        
        ss << "[ SATELLITE STATUS ]\n";
        for (const auto& sat : sats) {
            Vector3 p = getSatellitePosition(sat, time);
            double dist = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
            double altKm = (dist - R_EARTH_REAL) / 1000.0;
            double v = std::sqrt(G * M_EARTH * (2.0/dist - 1.0/std::pow(G*M_EARTH/(sat.meanMotion*sat.meanMotion), 1.0/3.0)));
            
            ss << "> " << sat.name << "\n";
            ss << "   Alt: " << (int)altKm << " km\n";
            ss << "   Vel: " << (int)(v/1000.0) << " km/s\n\n";
        }
        hudText.setString(ss.str());

        // Background Box for UI
        sf::FloatRect bounds = hudText.getGlobalBounds();
        sf::RectangleShape bg({bounds.size.x + 20, bounds.size.y + 20});
        bg.setFillColor(sf::Color(0, 0, 0, 150));
        bg.setPosition({0, 0});
        
        window.draw(bg);
        window.draw(hudText);

        window.display();
    }
}