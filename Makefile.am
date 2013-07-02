## Copyright 2013 Mo McRoberts.
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

DIST_SUBDIRS = m4

SUBDIRS = .

EXTRA_DIST = LICENSE-2.0

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = build

build_SOURCES = p_build.h \
	build.c nx_getopt_long.c context.c \
	gnumake.c \
	xcodebuild.c \
	autoconf.c

build_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS) -I$(srcdir)/uthash-1.9.3/src
