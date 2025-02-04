#include "fluid.hpp"
#include <new>
#include <bits/stdc++.h>



Fluid::Fluid(int size_, float diffusion, float viscosity, float dt_)
    : size(size_), dt(dt_), diff(diffusion), visc(viscosity)
{
    int N = size;

    s       = new float[N * N]();
    density = new float[N * N]();

    Vx  = new float[N * N]();
    Vy  = new float[N * N]();

    Vx0 = new float[N * N]();
    Vy0 = new float[N * N]();

    obstacle = new bool[N * N];
    memset(obstacle, 0, sizeof(bool) * N * N);
}

Fluid::~Fluid()
{
    delete[] s;
    delete[] density;

    delete[] Vx;
    delete[] Vy;

    delete[] Vx0;
    delete[] Vy0;

    delete[] obstacle;
}



void Fluid::addDensity(int x, int y, float amount)
{
    int N = size;
    int idx = IX(x, y);
    density[idx] += amount;
}

void Fluid::addVelocity(int x, int y, float amtX, float amtY)
{
    int N = size;
    int idx = IX(x, y);
    Vx[idx] += amtX;
    Vy[idx] += amtY;
}

void Fluid::addInlet(int x, int y, float densityInjection, float vxInjection, float vyInjection) 
{
    if(x < 1 || x > size - 2 || y < 1 || y > size - 2) return;
    FluidSource inlet;
    inlet.x = x;
    inlet.y = y;
    inlet.densityChange = densityInjection;
    inlet.vxInjection = vxInjection;
    inlet.vyInjection = vyInjection;
    inlet.active = false;
    sources.push_back(inlet);
}

void Fluid::addOutlet(int x, int y, float densityRemoval, float vxInjection, float vyInjection) 
{
    if(x < 1 || x > size - 2 || y < 1 || y > size - 2) return;
    FluidSource outlet;
    outlet.x = x;
    outlet.y = y;
    outlet.densityChange = -densityRemoval; // negative to remove fluid
    outlet.vxInjection = vxInjection;
    outlet.vyInjection = vyInjection;
    outlet.active = false;
    sources.push_back(outlet);
}

void Fluid::toggleSource() 
{
    for (FluidSource& src : sources) {
        src.active = !src.active;      
    }
}

// Add a rectangular obstacle: mark cells in the range [x, x+w) and [y, y+h) as obstacles.
void Fluid::addObstacle(int x, int y, int w, int h) 
{
    int N = size;

    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + w > size)  w = size - x;
    if(y + h > size)  h = size - y;

    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            obstacle[IX(i, j)] = true;
        }
    }
}

// Remove a rectangular obstacle: mark cells in the range as free.
void Fluid::removeObstacle(int x, int y, int w, int h) 
{
    int N = size;

    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + w > size)  w = size - x;
    if(y + h > size)  h = size - y;

    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            obstacle[IX(i, j)] = false;
        }
    }
}

void Fluid::removeObstacle() 
{
    std::fill(obstacle, obstacle + size * size, false);
}


void Fluid::step()
{
    int N = size;

    for (const FluidSource& src : sources) {
        if (src.active) {
            int idx = IX(src.x, src.y);
            density[idx] += src.densityChange;
            Vx[idx] += src.vxInjection;
            Vy[idx] += src.vyInjection;
        }
    }
    // Diffuse velocity
    diffuse(1, Vx0, Vx,  visc, dt, 4, N);
    diffuse(2, Vy0, Vy,  visc, dt, 4, N);

    // Project intermediate velocity
    project(Vx0, Vy0, Vx, Vy, 4, N);

    // Advect velocity
    advect(1, Vx, Vx0, Vx0, Vy0, dt, N);
    advect(2, Vy, Vy0, Vx0, Vy0, dt, N);

    // Project final velocity
    project(Vx, Vy, Vx0, Vy0, 4, N);

    // Diffuse density
    diffuse(0, s, density, diff, dt, 4, N);
    // Advect density
    advect(0, density, s, Vx, Vy, dt, N);

    for (int j = 0; j < size; j++) {
        for (int i = 0; i < size; i++) {
            if (obstacle[IX(i,j)]) {
                density[IX(i,j)] = density[IX(i,j)];
                Vx[IX(i,j)] = (Vx[IX(i,j)] * -0.76);
                Vy[IX(i,j)] = (Vy[IX(i,j)] * -0.76);
            }
        }
    }
}



