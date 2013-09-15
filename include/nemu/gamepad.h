#ifndef NEMU_GAMEPAD_H_INCLUDED
#define NEMU_GAMEPAD_H_INCLUDED

#include <lib6502/memory.h>

class GamePad : public lib6502::Memory
{
    public:
	GamePad();
	~GamePad();

	void pollEvents();

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

    private:
	uint8_t* m_data;
	unsigned m_position;

	bool m_resetInProgress;
};

#endif
