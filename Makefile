CFLAGS += -O3 -fPIC -flto -I. -I./include
LDFLAGS += -lglfw -framework OpenAL -flto

OBJ = main.o glad.o audio.o ini.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

rombundler: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm $(OBJ) rombundler
