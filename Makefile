#   Copyright (C) 2013  Povilas Kanapickas <tir5c3@yahoo.co.uk>
#
#   This file is part of libbitwiddle
#
#   Distributed under the Boost Software License, Version 1.0. (See accompanying
#   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

SHELL := /bin/bash

#Common prefixes

prefix = /usr
includedir = $(prefix)/include

#Version

VERSION=0.9

#STANDARD RULES

all: check

DISTFILES=	\
	libbittwiddle/libbittwiddle.h	\
	test/test.cc					\
	Makefile						\
	README

CLEANFILES= \
    test/test	\
    test/test.o


CXXFLAGS=-I. -std=c++11
LDFLAGS=-lboost_unit_test_framework

clean:
	rm -rf $(CLEANFILES)

check: test/test
	./test/test

test/test: test/test.o
	$(CXX) $^ $(LDFLAGS) -o $@

test/test.o: test/test.cc
	$(CXX) $^ -c $(CXXFLAGS) -o $@

dist: clean
	mkdir -p "libbittwiddle-$(VERSION)"
	cp -r $(DISTFILES) "libbittwiddle-$(VERSION)"
	tar czf "libbittwiddle-$(VERSION).tar.gz" "libbittwiddle-$(VERSION)"
	rm -rf "libbittwiddle-$(VERSION)"

install:
	install -DT -m 644 libbittwiddle/libbittwiddle.h \
		"$(DESTDIR)$(includedir)/libbittwiddle/libbittwiddle.h"

uninstall:
	rm -rf "$(DESTDIR)$(includedir)/libbittwiddle/libbittwiddle.h"

