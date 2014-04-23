AC_INIT([upcxx], [0.1], [yzheng@lbl.gov], [upcxx], [https://bitbucket.org/upcxx/upcxx])
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([1.10 no-define foreign])

LT_INIT([disable-shared])
dnl LT_INIT()
LT_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h) dnl internal config header 
AC_CONFIG_HEADERS(upcxx_config.h) dnl external config header
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  include/upcxx.mak
  src/Makefile
  examples/Makefile
  examples/basic/Makefile
  examples/gups/Makefile
  examples/samplesort/Makefile
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

AC_LANG(C++)

dnl option to disable using C++11  (default is enable auto detection)
AC_ARG_ENABLE([cxx11],
    AS_HELP_STRING([--disable-cxx11], [Disable C++11 features]))

AS_IF([test x$enableval = xno], 
      [:], 
      [ dnl Do the stuff needed for enabling C++11
        dnl Check for C++11   
        AX_CXX_COMPILE_STDCXX_11(noext,optional)
        if test "$HAVE_CXX11" = 1; then
          AC_DEFINE(UPCXX_HAVE_CXX11, 1, [define if the C++11 features are enabled])
          AC_SUBST(UPCXX_HAVE_CXX11)
          CXXFLAGS+=" -DUSE_CXX11 "
        fi
      ])

dnl Add flags for building UPC++
UPCXX_FLAGS=" -DGASNET_SEQ -DUSE_GASNET_FAST_SEGMENT -DONLY_MSPACES "
CFLAGS+=${UPCXX_FLAGS}
CXXFLAGS+=${UPCXX_FLAGS}

dnl Check for Clang
AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])], [CLANG=yes], [CLANG=no])
AC_MSG_RESULT([$CLANG])
## for clang
if test "x$CLANG" = "xyes"; then
  ## suppress warnings due to legacy usage of C in GASNet
  CXXFLAGS+=" -Wno-reserved-user-defined-literal -Wno-deprecated-register"
fi

dnl Option to enable UPC++ multidimensional arrays (default is disable)
AC_ARG_ENABLE([md-array],
    AS_HELP_STRING([--enable-md-array], [Enable Titanium-style multidimensional arrays]))

AS_IF([test x$enableval = xyes], [
  dnl Do the stuff needed for enabling md-array
  AC_DEFINE(UPCXX_HAVE_MD_ARRAY, 1, [define if the md-array feature is enabled])
  AC_SUBST(UPCXX_HAVE_MD_ARRAY)
])

AC_OUTPUT
