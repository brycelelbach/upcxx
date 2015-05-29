#include "Test.h"

#define minusArrayLength 10
#define plusArrayLength 10

void Test::populateRHSGrid(Grid &gridA, int startLevel) {
  Point<3> minusOnePoints[minusArrayLength];
  Point<3> plusOnePoints[plusArrayLength];

  switch(startLevel) {
  case 5: // class S
    minusOnePoints[0] = PT(0, 11, 2);
    minusOnePoints[1] = PT(13, 8, 17);
    minusOnePoints[2] = PT(5, 14, 0);
    minusOnePoints[3] = PT(4, 28, 15);
    minusOnePoints[4] = PT(12, 2, 1);
    minusOnePoints[5] = PT(5, 17, 8);
    minusOnePoints[6] = PT(20, 19, 11);
    minusOnePoints[7] = PT(26, 15, 31);
    minusOnePoints[8] = PT(8, 25, 22);
    minusOnePoints[9] = PT(7, 14, 26);

    plusOnePoints[0] = PT(7, 1, 20);
    plusOnePoints[1] = PT(19, 29, 31);
    plusOnePoints[2] = PT(2, 0, 3);
    plusOnePoints[3] = PT(4, 22, 3);
    plusOnePoints[4] = PT(1, 16, 21);
    plusOnePoints[5] = PT(21, 31, 6);
    plusOnePoints[6] = PT(12, 15, 12);
    plusOnePoints[7] = PT(30, 4, 25);
    plusOnePoints[8] = PT(28, 0, 28);
    plusOnePoints[9] = PT(17, 26, 17);
    break;
  case 6: // class W
    minusOnePoints[0] = PT(38, 60, 51);
    minusOnePoints[1] = PT(50, 15, 23);
    minusOnePoints[2] = PT(18, 45, 36);
    minusOnePoints[3] = PT(25, 14, 36);
    minusOnePoints[4] = PT(26, 25, 25);
    minusOnePoints[5] = PT(32, 37, 0);
    minusOnePoints[6] = PT(29, 62, 54);
    minusOnePoints[7] = PT(39, 49, 57);
    minusOnePoints[8] = PT(12, 29, 28);
    minusOnePoints[9] = PT(63, 46, 25);

    plusOnePoints[0] = PT(27, 32, 45);
    plusOnePoints[1] = PT(39, 0, 5);
    plusOnePoints[2] = PT(45, 23, 49);
    plusOnePoints[3] = PT(20, 32, 58);
    plusOnePoints[4] = PT(23, 47, 57);
    plusOnePoints[5] = PT(17, 43, 53);
    plusOnePoints[6] = PT(8, 16, 48);
    plusOnePoints[7] = PT(51, 46, 26);
    plusOnePoints[8] = PT(58, 19, 62);
    plusOnePoints[9] = PT(58, 15, 54);
    break;
  case 8: // class A and B
    minusOnePoints[0] = PT(221, 40, 238);
    minusOnePoints[1] = PT(152, 160, 34);
    minusOnePoints[2] = PT(80, 182, 253);
    minusOnePoints[3] = PT(248, 168, 155);
    minusOnePoints[4] = PT(197, 5, 201);
    minusOnePoints[5] = PT(90, 61, 203);
    minusOnePoints[6] = PT(15, 203, 30);
    minusOnePoints[7] = PT(99, 154, 57);
    minusOnePoints[8] = PT(100, 136, 110);
    minusOnePoints[9] = PT(209, 152, 96);

    plusOnePoints[0] = PT(52, 207, 38);
    plusOnePoints[1] = PT(241, 170, 12);
    plusOnePoints[2] = PT(201, 16, 196);
    plusOnePoints[3] = PT(200, 81, 207);
    plusOnePoints[4] = PT(113, 121, 205);
    plusOnePoints[5] = PT(210, 5, 246);
    plusOnePoints[6] = PT(43, 192, 232);
    plusOnePoints[7] = PT(174, 244, 162);
    plusOnePoints[8] = PT(3, 116, 173);
    plusOnePoints[9] = PT(55, 118, 165);
    break;
  case 9: // class C
    minusOnePoints[0] = PT(397, 310, 198);
    minusOnePoints[1] = PT(94, 399, 236);
    minusOnePoints[2] = PT(221, 276, 59);
    minusOnePoints[3] = PT(342, 137, 166);
    minusOnePoints[4] = PT(381, 72, 281);
    minusOnePoints[5] = PT(350, 192, 416);
    minusOnePoints[6] = PT(16, 19, 455);
    minusOnePoints[7] = PT(152, 336, 8);
    minusOnePoints[8] = PT(400, 502, 447);
    minusOnePoints[9] = PT(72, 0, 105);

    plusOnePoints[0] = PT(308, 359, 9);
    plusOnePoints[1] = PT(9, 491, 116);
    plusOnePoints[2] = PT(449, 268, 441);
    plusOnePoints[3] = PT(147, 115, 197);
    plusOnePoints[4] = PT(241, 85, 3);
    plusOnePoints[5] = PT(507, 41, 125);
    plusOnePoints[6] = PT(161, 278, 73);
    plusOnePoints[7] = PT(144, 91, 310);
    plusOnePoints[8] = PT(201, 8, 49);
    plusOnePoints[9] = PT(149, 399, 329);
    break;
  case 10: // class D
    minusOnePoints[0] = PT(186, 374, 694);
    minusOnePoints[1] = PT(773, 345, 474);
    minusOnePoints[2] = PT(478, 874, 804);
    minusOnePoints[3] = PT(306, 75, 624);
    minusOnePoints[4] = PT(397, 667, 49);
    minusOnePoints[5] = PT(606, 199, 59);
    minusOnePoints[6] = PT(892, 70, 361);
    minusOnePoints[7] = PT(844, 261, 252);
    minusOnePoints[8] = PT(221, 906, 14);
    minusOnePoints[9] = PT(85, 327, 232);

    plusOnePoints[0] = PT(739, 879, 781);
    plusOnePoints[1] = PT(742, 641, 147);
    plusOnePoints[2] = PT(335, 295, 600);
    plusOnePoints[3] = PT(982, 944, 696);
    plusOnePoints[4] = PT(622, 881, 180);
    plusOnePoints[5] = PT(956, 217, 952);
    plusOnePoints[6] = PT(777, 453, 706);
    plusOnePoints[7] = PT(258, 730, 482);
    plusOnePoints[8] = PT(271, 75, 815);
    plusOnePoints[9] = PT(78, 276, 250);
    break;
  }

  Point<3> myBlockPos = gridA.myBlockPos;
  ndarray<double, 3 UNSTRIDED> myAPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];
  for (int i = 0; i < minusArrayLength; i++) {
    if (myAPoints.domain().contains(minusOnePoints[i])) {
      myAPoints[minusOnePoints[i]] = -1.0;
    }
  }
  for (int i = 0; i < plusArrayLength; i++) {
    if (myAPoints.domain().contains(plusOnePoints[i])) {
      myAPoints[plusOnePoints[i]] = +1.0;
    }
  }
  barrier();
}
