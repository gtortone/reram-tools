CC=gcc

# Include headers files
INCLUDE_DIRS = include
INCLUDE = $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))

CFLAGS = -Wall -O3 -Wno-unused-result
LDFLAGS = -O3 -L/usr/lib

all: bin/rrtest bin/rrcycle

OBJS_DIR = obj

SRC_TEST=src/rrtest.c src/reram.c
OBJ_TEST=$(SRC_TEST:.c=.o)

SRC_CYCLE=src/rrcycle.c src/reram.c
OBJ_CYCLE=$(SRC_CYCLE:.c=.o)

# use DEBUG=1 to include debugging
ifdef DEBUG
  CFLAGS += -g	
endif

MKDIR = mkdir -p

bin/rrtest: $(OBJ_TEST)
	@$(MKDIR) bin
	$(CC) -o $@ $(CFLAGS) -I$(INCLUDE_DIRS) $(LDFLAGS) $^

bin/rrcycle: $(OBJ_CYCLE)
	@$(MKDIR) bin
	$(CC) -o $@ $(CFLAGS) -I$(INCLUDE_DIRS) $(LDFLAGS) $^

%.o: %.c
	$(CC) -c -o $@ -I$(INCLUDE_DIRS) $(CFLAGS)  $^

clean:
	rm -rf $(OBJ_TEST) $(OBJ_CYCLE) 
	rm -rf bin/rrtest bin/rrcycle 
