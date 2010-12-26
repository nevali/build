CFLAGS = -W -Wall -O0 -g -DENABLE_XCODEBUILD=1

OUT = build
OBJ = build.o nx_getopt_long.o context.o gnumake.o xcodebuild.o

$(OUT): $(OBJ)

clean:
	rm -f $(OUT) $(OBJ)

