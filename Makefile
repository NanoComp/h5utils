OBJ_h5topng = h5topng.o arrayh5.o writepng.o
OBJ_h5read = h5read.cc arrayh5.o
LIBS_arrayh5 = -lhdf5 -lz -lm

TARGETS = h5topng h5read.oct

all: $(TARGETS)

h5read.oct: $(OBJ_h5read)
	mkoctfile $(OBJ_h5read) $(LIBS_arrayh5)

h5topng: $(OBJ_h5topng)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ_h5topng) -lpng $(LIBS_arrayh5) -o $@

clean:
	rm -f $(OBJ_h5topng) $(TARGETS) core a.out octave-core
