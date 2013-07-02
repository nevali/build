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

AC_INIT([build],[master])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])

BT_PROG_XCODE
AC_PROG_CC([clang gcc c99 cc])
AC_PROG_CC_C99
AC_HEADER_STDC

BT_PROG_CC_WARN

AC_CHECK_PROG([XCODEBUILD],[xcodebuild],[xcodebuild])

if ! test -z "$XCODEBUILD" ; then
	AC_DEFINE_UNQUOTED([ENABLE_XCODEBUILD], [1], [Define to enable the xcodebuild handler])
fi

AC_CONFIG_FILES([Makefile
m4/Makefile
])

AC_OUTPUT
