TARGET := rombundler
AL := -lopenal

ifeq ($(shell uname -s),) # win
	TARGET := rombundler.exe
else ifneq ($(findstring MINGW,$(shell uname -s)),) # win
	TARGET := rombundler.exe
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	AL := -framework OpenAL
endif

CFLAGS += -Wall -O3 -fPIC -flto -I. -I./include
LDFLAGS += -dl -lglfw -flto $(AL)

OBJ = main.o glad.o config.o audio.o video.o ini.o utils.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)
