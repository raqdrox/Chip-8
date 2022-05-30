#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "chip8.h"

#include <chrono>
#include <iostream>
#include <string>
#include "Platform.cpp"

const int VIDEO_WIDTH = 64;
const int VIDEO_HEIGHT = 32;


void dump_gfx(uint8_t* gfx);

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);
	}
	
	
	int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFilename =argv[3];
	Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);
	Chip8 chip8;
	chip8.trace_mode = true;
	bool loaded=chip8.load(romFilename);
	int videoPitch = sizeof(chip8.gfx[0]) * VIDEO_WIDTH;

	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit)
	{
		quit = platform.ProcessInput(chip8.key);
		if (chip8.key_pressed(15))
		{
			dump_gfx(chip8.gfx);
		}
			
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (dt > cycleDelay)
		{
			lastCycleTime = currentTime;

			chip8.emulate_cycle();
			if(chip8.drawFlag)
				platform.Update(chip8.gfx, videoPitch);
			chip8.drawFlag = false;
		}
	}
	printf("QUIT");
	return 0;
}
void dump_gfx(uint8_t* gfx)
{
	for (int i = 0; i < VIDEO_HEIGHT; i++)
	{
		for (int j = 0; j < VIDEO_WIDTH; j++)
		{
			printf("%X", gfx[j + (i * VIDEO_WIDTH)]);
		}
		printf("\n");
	}
	printf("");
}