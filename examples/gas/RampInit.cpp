#include "RampInit.h"

double RampInit::dnsubinv = 1.0 / (double) RampInit::nsub;
double RampInit::rn2 = RampInit::dnsubinv * RampInit::dnsubinv;

void RampInit::initData(rectdomain<SPACE_DIM> D,
                        ndarray<Quantities, SPACE_DIM, global> &U,
                        double dx) {
  double subdx = dx * dnsubinv;
  double leftBase = ends[-1][LENGTH_DIM];
  double bottomBase = ends[-1][HEIGHT_DIM];
  foreach (p, D) {
    double left = leftBase + (double) p[LENGTH_DIM] * dx;
    double right = left + dx;
    double bottom = bottomBase + (double) p[HEIGHT_DIM] * dx;
    double top = bottom + dx;
   
    if (right <= shockLocBottom + bottom / slope) {
      U[p] = U1;
    } else if (left >= shockLocBottom + top / slope) {
      U[p] = U0;
    } else {
      int iwts = 0;
      for (int jsub = 1; jsub <= nsub; jsub++) {
        for (int ksub = 1; ksub <= nsub; ksub++) {
          double xsub = left + ((double) jsub - 0.5) * subdx;
          double ysub = bottom + ((double) ksub - 0.5) * subdx;
          if (xsub <= shockLocBottom + ysub / slope) iwts++;
        }
      }
      double wt = (double) iwts * rn2;
      U[p] = U1 * wt + U0 * (1.0 - wt);
    }
  }
}
