#ifndef FLUID_H
#define FLUID_H

#include <cmath>
#include <vector>


// Index macro for 2D
#define IX(x, y) ((x) + (y) * N)

struct FluidSource {
    int x;
    int y;
    float densityChange;
    float vxInjection;
    float vyInjection;
    bool active;
};

class Fluid {
public:
    Fluid(int size, float diffusion, float viscosity, float dt);
    ~Fluid();

    void addDensity(int x, int y, float amount);
    void addVelocity(int x, int y, float amtX, float amtY);

    void addInlet(int x, int y, float densityInjection, float vxInjection, float vyInjection);
    void addOutlet(int x, int y, float densityRemoval, float vxInjection = 0.0f, float vyInjection = 0.0f);
    void toggleSource();

    void addObstacle(int x, int y, int w, int h);
    void removeObstacle(int x, int y, int w, int h);
    void removeObstacle();


    void step();

    int                     size;    // N
    float                   dt;      // timestep
    float                   diff;    // diffusion
    float                   visc;    // viscosity

    float*                  s;       // temp array for density
    float*                  density; // density array

    float*                  Vx;      // velocity x
    float*                  Vy;      // velocity y
    float*                  Vx0;     // temp velocity x
    float*                  Vy0;     // temp velocity y
    bool*                   obstacle;


private:
    std::vector<FluidSource> sources;

};

// Boundary functions and solver helpers
void set_bnd(int b, float* x, int N);
void lin_solve(int b, float* x, float* x0, float a, float c, int iter, int N);
void diffuse(int b, float* x, float* x0, float diff, float dt, int iter, int N);
void advect(int b, float* d, float* d0, float* velx, float* vely, float dt, int N);
void project(float* velx, float* vely, float* p, float* div, int iter, int N);

#endif
