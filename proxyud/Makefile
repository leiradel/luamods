ifeq ($(shell uname -s),)
    TARGET=proxyud.dll
else ifneq ($(findstring MINGW,$(shell uname -a)),)
    TARGET=proxyud.dll
else ifneq ($(findstring MSYS,$(shell uname -a)),)
    TARGET=proxyud.dll
else ifneq ($(findstring win,$(shell uname -a)),)
    TARGET=proxyud.dll
else ifneq ($(findstring Darwin,$(shell uname -a)),)
    TARGET=proxyud.dylib
else
    TARGET=proxyud.so
endif

%.o: %.c
	gcc -Isrc -O2 -Werror -Wall -Wpedantic -fPIC $(CFLAGS) `pkg-config lua --cflags` -c $< -o $@

all: $(TARGET)

$(TARGET): src/proxyud.o
	gcc -shared -fPIC -o $@ $+ -lm $(LDFLAGS) `pkg-config lua --libs`

src/proxyud.o: src/proxyud.c src/proxyud.h

src/proxyud.h: src/proxyud.lua
	echo "static char const `basename "$<" | sed 's/\./_/'`[] = {`cat "$<" | xxd -i`};" > "$@"

clean:
	rm -f $(TARGET) src/proxyud.o src/proxyud.h

.PHONY: clean
