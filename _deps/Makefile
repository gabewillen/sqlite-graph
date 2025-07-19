# _deps/Makefile

.PHONY: all clean

all: ../build/libsqlite3.a ../build/libunity.a

../build/libsqlite3.a: ../build/obj/_deps/sqlite-src/sqlite3.o
	ar rcs $@ $^

../build/obj/_deps/sqlite-src/sqlite3.o:
	mkdir -p ../build/obj/_deps/sqlite-src
	$(CC) $(CFLAGS) -fPIC -Wno-implicit-fallthrough -c sqlite-src/sqlite3.c -o $@

../build/libunity.a: Unity-2.5.2/src/unity.o
	ar rcs $@ $^

Unity-2.5.2/src/unity.o:
	$(CC) $(CFLAGS) -c Unity-2.5.2/src/unity.c -o $@

clean:
	rm -f ../build/obj/_deps/sqlite-src/sqlite3.o Unity-2.5.2/src/unity.o ../build/libunity.a ../build/libsqlite3.a
