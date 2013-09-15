#ifndef NEMU_MEMORY_ROM_H_INCLUDED
#define NEMU_MEMORY_ROM_H_INCLUDED

#include <lib6502/memory.h>

namespace memory
{

class ROM : public lib6502::Memory
{
    public:
	ROM(uint8_t* data, unsigned size);
	~ROM();

	unsigned size() const;

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

    private:
	uint8_t* m_data;
	unsigned m_size;
};

}

#endif
