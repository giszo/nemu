#include <nemu/ppu/palette.h>

#include <cassert>

// =====================================================================================================================
PaletteMemory::PaletteMemory()
    : RAM(0x20)
{
}

// =====================================================================================================================
static inline uint16_t translateAddress(uint16_t address)
{
    switch (address)
    {
	case 0x10 : return 0x00;
	case 0x14 : return 0x04;
	case 0x18 : return 0x08;
	case 0x1c : return 0x0c;
    }

    return address;
}

// =====================================================================================================================
uint8_t PaletteMemory::read(uint16_t address)
{
    assert(address < size());
    address = translateAddress(address);
    return RAM::read(address);
}

// =====================================================================================================================
void PaletteMemory::write(uint16_t address, uint8_t data)
{
    assert(address < size());
    address = translateAddress(address);
    RAM::write(address, data);
}
