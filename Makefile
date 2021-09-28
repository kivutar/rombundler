TARGET := rombundler
VERSION ?= devel

TARGET := index.html
LDFLAGS := -ldl -lopenal -s USE_WEBGL2=1 -s USE_GLFW=3 -s WASM=1 -s MAIN_MODULE=1 \
	-s EXIT_RUNTIME=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	--no-heap-copy \
	--preload-file config.ini \
	--preload-file nes_libretro.wasm.so \
	--preload-file mm.nes

CFLAGS += -Wall -O3 -fPIC -flto -I. -Iinclude -Ideps/include

OBJ = main.o glad.o config.o core.o audio.o video.o input.o options.o ini.o utils.o srm.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) -flto

bundle: $(TARGET)
	mkdir -p ROMBundler-$(OS)-$(VERSION)
	cp $(TARGET) ROMBundler-$(OS)-$(VERSION)
	cp *.dll ROMBundler-$(OS)-$(VERSION) || :
	cp config.ini ROMBundler-$(OS)-$(VERSION)
	cp README.md ROMBundler-$(OS)-$(VERSION)
	cp COPYING ROMBundler-$(OS)-$(VERSION)
	zip -r ROMBundler-$(OS)-$(VERSION).zip ROMBundler-$(OS)-$(VERSION)

clean:
	rm -rf $(OBJ) $(TARGET) ROMBundler-*
