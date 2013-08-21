#include <nemu/ppu.h>

#include <lib6502/makestring.h>

#include <iostream>
#include <string.h>

using lib6502::MakeString;

// =====================================================================================================================
PPU::PPU(uint8_t* vrom)
    : m_ctrl(0),
      m_mask(0),
      m_status(0x80),
      m_address(0),
      m_firstAddrWrite(true)
{
    m_ram = new uint8_t[0x4000];
    memcpy(m_ram, vrom, 8 * 1024);

    m_screen = SDL_SetVideoMode(256, 240, 32, SDL_SWSURFACE);
}

// =====================================================================================================================
PPU::~PPU()
{
    delete[] m_ram;
}

// =====================================================================================================================
uint8_t PPU::read(uint16_t address)
{
    switch (address)
    {
	case 0x2002 :
	    // status
	    return readStatusRegister();

	case 0x2007 :
	    // data register
	    return readDataRegister();

	default :
	    throw PPUException(MakeString() << "invalid register read: " << std::hex << address);
    }
}

// =====================================================================================================================
void PPU::write(uint16_t address, uint8_t data)
{
    switch (address)
    {
	case 0x2000 :
	    m_ctrl = data;
	    std::cout << "PPU ctrl: " << std::hex << (int)m_ctrl << std::endl;
	    break;

	case 0x2001 :
	    m_mask = data;
	    std::cout << "PPU mask: " << std::hex << (int)m_mask << std::endl;
	    break;

	case 0x2003 :
	    // OAM address
	    break;

	case 0x2005 :
	    // scroll register
	    std::cout << "PPU scroll: " << (int)data << std::endl;
	    break;

	case 0x2006 :
	    // address register
	    writeAddressRegister(data);
	    break;

	case 0x2007 :
	    // data register
	    writeDataRegister(data);
	    break;

	default :
	    throw PPUException(MakeString() << "invalid register write: " << std::hex << address);
    }
}

// =====================================================================================================================
void PPU::dumpNameTables()
{
    std::cout << "Name table #0: ";
    for (unsigned i = 0x2000; i < 0x2400; ++i)
	std::cout << (int)m_ram[i] << " ";
    std::cout << std::endl;

    std::cout << "Name table #1: ";
    for (unsigned i = 0x2400; i < 0x2800; ++i)
	std::cout << (int)m_ram[i] << " ";
    std::cout << std::endl;

    std::cout << "Background palette: ";
    for (unsigned i = 0; i < 0x10; ++i)
	std::cout << (int)m_ram[0x3f00 + i] << " ";
    std::cout << std::endl;
}

// =====================================================================================================================
void PPU::renderVideo()
{
    static uint32_t palette[16] = {0x5f97ff, 0x83d313, 0x00ab00, 0x000000, 0x000000, 0xffbfb3, 0xcb4f0f, 0x000000, 0x000000, 0xffffff, 0x3fbfff, 0x000000, 0x000000, 0xff9b3b, 0xcb4f0f, 0x000000};

    for (int row = 0; row < 30; ++row)
    {
	for (int col = 0; col < 32; ++col)
	{
	    uint32_t tileIndex = row * 32 + col;
	    uint16_t attrByteIndex = (row / 4) * 8 + (col / 4);

	    //std::cout << "row=" << row << ", col=" << col << std::endl;
	    //std::cout << "tile=" << tileIndex << ", attrByte=" << attrByteIndex << std::endl;

	    unsigned rowMod = row % 2;
	    unsigned colMod = col % 2;
	    unsigned attrShift = (rowMod * 2 + colMod) * 2 /* 2 bit per block */;

	    uint8_t nameEntry = m_ram[0x2000 /* name table #0 */ + row * 32 + col];
	    uint8_t attrData = (m_ram[0x2000 + 30 * 32 /* tiles */ + attrByteIndex] >> (6 - attrShift)) & 0x3;

	    //std::cout << "name=" << (int)nameEntry << ", attr=" << (int)attrData << std::endl;

	    for (int tileRow = 0; tileRow < 8; ++tileRow)
	    {
		uint8_t layer1 = m_ram[(nameEntry << 4) + tileRow];
		uint8_t layer2 = m_ram[(nameEntry << 4) + 8 + tileRow];

		for (int tileCol = 0; tileCol < 8; ++tileCol)
		{
		    uint8_t pixelData = (((layer2 >> (7 - tileCol)) & 1) << 1) | ((layer1 >> (7 - tileCol)) & 1);
		    uint16_t paletteIndex = pixelData | (attrData << 2);
		    //std::cout << "paletteIndex=" << (int)paletteIndex << std::endl;

		    uint8_t paletteData = m_ram[0x3f00 + paletteIndex];

		    Uint8* p = (Uint8*)m_screen->pixels + (row * 8 + tileRow) * m_screen->pitch + (col * 8 + tileCol) * 4;
		    uint32_t* pixel = (uint32_t*)p;
		    *pixel = palette[paletteData];
		}
	    }
	}
    }

    SDL_Flip(m_screen);
}

// =====================================================================================================================
uint8_t PPU::readStatusRegister()
{
    // reset the state of the address latch
    m_firstAddrWrite = true;

    // return the current value of the status register
    return m_status;
}

// =====================================================================================================================
uint8_t PPU::readDataRegister()
{
    uint8_t data = m_ram[m_address];
    incrementAddress();
    return data;
}

// =====================================================================================================================
void PPU::writeAddressRegister(uint8_t data)
{
    if (m_firstAddrWrite)
	m_address = data << 8;
    else
    {
	m_address |= data;
	std::cout << "PPU address: " << m_address << std::endl;

	if (m_address >= 0x4000)
	{
	    // TODO: mirroring ...
	    std::cerr << "Invalid PPU address: " << m_address << std::endl;
	    abort();
	}
    }

    m_firstAddrWrite = !m_firstAddrWrite;
}

// =====================================================================================================================
void PPU::writeDataRegister(uint8_t data)
{
    //    if (m_address >= 0x2000 && m_address < 0x23c0)
    //	std::cerr << "writing name table #0 at " << (int)(m_address - 0x2000) << std::endl;
        if (m_address >= 0x3f00 && m_address < 0x3f20)
    	std::cerr << "writing palette data" << std::endl;
    m_ram[m_address] = data;
    incrementAddress();
}

// =====================================================================================================================
void PPU::incrementAddress()
{
    if (m_ctrl & 0x04)
	m_address -= 32;
    else
	++m_address;
}
