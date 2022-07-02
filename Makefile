.POSIX:

CFLAGS = -std=c99 -D_XOPEN_SOURCE=700 -Wall -Wextra -Wpedantic
BIN = aardvarc
OBJ = $(BIN).o

$(BIN): $(OBJ)

clean:
	rm -f $(BIN) $(OBJ)

