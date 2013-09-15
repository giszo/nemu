#ifndef NEMU_LOADER_H_INCLUDED
#define NEMU_LOADER_H_INCLUDED

#include <nemu/memory/rom.h>

#include <memory>

class Loader
{
    public:
	const std::shared_ptr<memory::ROM> rom() const;
	const std::shared_ptr<memory::ROM> vrom() const;

	bool load(const std::string& file);

    private:
	std::shared_ptr<memory::ROM> m_rom;
	std::shared_ptr<memory::ROM> m_vrom;
};

#endif
