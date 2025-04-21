TARGET  := rombundler
APP     := ROMBundler
VERSION ?= devel
UNAME_S := $(shell uname -s)
ARCH    := $(shell uname -m)
APP.DIR := $(APP)-$(ARCH).app

ifeq ($(UNAME_S),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -mwindows
	OS ?= Windows
else ifneq ($(findstring MINGW,$(UNAME_S)),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -mwindows
	OS ?= Windows
else ifneq ($(findstring Darwin,$(UNAME_S)),) # osx
	LDFLAGS := -Ldeps/osx_$(ARCH)/lib -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit
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
	mkdir -p ROMBundler-$(OS)-$(VERSION)-$(ARCH)
	cp $(TARGET) ROMBundler-$(OS)-$(VERSION)-$(ARCH)
	cp *.dll ROMBundler-$(OS)-$(VERSION)-$(ARCH) || :
	cp config.ini ROMBundler-$(OS)-$(VERSION)-$(ARCH)
	cp README.md ROMBundler-$(OS)-$(VERSION)-$(ARCH)
	cp COPYING ROMBundler-$(OS)-$(VERSION)-$(ARCH)
	zip -r ROMBundler-$(OS)-$(VERSION)-$(ARCH).zip ROMBundler-$(OS)-$(VERSION)-$(ARCH)

$(APP).app: $(TARGET)
	mkdir -p $(APP.DIR)/Contents/MacOS
	mkdir -p $(APP.DIR)/Contents/Resources/$(APP).iconset
	cp bundle/Darwin/Info.plist $(APP.DIR)/Contents/
	sed -i.bak 's/0.1.0/$(VERSION)/' $(APP.DIR)/Contents/Info.plist
	rm $(APP.DIR)/Contents/Info.plist.bak
	echo "APPL????" > $(APP.DIR)/Contents/PkgInfo
	cp config.ini $(APP.DIR)/
	#cp options.ini $(APP.DIR)/Contents/Resources
	sips -z 16 16   bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_16x16.png
	sips -z 32 32   bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_16x16@2x.png
	sips -z 32 32   bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_32x32.png
	sips -z 64 64   bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_32x32@2x.png
	sips -z 128 128 bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_128x128.png
	sips -z 256 256 bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_128x128@2x.png
	sips -z 256 256 bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_256x256.png
	sips -z 512 512 bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_256x256@2x.png
	sips -z 512 512 bundle/Darwin/icon.png --out $(APP.DIR)/Contents/Resources/$(APP).iconset/icon_512x512.png
	cp bundle/Darwin/launcher.command $(APP.DIR)/Contents/MacOS
	cp rombundler $(APP.DIR)/Contents/MacOS
	iconutil -c icns -o $(APP.DIR)/Contents/Resources/$(APP).icns $(APP.DIR)/Contents/Resources/$(APP).iconset
	rm -rf $(APP.DIR)/Contents/Resources/$(APP).iconset
	zip -r ROMBundler-$(OS)-$(VERSION)-$(ARCH).app.zip $(APP.DIR)

clean:
	rm -rf $(OBJ) $(TARGET) ROMBundler-* ROMBundler-$(ARCH).*
