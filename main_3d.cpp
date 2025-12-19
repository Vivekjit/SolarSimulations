#include <SFML/Graphics.hpp>
#include <bits/stdc++.h>
using namespace std;

//--CONSTANTS---

const int WIDTH =1200;
const int HEIGHT =900;
const double R_EARTH =200.0;//Scaled down rad for drawingn 

struct Point3D{
    double x,y,z;
};

//---MATH HELPER : 3D ROTATION ---
Point3D rotateX(Point3D p ,double angle ){//about X
    return {p.x, p.y * cos(angle) - p.z * sin(angle), p.y * sin(angle) + p.z * cos(angle)};
}
Point3D rotateY(Point3D p, double angle) {
    return {p.x * cos(angle) + p.z * sin(angle), p.y, -p.x * sin(angle) + p.z * cos(angle)};
}
Point3D rotateZ(Point3D p, double angle) {
    return {p.x * cos(angle) - p.y * sin(angle), p.x * sin(angle) + p.y * cos(angle), p.z};
}


//--MAIN--

int main(){

    sf::RenderWindow window(sf::VideoMode({(unsigned int)WIDTH, (unsigned int)HEIGHT}), "OrbitView 3D Engine");
    window.setFramerateLimit(60);

    //---Generate the earth mesh---
    vector<Point3D> earthPoints;

    //Lat Long Loops 
    for (int lat = -90; lat <= 90; lat += 5) {//we can i guess make it increase by 5 or 1
        double r_lat = cos(lat * M_PI / 180.0) * R_EARTH;
        double y = sin(lat * M_PI / 180.0) * R_EARTH;

        for (int lon = 0; lon < 360; lon += 5) {
            double x = r_lat * cos(lon * M_PI / 180.0);
            double z = r_lat * sin(lon * M_PI / 180.0);
            earthPoints.push_back({x, y, z});
        }
    }

    //Camera Rotation angles

    double angleX=0.0;;
    double angleY =0.0;


    while (window.isOpen()){
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        //---INPUT TO ROTATE THE WORLD 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left))  angleY -= 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) angleY += 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up))    angleX -= 0.03;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down))  angleX += 0.03;

        window.clear(sf::Color::Black);

        //--RENDER THE EARTH WOHHOOO!!---

        //For every point : Rotate ->Project ->Draw //might be CPU intensive 

        for (auto &p :earthPoints){
            //Apply the Rotation (Camera View)

            Point3D r = rotateY(p, angleY);
            r = rotateX(r, angleX);

            //Perspective Projetction 

            double dist = 400.0;//Distance from camera to object
            double scale =500.0/(dist - r.z);//The "Z- divide "(things get smaller )
            
            double screenX = r.x * scale + WIDTH / 2.0;
            double screenY = r.y * scale + HEIGHT / 2.0;

            //3.Draw Point 
            //Make points darker if they are "behind " the sphere (FALSE LIGHTINIG HEHE)

            sf::Color color = sf::Color::Cyan;
            if (r.z > 0) color = sf::Color(0, 100, 100); // Back side dimmer
            sf::RectangleShape dot({2, 2});
            dot.setPosition({(float)screenX, (float)screenY});
            dot.setFillColor(color);
            window.draw(dot);

        }


        window.display();

    }

}

