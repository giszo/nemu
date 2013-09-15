#include <nemu/nesemulator.h>
#include <nemu/loader.h>
#include <nemu/ppu.h>
#include <nemu/memory/ram.h>

#include <lib6502/cpu.h>

#include <SDL/SDL.h>

#include <iostream>
#include <iomanip>

static uint64_t s_tick = 0;

// =====================================================================================================================
class CpuTracer : public lib6502::InstructionTracer
{
    public:
	void trace(const lib6502::Cpu::State& state, const std::string& inst) override
	{
	    std::cerr << std::hex;
	    std::cerr << "t=" << std::setw(8) << std::setfill('0') << s_tick << " ";
	    std::cerr << "CPU [";
	    std::cerr << "PC=" << std::setw(4) << std::setfill('0') << state.m_PC << " ";
	    std::cerr << "A=" << std::setw(2) << std::setfill('0') << (int)state.m_A << " ";
	    std::cerr << "X=" << std::setw(2) << std::setfill('0') << (int)state.m_X << " ";
	    std::cerr << "Y=" << std::setw(2) << std::setfill('0') << (int)state.m_Y << " ";
	    std::cerr << "S=" << std::setw(2) << std::setfill('0') << (int)state.m_status << " ";
	    std::cerr << "SP=" << std::setw(2) << std::setfill('0') << (int)state.m_SP << " ";
	    std::cerr << "int=" << (state.m_inInterrupt ? "Y" : "N");
	    std::cerr << "] ";
	    std::cerr << inst;
	    std::cerr << std::endl;
	}
};

// =====================================================================================================================
NesEmulator::NesEmulator()
    : m_running(true)
{
}

// =====================================================================================================================
int NesEmulator::run(int argc, char** argv)
{
    if (argc != 2)
	return 1;

    SDL_Init(SDL_INIT_EVERYTHING);

    if (!loadCartridge(argv[1]))
    {
	std::cerr << "Unable to load cartridge: " << argv[1] << std::endl;
	return 1;
    }

    // register 2kB system memory
    std::shared_ptr<memory::RAM> ram(new memory::RAM(0x800));
    for (unsigned i = 0; i < 4; ++i)
	m_memory.registerHandler(i * 0x800, 0x800, ram);

    m_cpu.reset(new lib6502::Cpu(m_memory));

    // register PPU mappnigs
    m_memory.registerHandler(0x2000, 8, m_ppu);
    m_ppu->setNmiCallback(std::bind(&NesEmulator::frameComplete, this));

    try
    {
	while (m_running)
	{
	    m_ppu->tick();
	    m_ppu->tick();
	    m_ppu->tick();

	    // TODO: eliminate this try-catch by registering fake APU registers
	    try
	    {
		m_cpu->tick();
	    }
	    catch (const memory::Dispatcher::InvalidAddressException& e)
	    {
		if (e.getAddress() >= 0x4000 && e.getAddress() <= 0x4019)
		{ /* this is ok ... */ }
		else
		{
		    std::cerr << "Invalid memory access at $" << std::hex << e.getAddress() << std::endl;
		    return 1;
		}
	    }
	}
    }
    catch (const PPUException& e)
    {
	std::cerr << "PPU error: " << e.what() << std::endl;
	std::cerr << "PC=" << std::hex << m_cpu->getState().m_PC << std::endl;
	return 1;
    }
    catch (const lib6502::CpuException& e)
    {
	std::cerr << "CPU error: " << e.what() << std::endl;
	std::cerr << "PC=" << std::hex << m_cpu->getState().m_PC << std::endl;
	return 1;
    }

    SDL_Quit();

    return 0;
}

// =====================================================================================================================
bool NesEmulator::loadCartridge(const std::string& file)
{
    Loader ldr;

    if (!ldr.load(file))
	return false;

    const auto& rom = ldr.rom();

    std::cout << "Program ROM size: " << rom->size() << " bytes" << std::endl;

    // program ROM
    m_memory.registerHandler(0x8000, rom->size(), rom);

    if (rom->size() < 32 * 1024)
	m_memory.registerHandler(0xc000, rom->size(), rom);

    // video ROM
    m_ppu.reset(new PPU(ldr.vrom()));

    return true;
}

// =====================================================================================================================
void NesEmulator::frameComplete()
{
    SDL_Event event;

    // generate an NMI at the end of a PPU frame
    m_cpu->nmi();

    // handle SDL events
    while (SDL_PollEvent(&event))
    {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
	    m_running = false;
    }

    // calculate FPS
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

    if (!m_lastFrameEnd.is_not_a_date_time())
    {
	double fps = 1000000.0f / (now - m_lastFrameEnd).total_microseconds();
	std::cout << "FPS: " << fps << "          \r";
    }

    m_lastFrameEnd = now;
}
