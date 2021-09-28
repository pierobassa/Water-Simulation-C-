/*
Created by Piero Bassa thanks to Professor William Spataro @ University Of Calabria
Sustained for Parallel Algorithms and Distributed Systems. A/A 2020/2021
*/

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const float MinMass = 0.0001;  //Ignore cells that are almost dry
const float MaxMass = 0.5; //The normal, un-pressurized mass of a full water cell
const float MaxCompress = 2; //How much excess water a cell can store, compared to the cell above it
const float MaxSpeed = 30; //max units of water moved out of one block to another, per timestep
const float MinFlow = 0.01; //  Mininum flow between cells per generation


const float MinDraw = 0.01; //For water colors based on mass
const float MaxDraw = 1.1; 

//Map dimensions
const int map_width = 40; 
const int map_height = 40; 

//Block types
const int AIR = 0;
const int GROUND = 1;
const int WATER = 2;

//Data structures
int blocks[map_width + 2][map_height + 2];

float mass[map_width + 2][map_height + 2];
float new_mass[map_width + 2][map_height + 2];


float map(float inValue, float minInRange, float maxInRange, float minOutRange, float maxOutRange) {
    float x = (inValue - minInRange) / (maxInRange - minInRange);
    float result = minOutRange + (maxOutRange - minOutRange) * x;
    return result;
}

void waterColor(float m, int &r, int &g, int &b) {

    if (m < MinDraw)
        m = MinDraw;
    else if (m > MaxDraw)
        m = MaxDraw;

    r = 50;
    g = 50;

    if (m < 1) {
        b = int(map(m, 0.01, 1, 255, 200));
        r = int(map(m, 0.01, 1, 240, 50));

        if (r < 50)
            r = 50;
        else if (r > 240)
            r = 240;
        g = r;
    }
    else {
        b = int(map(m, 1, 1.1, 190, 140));
    }


    if (b < 140)
        b = 140;
    else if (b > 255)
        b = 255;
}

void initContainerMap() {
    for (int x = 0; x < map_height + 2; x++) {
        for (int y = 0; y < map_height + 2; y++) {
            blocks[x][y] = AIR;

        }
    }

    for (int i = 0; i < map_height; i++) {
        blocks[i][0] = GROUND;
        blocks[i][map_height-1] = GROUND;
    }
    for (int j = 0; j < map_height; j++) {
        blocks[0][j] = GROUND;
        blocks[map_width-1][j] = GROUND;
    }

    //INIT CENTRAL CONTAINER
    for (int i = map_width / 2 - 3; i <= map_width / 2 + 3; ++i) {
        blocks[i][map_width / 2 - 2] = GROUND;
    }
    for (int j = map_width / 2 - 2; j >= map_width / 2 - 5; j--) {
        blocks[map_width / 2 - 3][j] = GROUND;
    }
    for (int j = map_width / 2 - 2; j >= map_width / 2 - 5; j--) {
        blocks[map_width / 2 + 3][j] = GROUND;
    }

    //More ground for water flow demonstration
    blocks[map_width / 2 - 1][3] = GROUND;
    blocks[map_width / 2 - 2][3] = GROUND;
    blocks[map_width / 2 + 1][3] = GROUND;
    blocks[map_width / 2 + 2][3] = GROUND;

    blocks[16][map_width / 2 + 2] = GROUND;
    blocks[15][map_width / 2 + 3] = GROUND;
    blocks[17][map_width / 2 + 3] = GROUND;
    blocks[16][map_width / 2 + 3] = GROUND;
    blocks[16][map_width / 2 + 4] = GROUND;
    blocks[15][map_width / 2 + 4] = GROUND;
    blocks[17][map_width / 2 + 4] = GROUND;
    blocks[18][map_width / 2 + 4] = GROUND;
    blocks[14][map_width / 2 + 4] = GROUND;

    blocks[24][map_width / 2 + 2] = GROUND;
    blocks[23][map_width / 2 + 3] = GROUND;
    blocks[25][map_width / 2 + 3] = GROUND;
    blocks[24][map_width / 2 + 3] = GROUND;
    blocks[24][map_width / 2 + 4] = GROUND;
    blocks[23][map_width / 2 + 4] = GROUND;
    blocks[25][map_width / 2 + 4] = GROUND;
    blocks[26][map_width / 2 + 4] = GROUND;
    blocks[22][map_width / 2 + 4] = GROUND;


    //GROUND BLOCKS FOR WATER FLOW
    for (int j = 15; j < 26; j++) {
        blocks[j][32] = GROUND;
    }
}

