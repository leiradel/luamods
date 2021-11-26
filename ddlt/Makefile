ifeq ($(shell uname -s),)
    TARGET=ddlt.dll
else ifneq ($(findstring MINGW,$(shell uname -a)),)
    TARGET=ddlt.dll
else ifneq ($(findstring MSYS,$(shell uname -a)),)
    TARGET=ddlt.dll
else ifneq ($(findstring win,$(shell uname -a)),)
    TARGET=ddlt.dll
else ifneq ($(findstring Darwin,$(shell uname -a)),)
    TARGET=ddlt.dylib
else
    TARGET=ddlt.so
endif

%.o: %.c
	gcc -Isrc -O2 -Werror -Wall -Wpedantic -fPIC $(CFLAGS) `pkg-config lua --cflags` -c $< -o $@

all: $(TARGET)

$(TARGET): src/ddlt.o src/lexer.o src/path.o src/templ.o
	gcc -shared -fPIC -o $@ $+ -lm $(LDFLAGS) `pkg-config lua --libs`

src/ddlt.o: src/ddlt.c src/lexer.h src/path.h src/templ.h src/boot_lua.h

src/lexer.o: src/lexer.c src/lexer.h src/lexer_cpp.c src/lexer_bas.c src/lexer_pas.c

src/path.o: src/path.c src/path.h src/realpath.c

src/templ.o: src/templ.c src/templ.h

src/boot_lua.h: src/boot.lua
	echo -e "static char const `basename "$<" | sed 's/\./_/'`[] = {\n`cat "$<" | xxd -i`\n};" > "$@"

clean:
	rm -f $(TARGET) src/ddlt.o src/lexer.o src/path.o src/templ.o src/boot_lua.h

.PHONY: clean
