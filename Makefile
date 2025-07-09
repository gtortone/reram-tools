CPP=g++

# Include headers files
INCLUDE_DIRS = include
INCLUDE = $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))

CFLAGS = -Wall -O3 -Wno-unused-result -std=c++17
LDFLAGS = -L/usr/lib

all: bin/rrmain

OBJS_DIR = obj

SRC_MAIN=src/rrmain.cpp src/mb85as12mt.cpp src/argparse.cpp src/utils.cpp
OBJ_MAIN=$(SRC_MAIN:.cpp=.o)

# use DEBUG=1 to include debugging
ifdef DEBUG
  CFLAGS += -g	
endif

MKDIR = mkdir -p

bin/rrmain: $(OBJ_MAIN)
	@$(MKDIR) bin
	$(CPP) -o $@ $(CFLAGS) -I$(INCLUDE_DIRS) $(LDFLAGS) $^
	ln -sr  bin/rrmain bin/rrdump 
	ln -sr  bin/rrmain bin/rrinfo 
	ln -sr  bin/rrmain bin/rrfill 
	ln -sr  bin/rrmain bin/rrread 
	ln -sr  bin/rrmain bin/rrwrite

%.o: %.cpp
	$(CPP) -c -o $@ -I$(INCLUDE_DIRS) $(CFLAGS)  $^

clean:
	rm -rf $(OBJ_MAIN)
	rm -rf bin/rrmain
	rm -rf bin/rrdump
	rm -rf bin/rrinfo
	rm -rf bin/rrfill
	rm -rf bin/rrread
	rm -rf bin/rrwrite
