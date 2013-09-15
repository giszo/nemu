#include <nemu/memory/dispatcher.h>

#include <iomanip>
#include <iostream>

using memory::Dispatcher;

// =====================================================================================================================
void Dispatcher::registerHandler(uint16_t address, const std::shared_ptr<lib6502::Memory>& handler)
{
    registerHandler(address, 1, handler);
}

// =====================================================================================================================
void Dispatcher::registerHandler(uint16_t address, uint16_t size, const std::shared_ptr<lib6502::Memory>& handler)
{
    m_handlers.push_back({address, size, handler});
}

// =====================================================================================================================
uint8_t Dispatcher::read(uint16_t address)
{
    const auto& h = findHandler(address);
    return h.m_handler->read(address - h.m_base);
}

// =====================================================================================================================
void Dispatcher::write(uint16_t address, uint8_t data)
{
    const auto& h = findHandler(address);
    h.m_handler->write(address - h.m_base, data);
}

// =====================================================================================================================
const Dispatcher::Handler& Dispatcher::findHandler(uint16_t address)
{
    for (const auto& h : m_handlers)
    {
	if ((h.m_base <= address) && (address <= (h.m_base + h.m_size - 1)))
	    return h;
    }

    throw InvalidAddressException(address);
}
