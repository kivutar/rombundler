TARGET := rombundler

ifeq ($(shell uname -s),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll
	LD := $(CC)
else ifneq ($(findstring MINGW,$(shell uname -s)),) # win
	TARGET := rombundler.exe
	LDFLAGS += -L./lib -lglfw3dll -lOpenal32.dll -flto
	LD := $(CC)
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	LDFLAGS := -lglfw -flto -framework OpenAL
	LD := $(CC)
else
	LDFLAGS := -lglfw -flto -lopenal
endif

CFLAGS += -Wall -O3 -fPIC -flto -I. -I./include

OBJ = main.o glad.o config.o audio.o video.o ini.o utils.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)
