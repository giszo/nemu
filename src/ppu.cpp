#include <nemu/ppu.h>

#include <lib6502/makestring.h>

#include <iostream>
#include <iomanip>

#include <string.h>

using lib6502::MakeString;

// =====================================================================================================================
PPU::PPU(uint8_t* vrom)
    : m_ctrl(0),
      m_mask(0),
      m_status(0x80),
      m_address(0),
      m_firstAddrWrite(true),
      m_dataLatch(0)
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
	    break;

	case 0x2001 :
	    m_mask = data;
	    break;

	case 0x2003 :
	    // OAM address
	    break;

	case 0x2004 :
	    // OAM data
	    break;

	case 0x2005 :
	    // scroll register
	    //std::cout << "PPU scroll: " << (int)data << std::endl;
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
    std::cout << "Name table #0: " << std::endl;
    std::cout << std::hex;
    for (unsigned i = 0; i < 32 * 30; ++i)
    {
	if (i > 0 && (i % 32) == 0)
	    std::cout << std::endl;
	std::cout << std::setw(2) << std::setfill('0') << (int)m_ram[0x2000 + i] << " ";
    }
    std::cout << std::endl;
}

// =====================================================================================================================
void PPU::renderVideo()
{
    static uint32_t palette[64] = {
	// 1                                                                  8
	0x747474, 0x24188c, 0x0000a8, 0x44009c, 0x8c0074, 0xa80010, 0xa40000, 0x7c0800, 0x402c00, 0x004400, 0x005000, 0x003c14, 0x183c5c, 0x000000, 0x000000, 0x000000,
	0xbcbcbc, 0x0070ec, 0x2038ec, 0x8000f0, 0xbc00bc, 0xe40058, 0xd82800, 0xc84c0c, 0x887000, 0x009400, 0x00a800, 0x009038, 0x008088, 0x000000, 0x000000, 0x000000,
	0xf8f8f8, 0x3cbcfc, 0x5c94fc, 0x4088fc, 0xf478fc, 0xfc74b4, 0xfc7460, 0xfc9838, 0xf0bc3c, 0x80d010, 0x4cdc48, 0x58f898, 0x00e8d8, 0x787878, 0x000000, 0x000000,
	0xffffff, 0xa8e4fc, 0xc4d4fc, 0xd4c8fc, 0xfcc4fc, 0xfcc4d8, 0xfcbcb0, 0xfcd8a8, 0xfce4a0, 0xe0fca0, 0xa8f0bc, 0xb0fccc, 0x9cfcf0, 0xc4c4c4, 0x000000, 0x000000
    };

    uint16_t patternTable = (m_ctrl & 0x10) ? 0x1000 : 0x0000;
    uint16_t nameTable = 0;

    switch (m_ctrl & 0x3)
    {
	case 0x00 : nameTable = 0x2000; break;
	case 0x01 : nameTable = 0x2400; break;
	case 0x02 : nameTable = 0x2800; break;
	case 0x03 : nameTable = 0x2c00; break;
    }

    //std::cout << "PPU: using pattern table #" << (m_ctrl & 0x10 ? 1 : 0) << " for background rendering" << std::endl;
    //std::cout << "PPU: base name table: " << std::hex << nameTable << std::endl;

    for (unsigned row = 0; row < 30; ++row)
    {
	for (unsigned col = 0; col < 32; ++col)
	{
	    uint32_t tileIndex = row * 32 + col;
	    uint16_t attrByteIndex = (row / 4) * 8 + (col / 4);

	    //std::cout << "row=" << row << ", col=" << col << std::endl;
	    //std::cout << "tile=" << tileIndex << ", attrByte=" << attrByteIndex << std::endl;

	    unsigned rowMod = row % 2;
	    unsigned colMod = col % 2;
	    unsigned attrShift = (rowMod * 2 + colMod) * 2 /* 2 bit per block */;

	    uint8_t nameEntry = m_ram[nameTable + tileIndex];
	    uint8_t attrData = (m_ram[nameTable + 30 * 32 /* tiles */ + attrByteIndex] >> (6 - attrShift)) & 0x3;

	    //std::cout << "name=" << (int)nameEntry << ", attr=" << (int)attrData << std::endl;

	    for (unsigned tileRow = 0; tileRow < 8; ++tileRow)
	    {
		uint8_t layer1 = m_ram[patternTable + (nameEntry << 4) + tileRow];
		uint8_t layer2 = m_ram[patternTable + (nameEntry << 4) + 8 + tileRow];

		//std::cout << "tileRow=" << std::dec << tileRow << " L1=" << std::hex << (int)layer1 << " Y=" << std::hex << (int)layer2 << std::endl;

		for (unsigned tileCol = 0; tileCol < 8; ++tileCol)
		{
		    unsigned x = col * 8 + tileCol;
		    unsigned y = row * 8 + tileRow;
		    //std::cout << std::dec << "X=" << x << ", Y=" << y << std::endl;

		    uint8_t pixelData = (((layer2 >> (7 - tileCol)) & 1) << 1) | ((layer1 >> (7 - tileCol)) & 1);
		    uint16_t paletteIndex = pixelData | (attrData << 2);
		    //std::cout << "pixelData=" << (int)pixelData << " paletteIndex=" << (int)paletteIndex << std::endl;

		    uint8_t paletteData = m_ram[0x3f00 + paletteIndex] & 0x3f;

		    Uint8* p = (Uint8*)m_screen->pixels + y * m_screen->pitch + x * 4;
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
    static unsigned accessCounter = 0;
    ++accessCounter;

    // reset the state of the address latch
    m_firstAddrWrite = true;

    // return the current value of the status register
    return m_status | ((accessCounter % 2) == 0 ? 0x40 : 0x00);
}

// =====================================================================================================================
uint8_t PPU::readDataRegister()
{
    uint8_t data = m_dataLatch;
    m_dataLatch = m_ram[m_address];
    incrementAddress();
    return data;
}

// =====================================================================================================================
void PPU::writeAddressRegister(uint8_t data)
{
    if (m_firstAddrWrite)
	m_address = (m_address & 0xff) | (data << 8);
    else
    {
	m_address = (m_address & 0xff00) | data;
	m_address &= 0x3fff;
	//std::cout << "PPU address: " << std::hex << m_address << std::endl;
    }

    m_firstAddrWrite = !m_firstAddrWrite;
}

// =====================================================================================================================
void PPU::writeDataRegister(uint8_t data)
{
    m_ram[m_address] = data;
    incrementAddress();
}

// =====================================================================================================================
void PPU::incrementAddress()
{
    if (m_ctrl & 0x04)
	m_address += 32;
    else
	++m_address;
}
