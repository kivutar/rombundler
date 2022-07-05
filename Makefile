TARGET := rombundler
APP := ROMBundler
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

$(APP).app: $(TARGET)
	mkdir -p $(APP).app/Contents/MacOS
	mkdir -p $(APP).app/Contents/Resources/$(APP).iconset
	cp Info.plist $(APP).app/Contents/
	sed -i.bak 's/0.1.0/$(VERSION)/' $(APP).app/Contents/Info.plist
	rm $(APP).app/Contents/Info.plist.bak
	echo "APPL????" > $(APP).app/Contents/PkgInfo
	cp config.ini $(APP).app/Contents/MacOS
	#cp options.ini $(APP).app/Contents/Resources
	sips -z 16 16   icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_16x16.png
	sips -z 32 32   icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_16x16@2x.png
	sips -z 32 32   icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_32x32.png
	sips -z 64 64   icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_32x32@2x.png
	sips -z 128 128 icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_128x128.png
	sips -z 256 256 icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_128x128@2x.png
	sips -z 256 256 icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_256x256.png
	sips -z 512 512 icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_256x256@2x.png
	sips -z 512 512 icon.png --out $(APP).app/Contents/Resources/$(APP).iconset/icon_512x512.png
	cp rombundler $(APP).app/Contents/MacOS
	iconutil -c icns -o $(APP).app/Contents/Resources/$(APP).icns $(APP).app/Contents/Resources/$(APP).iconset
	rm -rf $(APP).app/Contents/Resources/$(APP).iconset

clean:
	rm -rf $(OBJ) $(TARGET) ROMBundler-* ROMBundler.*
