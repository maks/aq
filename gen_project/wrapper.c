#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "dsp/dsp.h"

// Forward declaration of the generated function
void setup_graph(void);

// A simple tick function, does nothing in this minimal example
static void tick_callback(void) {
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("Initializing DSP...\n");
    dsp_init(tick_callback);

    printf("Setting up graph from generated C code...\n");
    setup_graph();

    printf("Running audio engine for 5 seconds...\n");
    
    // Play the generated sound for 5 seconds
    SDL_Delay(5000);
    
    printf("Done.\n");
    SDL_Quit();
    return 0;
}
