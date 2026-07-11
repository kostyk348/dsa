CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11 -fPIC -Iinclude
LDFLAGS = -lm -lpthread -lasound -lmpg123

DSASRC = src/arena.c src/mixer.c src/system.c src/output_alsa.c src/decoder.c
DSADBJ = $(DSASRC:.c=.o)
FMODSRC = src/fmod_compat.c src/fmod_cxxabi.c
FMODOBJ = $(FMODSRC:.c=.o)
STUDIOSRC = src/studio_compat.c
STUDIOOBJ = $(STUDIOSRC:.c=.o)

LIB = libdsa.so
FMOD_LIB = libfmod.so
STUDIO_LIB = libfmodstudio.so
EXAMPLE = example

.PHONY: all clean test

all: $(LIB) $(FMOD_LIB) $(STUDIO_LIB) $(EXAMPLE)

%.o: %.c include/dsa.h include/dsa_internal.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(DSADBJ)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# libfmod.so is self-contained — includes DSA + FMOD compat + C++ ABI
# No runtime -ldsa dependency needed for LD_PRELOAD
# SONAME=libfmod.so.14 so GodotFmod's DT_NEEDED resolves
$(FMOD_LIB): $(DSADBJ) $(FMODOBJ)
	$(CC) -shared -Wl,-soname,$(FMOD_LIB).14 -o $@ $^ $(LDFLAGS)
	ln -sf $(FMOD_LIB) $(FMOD_LIB).14

# libfmodstudio.so depends on libfmod.so for DSA symbols
# No -ldsa needed — uses libfmod.so's DSA
$(STUDIO_LIB): $(STUDIOOBJ) $(FMOD_LIB)
	$(CC) -shared -Wl,-soname,$(STUDIO_LIB).14 -o $@ $< -L. -lfmod $(LDFLAGS)
	ln -sf $(STUDIO_LIB) $(STUDIO_LIB).14

$(EXAMPLE): examples/example.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -ldsa $(LDFLAGS)

test: all
	LD_LIBRARY_PATH=. ./$(EXAMPLE)

clean:
	rm -f src/*.o $(LIB) $(FMOD_LIB) $(STUDIO_LIB) $(FMOD_LIB).14 $(STUDIO_LIB).14 $(EXAMPLE)
