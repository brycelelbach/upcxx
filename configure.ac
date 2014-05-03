AC_INIT([upcxx], [0.1], [yzheng@lbl.gov], [upcxx], [https://bitbucket.org/upcxx/upcxx])
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([1.10 no-define foreign])

LT_INIT([disable-shared])
dnl LT_INIT()
LT_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  include/upcxx.mak
  src/Makefile
  examples/Makefile
  examples/basic/Makefile
  examples/cg/Makefile
  examples/gsrb/Makefile
  examples/gups/Makefile
  examples/knapsack/Makefile
  examples/mg/Makefile
  examples/pps/Makefile
  examples/samplesort/Makefile
  examples/stencil/Makefile
])

AC_PROG_CXX
AC_PROG_CC

AC_ARG_WITH([gasnet],
  [AS_HELP_STRING([--with-gasnet],
    [path to gasnet conduit-specific makefile])],
  [AC_CHECK_FILE(["$withval"],
    AC_SUBST([GASNET_MAKEFILE], ["$withval"]),
    AC_MSG_FAILURE([Could not find GASNet Makefile.]))],
  [AC_MSG_FAILURE([No GASNet Makefile specified. Re-configure with --with-gasnet=/opt/gasnet/smp-conduit/smp-seq.mak etc.])]
)

## Check for C++11
AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_11(noext,optional)

if test "$HAVE_CXX11" = 1; then
  CXXFLAGS+=" -DUSE_CXX11 "
fi

## Add flags for building UPC++
UPCXX_FLAGS=" -DGASNET_SEQ -DUSE_GASNET_FAST_SEGMENT -DONLY_MSPACES "
CFLAGS+=${UPCXX_FLAGS}
CXXFLAGS+=${UPCXX_FLAGS}

## Check for Clang
AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])],
[CLANG=yes], [CLANG=no])

AC_MSG_RESULT([$CLANG])

## -std=c++0x -DUSE_CXX11"

## for clang
if test "x$CLANG" = "xyes"; then
  ## suppress warnings due to legacy usage of C in GASNet
  CXXFLAGS+=" -Wno-reserved-user-defined-literal -Wno-deprecated-register"
fi


AC_OUTPUT
