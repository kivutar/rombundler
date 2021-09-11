CFLAGS += -Wall -O3 -fPIC -flto -I. -I./include
LDFLAGS += -dl -lglfw -framework OpenAL -flto

OBJ = main.o glad.o config.o audio.o video.o ini.o utils.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: rombundler
rombundler: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) rombundler
