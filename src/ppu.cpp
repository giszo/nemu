#include <nemu/ppu.h>

#include <lib6502/makestring.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#include <string.h>

using lib6502::MakeString;

uint32_t PPU::s_rgbPalette[64] = {
    // 1                                                                  8
    0x747474, 0x24188c, 0x0000a8, 0x44009c, 0x8c0074, 0xa80010, 0xa40000, 0x7c0800, 0x402c00, 0x004400, 0x005000, 0x003c14, 0x183c5c, 0x000000, 0x000000, 0x000000,
    0xbcbcbc, 0x0070ec, 0x2038ec, 0x8000f0, 0xbc00bc, 0xe40058, 0xd82800, 0xc84c0c, 0x887000, 0x009400, 0x00a800, 0x009038, 0x008088, 0x000000, 0x000000, 0x000000,
    0xf8f8f8, 0x3cbcfc, 0x5c94fc, 0x4088fc, 0xf478fc, 0xfc74b4, 0xfc7460, 0xfc9838, 0xf0bc3c, 0x80d010, 0x4cdc48, 0x58f898, 0x00e8d8, 0x787878, 0x000000, 0x000000,
    0xffffff, 0xa8e4fc, 0xc4d4fc, 0xd4c8fc, 0xfcc4fc, 0xfcc4d8, 0xfcbcb0, 0xfcd8a8, 0xfce4a0, 0xe0fca0, 0xa8f0bc, 0xb0fccc, 0x9cfcf0, 0xc4c4c4, 0x000000, 0x000000
};

// =====================================================================================================================
PPU::PPU(const std::shared_ptr<memory::ROM>& vrom)
    : m_ctrl(0),
      m_mask(0),
      m_status(0),
      m_address(0),
      m_firstAddrWrite(true),
      m_dataLatch(0),
      m_scrollX(0),
      m_scrollY(0),
      m_tickCounter(0),
      m_currentScanLine(0)
{
    // register video ROM
    m_memory.registerHandler(0, vrom->size(), vrom);

    // register name table RAM regions
    for (unsigned i = 0; i < 4; ++i)
    {
	m_nameTables[i] = std::make_shared<memory::RAM>(0x400);
	m_memory.registerHandler(0x2000 + i * 0x400, 0x400, m_nameTables[i]);
    }

    // register palette memory
    m_palette = std::make_shared<PaletteMemory>();
    m_memory.registerHandler(0x3f00, 0x20, m_palette);

    // create sprite memory
    m_sprite.reset(new memory::RAM(64 * 4));

    m_screen = SDL_SetVideoMode(256, 240, 32, SDL_SWSURFACE);
}

// =====================================================================================================================
void PPU::setNmiCallback(const std::function<void ()>& nmiCallback)
{
    m_nmiCallback = nmiCallback;
}

// =====================================================================================================================
void PPU::tick()
{
    ++m_tickCounter;

    if (m_tickCounter == 341)
    {
	renderScanLine(m_currentScanLine);
	renderSpriteLine(m_currentScanLine);

	++m_currentScanLine;
	m_tickCounter = 0;

	if (m_currentScanLine == 260)
	{
	    finishRendering();

	    m_status |= VBLANK;
	    m_status &= ~SPRITE0_HIT;

	    m_currentScanLine = 0;

	    if (m_ctrl & 0x80)
		m_nmiCallback();
	}
	else if (m_currentScanLine == 20)
	    m_status &= ~VBLANK;
    }
}

// =====================================================================================================================
const std::shared_ptr<memory::RAM>& PPU::spriteRam() const
{
    return m_sprite;
}

// =====================================================================================================================
uint8_t PPU::read(uint16_t address)
{
    switch (address)
    {
	case PPUSTATUS :
	    // status
	    return readStatusRegister();

	case PPUDATA :
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
	case PPUCTRL :
	    m_ctrl = data;

	    // TODO: for now only 8x8 sprites are supported
	    assert((m_ctrl & 0x20) == 0);

	    break;

	case PPUMASK :
	    m_mask = data;
	    break;

	case OAMADDR :
	    break;

	case OAMDATA :
	    break;

	case PPUSCROLL :
	    writeScrollRegister(data);
	    break;

	case PPUADDR :
	    writeAddressRegister(data);
	    break;

	case PPUDATA :
	    writeDataRegister(data);
	    break;

	default :
	    throw PPUException(MakeString() << "invalid register write: " << std::hex << address);
    }
}

// =====================================================================================================================
uint8_t PPU::readStatusRegister()
{
    static unsigned accessCounter = 0;
    ++accessCounter;

    // reset the state of the address latch
    m_firstAddrWrite = true;

    // return the current value of the status register
    uint8_t status = m_status | (accessCounter % 2 == 0 ? 0x40 : 0);

    // clear vblank flag
    m_status &= ~VBLANK;

    return status;
}

