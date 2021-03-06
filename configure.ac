#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libgamecommon], [1.2],
	[https://github.com/Malvineous/libgamecommon/issues],
	[], [http://www.shikadi.net/camoto])
AM_INIT_AUTOMAKE([foreign dist-bzip2 no-dist-gzip])
AC_CONFIG_SRCDIR([src/stream.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C++)

# Overarching version of all libraries (used for installation dir)
AC_SUBST(camoto_release, camoto-1.0)

AC_PROG_CXX
AC_PROG_LIBTOOL

BOOST_REQUIRE([1.46])
BOOST_FILESYSTEM
BOOST_PROGRAM_OPTIONS
BOOST_TEST

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],[enable extra debugging output]))

dnl Check for --enable-debug and add appropriate flags for gcc
if test "x$enable_debug" = "xyes";
then
	# Add gdb info (-g), disable any default optimisation
	AC_SUBST(DEBUG_CXXFLAGS, "-O0 -g")
	# Add DEBUG define
	AC_DEFINE([DEBUG], [1], [Define to include extra debugging output])
fi

case "$host_os" in
*mingw*)
	# Must specify this parameter when compiling for Windows
	LDFLAGS="$LDFLAGS -no-undefined"
	;;
esac

AM_SILENT_RULES([yes])

AC_OUTPUT(Makefile src/Makefile include/Makefile include/camoto/Makefile tests/Makefile $PACKAGE.pc)
