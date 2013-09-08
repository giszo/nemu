#ifndef NEMU_PPU_H_INCLUDED
#define NEMU_PPU_H_INCLUDED

#include "ppu/palette.h"

#include <lib6502/memory.h>

#include <SDL/SDL.h>

#include <stdexcept>
#include <functional>

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
	PPU(uint8_t* vrom);
	~PPU();

	void tick();

	void setNmiCallback(const std::function<void()>& nmiCallback);

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

	void dumpNameTables();

	void renderScanLine(unsigned line);
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

	uint8_t* m_ram;
	PaletteMemory m_palette;

	uint8_t m_scrollX;
	uint8_t m_scrollY;
	bool m_firstScrollWrite;

	unsigned m_tickCounter;
	unsigned m_currentScanLine;

	SDL_Surface* m_screen;

	std::function<void()> m_nmiCallback;

	static const uint8_t VBLANK = 0x80;
};

#endif
