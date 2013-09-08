#include <nemu/ppu/palette.h>

#include <cassert>

// =====================================================================================================================
PaletteMemory::PaletteMemory()
{
    m_data = new uint8_t[32];
}

// =====================================================================================================================
PaletteMemory::~PaletteMemory()
{
    delete[] m_data;
}

// =====================================================================================================================
static inline uint16_t translateAddress(uint16_t address)
{
    switch (address)
    {
	case 0x3f10 : return 0x3f00;
	case 0x3f14 : return 0x3f04;
	case 0x3f18 : return 0x3f08;
	case 0x3f1c : return 0x3f0c;
    }

    return address;
}

// =====================================================================================================================
uint8_t PaletteMemory::read(uint16_t address)
{
    assert(isPaletteMemory(address));
    address = translateAddress(address);
    return m_data[address - 0x3f00];
}

// =====================================================================================================================
void PaletteMemory::write(uint16_t address, uint8_t data)
{
    assert(isPaletteMemory(address));
    address = translateAddress(address);
    m_data[address - 0x3f00] = data;
}

// =====================================================================================================================
bool PaletteMemory::isPaletteMemory(uint16_t address)
{
    return (address >= 0x3f00) && (address <= 0x3f1f);
}
