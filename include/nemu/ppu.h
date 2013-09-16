#ifndef NEMU_PPU_H_INCLUDED
#define NEMU_PPU_H_INCLUDED

#include <nemu/memory/rom.h>
#include <nemu/memory/dispatcher.h>
#include <nemu/ppu/palette.h>

#include <SDL/SDL.h>

#include <stdexcept>
#include <functional>
#include <memory>

class PPUException : public std::runtime_error
{
    public:
	PPUException(const std::string& error)
	    : runtime_error(error)
	{}
};

class PPU : public lib6502::Memory
{
    public:
	enum
	{
	    PPUCTRL,
	    PPUMASK,
	    PPUSTATUS,
	    OAMADDR,
	    OAMDATA,
	    PPUSCROLL,
	    PPUADDR,
	    PPUDATA
	};

	// status register bits
	enum
	{
	    SPRITE0_HIT = 0x40,
	    VBLANK = 0x80
	};

	PPU(const std::shared_ptr<memory::ROM>& vrom);

	void tick();

	const std::shared_ptr<memory::RAM>& spriteRam() const;

	void setNmiCallback(const std::function<void()>& nmiCallback);

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

	void renderScanLine(unsigned line);
	void renderSpriteLine(unsigned line);
	void finishRendering();

    private:
	uint8_t readStatusRegister();
	uint8_t readDataRegister();

	void writeAddressRegister(uint8_t data);
	void writeDataRegister(uint8_t data);
	void writeScrollRegister(uint8_t data);

	void incrementAddress();

    private:
	uint8_t m_ctrl;
	uint8_t m_mask;

	// status register - read only
	uint8_t m_status;

	uint16_t m_address;
	bool m_firstAddrWrite;

	uint8_t m_dataLatch;

	uint8_t m_scrollX;
	uint8_t m_scrollY;

	unsigned m_tickCounter;
	unsigned m_currentScanLine;

	SDL_Surface* m_screen;

	std::function<void()> m_nmiCallback;

	memory::Dispatcher m_memory;
	std::shared_ptr<memory::RAM> m_nameTables[4];
	std::shared_ptr<PaletteMemory> m_palette;
	std::shared_ptr<memory::RAM> m_sprite;

	unsigned m_scanLineData[256];

	static uint32_t s_rgbPalette[64];
};

#endif
