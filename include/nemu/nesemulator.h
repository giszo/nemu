#ifndef NEMU_NESEMULATOR_H_INCLUDED
#define NEMU_NESEMULATOR_H_INCLUDED

#include <lib6502/cpu.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <memory>

class NesEmulator
{
    public:
	NesEmulator();

	int run(int argc, char** argv);

    private:
	/// called when the PPU finished rendering of a frame
	void frameComplete();

    private:
	/// true while the mainloop of the emulator is running
	bool m_running;

	std::unique_ptr<lib6502::Cpu> m_cpu;

	boost::posix_time::ptime m_lastFrameEnd;
};

#endif
