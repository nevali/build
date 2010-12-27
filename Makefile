CFLAGS = -W -Wall -O0 -g -DENABLE_XCODEBUILD=1 -Iuthash-1.9.3/src

OUT = build
OBJ = build.o nx_getopt_long.o context.o gnumake.o xcodebuild.o autoconf.o

$(OUT): $(OBJ)

clean:
	rm -f $(OUT) $(OBJ)

