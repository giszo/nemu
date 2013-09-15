#ifndef NEMU_MEMORYDISPATCHER_H_INCLUDED
#define NEMU_MEMORYDISPATCHER_H_INCLUDED

#include <lib6502/memory.h>

#include <memory>
#include <vector>
#include <stdexcept>
namespace memory
{

class Dispatcher : public lib6502::Memory
{
    public:
	class InvalidAddressException : public std::runtime_error
	{
	    public:
		InvalidAddressException(uint16_t address)
		    : runtime_error("invalid memory access"),
		      m_address(address)
		{}

		uint16_t getAddress() const
		{ return m_address; }

	    private:
		uint16_t m_address;
	};

	void registerHandler(uint16_t address, const std::shared_ptr<lib6502::Memory>& handler);
	void registerHandler(uint16_t address, uint16_t size, const std::shared_ptr<lib6502::Memory>& handler);

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

    private:
	struct Handler
	{
	    uint16_t m_base;
	    uint16_t m_size;
	    std::shared_ptr<lib6502::Memory> m_handler;
	};

	const Handler& findHandler(uint16_t address);

	std::vector<Handler> m_handlers;
};

}

#endif
