// This file was GENERATED by command:
//     pump.py async_templates.h.pump
// DO NOT EDIT BY HAND!!!


/**
 * \internal
 * aysnc_templates.h - templates for asynchronous tasks
 *
 * Assumptions:
 *   1) up to 16 user function arguments.
 *   2) function arguments are passed by values!
 * \endinternal
 */

/// \cond SHOW_INTERNAL

#pragma once

namespace upcxx
{
  /*************************************/
  /* Active Message argument templates */
  /*************************************/


  template <typename Function>
  struct generic_arg0 {
    Function kernel;


    generic_arg0(Function k) :
      kernel(k) { }
  };

  template <typename Function, typename T1>
  struct generic_arg1 {
    Function kernel;
    T1 arg1;

    generic_arg1(Function k, T1 a1) :
      kernel(k), arg1(a1) { }
  };

  template <typename Function, typename T1, typename T2>
  struct generic_arg2 {
    Function kernel;
    T1 arg1;
    T2 arg2;

    generic_arg2(Function k, T1 a1, T2 a2) :
      kernel(k), arg1(a1), arg2(a2) { }
  };

  template <typename Function, typename T1, typename T2, typename T3>
  struct generic_arg3 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;

    generic_arg3(Function k, T1 a1, T2 a2, T3 a3) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4>
  struct generic_arg4 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;

    generic_arg4(Function k, T1 a1, T2 a2, T3 a3, T4 a4) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5>
  struct generic_arg5 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;

