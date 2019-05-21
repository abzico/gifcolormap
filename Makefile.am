ACLOCAL_AMFLAGS = -I m4

# cli program
bin_PROGRAMS = gifcolormap
gifcolormap_CFLAGS = -Wall \
										-std=c99 \
										-O2 \
										-pedantic
gifcolormap_LDFLAGS = -lgif
gifcolormap_SOURCES = src/gifcolormap.c

EXTRA_DIST = README.md LICENSE autogen.sh
