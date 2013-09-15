env = Environment()

sources = [
    "main.cpp",
    "loader.cpp",
    "ppu.cpp",
    "ppu/palette.cpp",
    "nesemulator.cpp",
    "memory/dispatcher.cpp",
    "memory/rom.cpp",
    "memory/ram.cpp"
]

env.Program(
    "nemu",
    source = ["src/%s" % s for s in sources],
    CPPFLAGS = ["-O2", "-Wall", "-std=c++11"],
    CPPPATH = ["include/"],
    LIBS = ["6502", "SDL"]
)