float get_stable_state_b(float total_mass) {
    if (total_mass <= 1) {
        return 1;
    }
    else if (total_mass < 2 * MaxMass + MaxCompress) {
        return (MaxMass * MaxMass + total_mass * MaxCompress) / (MaxMass + MaxCompress);
    }
    else {
        return (total_mass + MaxCompress) / 2;
    }
}

void simulate_compression() {
    float Flow = 0;
    float remaining_mass;

    //Calculate and apply flow for each block
    for (int x = 1; x <= map_width; x++) {
        for (int y = 1; y <= map_height; y++) {
            //Skip inert ground blocks
            if (blocks[x][y] == GROUND)
                continue;

            //Custom push-only flow
            Flow = 0;
            remaining_mass = mass[x][y];
            if (remaining_mass <= 0)
                continue;

            //The block below this one
            if ((blocks[x][y+1] != GROUND)) {
                Flow = get_stable_state_b(remaining_mass + mass[x][y + 1]) - mass[x][y + 1];
                if (Flow > MinFlow) {
                    Flow *= 0.5; //leads to smoother flow
                }
                
                if (Flow > min(MaxSpeed, remaining_mass))
                    Flow = min(MaxSpeed, remaining_mass);
                else if (Flow < 0)
                    Flow = 0;

                new_mass[x][y] -= Flow;
                new_mass[x][y + 1] += Flow;
                remaining_mass -= Flow;
            }

            if (remaining_mass <= 0) continue;

            //Left
            if (blocks[x-1][y] != GROUND) { 
                //Equalize the amount of water in this block and it's neighbour
                Flow = (mass[x][y] - mass[x - 1][y]) / 4;
                if (Flow > MinFlow) {
                    Flow *= 0.5;
                }
               
                if (Flow > remaining_mass)
                    Flow = remaining_mass;
                else if (Flow < 0)
                    Flow = 0;


                new_mass[x][y] -= Flow;
                new_mass[x - 1][y] += Flow;
                remaining_mass -= Flow;
            }

            if (remaining_mass <= 0) continue;

            //Right
            if (blocks[x+1][y] != GROUND) { 
                //Equalize the amount of water in this block and it's neighbour
                Flow = (mass[x][y] - mass[x + 1][y]) / 4;
                if (Flow > MinFlow) { 
                    Flow *= 0.5; 
                }
                
                if (Flow > remaining_mass)
                    Flow = remaining_mass;
                else if (Flow < 0)
                    Flow = 0;

                new_mass[x][y] -= Flow;
                new_mass[x + 1][y] += Flow;
                remaining_mass -= Flow;
            }

            if (remaining_mass <= 0) continue;

            //Up. Only compressed water flows upwards.
            if (blocks[x][y-1] != GROUND) { 
                Flow = remaining_mass - get_stable_state_b(remaining_mass + mass[x][y - 1]);
                if (Flow > MinFlow) { 
                    //Flow *= 0.5; 
                }
                
                if (Flow > min(MaxSpeed, remaining_mass))
                    Flow = min(MaxSpeed, remaining_mass);
                else if (Flow < 0)
                    Flow = 0;

                new_mass[x][y] -= Flow;
                new_mass[x][y - 1] += Flow;
                remaining_mass -= Flow;
            }


        }
    }

    //Copy the new mass values to the mass array
    for (int x = 0; x < map_width + 2; x++) {
        for (int y = 0; y < map_height + 2; y++) {
            mass[x][y] = new_mass[x][y];
        }
    }

    for (int x = 1; x <= map_width; x++) {
        for (int y = 1; y <= map_height; y++) {
            //Skip ground blocks
            if (blocks[x][y] == GROUND) continue;
            //Flag/unflag water blocks
            if (mass[x][y] > MinMass) {
                blocks[x][y] = WATER;
            }
            else {
                blocks[x][y] = AIR;
            }
        }
    }

    //Remove any water that has left the map
    for (int x = 0; x < map_width + 2; x++) {
        mass[x][0] = 0;
        mass[x][map_height + 1] = 0;
    }
    for (int y = 1; y < map_height + 1; y++) {
        mass[0][y] = 0;
        mass[map_width + 1][y] = 0;
    }

}



