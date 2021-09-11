TARGET := rombundler
VERSION ?= devel

ifeq ($(shell uname -s),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll
	LD := $(CC)
	OS ?= Windows
else ifneq ($(findstring MINGW,$(shell uname -s)),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -flto
	LD := $(CC)
	OS ?= Windows
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	LDFLAGS := -lglfw -flto -framework OpenAL
	LD := $(CC)
	OS ?= OSX
else
	LDFLAGS := -lglfw -flto -lopenal
	OS ?= Linux
endif

CFLAGS += -Wall -O3 -fPIC -flto -I. -I./include

OBJ = main.o glad.o config.o audio.o video.o ini.o utils.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

bundle: $(TARGET)
	mkdir -p ROMBundler-$(OS)-$(VERSION)
	cp $(TARGET) ROMBundler-$(OS)-$(VERSION)
	cp *.dll ROMBundler-$(OS)-$(VERSION) || :
	cp config.ini ROMBundler-$(OS)-$(VERSION)
	cp README.md ROMBundler-$(OS)-$(VERSION)
	cp COPYING ROMBundler-$(OS)-$(VERSION)
	zip -r ROMBundler-$(OS)-$(VERSION).zip ROMBundler-$(OS)-$(VERSION)

clean:
	rm -f $(OBJ) $(TARGET) ROMBundler-*
