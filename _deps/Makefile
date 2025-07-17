# _deps/Makefile

.PHONY: all clean

all: ../build/libsqlite3_lib.a ../build/libunity.a

../build/libsqlite3_lib.a: sqlite-src/sqlite3.o
	mkdir -p ../build
	ar rcs $@ $^

../build/libunity.a: Unity-2.5.2/src/unity.o
	ar rcs $@ $^

sqlite-src/sqlite3.o:
	$(CC) $(CFLAGS) -Wno-implicit-fallthrough -c sqlite-src/sqlite3.c -o $@

Unity-2.5.2/src/unity.o:
	$(CC) $(CFLAGS) -c Unity-2.5.2/src/unity.c -o $@

clean:
	rm -f sqlite-src/sqlite3.o Unity-2.5.2/src/unity.o ../build/libsqlite3_lib.a ../build/libunity.a