void print(int spacing) {
    int r, g, b;

    for (int i = 0; i < map_height; i++) {
        for (int j = 0; j < map_width; j++) {
            if (blocks[i][j] == WATER) {
                waterColor(mass[i][j], r, g, b);
                al_draw_filled_rectangle(i * spacing, j * spacing, i * spacing + spacing, j * spacing + spacing, al_map_rgb(r, g, b));
            }
            else if (blocks[i][j] == AIR)
                al_draw_filled_rectangle(i * spacing, j * spacing, i * spacing + spacing, j * spacing + spacing, al_map_rgb(0, 0, 0));
            else if (blocks[i][j] == GROUND)
                al_draw_filled_rectangle(i * spacing, j * spacing, i * spacing + spacing, j * spacing + spacing, al_map_rgb(245, 245, 60));
        }
    }

    al_flip_display();
}

bool withinGrid(int col, int row) {
    if ((col < 0) || (row < 0))
        return false;
    if ((col >= 30) || (row >= 30))
        return false;

    return true;
}

void addWater() {
    blocks[map_height/2][1] = WATER;
    mass[map_height / 2][1] = blocks[map_height / 2][1] == WATER ? MaxMass : 0.0;
    new_mass[map_height / 2][1] = blocks[map_height / 2][1] == WATER ? MaxMass : 0.0;

    blocks[map_height / 2 - 1][1] = WATER;
    mass[map_height / 2 - 1][1] = blocks[map_height / 2 - 1][1] == WATER ? MaxMass : 0.0;
    new_mass[map_height / 2][1] = blocks[map_height / 2 - 1][1] == WATER ? MaxMass : 0.0;

    blocks[map_height / 2 + 1][1] = WATER;
    mass[map_height / 2 + 1][1] = blocks[map_height / 2 + 1][1] == WATER ? MaxMass : 0.0;
    new_mass[map_height / 2 + 1][1] = blocks[map_height / 2 + 1][1] == WATER ? MaxMass : 0.0;
}

void addWater(int x, int y) {
    blocks[x][y] = WATER;
    mass[x][y] = blocks[x][y] == WATER ? MaxMass : 0.0;
    new_mass[x][y] = blocks[x][y] == WATER ? MaxMass : 0.0;
}

void addGround(int x, int y) {
    blocks[x][y] = GROUND;
}



int main(int argc, char* argv[]) {

    int display_height = 880; 
    int display_width = 880; 
    int blockNumberPerSide = map_height;

    int spacing = display_height / blockNumberPerSide;

    // Allegro Setup
    ALLEGRO_DISPLAY* display;
    al_init();
    display = al_create_display(display_height, display_width);
    al_init_primitives_addon();

    initContainerMap();

    int cont = 0;
    
    al_install_mouse();
 
    while(true){
        ALLEGRO_MOUSE_STATE state;
        
        auto start = high_resolution_clock::now();

        al_get_mouse_state(&state);
        if (state.buttons & 1) {
            /* Primary (e.g. left) mouse button is held. */
            int pos_x = (int)map(state.x, 0, 900, 0, map_height - 1);
            int pos_y = (int)map(state.y, 0, 900, 0, map_height - 1);
            addWater(pos_x, pos_y);
        }

        if (state.buttons & 2) {
            /* Secondary (e.g. right) mouse button is held. */
            int pos_x = (int)map(state.x, 0, 900, 0, map_height - 1);
            int pos_y = (int)map(state.y, 0, 900, 0, map_height - 1);
            addGround(pos_x, pos_y);
        }

        print(spacing);
        simulate_compression();

        //Timings per state
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << duration.count() << endl;
        
    }

    return 0;
}