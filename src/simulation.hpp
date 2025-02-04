#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "fluid.hpp"
#include <random>
#include <raylib.h>
#define Debug false

class Simulation {
public:
    Simulation();
    ~Simulation();

    void run();  // main Simulation loop

private:
    void update();
    void draw();

    float generate_random_num(float, float);
    void handleMouseClick_Left();
    void handleSpewer();
    void addWind();



        int                     screenWidth;
        int                     screenHeight;
        bool                    isSpewerOn;
        bool                    isWindTunnelOn;
        bool                    showFps;
        float                   deltaT;
        const float             spawnInterval { 0.06f };
        float                   timeSinceLastPlace;

        float                   itteratorK;
        Fluid*                  fluid;

        float                   cellSize;

        Texture2D               simTexture;
        unsigned char*          pixelData;
};










#endif