    generic_arg5(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6>
  struct generic_arg6 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;

    generic_arg6(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7>
  struct generic_arg7 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;

    generic_arg7(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8>
  struct generic_arg8 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;

    generic_arg8(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9>
  struct generic_arg9 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;

    generic_arg9(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10>
  struct generic_arg10 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;

    generic_arg10(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11>
  struct generic_arg11 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;

    generic_arg11(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12>
  struct generic_arg12 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;
    T12 arg12;

    generic_arg12(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11, T12 a12) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11), arg12(a12) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13>
  struct generic_arg13 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;
    T12 arg12;
    T13 arg13;

    generic_arg13(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11, T12 a12, T13 a13) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11), arg12(a12),
          arg13(a13) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14>
  struct generic_arg14 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;
    T12 arg12;
    T13 arg13;
    T14 arg14;

    generic_arg14(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11, T12 a12, T13 a13, T14 a14) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11), arg12(a12),
          arg13(a13), arg14(a14) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14, typename T15>
  struct generic_arg15 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;
    T12 arg12;
    T13 arg13;
    T14 arg14;
    T15 arg15;

    generic_arg15(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11, T12 a12, T13 a13, T14 a14, T15 a15) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11), arg12(a12),
          arg13(a13), arg14(a14), arg15(a15) { }
  };

  template <typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14, typename T15, typename T16>
  struct generic_arg16 {
    Function kernel;
    T1 arg1;
    T2 arg2;
    T3 arg3;
    T4 arg4;
    T5 arg5;
    T6 arg6;
    T7 arg7;
    T8 arg8;
    T9 arg9;
    T10 arg10;
    T11 arg11;
    T12 arg12;
    T13 arg13;
    T14 arg14;
    T15 arg15;
    T16 arg16;

    generic_arg16(Function k, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7,
        T8 a8, T9 a9, T10 a10, T11 a11, T12 a12, T13 a13, T14 a14, T15 a15,
        T16 a16) :
      kernel(k), arg1(a1), arg2(a2), arg3(a3), arg4(a4), arg5(a5), arg6(a6),
          arg7(a7), arg8(a8), arg9(a9), arg10(a10), arg11(a11), arg12(a12),
          arg13(a13), arg14(a14), arg15(a15), arg16(a16) { }
  };

  /************************************/
  /* Active Message wrapper functions */
  /************************************/


  template<typename Function>
  void async_wrapper0(void *args)
  {
    generic_arg0<Function> *a =
      (generic_arg0<Function> *)args;

    a->kernel();
  }

  template<typename Function, typename T1>
  void async_wrapper1(void *args)
  {
    generic_arg1<Function, T1> *a =
      (generic_arg1<Function, T1> *)args;

    a->kernel(a->arg1);
  }

  template<typename Function, typename T1, typename T2>
  void async_wrapper2(void *args)
  {
    generic_arg2<Function, T1, T2> *a =
      (generic_arg2<Function, T1, T2> *)args;

    a->kernel(a->arg1, a->arg2);
  }

  template<typename Function, typename T1, typename T2, typename T3>
  void async_wrapper3(void *args)
  {
    generic_arg3<Function, T1, T2, T3> *a =
      (generic_arg3<Function, T1, T2, T3> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4>
  void async_wrapper4(void *args)
  {
    generic_arg4<Function, T1, T2, T3, T4> *a =
      (generic_arg4<Function, T1, T2, T3, T4> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5>
  void async_wrapper5(void *args)
  {
    generic_arg5<Function, T1, T2, T3, T4, T5> *a =
      (generic_arg5<Function, T1, T2, T3, T4, T5> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6>
  void async_wrapper6(void *args)
  {
    generic_arg6<Function, T1, T2, T3, T4, T5, T6> *a =
      (generic_arg6<Function, T1, T2, T3, T4, T5, T6> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7>
  void async_wrapper7(void *args)
  {
    generic_arg7<Function, T1, T2, T3, T4, T5, T6, T7> *a =
      (generic_arg7<Function, T1, T2, T3, T4, T5, T6, T7> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8>
  void async_wrapper8(void *args)
  {
    generic_arg8<Function, T1, T2, T3, T4, T5, T6, T7, T8> *a =
      (generic_arg8<Function, T1, T2, T3, T4, T5, T6, T7, T8> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9>
  void async_wrapper9(void *args)
  {
    generic_arg9<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9> *a =
      (generic_arg9<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10>
  void async_wrapper10(void *args)
  {
    generic_arg10<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> *a =
      (generic_arg10<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11>
  void async_wrapper11(void *args)
  {
    generic_arg11<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> *a =
      (generic_arg11<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10,
          T11> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12>
  void async_wrapper12(void *args)
  {
    generic_arg12<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
        T12> *a =
      (generic_arg12<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
          T12> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11, a->arg12);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13>
  void async_wrapper13(void *args)
  {
    generic_arg13<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12,
        T13> *a =
      (generic_arg13<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
          T12, T13> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11, a->arg12, a->arg13);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14>
  void async_wrapper14(void *args)
  {
    generic_arg14<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12,
        T13, T14> *a =
      (generic_arg14<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
          T12, T13, T14> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11, a->arg12, a->arg13, a->arg14);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14, typename T15>
  void async_wrapper15(void *args)
  {
    generic_arg15<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12,
        T13, T14, T15> *a =
      (generic_arg15<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
          T12, T13, T14, T15> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11, a->arg12, a->arg13, a->arg14,
        a->arg15);
  }

  template<typename Function, typename T1, typename T2, typename T3,
      typename T4, typename T5, typename T6, typename T7, typename T8,
      typename T9, typename T10, typename T11, typename T12, typename T13,
      typename T14, typename T15, typename T16>
  void async_wrapper16(void *args)
  {
    generic_arg16<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12,
        T13, T14, T15, T16> *a =
      (generic_arg16<Function, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
          T12, T13, T14, T15, T16> *)args;

    a->kernel(a->arg1, a->arg2, a->arg3, a->arg4, a->arg5, a->arg6, a->arg7,
        a->arg8, a->arg9, a->arg10, a->arg11, a->arg12, a->arg13, a->arg14,
        a->arg15, a->arg16);
  }

} // end of namespace upcxx
