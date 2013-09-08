#ifndef PPU_PALETTE_H_INCLUDED
#define PPU_PALETTE_H_INCLUDED

#include <lib6502/memory.h>

class PaletteMemory : public lib6502::Memory
{
    public:
	PaletteMemory();
	virtual ~PaletteMemory();

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

	static bool isPaletteMemory(uint16_t address);

    private:
	uint8_t* m_data;
};

#endif