// =====================================================================================================================
uint8_t PPU::readDataRegister()
{
    uint8_t data = m_dataLatch;
    m_dataLatch = m_memory.read(m_address);

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
    m_memory.write(m_address, data);

    incrementAddress();
}

// =====================================================================================================================
void PPU::writeScrollRegister(uint8_t data)
{
    if (m_firstAddrWrite)
	m_scrollX = data;
    else
	m_scrollY = data;

    m_firstAddrWrite = !m_firstAddrWrite;
}

// =====================================================================================================================
void PPU::incrementAddress()
{
    if (m_ctrl & 0x04)
	m_address += 32;
    else
	++m_address;
}

// =====================================================================================================================
void PPU::renderScanLine(unsigned _line)
{
    if (_line < 20)
	return;

    unsigned line = _line - 20;

    unsigned* data = m_scanLineData;

    for (unsigned c = 0; c < 256; ++c)
    {
	unsigned col = c + m_scrollX;

	unsigned nameTableIdx = (col / 256) % 2;
	unsigned pixelIdx = col % 256;
	unsigned tileIdx = (line / 8) * 32 + pixelIdx / 8;

	unsigned attrByteIdx = (line / 32) * 8 + (pixelIdx / 32);
	unsigned attrRow = (line / 16) % 2;
	unsigned attrCol = (pixelIdx / 16) % 2;
	unsigned attrShift = (attrRow * 2 + attrCol) * 2 /* 2 bit per block */;

	unsigned nameTable = ((m_ctrl & 0x3) + nameTableIdx) % 2;
	uint8_t nameTableEntry = m_nameTables[nameTable]->read(tileIdx);
	uint8_t attrData = (m_nameTables[nameTable]->read(30 * 32 /* tiles */ + attrByteIdx) >> attrShift) & 0x3;

	uint16_t patternTable = 0x1000;
	uint8_t layer1 = m_memory.read(patternTable + (nameTableEntry << 4) + line % 8);
	uint8_t layer2 = m_memory.read(patternTable + (nameTableEntry << 4) + 8 + (line % 8));

	unsigned tileCol = pixelIdx % 8;
	uint8_t pixelData = (((layer2 >> (7 - tileCol)) & 1) << 1) | ((layer1 >> (7 - tileCol)) & 1);

	if (pixelData != 0)
	    *data = pixelData | (attrData << 2);
	else
	    *data = 0;

	++data;
    }

    data = m_scanLineData;
    uint32_t* pixel = (uint32_t*)((uint8_t*)m_screen->pixels + line * m_screen->pitch);

    for (unsigned c = 0; c < 256; ++c)
	*pixel++ = s_rgbPalette[m_palette->read(*data++ & 0x3f)];
}

// =====================================================================================================================
void PPU::renderSpriteLine(unsigned _line)
{
    if (_line < 20)
	return;

    unsigned line = _line - 20;

    for (unsigned i = 0; i < 64; ++i)
    {
	uint8_t y = m_sprite->read(i * 4 + 0);

	// skip this one if it is not visible in this line
	if (line < y || line >= (y + 8u))
	    continue;

	uint8_t x = m_sprite->read(i * 4 + 3);

	unsigned visX = 256 - x;

	uint8_t idx = m_sprite->read(i * 4 + 1);
	uint8_t attr = m_sprite->read(i * 4 + 2);
	bool flipHoriz = attr & 0x40;
	bool flipVert = attr & 0x80;

	uint16_t patternTable = (m_ctrl & 0x8) ? 0x1000 : 0x000;

	// find out which row of the sprite we are rendering actually
	unsigned row = line - y;

	uint8_t layer1;
	uint8_t layer2;

	if (flipVert)
	{
	    layer1 = m_memory.read(patternTable + idx * 16 + (7 - row));
	    layer2 = m_memory.read(patternTable + idx * 16 + 8 + (7 - row));
	}
	else
	{
	    layer1 = m_memory.read(patternTable + idx * 16 + row);
	    layer2 = m_memory.read(patternTable + idx * 16 + 8 + row);
	}

	uint32_t* pixel = (uint32_t*)((uint8_t*)m_screen->pixels + line * m_screen->pitch + (x * 4));

	for (unsigned col = 0; col < std::min(visX, 8u); ++col)
	{
	    uint8_t pixelData;

	    if (flipHoriz)
		pixelData = (((layer2 >> col) & 1) << 1) | ((layer1 >> col) & 1);
	    else
		pixelData = (((layer2 >> (7 - col)) & 1) << 1) | ((layer1 >> (7 - col)) & 1);

	    if (pixelData != 0)
	    {
		//if (i == 0 && m_scanLineData[x + col] != 0)
		//    m_status |= SPRITE0_HIT;

		uint8_t attrData = attr & 0x3;
		uint8_t paletteData = m_palette->read(0x10 | (attrData << 2) | pixelData);
		*pixel = s_rgbPalette[paletteData];
	    }

	    ++pixel;
	}
    }
}

// =====================================================================================================================
void PPU::finishRendering()
{
    SDL_Flip(m_screen);
}
