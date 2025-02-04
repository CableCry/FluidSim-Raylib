#include "simulation.hpp"
#include <iostream>
#include <algorithm>
#include "stb_perlin.h"
#include <stdlib.h>


void consoleInteraction()
{
    std::cout << R"(
__/\\\\\\\\\\\\\\\__/\\\\\\________________________________/\\\__        
 _\/\\\///////////__\////\\\_______________________________\/\\\__       
  _\/\\\________________\/\\\___________________/\\\________\/\\\__      
   _\/\\\\\\\\\\\________\/\\\_____/\\\____/\\\_\///_________\/\\\__     
    _\/\\\///////_________\/\\\____\/\\\___\/\\\__/\\\___/\\\\\\\\\__    
     _\/\\\________________\/\\\____\/\\\___\/\\\_\/\\\__/\\\////\\\__   
      _\/\\\________________\/\\\____\/\\\___\/\\\_\/\\\_\/\\\__\/\\\__  
       _\/\\\______________/\\\\\\\\\_\//\\\\\\\\\__\/\\\_\//\\\\\\\/\\_ 
        _\///______________\/////////___\/////////___\///___\///////\//__)" << "\n";
    std::cout << R"(         _________________________________________________________________)" << "\n";
    std::cout << R"(          By CableCry)" << "\n";
    std::cout << "Controls: \n";   
    std::cout << R"(
    R - Resets ink on screen
    S - Start Wind tunnel
    Space Bar - Creates spewer in the middle of screen
    Left Click - Draws on canvas
    Right Click - Draws obstacle
    ` - Show FPS)" << "\n";       


}



// float Simulation::generate_random_num(float lower, float higher)
//     {
//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::uniform_int_distribution<> dist(lower, higher); 
//         return dist(gen);
//     };


float Simulation::generate_random_num(float min, float max)
{
    static unsigned int seed = 42; // Initialize with a fixed seed for testing
    seed = (seed * 12345 + 67890);
    float random = ((float)(seed % 32767) / 32767.0f);
    return min + random * (max - min);
};


Simulation::Simulation()
{
    // Example dimensions
    screenWidth  = 1024;
    screenHeight = 1024;
    isSpewerOn = false;
    isWindTunnelOn = false;
    showFps = false;
    itteratorK = 0;
    timeSinceLastPlace = 0.0f;


    InitWindow(screenWidth, screenHeight, "2D Fluid Example");
    SetTargetFPS(60);

    int N = 512;
    float diff = 0.0f;
    float visc = 0.0f;
    float dt   = 0.1f;

    fluid = new Fluid(N, diff, visc, dt);

    // We'll calculate cellSize based on screenWidth / N
    cellSize = (float)screenWidth / (float)N;

    addWind();

    pixelData = new unsigned char[N*N*4];

    // Create an image to then make a texture
    Image simImage = {
        pixelData,
        N,
        N,
        1,
        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    // Make the 2D Texture 
    simTexture = LoadTextureFromImage(simImage);
    SetTextureFilter(simTexture, TEXTURE_FILTER_POINT);

    // Unload image as its not needed anymore 
    simImage.data = nullptr;
    UnloadImage(simImage);

}

void Simulation::addWind()
{

    int N = fluid->size;
    int center = N/2;           // Y - coord
    int leftAllign = 3;         // X - coord
    int rightAllign = N - 3;    // X - coord
    int constantVal = 30;

    fluid->addInlet(leftAllign, center - constantVal, 255.0f, 0.2f, 0.0f);
    fluid->addOutlet(rightAllign, center - constantVal, 255.0f, 0.0f, 0.0f);

    fluid->addInlet(leftAllign, center, 255.0f, 0.2f, 0.0f);
    fluid->addOutlet(rightAllign, center, 255.0f, 0.0f, 0.0f);

    fluid->addInlet(leftAllign, center + constantVal, 255.0f, 0.2f, 0.0f);
    fluid->addOutlet(rightAllign, center + constantVal, 255.0f, 0.0f, 0.0f);

}

Simulation::~Simulation()
{
    delete fluid;
    UnloadTexture(simTexture);  // Unload the texture
    delete[] pixelData;         // Free the pixel buffer
    CloseWindow();
}

void Simulation::run()
{

    consoleInteraction();

    while (!WindowShouldClose())
    {
        deltaT = GetFrameTime();
        timeSinceLastPlace += deltaT;
        update();
        draw();
    }
}

void Simulation::update()
{

    if (IsKeyPressed(KEY_GRAVE))
    {
        showFps = !showFps;
    }

    if (IsKeyPressed(KEY_S))
    {
        fluid->toggleSource();
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && timeSinceLastPlace > spawnInterval)
    {
        handleMouseClick_Left();
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        isSpewerOn = !isSpewerOn;
    }

    if (isSpewerOn)
    {
        handleSpewer();
    }

    if (IsKeyPressed(KEY_R))    // Get rid of all density to clear screen
    {
        int totalCells = fluid->size * fluid->size;
        std::fill(fluid->density, fluid->density + totalCells, 0.0f);
        std::fill(fluid->s,       fluid->s       + totalCells, 0.0f);
    }

    if (IsKeyPressed(KEY_Q))
    {
        fluid->removeObstacle();
    }
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) 
    {
        int mouseX = GetMouseX();
        int mouseY = GetMouseY();

        // Convert screen coordinates to grid indices.
        int gridX = static_cast<int>(mouseX / cellSize);
        int gridY = static_cast<int>(mouseY / cellSize);

        // Define a brush size (for example, 5x5 cells) for removal.
        const int brushSize = 10;
        int startX = gridX - brushSize / 2;
        int startY = gridY - brushSize / 2;

        // Call removeObstacle to clear the brush area.
        fluid->removeObstacle(startX, startY, brushSize, brushSize);
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) 
    {
        int mouseX = GetMouseX();
        int mouseY = GetMouseY();
        
        // Convert screen coordinates to grid coordinates
        int gridX = static_cast<int>(mouseX / cellSize);
        int gridY = static_cast<int>(mouseY / cellSize);
        
        // Set a brush size, for example, a 5x5 block of cells.
        const int brushSize = 5;
        int startX = gridX - brushSize / 2;
        int startY = gridY - brushSize / 2;
        
        // Add the obstacle in the brush area.
        fluid->addObstacle(startX, startY, brushSize, brushSize);
    }

    fluid->step();
}


void Simulation::draw() 
{
    int N = fluid->size;
    
    // Loop over each cell in the grid.
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            int idx = IX(i, j);           // 1D index for the cell
            int pixelIndex = idx * 4;       // Each pixel has 4 bytes (RGBA)

            if (fluid->obstacle[idx]) {
                pixelData[pixelIndex + 0] = 190;  // R
                pixelData[pixelIndex + 1] = 253;  // G
                pixelData[pixelIndex + 2] = 183;  // B
                pixelData[pixelIndex + 3] = 100;  // A
            } else {
                // Otherwise, use the fluid density to compute a grayscale color.
                float d = fluid->density[idx];
                d = (d < 0) ? 0 : ((d > 255) ? 255 : d);
                unsigned char val = static_cast<unsigned char>(d);
                pixelData[pixelIndex + 0] = val;  // R
                pixelData[pixelIndex + 1] = val;  // G
                pixelData[pixelIndex + 2] = val;  // B
                pixelData[pixelIndex + 3] = 255;  // A
            }
        }
    }

    // Update the texture with the new pixel data.
    UpdateTexture(simTexture, pixelData);

    BeginDrawing();
    ClearBackground(BLACK);

    // Draw the updated texture (scale as needed, here scaled by a factor of 2)
    DrawTextureEx(simTexture, {0.0f, 0.0f}, 0, 2.0f, WHITE);

    if (showFps) {
        DrawFPS(10, 10);
    }
    EndDrawing();
}


void Simulation::handleMouseClick_Left()
{
        int mouseX = GetMouseX();
        int mouseY = GetMouseY();

        // Convert screen coords -> fluid grid coords
        int i = (int)(mouseX / cellSize);
        int j = (int)(mouseY / cellSize);

        // clamp to [1..N-2] so we don't go out of bounds
        if (i < 1) i = 1;
        if (i > fluid->size-2) i = fluid->size-2;
        if (j < 1) j = 1;
        if (j > fluid->size-2) j = fluid->size-2;

        // clamp neighbors aswell
        int ir = (i + 1 >= fluid->size-1) ? fluid->size-2 : (i + 1);
        int il = (i - 1 < 1) ? 1 : (i - 1);
        int ju = (j + 1 >= fluid->size-1) ? fluid->size-2 : (j + 1);
        int jd = (j - 1 < 1) ? 1 : (j - 1);

        fluid->addDensity(i,   j,   255.0f);
        fluid->addDensity(ir,  j,   255.0f);
        fluid->addDensity(il,  j,   255.0f);
        fluid->addDensity(i,   ju,  255.0f);
        fluid->addDensity(i,   jd,  255.0f);
        
        float amtX = GetMouseDelta().x;
        float amtY = GetMouseDelta().y;
        
        fluid->addVelocity(i, j, amtX, amtY);
}


void Simulation::handleSpewer()
{
    int N = fluid->size;
    int center = N / 2;
    int i = center;
    int j = center;
    float t = itteratorK * 0.03f;

    fluid->addDensity(i, j, 255.0f);           // Center
    fluid->addDensity(i+1, j, 127.0f);         // East
    fluid->addDensity(i-1, j, 127.0f);         // West
    fluid->addDensity(i, j+1, 127.0f);         // North
    fluid->addDensity(i, j-1, 127.0f);         // South
    fluid->addDensity(i+1, j+1, 63.0f);       // North West
    fluid->addDensity(i+1, j-1, 63.0f);       // South West
    fluid->addDensity(i-1, j+1, 63.0f);       // North East
    fluid->addDensity(i-1, j-1, 63.0f);       // South East


    float velocityX = stb_perlin_noise3((float)i*0.05f,
                                        (float)j*0.05f, 
                                        t,0,0,0);

    float velocityY = stb_perlin_noise3((float)i*0.05f, 
                                        (float)j*0.05f, 
                                        t + 100.0f,0,0,0);

    // float normalized_VelX = (velocityX * 2.0f) - 1.0f;
    // float normalized_VelY = (velocityY * 2.0f) - 1.0f;

    fluid->addVelocity(i, j, velocityX, velocityY);
    fluid->addVelocity(i+1, j, velocityX, velocityY);         // Center
    fluid->addVelocity(i-1, j, velocityX, velocityY);         // West
    fluid->addVelocity(i, j+1, velocityX, velocityY);         // North
    fluid->addVelocity(i, j-1, velocityX, velocityY);         // South
    fluid->addVelocity(i+1, j+1, velocityX, velocityY);       // North West
    fluid->addVelocity(i+1, j-1, velocityX, velocityY);       // South West
    fluid->addVelocity(i-1, j+1, velocityX, velocityY);       // North East
    fluid->addVelocity(i-1, j-1, velocityX, velocityY); 

    itteratorK++;
};

