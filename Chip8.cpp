#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <ctime>
#include <fstream>

#include "chip8.h"

const uint8_t FONTSET_START_ADDRESS = 0;

unsigned char chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

Chip8::Chip8()
{
	pc = 0x200; // Program counter starts at 0x200
	opcode = 0; // Reset current opcode	
	I = 0; // Reset index register
	sp = 0; // Reset stack pointer
	
	memset(gfx, 0, sizeof(gfx));//Reset Display
	memset(stack, 0, sizeof(stack));//Reset Stack
	memset(V, 0, sizeof(V));//Reset Registers
	memset(memory, 0, sizeof(memory));//Reset Memory

	// Load fontset
	for (int i = FONTSET_START_ADDRESS; i < FONTSET_START_ADDRESS + 80; ++i)
		memory[i] = chip8_fontset[i];

	//Reset Keys
	memset(key, 0, sizeof(key));

	// Reset timers
	delay_timer = 0;
	sound_timer = 0;
}

bool Chip8::load(const char* file_path)
{
	std::ifstream rf(file_path, std::ios::binary | std::ios::in);
	if (!rf.is_open())
	{
		return false;
	}

	char byte;
	uint8_t used_mem = 512;
	for (int i = 0x200; rf.get(byte); i++)
	{
		if (used_mem >= 4096)
		{
			return false;
		}
		memory[i] = static_cast<uint8_t>(byte);
		used_mem++;
	}
	return true;
}

