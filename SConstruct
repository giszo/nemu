env = Environment()

env.Program(
    "nemu",
    source = ["src/main.cpp", "src/ppu.cpp", "src/ppu/palette.cpp"],
    CPPFLAGS = ["-O2", "-Wall", "-std=c++11"],
    CPPPATH = ["include/"],
    LIBS = ["6502", "SDL"]
)
