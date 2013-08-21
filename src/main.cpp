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

	    if (address == 0x4016 || address == 0x4017)
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

	    if (address == 0x4014 || address == 0x4016 /*???*/)
		return; // some DMA stuff

	    std::cerr << "Invalid memory write at address: " << std::hex << address << std::endl;
	    abort();
	}

    private:
	uint8_t* m_ram;
	uint8_t* m_prgData;

	PPU& m_ppu;
};

// =====================================================================================================================
class CpuTracer : public lib6502::InstructionTracer
{
    public:
	void trace(const lib6502::Cpu::State& state, const std::string& inst) override
	{
	    std::cerr << "CPU [PC="<< std::hex << std::setw(4) << std::setfill('0') << state.m_PC << "]: " << inst << std::endl;
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
int main(int argc, char** argv)
{
    if (argc != 2)
	return 1;

    //SDL_Init(SDL_INIT_EVERYTHING);

    uint8_t* rom;
    uint8_t* vrom;
    loadNesFile(argv[1], &rom, &vrom);

    PPU ppu(vrom);
    TestMemory m(rom, ppu);
    lib6502::Cpu cpu(m);
    cpu.setInstructionTracer(new CpuTracer());

    try
    {
	for (int i = 0; i < 500000; ++i)
	    cpu.tick();

	// perform an NMI
	for (int i = 0; i < 30; ++i)
	{
	    std::cout << "Simulating NMI" << std::endl;
	    cpu.nmi();
	    while (cpu.isInInterrupt())
		cpu.tick();
	}
    }
    catch (const PPUException& e)
    {
	std::cerr << "PPU error: " << e.what() << std::endl;
	std::cerr << "CPU state:" << std::endl;
	cpu.dumpState(std::cerr);
	return 1;
    }
    catch (const lib6502::CpuException& e)
    {
	std::cerr << "CPU error: " << e.what() << std::endl;
	cpu.dumpState(std::cerr);
	return 1;
    }

    ppu.dumpNameTables();
    ppu.renderVideo();

    //SDL_Delay(5000);
    //SDL_Quit();

    return 0;
}
