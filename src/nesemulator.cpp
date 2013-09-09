#include <nemu/nesemulator.h>

#include <nemu/ppu.h>

#include <lib6502/cpu.h>

#include <SDL/SDL.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

// =====================================================================================================================
struct NesHeader
{
	uint8_t signature[4];
	uint8_t prgRomCount;
	uint8_t chrRomCount;
	uint8_t flags6;
	uint8_t flags7;
	uint8_t prgRamCount;
	uint8_t flags9;
	uint8_t flags10;
	uint8_t zero[5];
} __attribute__((packed));

// =====================================================================================================================
class TestMemory : public lib6502::Memory
{
    public:
	TestMemory(uint8_t* prgData, PPU& ppu)
	    : m_prgData(prgData),
	      m_ppu(ppu)
	{
	    m_ram = new uint8_t[0x800];
	    memset(m_ram, 0xff, 0x800);
	}

	~TestMemory()
	{
	    delete[] m_ram;
	    delete[] m_prgData;
	}

	uint8_t read(uint16_t address) override
	{
	    // ram
	    if (address >= 0x0000 && address <= 0x1fff)
		return m_ram[address % 0x800];

	    // prg rom
	    if (address >= 0x8000 && address <= 0xffff)
		return m_prgData[address - 0x8000];

	    // PPU
	    if (address >= 0x2000 && address <= 0x2007)
		return m_ppu.read(address);

	    if (address == 0x4015 || address == 0x4016 || address == 0x4017)
		return 0; // ???

	    std::cerr << "Invalid memory read at address: " << std::hex << address << std::endl;
	    abort();

	    return 0;
	}

	void write(uint16_t address, uint8_t data) override
	{
	    if (address >= 0x0000 && address <= 0x1fff)
	    {
		m_ram[address % 0x800] = data;
		return;
	    }

	    // PPU
	    if (address >= 0x2000 && address <= 0x2007)
	    {
		m_ppu.write(address, data);
		return;
	    }

	    if ((address >= 0x4000 && address <= 0x4013) || (address == 0x4015))
	    {
		// TODO: sound ...
		return;
	    }

	    if (address == 0x4014 || address == 0x4016 /*???*/ || address == 0x4017 /* extra WTF?! */)
		return; // some DMA stuff

	    std::cerr << "Invalid memory write at address: " << std::hex << address << std::endl;
	    abort();
	}

    private:
	uint8_t* m_ram;
	uint8_t* m_prgData;

	PPU& m_ppu;
};

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
static void loadNesFile(char* filename, uint8_t** _rom, uint8_t** _vrom)
{
    int f = open(filename, O_RDONLY);

    NesHeader hdr;
    read(f, &hdr, sizeof(hdr));

    std::cout << "PRG size: " << hdr.prgRomCount * 16384 << " bytes" << std::endl;

    // allocate and read PRG rom contents
    uint8_t* rom = new uint8_t[32 * 1024];
    read(f, rom, hdr.prgRomCount * 16384);
    *_rom = rom;

    std::cout << "VROM size: " << hdr.chrRomCount * 8192 << " bytes" << std::endl;

    uint8_t* vrom = new uint8_t[hdr.chrRomCount * 8192];
    read(f, vrom, hdr.chrRomCount * 8192);
    *_vrom = vrom;

    close(f);
}

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

    uint8_t* rom;
    uint8_t* vrom;
    loadNesFile(argv[1], &rom, &vrom);

    PPU ppu(vrom);
    ppu.setNmiCallback(std::bind(&NesEmulator::frameComplete, this));

    TestMemory m(rom, ppu);
    m_cpu.reset(new lib6502::Cpu(m));
    //cpu.setInstructionTracer(new CpuTracer());

    try
    {
	while (m_running)
	{
	    ppu.tick();
	    ppu.tick();
	    ppu.tick();
	    m_cpu->tick();
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
	double fps = 1000.0f / (now - m_lastFrameEnd).total_milliseconds();
	std::cout << "FPS: " << fps << "\r";
    }

    m_lastFrameEnd = now;
}
