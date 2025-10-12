.PHONY        := cleanup
CC            := gcc
STD           := -std=c99
INCLUDES      :=
FLAGS         := -Wall -Wextra -Wpedantic -Wconversion -Wstrict-overflow=5 -Wshadow -Wunused-macros -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wdangling-else -Wlogical-op -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Winline -Wno-unknown-warning-option
SOURCE        := *.c

static: $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -D NDEBUG -O3 -c $(SOURCE)
	ar rcs cjson.a *.o
	make cleanup

test1.exe: ./tests/test1.c $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -g -Og -o test1 ./tests/test1.c $(SOURCE)

test2_debug.exe: ./tests/test2.c $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -D COUNT_ALLOCATIONS -D BENCHMARK -g -Og -o test2_debug ./tests/test2.c $(SOURCE)

test2_release.exe: ./tests/test2.c $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -D NDEBUG -O3 -o test2_release ./tests/test2.c $(SOURCE)

cleanup:
	rm *.o
