CFLAGS += -Wall -O3 -fPIC -flto -I. -I./include
LDFLAGS += -dl -lglfw -framework OpenAL -flto

OBJ = main.o glad.o audio.o ini.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean

all: rombundler
rombundler: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm $(OBJ) rombundler
