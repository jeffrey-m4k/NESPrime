#include "NES.h"

#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <nfd.h>

int main(int argc, char *argv[]) {
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) != 0 || TTF_Init() != 0 )
	{
		SDL_Log( "Unable to initialize SDL: %s", SDL_GetError() );
		exit( EXIT_FAILURE );
	}
	SDL_Log( "SDL initialized " );

    NFD_Init();

    NES* nes = new NES();
    nes->run();

    NFD_Quit();
	SDL_Quit();
    return EXIT_SUCCESS;
}