AC_INIT([upcxx], [0.1], [yzheng@lbl.gov], [upcxx], [https://bitbucket.org/upcxx/upcxx])
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])

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
  scripts/upc++
  examples/Makefile
  examples/basic/Makefile
  examples/cg/Makefile
  examples/gups/Makefile
  examples/knapsack/Makefile
  examples/mg/Makefile
  examples/samplesort/Makefile
  examples/spmv/Makefile
  examples/stencil/Makefile
])

AC_PROG_CXX
AC_PROG_CC

AC_ARG_WITH([gasnet],
  [AS_HELP_STRING([--with-gasnet], [path to gasnet conduit-specific makefile])],
  [
    if test -f "$withval" ; then
      cp $withval .
      gasnet_makefile=$(basename $withval)
      chmod o+w $gasnet_makefile
      sed -i.BAK 's/-Wl,--whole-archive,-lhugetlbfs,--no-whole-archive/-lhugetlbfs/' $gasnet_makefile
      AC_SUBST([GASNET_MAKEFILE], ["$gasnet_makefile"])
      dnl AC_SUBST([GASNET_MAKEFILE], ["$withval"])
    else    
      AC_MSG_FAILURE([Could not find GASNet Makefile.])
    fi
  ],
  [AC_MSG_FAILURE([No GASNet Makefile specified. Re-configure with --with-gasnet=$gasnet_install_path/mpi-conduit/mpi-seq.mak or $bupc_install_path/opt/include/smp-conduit/mpi-seq.mak])]
)

AC_LANG(C++)

dnl option to disable using C++11  (default is enable auto detection)
AC_ARG_ENABLE([cxx11],
    AS_HELP_STRING([--disable-cxx11], [Disable C++11 features]))

AS_IF([test "x$enable_cxx11" = "xno"],
      [:], 
      [ dnl Do the stuff needed for enabling C++11
        dnl Check for C++11   
        AX_CXX_COMPILE_STDCXX_11(noext,optional)
        if test "$HAVE_CXX11" = 1; then
          AC_DEFINE(UPCXX_HAVE_CXX11, 1, [define if the C++11 features are enabled])
          AC_SUBST(UPCXX_HAVE_CXX11)
          AC_SUBST(CXX11FLAG)
          dnl Check for support for is_trivially_destructible
          AC_MSG_CHECKING([whether $CXX supports std::is_trivially_destructible])
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                #include <type_traits>
                bool test = std::is_trivially_destructible<double>::value;
                ])], [TRIVIALDESTRUCT=yes], [TRIVIALDESTRUCT=no])
          AC_MSG_RESULT([$TRIVIALDESTRUCT])
          if test "x$TRIVIALDESTRUCT" = "xyes"; then
            AC_DEFINE(UPCXX_HAVE_TRIVIALLY_DESTRUCTIBLE, 1, [define if libcxx implements std::is_trivially_destructible])
          fi
          dnl Check for support for initializer lists
          AC_MSG_CHECKING([whether $CXX supports std::initializer_list])
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                #include <initializer_list>
                void test(std::initializer_list<double> &ilist) {}
                ])], [INITIALIZERLIST=yes], [INITIALIZERLIST=no])
          AC_MSG_RESULT([$INITIALIZERLIST])
          if test "x$INITIALIZERLIST" = "xyes"; then
            AC_DEFINE(UPCXX_HAVE_INITIALIZER_LIST, 1, [define if libcxx implements std::initializer_list])
          fi
        fi
      ])

dnl Check for Clang
AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])], [CLANG=yes], [CLANG=no])
AC_MSG_RESULT([$CLANG])
dnl for clang++
if test "x$CLANG" = "xyes"; then
  ## suppress warnings due to legacy usage of C in GASNet
  UPCXX_FLAGS+=" -Wno-reserved-user-defined-literal -Wno-deprecated-register"
fi

dnl Check for GCC
AC_MSG_CHECKING([if compiling with g++])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#if !defined(__GNUG__) || defined(__INTEL_COMPILER) || defined(__clang__)
       not g++
#endif
]])], [GCC=yes], [GCC=no])
AC_MSG_RESULT([$GCC])
dnl for g++
if test "x$GCC" = "xyes"; then
  ## suppress warnings due to legacy usage of C in GASNet
  UPCXX_FLAGS+=" -Wno-literal-suffix -DGASNETT_USE_GCC_ATTRIBUTE_MAYALIAS=1 "
fi

dnl Add flags for building UPC++
UPCXX_FLAGS+=" -DGASNET_ALLOW_OPTIMIZED_DEBUG "
AC_SUBST(UPCXX_FLAGS)

CFLAGS+=${UPCXX_FLAGS}
CXXFLAGS+=${UPCXX_FLAGS}

dnl Option to enable shorter UPC++ macro names (default is disable)
AC_ARG_ENABLE([short-names],
    AS_HELP_STRING([--enable-short-names], [Enable shorter names for common UPC++ keywords]))

AS_IF([test "x$enable_short_names" = "xyes"], [
  dnl Do the stuff needed for enabling md-array
  AC_DEFINE(UPCXX_SHORT_MACROS, 1, [define if short-names feature is enabled])
  AC_SUBST(UPCXX_SHORT_MACROS)
])

dnl Option to enable UPC++ multidimensional arrays (default is disable)
AC_ARG_ENABLE([md-array],
    AS_HELP_STRING([--enable-md-array], [Enable Titanium-style multidimensional arrays]))

AS_IF([test "x$enable_md_array" = "xyes"], [
  dnl Do the stuff needed for enabling md-array
  AC_DEFINE(UPCXX_HAVE_MD_ARRAY, 1, [define if md-array feature is enabled])
  AC_SUBST(UPCXX_HAVE_MD_ARRAY)
])

AM_CONDITIONAL([UPCXX_MD_ARRAY], [test "x$enable_md_array" = "xyes"])

dnl Option to enable using the Cray DMAPP API on Aries network (default is disable)
AC_ARG_ENABLE([dmapp],
    AS_HELP_STRING([--enable-dmapp], [Enable using DMAPP on Cray Aries network]))

AS_IF([test "x$enable_dmapp" = "xyes"], [
  dnl Do the stuff needed for enabling dmapp
  AC_DEFINE(UPCXX_USE_DMAPP, 1, [define if dmapp feature is enabled])
  AC_SUBST(UPCXX_USE_DMAPP)
])

AM_CONDITIONAL([UPCXX_USE_DMAPP], [test "x$enable_dmapp" = "xyes"])

// UPCXX_THREAD_SAFE
dnl Option to enable UPC++ thread safety (default is disable)
AC_ARG_ENABLE([thread-safe],
    AS_HELP_STRING([--enable-thread-safe], [Enable thread safety for the UPC++ runtime]))

AS_IF([test "x$enable_thread_safe" = "xyes"], [
  dnl Do the stuff needed for enabling thread safety
  AC_DEFINE(UPCXX_THREAD_SAFE, 1, [define if thread safety is enabled])
  AC_SUBST(UPCXX_THREAD_SAFE)
])

AM_CONDITIONAL([UPCXX_THREAD_SAFE], [test "x$enable_thread_safe" = "xyes"])

AC_OUTPUT