void Chip8::emulate_cycle()
{
	// Fetch Opcode

	opcode = memory[pc] << 8 | memory[pc + 1];
	
	if (trace_mode)
	{
		printf("\ncycle\n");
		printf("%.4X %.4X %.2X %.4X \n", pc, opcode, sp,I);
		for (int i = 0; i <= 15; i++)
		{
			printf("V%d:%.2X ",i, V[i]);
		}
		printf("\n");
	}

	// Decode Opcode-

	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x00E0: Clear Display
			memset(gfx, 0, sizeof(gfx));
			pc += 2;
			break;
		case 0x000E: // 0x00EE: Returns from a subroutine.
			pc = stack[sp];
			sp--;
			break;
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;
	case 0x1000: //	0x1000: Jumps to address NNN.
		pc = opcode & 0x0FFF;
		break;
	case 0x2000: //	0x2000: Calls subroutine at NNN
		sp++;
		stack[sp] = pc;
		pc = opcode & 0x0FFF;
		break;
	case 0x3000: //	0x3000: Skips the next instruction if VX equals NN.
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;
	case 0x4000: //	0x4000: Skips the next instruction if VX does not equal NN.
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;
	case 0x5000: //	0x5000: Skips the next instruction if VX equals VY.
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;
	case 0x6000: //	0x6000: Sets VX to NN.
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;
	case 0x7000: //	0x7000: Adds NN to VX.
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;
	case 0x8000: //	0x8000
		switch (opcode & 0x000F)
		{
		case 0x0000: //	0x0000: Sets VX to the value of VY.
			{
				V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
				pc += 2;
			}
			break;
		case 0x0001: //	0x0001: Sets VX to VX or VY.
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0002: //	0x0002: Sets VX to VX and VY. 
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0003: //	0x0003: Sets VX to VX xor VY.
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0004: //	0x0004: Adds VY to VX.
			{
				uint16_t sum = V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4];
				V[(opcode & 0x0F00) >> 8] = sum & 0x00FF;
				V[0xF] = (sum & 0x0F00) >> 8;
				pc += 2;
			}
			break;
		case 0x0005: //	0x0005:VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
			V[0xF] = V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4] ? 1 : 0;
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0006: //	0x0006: Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;
		case 0x0007: //	0x0007: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
			V[0xF] = V[(opcode & 0x0F00) >> 8] < V[(opcode & 0x00F0) >> 4] ? 1 : 0;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x000E: //	0x000E: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
			V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x80) >> 0x07;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;
		default: printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
			pc += 2;
		}
		break;
	case 0x9000: //	0x9000: Skips the next instruction if VX does not equal VY. 
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;
	case 0xA000: // 0xA000: Sets I to the address NNN
		I = opcode & 0x0FFF;
		pc += 2;
		break;
	case 0xB000: //	0xB000: Jumps to the address NNN plus V0.
		pc = (opcode & 0x0FFF) + V[0x0];
		break;
	case 0xC000: //	0xC000: Sets VX to the result of a bitwise and operation on a random number and NN.  
		V[(opcode & 0x0F00) >> 8] = rand() & (opcode & 0x00FF);
		pc += 2;
		break;
	case 0xD000:
		//	0xD000: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
		draw_on_display(V[(opcode & 0x0F00) >> 8], V[(opcode & 0x00F0) >> 4], opcode & 0x000F);
		pc += 2;
		break;
	case 0xE000: //	0xE000
		switch (opcode & 0x00FF)
		{
		case 0x009E: //	0x009E: Skips the next instruction if the key stored in VX is pressed.
			if (key_pressed(V[(opcode & 0x0F00) >> 8]))
				pc += 4;
			else
				pc += 2;
			break;
		case 0x00A1: //	0x00A1: Skips the next instruction if the key stored in VX is not pressed.
			if (!key_pressed(V[(opcode & 0x0F00) >> 8]))
				pc += 4;
			else
				pc += 2;
			break;
		default:
			printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
		}
		break;
	case 0xF000: //	0xF000
		switch (opcode & 0x00FF)
		{
		case 0x0007: //	0x0007: Sets VX to the value of the delay timer.
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;
		case 0x000A: //	0x000A: A key press is awaited, and then stored in VX.
			{
				bool pressed = false;
				for (int i = 0; i < 16; i++)
				{
					if (key_pressed(i))
					{
						pressed = true;
						V[(opcode & 0x0F00) >> 8] = static_cast<uint8_t>(i);
					}
				}
				if (pressed)
				{
					pc += 2;
				}
			}
			break;
		case 0x0015: //	0x0015: Sets the delay timer to VX.
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x0018: //	0x0018: Sets the sound timer to VX.
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x001E: //	0x001E: Adds VX to I. VF is not affected*.
			//* Spacefight 2091, depends on VF being affected.
			//* Animal Race, depends on VF not being affected.
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x0029: //	0x0029: Sets I to the location of the sprite for the character in VX.
			//SPRITE ADDRESS OF Vx IN I
			I = FONTSET_START_ADDRESS + (5 * V[(opcode & 0x0F00) >> 8]);
			pc += 2;
			break;
		case 0x0033: //	0x0033: Stores the binary-coded decimal representation of VX.
			memory[I] = static_cast<uint8_t>(V[(opcode & 0x0F00) >> 8] / 100);
			memory[I + 1] = static_cast<uint8_t>((V[(opcode & 0x0F00) >> 8] / 10) % 10);
			memory[I + 2] = static_cast<uint8_t>((V[(opcode & 0x0F00) >> 8] % 100) % 10);
			pc += 2;
			break;
		case 0x0055: //	0x0055: Stores from V0 to VX (including VX) in memory, starting at address I.

			for (int offset = 0; offset <= ((opcode & 0x0F00) >> 8); offset++)
			{
				memory[I + offset] = V[offset];
			}
			pc += 2;
			break;
		case 0x0065: //	0x0065: Fills from V0 to VX (including VX) with values from memory, starting at address I.
			for (int offset = 0; offset <= ((opcode & 0x0F00) >> 8); offset++)
			{
				V[offset] = memory[I + offset];
			}
			pc += 2;
			break;
		default:
			printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
		}
		break;
	}
	
	if (delay_timer != 0)
	{
		delay_timer -= 1;
	}
	if (sound_timer != 0)
	{
		//BEEP
		sound_timer -= 1;
	}
}

void Chip8::draw_on_display(uint8_t Vx, uint8_t Vy, uint8_t N)
{
	if (trace_mode)
		printf("Draw");

	V[0xF] = 0; //Reset VF
	for (int i = 0; i < N; i++)
	{
		int pixel = memory[I + i];
		for (int j = 0; j < 8; j++)
		{
			if ((pixel & (0x80 >> j)) != 0)
			{
				int index = ((Vx + j) + ((Vy + i) * 64)) % 2048;
				if (gfx[index] == 1)
				{
					V[0x0F] = 1;
				}
				gfx[index] ^= 1;
			}
		}
	}
	drawFlag = true;
}

bool Chip8::key_pressed(uint8_t keyId)
{
	return key[keyId] != 0;
}
Chip8::~Chip8()
{
	
}
