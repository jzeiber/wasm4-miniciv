# Build dependencies
CC = clang
LD = wasm-ld
WASM_OPT = wasm-opt

# Whether to build for debugging instead of release
DEBUG = 0

# Compilation flags
CFLAGS = -std=c99 -nostdlib --target=wasm32 -W -Wall -Wextra -Werror -Wno-unused -MMD -MP
CXXFLAGS = -std=c++17 -nostdlib --target=wasm32 -W -Wall -Wextra -Wno-unused -MMD -MP -fno-threadsafe-statics -fno-rtti -ffreestanding -fno-builtin

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -O0 -g
	CXXFLAGS += -DDEBUG -O0 -g
	#CXXFLAGS += -DDEBUG -O0 -g
else
	CFLAGS += -DNDEBUG -Oz -flto
#	CFLAGS += -DNDEBUG -Oz
	CXXFLAGS += -DNDEBUG -Oz -flto
#	CXXFLAGS += -DNDEBUG -Oz
endif

# Linker flags
LDFLAGS = --no-entry --import-memory --initial-memory=65536 --max-memory=65536 \
	--global-base=6560 -zstack-size=3072
ifeq ($(DEBUG), 1)
	#LDFLAGS += --export-all --no-gc-sections
	LDFLAGS += --export-dynamic --gc-sections
	#LDFLAGS += --strip-all --export-dynamic --gc-sections --lto-O3 -O3
else
	LDFLAGS += --strip-all --export-dynamic --gc-sections --lto-O3 -O3
endif

# wasm-opt flags
WASM_OPT_FLAGS = -Oz --zero-filled-memory --strip-producers
# --coalesce-locals-learning

SOURCES = $(wildcard src/*.c)
SOURCESCXX = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))
OBJECTSCXX = $(patsubst src/%.cpp, build/%.o, $(SOURCESCXX))
DEPS = $(OBJECTS:.o=.d) $(OBJECTSCXX:.o=.d)

all: build/cart.wasm

# Link cart.wasm from all object files and run wasm-opt
build/cart.wasm: $(OBJECTS) $(OBJECTSCXX)
	$(LD) -o $@ $(OBJECTS) $(OBJECTSCXX) $(LDFLAGS)
ifneq ($(DEBUG), 1)
ifeq (, $(shell where $(WASM_OPT)))
	@echo Tip: $(WASM_OPT) was not found. Install it from binaryen for smaller builds!
else
	$(WASM_OPT) $(WASM_OPT_FLAGS) $@ -o $@
endif
endif

# Compile each C source file to an object file
build/%.o: src/%.c
	@mkdir build 2> NUL | echo > NUL
	$(CC) -c $< -o $@ $(CFLAGS)
	
build/%.o: src/%.cpp
	@mkdir build 2> NUL | echo > NUL
	$(CC) -c $< -o $@ $(CXXFLAGS)

.PHONY: clean
clean:
	@rmdir /s /q build
	@echo Done

-include $(DEPS)
