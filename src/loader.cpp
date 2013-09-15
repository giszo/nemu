#include <nemu/loader.h>

#include <fcntl.h>
#include <unistd.h>

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
const std::shared_ptr<memory::ROM> Loader::rom() const
{
    return m_rom;
}

// =====================================================================================================================
const std::shared_ptr<memory::ROM> Loader::vrom() const
{
    return m_vrom;
}

// =====================================================================================================================
bool Loader::load(const std::string& file)
{
    uint8_t* data;

    int f = ::open(file.c_str(), O_RDONLY);

    NesHeader hdr;
    if (read(f, &hdr, sizeof(hdr)) != sizeof(NesHeader))
	return false;

    data = new uint8_t[hdr.prgRomCount * 16384];

    // read PRG rom contents (TODO: close the file in case of error)
    if (read(f, data, hdr.prgRomCount * 16384) != hdr.prgRomCount * 16384)
	return false;

    m_rom.reset(new memory::ROM(data, hdr.prgRomCount * 16384));

    data = new uint8_t[hdr.chrRomCount * 8192];

    // TODO: see above ...
    if (read(f, data, hdr.chrRomCount * 8192) != hdr.chrRomCount * 8192)
	return false;

    m_vrom.reset(new memory::ROM(data, hdr.chrRomCount * 8192));

    close(f);

    return true;
}
