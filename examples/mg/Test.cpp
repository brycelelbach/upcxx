#include "Test.h"

#define minusArrayLength 10
#define plusArrayLength 10

void Test::populateRHSGrid(Grid &gridA, int startLevel) {
  Point<3> minusOnePoints[minusArrayLength];
  Point<3> plusOnePoints[plusArrayLength];

  switch(startLevel) {
  case 5: // class S
    minusOnePoints[0] = POINT(0, 11, 2);
    minusOnePoints[1] = POINT(13, 8, 17);
    minusOnePoints[2] = POINT(5, 14, 0);
    minusOnePoints[3] = POINT(4, 28, 15);
    minusOnePoints[4] = POINT(12, 2, 1);
    minusOnePoints[5] = POINT(5, 17, 8);
    minusOnePoints[6] = POINT(20, 19, 11);
    minusOnePoints[7] = POINT(26, 15, 31);
    minusOnePoints[8] = POINT(8, 25, 22);
    minusOnePoints[9] = POINT(7, 14, 26);

    plusOnePoints[0] = POINT(7, 1, 20);
    plusOnePoints[1] = POINT(19, 29, 31);
    plusOnePoints[2] = POINT(2, 0, 3);
    plusOnePoints[3] = POINT(4, 22, 3);
    plusOnePoints[4] = POINT(1, 16, 21);
    plusOnePoints[5] = POINT(21, 31, 6);
    plusOnePoints[6] = POINT(12, 15, 12);
    plusOnePoints[7] = POINT(30, 4, 25);
    plusOnePoints[8] = POINT(28, 0, 28);
    plusOnePoints[9] = POINT(17, 26, 17);
    break;
  case 6: // class W
    minusOnePoints[0] = POINT(38, 60, 51);
    minusOnePoints[1] = POINT(50, 15, 23);
    minusOnePoints[2] = POINT(18, 45, 36);
    minusOnePoints[3] = POINT(25, 14, 36);
    minusOnePoints[4] = POINT(26, 25, 25);
    minusOnePoints[5] = POINT(32, 37, 0);
    minusOnePoints[6] = POINT(29, 62, 54);
    minusOnePoints[7] = POINT(39, 49, 57);
    minusOnePoints[8] = POINT(12, 29, 28);
    minusOnePoints[9] = POINT(63, 46, 25);

    plusOnePoints[0] = POINT(27, 32, 45);
    plusOnePoints[1] = POINT(39, 0, 5);
    plusOnePoints[2] = POINT(45, 23, 49);
    plusOnePoints[3] = POINT(20, 32, 58);
    plusOnePoints[4] = POINT(23, 47, 57);
    plusOnePoints[5] = POINT(17, 43, 53);
    plusOnePoints[6] = POINT(8, 16, 48);
    plusOnePoints[7] = POINT(51, 46, 26);
    plusOnePoints[8] = POINT(58, 19, 62);
    plusOnePoints[9] = POINT(58, 15, 54);
    break;
  case 8: // class A and B
    minusOnePoints[0] = POINT(221, 40, 238);
    minusOnePoints[1] = POINT(152, 160, 34);
    minusOnePoints[2] = POINT(80, 182, 253);
    minusOnePoints[3] = POINT(248, 168, 155);
    minusOnePoints[4] = POINT(197, 5, 201);
    minusOnePoints[5] = POINT(90, 61, 203);
    minusOnePoints[6] = POINT(15, 203, 30);
    minusOnePoints[7] = POINT(99, 154, 57);
    minusOnePoints[8] = POINT(100, 136, 110);
    minusOnePoints[9] = POINT(209, 152, 96);

    plusOnePoints[0] = POINT(52, 207, 38);
    plusOnePoints[1] = POINT(241, 170, 12);
    plusOnePoints[2] = POINT(201, 16, 196);
    plusOnePoints[3] = POINT(200, 81, 207);
    plusOnePoints[4] = POINT(113, 121, 205);
    plusOnePoints[5] = POINT(210, 5, 246);
    plusOnePoints[6] = POINT(43, 192, 232);
    plusOnePoints[7] = POINT(174, 244, 162);
    plusOnePoints[8] = POINT(3, 116, 173);
    plusOnePoints[9] = POINT(55, 118, 165);
    break;
  case 9: // class C
    minusOnePoints[0] = POINT(397, 310, 198);
    minusOnePoints[1] = POINT(94, 399, 236);
    minusOnePoints[2] = POINT(221, 276, 59);
    minusOnePoints[3] = POINT(342, 137, 166);
    minusOnePoints[4] = POINT(381, 72, 281);
    minusOnePoints[5] = POINT(350, 192, 416);
    minusOnePoints[6] = POINT(16, 19, 455);
    minusOnePoints[7] = POINT(152, 336, 8);
    minusOnePoints[8] = POINT(400, 502, 447);
    minusOnePoints[9] = POINT(72, 0, 105);

    plusOnePoints[0] = POINT(308, 359, 9);
    plusOnePoints[1] = POINT(9, 491, 116);
    plusOnePoints[2] = POINT(449, 268, 441);
    plusOnePoints[3] = POINT(147, 115, 197);
    plusOnePoints[4] = POINT(241, 85, 3);
    plusOnePoints[5] = POINT(507, 41, 125);
    plusOnePoints[6] = POINT(161, 278, 73);
    plusOnePoints[7] = POINT(144, 91, 310);
    plusOnePoints[8] = POINT(201, 8, 49);
    plusOnePoints[9] = POINT(149, 399, 329);
    break;
  case 10: // class D
    minusOnePoints[0] = POINT(186, 374, 694);
    minusOnePoints[1] = POINT(773, 345, 474);
    minusOnePoints[2] = POINT(478, 874, 804);
    minusOnePoints[3] = POINT(306, 75, 624);
    minusOnePoints[4] = POINT(397, 667, 49);
    minusOnePoints[5] = POINT(606, 199, 59);
    minusOnePoints[6] = POINT(892, 70, 361);
    minusOnePoints[7] = POINT(844, 261, 252);
    minusOnePoints[8] = POINT(221, 906, 14);
    minusOnePoints[9] = POINT(85, 327, 232);

    plusOnePoints[0] = POINT(739, 879, 781);
    plusOnePoints[1] = POINT(742, 641, 147);
    plusOnePoints[2] = POINT(335, 295, 600);
    plusOnePoints[3] = POINT(982, 944, 696);
    plusOnePoints[4] = POINT(622, 881, 180);
    plusOnePoints[5] = POINT(956, 217, 952);
    plusOnePoints[6] = POINT(777, 453, 706);
    plusOnePoints[7] = POINT(258, 730, 482);
    plusOnePoints[8] = POINT(271, 75, 815);
    plusOnePoints[9] = POINT(78, 276, 250);
    break;
  }

  Point<3> myBlockPos = gridA.myBlockPos;
  ndarray<double, 3> myAPoints = (ndarray<double, 3>) gridA.points[myBlockPos];
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
