TARGET := rombundler
VERSION ?= devel

ifeq ($(shell uname -s),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -mwindows
	OS ?= Windows
else ifneq ($(findstring MINGW,$(shell uname -s)),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -mwindows
	OS ?= Windows
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	LDFLAGS := -Ldeps/osx_$(shell uname -m)/lib -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit
	LDFLAGS += -framework OpenAL
	OS ?= OSX
else
	LDFLAGS := -ldl
	LDFLAGS += $(shell pkg-config --libs glfw3)
	LDFLAGS += $(shell pkg-config --libs openal)
	OS ?= Linux
endif

INCFLAGS += -I. -Iinclude -Ideps/include
INCFLAGS += -Iggpo/src/include -Iggpo/src/lib/ggpo -Iggpo/src/lib/ggpo/network -Iggpo/src/lib/ggpo/backends
CFLAGS += -Wall -O3 -fPIC -flto 
CXXFLAGS += -fno-rtti -fno-exceptions
LDFLAGS += -lc++

SOURCES_CXX = main.cpp \
	glad.cpp \
	config.cpp \
	core.cpp \
	audio.cpp \
	video.cpp \
	input.cpp \
	options.cpp \
	ini.cpp \
	utils.cpp \
	srm.cpp \
	netplay.cpp

SOURCES_CXX += ggpo/src/lib/ggpo/bitvector.cpp \
	ggpo/src/lib/ggpo/game_input.cpp \
	ggpo/src/lib/ggpo/input_queue.cpp \
	ggpo/src/lib/ggpo/log.cpp \
	ggpo/src/lib/ggpo/main.cpp \
	ggpo/src/lib/ggpo/platform_unix.cpp \
	ggpo/src/lib/ggpo/poll.cpp \
	ggpo/src/lib/ggpo/sync.cpp \
	ggpo/src/lib/ggpo/timesync.cpp \
	ggpo/src/lib/ggpo/pevents.cpp \
	ggpo/src/lib/ggpo/network/udp.cpp \
	ggpo/src/lib/ggpo/network/udp_proto.cpp \
	ggpo/src/lib/ggpo/backends/p2p.cpp \
	ggpo/src/lib/ggpo/backends/spectator.cpp

OBJECTS := $(SOURCES_CXX:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(INCFLAGS) $(CFLAGS) $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $<  $(INCFLAGS) $(CFLAGS) -flto

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

bundle: $(TARGET)
	mkdir -p ROMBundler-$(OS)-$(VERSION)
	cp $(TARGET) ROMBundler-$(OS)-$(VERSION)
	cp *.dll ROMBundler-$(OS)-$(VERSION) || :
	cp config.ini ROMBundler-$(OS)-$(VERSION)
	cp README.md ROMBundler-$(OS)-$(VERSION)
	cp COPYING ROMBundler-$(OS)-$(VERSION)
	zip -r ROMBundler-$(OS)-$(VERSION).zip ROMBundler-$(OS)-$(VERSION)

clean:
	rm -rf $(OBJECTS) $(TARGET) ROMBundler-*
