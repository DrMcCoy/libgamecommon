lib_LTLIBRARIES = libgamecommon.la

libgamecommon_la_SOURCES = iostream_helpers.cpp
libgamecommon_la_SOURCES += bitstream.cpp
libgamecommon_la_SOURCES += error.cpp
libgamecommon_la_SOURCES += lzw.cpp
libgamecommon_la_SOURCES += filter.cpp
libgamecommon_la_SOURCES += filter_dummy.cpp
libgamecommon_la_SOURCES += iff.cpp
libgamecommon_la_SOURCES += metadata.cpp
libgamecommon_la_SOURCES += stream.cpp
libgamecommon_la_SOURCES += stream_file.cpp
libgamecommon_la_SOURCES += stream_filtered.cpp
libgamecommon_la_SOURCES += stream_memory.cpp
libgamecommon_la_SOURCES += stream_seg.cpp
libgamecommon_la_SOURCES += stream_string.cpp
libgamecommon_la_SOURCES += stream_sub.cpp
libgamecommon_la_SOURCES += suppitem.cpp

WARNINGS = -Wall -Wextra -Wno-unused-parameter

AM_CPPFLAGS  = $(BOOST_CPPFLAGS)
AM_CPPFLAGS += -I $(top_srcdir)/include
AM_CPPFLAGS += $(WARNINGS)

AM_CXXFLAGS  = $(DEBUG_CXXFLAGS)

libgamecommon_la_LDFLAGS  = $(AM_LDFLAGS)
libgamecommon_la_LDFLAGS += -version-info 1:0:0

libgamecommon_la_LIBADD = $(BOOST_SYSTEM_LIBS)
