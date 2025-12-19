#include <bits/stdc++.h>

//--physics constants---

using namespace std;

const double G=6.67430e-11;
const double M_EARTH = 5.972e24;
const double R_EARTH= 6371000.0;//in meters 

//Vector Helper

struct Vector3{
    double x,y,z;

    //Overload for cleaner math
    Vector3 operator+(const Vector3 & other ){return {x+other.x,y+other.y,z+other.z};} 
    Vector3 operator* (double scalar)const{return {x*scalar ,y*scalar,z*scalar};}

    double magnitude() const {
        return sqrt(x*x + y*y + z*z);
    }
};


//--The Sattelite Class--

class Sattelite {
    public :
        Vector3 pos;//position 
        Vector3 vel;//velocity 

        Sattelite(Vector3 startPos, Vector3 startVel):pos (startPos),vel(startVel){}//constructor 


        //---Physics engine step 
        void update (double dt ){
            double r=pos.magnitude();//Distance from the centre of the earth

        // Newton's Law of Gravitation: F = G * M * m / r^2
        // Acceleration a = F / m  =>  a = G * M / r^2
        // Vector form: a = -(G * M / r^3) * pos

        double accelMagnitude =(G* M_EARTH)/(r*r*r);
        Vector3 accel= pos * (-accelMagnitude);

        //Semi-Implicit Euler Integration (stable for the orbits )

        vel = vel + (accel * dt);
        pos = pos + (vel * dt);

        }
};

int main (){
    //altitude is 400km 
    double altitude =400000.0;
    double r_initial =R_EARTH +altitude ;

    double v_orbit = std::sqrt((G * M_EARTH) / r_initial);//GM/r2*r

    cout <<"---Simualtion Config ---"<<endl;
    cout <<"Target Altitude :"<<altitude /1000.0<<"km"<<endl;
    cout <<"required Velocity "<<v_orbit<<"m/s "<<endl;

    //Spaawn Sattelite ,Pos =Right side of earth(x-axis ),Moving up (Y-axis ) ot the orbit 

    //---RUN SIMULAITON IN VIRTUAL TIME---

    Sattelite sat({r_initial,0,0},{0,v_orbit,0});

    double dt =1.0;//Time step(10 sec per loop)
    double totalTime =0.0;

    // Run for one full orbit (approx 90 minutes = 5400 seconds)
    std::cout << "\n--- Starting Orbit ---" << std::endl;
    for (int i = 0; i <= 600; i++) {
        sat.update(dt);
        totalTime += dt;

        // Print stats every 10 minutes (600 seconds)
        if (i % 60 == 0) {
            double currentAltitude = sat.pos.magnitude() - R_EARTH;
            std::cout << "Time: " << totalTime / 60.0 << " min | "
                      << "Alt: " << currentAltitude / 1000.0 << " km | "
                      << "Pos: (" << (int)sat.pos.x << ", " << (int)sat.pos.y << ")" 
                      << std::endl;
        }
    }

    return 0;



}   