void set_bnd(int b, float* x, int N)
{
    // Handle edges for 2D
    for (int i = 1; i < N - 1; i++) {
        // top row
        x[IX(i, 0)]     = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        // bottom row
        x[IX(i, N-1)]   = b == 2 ? -x[IX(i, N-2)] : x[IX(i, N-2)];

        // left column
        x[IX(0, i)]     = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
        // right column
        x[IX(N-1, i)]   = b == 1 ? -x[IX(N-2, i)] : x[IX(N-2, i)];
    }

    // corners
    x[IX(0,     0    )] = 0.5f*( x[IX(1,   0   )] + x[IX(0,   1   )] );
    x[IX(0,     N-1  )] = 0.5f*( x[IX(1,   N-1 )] + x[IX(0,   N-2 )] );
    x[IX(N-1,   0    )] = 0.5f*( x[IX(N-2, 0   )] + x[IX(N-1, 1   )] );
    x[IX(N-1,   N-1  )] = 0.5f*( x[IX(N-2, N-1 )] + x[IX(N-1, N-2 )] );
}

void lin_solve(int b, float* x, float* x0, float a, float c, int iter, int N)
{
    float cRecip = 1.0f / c;
    for (int k = 0; k < iter; k++) {
        for (int j = 1; j < N - 1; j++) {
            for (int i = 1; i < N - 1; i++) {
                x[IX(i, j)] =
                    ( x0[IX(i, j)]
                      + a*( x[IX(i+1, j)]
                           +x[IX(i-1, j)]
                           +x[IX(i, j+1)]
                           +x[IX(i, j-1)]
                         )
                    ) * cRecip;
            }
        }
        set_bnd(b, x, N);
    }
}

void diffuse(int b, float* x, float* x0, float diff, float dt, int iter, int N)
{
    float a = dt * diff * (N - 2) * (N - 2);
    lin_solve(b, x, x0, a, 1 + 4*a, iter, N);
}

void advect(int b, float* d, float* d0,
            float* velx, float* vely, float dt, int N)
{
    float dtx = dt*(N - 2);
    float dty = dt*(N - 2);

    for (int j = 1; j < N - 1; j++) {
        for (int i = 1; i < N - 1; i++) {
            float tmpx = i - dtx*velx[IX(i,j)];
            float tmpy = j - dty*vely[IX(i,j)];

            if (tmpx < 0.5f)           tmpx = 0.5f;
            if (tmpx > (N - 1 + 0.5f)) tmpx = (N - 1 + 0.5f);
            if (tmpy < 0.5f)           tmpy = 0.5f;
            if (tmpy > (N - 1 + 0.5f)) tmpy = (N - 1 + 0.5f);

            int i0 = (int) floor(tmpx);
            int i1 = i0 + 1;
            int j0 = (int) floor(tmpy);
            int j1 = j0 + 1;

            float s1 = tmpx - i0;
            float s0 = 1.0f - s1;
            float t1 = tmpy - j0;
            float t0 = 1.0f - t1;

            // clamp indices
            if (i0 < 0) i0 = 0;
            if (i0 > N-1) i0 = N-1;
            if (i1 < 0) i1 = 0;
            if (i1 > N-1) i1 = N-1;

            if (j0 < 0) j0 = 0;
            if (j0 > N-1) j0 = N-1;
            if (j1 < 0) j1 = 0;
            if (j1 > N-1) j1 = N-1;


            d[IX(i,j)] =
                s0*( t0*d0[IX(i0, j0)] + t1*d0[IX(i0, j1)] )
              + s1*( t0*d0[IX(i1, j0)] + t1*d0[IX(i1, j1)] );
        }
    }
    set_bnd(b, d, N);
}

void project(float* velx, float* vely, float* p, float* div, int iter, int N)
{
    // compute divergence
    for (int j = 1; j < N - 1; j++) {
        for (int i = 1; i < N - 1; i++) {
            div[IX(i,j)] = -0.5f * (
                 velx[IX(i+1, j)] - velx[IX(i-1, j)]
               + vely[IX(i, j+1)] - vely[IX(i, j-1)]
            ) / N;
            p[IX(i,j)] = 0.0f;
        }
    }
    set_bnd(0, div, N);
    set_bnd(0, p, N);

    lin_solve(0, p, div, 1, 4, iter, N);
 
    // subtract gradient
    for (int j = 1; j < N-1; j++) {
        for (int i = 1; i < N-1; i++) {
            velx[IX(i,j)] -= 0.5f * ( p[IX(i+1,j)] - p[IX(i-1,j)] ) * N;
            vely[IX(i,j)] -= 0.5f * ( p[IX(i,j+1)] - p[IX(i,j-1)] ) * N;
        }
    }
    set_bnd(1, velx, N);
    set_bnd(2, vely, N);
}
