CPP=g++

# Include headers files
INCLUDE_DIRS = include
INCLUDE = $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))

CFLAGS = -Wall -O3 -Wno-unused-result
LDFLAGS = -O3 -L/usr/lib

all: bin/rrtest 

OBJS_DIR = obj

SRC_TEST=src/rrtest.cpp src/mb85as12mt.cpp
OBJ_TEST=$(SRC_TEST:.cpp=.o)

# use DEBUG=1 to include debugging
ifdef DEBUG
  CFLAGS += -g	
endif

MKDIR = mkdir -p

bin/rrtest: $(OBJ_TEST)
	@$(MKDIR) bin
	$(CPP) -o $@ $(CFLAGS) -I$(INCLUDE_DIRS) $(LDFLAGS) $^

%.o: %.cpp
	$(CPP) -c -o $@ -I$(INCLUDE_DIRS) $(CFLAGS)  $^

clean:
	rm -rf $(OBJ_TEST)
	rm -rf bin/rrtest
