#ifndef PPU_PALETTE_H_INCLUDED
#define PPU_PALETTE_H_INCLUDED

#include <nemu/memory/ram.h>

class PaletteMemory : public memory::RAM
{
    public:
	PaletteMemory();

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;
};

#endif
