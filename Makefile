INSDUMP_CFLAGS= \
	-ansi \
	-pedantic \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-O2

INSDUMP_LIBS= \
	-lopcodes

INSDUMP_MACROS= \
	-D_POSIX_SOURCE=1 \
	-D_POSIX_C_SOURCE=200809L \
	-DPACKAGE="insdump" \
	-DPACKAGE_VERSION="0.1"

all: insdump

insdump: insdump.c
	@echo "CC" $<
	@$(CC) $(INSDUMP_CFLAGS) $(INSDUMP_MACROS) -o $@ $< $(INSDUMP_LIBS)

clean:
	rm -rf insdump