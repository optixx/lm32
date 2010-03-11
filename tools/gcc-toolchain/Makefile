# Makefile for building lm32 toolchain (newlib based)
#
# By Martin Ongsiek

# Target
TARGET = lm32-elf
PWD=  /home/david/Devel/arch/fpga/devkit/

# Directories
SOURCES_DIR := ${PWD}/sources
BUILD_DIR := ${PWD}/build
PATCHES_DIR := ${PWD}/patches
PREFIX := ${PWD}/${TARGET}

#HOSTINCLUDE := /opt/local/include
#HOSTLIB := /opt/local/lib

# Source versions

VERSION_BINUTILS = 2.20
VERSION_GCC = 4.5-20091112
VERSION_GDB = 7.0
VERSION_NEWLIB = 1.18.0

VERSION_GMP = 4.3.2
VERSION_MPFR = 2.4.2

# Tool flags
MKDIR_FLAGS = -p
RM_FLAGS = -rf
WGET_FLAGS = -nc
TAR_FLAGS_BZ2 = xjf
TAR_FLAGS_GZ = xzf
TAR_FLAGS_LZMA = xv --lzma -f
BUNZIP2_FLAGS = -kf

# Add toolchain to path
PATH := ${PREFIX}/bin:${PATH}

# Default target
all:   dirs download extract binutils gcc newlib gdb

dirs:
	mkdir ${MKDIR_FLAGS} ${SOURCES_DIR}
	mkdir ${MKDIR_FLAGS} ${BUILD_DIR}
	mkdir ${MKDIR_FLAGS} ${PREFIX}

	touch dirs

download: dirs
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} ftp://gcc.gnu.org/pub/binutils/releases/binutils-${VERSION_BINUTILS}.tar.bz2
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} ftp://ftp.irisa.fr/pub/mirrors/gcc.gnu.org/gcc/snapshots/${VERSION_GCC}/gcc-core-${VERSION_GCC}.tar.bz2
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} ftp://sourceware.org/pub/gdb/releases/gdb-${VERSION_GDB}.tar.bz2
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} ftp://sources.redhat.com/pub/newlib/newlib-${VERSION_NEWLIB}.tar.gz
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} ftp://ftp.gmplib.org/pub/gmp-${VERSION_GMP}/gmp-${VERSION_GMP}.tar.bz2
	wget ${WGET_FLAGS} -P ${SOURCES_DIR} http://www.mpfr.org/mpfr-current/mpfr-${VERSION_MPFR}.tar.bz2

	touch download

extract: download
	tar ${TAR_FLAGS_BZ2} ${SOURCES_DIR}/binutils-${VERSION_BINUTILS}.tar.bz2 -C ${BUILD_DIR}
	tar ${TAR_FLAGS_BZ2} ${SOURCES_DIR}/gdb-${VERSION_GDB}.tar.bz2 -C ${BUILD_DIR}
	tar ${TAR_FLAGS_GZ} ${SOURCES_DIR}/newlib-${VERSION_NEWLIB}.tar.gz -C ${BUILD_DIR}

	tar ${TAR_FLAGS_BZ2} ${SOURCES_DIR}/gcc-core-4.5-20091112.tar.bz2 -C ${BUILD_DIR}

	tar ${TAR_FLAGS_BZ2} ${SOURCES_DIR}/gmp-${VERSION_GMP}.tar.bz2 -C ${BUILD_DIR}
	mv ${BUILD_DIR}/gmp-${VERSION_GMP} ${BUILD_DIR}/gcc-${VERSION_GCC}/gmp
	tar ${TAR_FLAGS_BZ2} ${SOURCES_DIR}/mpfr-${VERSION_MPFR}.tar.bz2 -C ${BUILD_DIR}
	mv ${BUILD_DIR}/mpfr-${VERSION_MPFR} ${BUILD_DIR}/gcc-${VERSION_GCC}/mpfr

	touch extract

binutils: extract
	mkdir ${BUILD_DIR}/build-binutils; \
	cd ${BUILD_DIR}/build-binutils; \
	../binutils-${VERSION_BINUTILS}/configure --target=${TARGET} --prefix=${PREFIX} \
	--with-gcc \
	--with-gnu-as \
	--with-gnu-ld \
	--with-dwarf2 --disable-werror  \
	--disable-nls; \
	make all-bfd TARGET-bfd=headers; \
	rm bfd/Makefile; \
	make configure-bfd; \
	make; \
	make install; \
	cd ../../
	touch binutils

# Note: --disable-werror is added to avoid compile break by warning (eg. ubuntu gcc is too restrictive)

# Bootstrap gcc (C only compiler)
gcc: binutils
	mkdir ${BUILD_DIR}/build-gcc; \
	cd ${BUILD_DIR}/build-gcc; \
	../gcc-${VERSION_GCC}/configure --target=${TARGET} --prefix=${PREFIX} \
	--enable-languages=c \
	--disable-libssp \
	--with-newlib; \
	make; \
	make install; \
	cd ../../
	touch gcc

newlib: gcc
	mkdir ${BUILD_DIR}/build-newlib; \
	cd ${BUILD_DIR}/build-newlib; \
	../newlib-${VERSION_NEWLIB}/configure --target=${TARGET} --prefix=${PREFIX}; \
	make; \
	make install; \
	cd ../../
	touch newlib

gdb:
	mkdir ${BUILD_DIR}/build-gdb; \
	cd ${BUILD_DIR}/build-gdb; \
	../gdb-${VERSION_GDB}/configure --target=${TARGET} --prefix=${PREFIX} \
	--disable-nls --disable-werror; \
	make all-bfd TARGET-bfd=headers; 
	make; \
	make install; \
	cd ../../
	touch gdb

# Fails on missing target bfd support! (configure: error: *** unknown target vector bfd_elf32_avr32_vec)

distclean: clean
	rm ${RM_FLAGS} ${SOURCES_DIR}

clean:
	rm ${RM_FLAGS} ${BUILD_DIR}
	rm ${RM_FLAGS} ${PATCHES_DIR}

.PHONY: all
