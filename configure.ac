AC_INIT([upcxx], [0.1], [yzheng@lbl.gov], [upcxx], [https://bitbucket.org/upcxx/upcxx])
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([1.10 no-define])

LT_INIT()
LT_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  src/Makefile
  examples/Makefile
  examples/basic/Makefile
  examples/gups/Makefile
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
dnl AC_LANG(C++)
dnl AX_CHECK_COMPILE_FLAG([-std=c++0x], [
dnl                         CXXFLAGS="$CXXFLAGS -std=c++0x"])

## Add flags for building UPC++
UPCXX_FLAGS=" -DGASNET_SEQ -DUSE_GASNET_FAST_SEGMENT -DONLY_MSPACES "
CFLAGS+=${UPCXX_FLAGS}
CXXFLAGS+=${UPCXX_FLAGS}

AC_OUTPUT
