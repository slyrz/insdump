CFLAGS ?= \
	-ansi \
	-pedantic \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-O2

CFLAGS += \
	-D_POSIX_SOURCE=1 \
	-D_POSIX_C_SOURCE=200809L \
	-DPACKAGE="insdump" \
	-DPACKAGE_VERSION="0.1"

LIBS= \
	-lopcodes

all: insdump

insdump: insdump.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

install: insdump
	install -d "${DESTDIR}${PREFIX}/bin"
	install -t "${DESTDIR}${PREFIX}/bin" $<

clean:
	rm -rf insdump
