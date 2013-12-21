#include <cmath>
#include "Test.h"
#include "FTDriver.h"
#include "Random.h"

#define PI 3.14159265358979323846

Test::Test(ndarray<global_ndarray<Complex, 3>, 1> array) {
  // copy from FTDriver
  nx = FTDriver::nx;
  ny = FTDriver::ny;
  nz = FTDriver::nz;
  maxdim = FTDriver::maxdim;
  numIterations = FTDriver::numIterations;

  // expArray in the same shape as array3
  global_ndarray<double, 3> myExpArray(array[MYTHREAD].domain());
  expArray = ndarray<global_ndarray<double, 3>, 1>(RECTDOMAIN((0), (THREADS)));
  expArray.exchange(myExpArray);

  if (MYTHREAD == 0) {
    checkSumArray = ndarray<Complex, 1>(RECTDOMAIN((1), (numIterations+1)));
  }

  // create "localComplexesOnProc0" and "myRemoteComplexesOnProc0"
  if (MYTHREAD == 0)  {
    localComplexesOnProc0 = ndarray<Complex, 1>(RECTDOMAIN((0), (THREADS)));
  }

  myRemoteComplexesOnProc0 = broadcast(localComplexesOnProc0, 0);

  // profiling

#ifdef TIMERS_ENABLED
  // myTimer = new Timer();
  myTimes = ndarray<ndarray<double, 1>, 1>(RECTDOMAIN((1), (3)));

  myTimes[T_EVOLVE] = ndarray<double, 1>(RECTDOMAIN((1), (numIterations+1)));
  myTimes[T_CHECKSUM] = ndarray<double, 1>(RECTDOMAIN((1), (numIterations+1)));
#endif
#ifdef COUNTERS_ENABLED
  myCounter = new PAPICounter(PAPI_MEASURE);
  myTimes = ndarray<ndarray<long, 1>, 1>(RECTDOMAIN((1), (3)));

  myCounts[T_EVOLVE] = ndarray<long, 1>(RECTDOMAIN((1), (numIterations+1)));
  myCounts[T_CHECKSUM] = ndarray<long, 1>(RECTDOMAIN((1), (numIterations+1)));
#endif
}

void Test::initialConditions(ndarray<global_ndarray<Complex, 3>, 1> array) {
  ndarray<Complex, 3> myArray = (ndarray<Complex, 3>) array[MYTHREAD];
  ndarray<double, 1> temp(RECTDOMAIN((0), (1)));
	
  int iMin = myArray.domain().min()[1];
  int iMax = myArray.domain().max()[1];

  // double *RanStarts = new double[maxdim];
  // seed has to be init here since is called 2 times 
  double seed = 314159265, a=pow(5.0,13);
  double start = seed;
  //---------------------------------------------------------------------
  // Jump to the starting element for our first plane.
  //---------------------------------------------------------------------
  Random rng(seed);
  double an = rng.ipow46(a, 2*nz, iMin*ny);
  temp[0]=start;
  rng.randlc(temp, an);
  start=temp[0];
  an = rng.ipow46(a, 2*nz, ny);

  //---------------------------------------------------------------------
  // Go through by z planes filling in one square at a time.
  //---------------------------------------------------------------------
  // now initialize myArray

  for (int i=iMin; i<=iMax; i++) {
    double x0 = start;
    ndarray<Complex, 2> mySlicedArray = myArray.slice(1,i);
    x0 = rng.vranlc(ny, nz, x0, a, mySlicedArray);
    temp[0] = start;
    start = rng.randlc(temp, an);
    start = temp[0];
  }
}

void Test::initializeExponentialArray() {
  const double c1 = (-4.0e-6) * PI * PI;
  ndarray<double, 3> myExpArray = (ndarray<double, 3>) expArray[MYTHREAD];

  int i, j, k;
  int halfnx = nx/2;
  int halfny = ny/2;
  int halfnz = nz/2;
  foreach (p, myExpArray.domain().shrink(PADDING, 3)) {
    i = ((p[3] < halfnx) ? p[3] : nx-p[3]);
    j = ((p[1] < halfny) ? p[1] : ny-p[1]);
    k = ((p[2] < halfnz) ? p[2] : nz-p[2]);
    myExpArray[p] = exp(c1 * (i*i + j*j + k*k));
  }
}

void Test::evolve(ndarray<global_ndarray<Complex, 3>, 1> array,
                  int iterationNum) {
  COUNTER_START(myCounter);
  TIMER_START(myTimer);

  ndarray<Complex, 3> myArray = (ndarray<Complex, 3>) array[MYTHREAD];
  ndarray<double, 3> myExpArray = (ndarray<double, 3>) expArray[MYTHREAD];

  foreach (p, myArray.domain().shrink(PADDING, 3)) {
    myArray[p] = myArray[p] * myExpArray[p];
  }

  TIMER_STOP_READ(myTimer, myTimes[T_EVOLVE][iterationNum]);
  COUNTER_STOP_READ(myCounter, myCounts[T_EVOLVE][iterationNum]);
}

void Test::checkSum(ndarray<global_ndarray<Complex, 3>, 1> array,
                    int iterationNum) {
  COUNTER_START(myCounter);
  TIMER_START(myTimer);

  ndarray<Complex, 3> myArray = (ndarray<Complex, 3>) array[MYTHREAD];
  RectDomain<3> myArrayDomain = myArray.domain();
  long ntotal = nx * ny * nz;
  Complex procCheckSum(0, 0);
	
  int q, r, s;
  for (int i=1; i <= 1024; i++) {
    q = i % nz;
    r = (3*i) % ny;
    s = (5*i) % nx;
    if (myArrayDomain.contains(POINT(q,s,r))) {
      procCheckSum = procCheckSum + myArray[POINT(q,s,r)];
    }
  }

  myRemoteComplexesOnProc0[MYTHREAD] = procCheckSum;
  barrier();
	
  if (MYTHREAD == 0) {
    Complex totalCheckSum;
    foreach (p, localComplexesOnProc0.domain()) {
      totalCheckSum = totalCheckSum + localComplexesOnProc0[p];
    }
    checkSumArray[iterationNum] = (totalCheckSum/(double)ntotal);
  }

  TIMER_STOP_READ(myTimer, myTimes[T_CHECKSUM][iterationNum]);
  COUNTER_STOP_READ(myCounter, myCounts[T_CHECKSUM][iterationNum]);
}

void Test::verifyCheckSum(char classType) {
  if (MYTHREAD == 0) {
    // set up verifyArray
    ndarray<Complex, 1> verifyArray(RECTDOMAIN((1), (numIterations+1)));
	    
    switch (classType) {
    case 'S':
      verifyArray[1] = Complex(5.546087004964e02, 4.845363331978e02);
      verifyArray[2] = Complex(5.546385409189e02, 4.865304269511e02);
      verifyArray[3] = Complex(5.546148406171e02, 4.883910722336e02);
      verifyArray[4] = Complex(5.545423607415e02, 4.901273169046e02);
      verifyArray[5] = Complex(5.544255039624e02, 4.917475857993e02);
      verifyArray[6] = Complex(5.542683411902e02, 4.932597244941e02);
      break;
    case 'W':
      verifyArray[1] = Complex(5.673612178944e02, 5.293246849175e02);
      verifyArray[2] = Complex(5.631436885271e02, 5.282149986629e02);
      verifyArray[3] = Complex(5.594024089970e02, 5.270996558037e02);
      verifyArray[4] = Complex(5.560698047020e02, 5.260027904925e02);
      verifyArray[5] = Complex(5.530898991250e02, 5.249400845633e02);
      verifyArray[6] = Complex(5.504159734538e02, 5.239212247086e02);
      break;
    case 'A':
      verifyArray[1] = Complex(5.046735008193e02, 5.114047905510e02);
      verifyArray[2] = Complex(5.059412319734e02, 5.098809666433e02);
      verifyArray[3] = Complex(5.069376896287e02, 5.098144042213e02);
      verifyArray[4] = Complex(5.077892868474e02, 5.101336130759e02);
      verifyArray[5] = Complex(5.085233095391e02, 5.104914655194e02);
      verifyArray[6] = Complex(5.091487099959e02, 5.107917842803e02);
      break;
    case 'B':
      verifyArray[1] = Complex(5.177643571579e02, 5.077803458597e02);
      verifyArray[2] = Complex(5.154521291263e02, 5.088249431599e02);
      verifyArray[3] = Complex(5.146409228649e02, 5.096208912659e02);
      verifyArray[4] = Complex(5.142378756213e02, 5.101023387619e02);
      verifyArray[5] = Complex(5.139626667737e02, 5.103976610617e02);
      verifyArray[6] = Complex(5.137423460082e02, 5.105948019802e02);
      verifyArray[7] = Complex(5.135547056878e02, 5.107404165783e02);
      verifyArray[8] = Complex(5.133910925466e02, 5.108576573661e02);
      verifyArray[9] = Complex(5.132470705390e02, 5.109577278523e02);
      verifyArray[10] = Complex(5.131197729984e02, 5.110460304483e02);
      verifyArray[11] = Complex(5.130070319283e02, 5.111252433800e02);
      verifyArray[12] = Complex(5.129070537032e02, 5.111968077718e02);
      verifyArray[13] = Complex(5.128182883502e02, 5.112616233064e02);
      verifyArray[14] = Complex(5.127393733383e02, 5.113203605551e02);
      verifyArray[15] = Complex(5.126691062020e02, 5.113735928093e02);
      verifyArray[16] = Complex(5.126064276004e02, 5.114218460548e02);
      verifyArray[17] = Complex(5.125504076570e02, 5.114656139760e02);
      verifyArray[18] = Complex(5.125002331720e02, 5.115053595966e02);
      verifyArray[19] = Complex(5.124551951846e02, 5.115415130407e02);
      verifyArray[20] = Complex(5.124146770029e02, 5.115744692211e02);
      break;
    case 'C':
      verifyArray[1] = Complex(5.195078707457e02, 5.149019699238e02);
      verifyArray[2] = Complex(5.155422171134e02, 5.127578201997e02);
      verifyArray[3] = Complex(5.144678022222e02, 5.122251847514e02);
      verifyArray[4] = Complex(5.140150594328e02, 5.121090289018e02);
      verifyArray[5] = Complex(5.137550426810e02, 5.121143685824e02);
      verifyArray[6] = Complex(5.135811056728e02, 5.121496764568e02);
      verifyArray[7] = Complex(5.134569343165e02, 5.121870921893e02);
      verifyArray[8] = Complex(5.133651975661e02, 5.122193250322e02);
      verifyArray[9] = Complex(5.132955192805e02, 5.122454735794e02);
      verifyArray[10] = Complex(5.132410471738e02, 5.122663649603e02);
      verifyArray[11] = Complex(5.131971141679e02, 5.122830879827e02);
      verifyArray[12] = Complex(5.131605205716e02, 5.122965869718e02);
      verifyArray[13] = Complex(5.131290734194e02, 5.123075927445e02);
      verifyArray[14] = Complex(5.131012720314e02, 5.123166486553e02);
      verifyArray[15] = Complex(5.130760908195e02, 5.123241541685e02);
      verifyArray[16] = Complex(5.130528295923e02, 5.123304037599e02);
      verifyArray[17] = Complex(5.130310107773e02, 5.123356167976e02);
      verifyArray[18] = Complex(5.130103090133e02, 5.123399592211e02);
      verifyArray[19] = Complex(5.129905029333e02, 5.123435588985e02);
      verifyArray[20] = Complex(5.129714421109e02, 5.123465164008e02);
      break;
    case 'D':
      verifyArray[1] = Complex(5.122230065252e02, 5.118534037109e02);
      verifyArray[2] = Complex(5.120463975765e02, 5.117061181082e02);
      verifyArray[3] = Complex(5.119865766760e02, 5.117096364601e02);
      verifyArray[4] = Complex(5.119518799488e02, 5.117373863950e02);
      verifyArray[5] = Complex(5.119269088223e02, 5.117680347632e02);
      verifyArray[6] = Complex(5.119082416858e02, 5.117967875532e02);
      verifyArray[7] = Complex(5.118943814638e02, 5.118225281841e02);
      verifyArray[8] = Complex(5.118842385057e02, 5.118451629348e02);
      verifyArray[9] = Complex(5.118769435632e02, 5.118649119387e02);
      verifyArray[10] = Complex(5.118718203448e02, 5.118820803844e02);
      verifyArray[11] = Complex(5.118683569061e02, 5.118969781011e02);
      verifyArray[12] = Complex(5.118661708593e02, 5.119098918835e02);
      verifyArray[13] = Complex(5.118649768950e02, 5.119210777066e02);
      verifyArray[14] = Complex(5.118645605626e02, 5.119307604484e02);
      verifyArray[15] = Complex(5.118647586618e02, 5.119391362671e02);
      verifyArray[16] = Complex(5.118654451572e02, 5.119463757241e02);
      verifyArray[17] = Complex(5.118665212451e02, 5.119526269238e02);
      verifyArray[18] = Complex(5.118679083821e02, 5.119580184108e02);
      verifyArray[19] = Complex(5.118695433664e02, 5.119626617538e02);
      verifyArray[20] = Complex(5.118713748264e02, 5.119666538138e02);
      verifyArray[21] = Complex(5.118733606701e02, 5.119700787219e02);
      verifyArray[22] = Complex(5.118754661974e02, 5.119730095953e02);
      verifyArray[23] = Complex(5.118776626738e02, 5.119755100241e02);
      verifyArray[24] = Complex(5.118799262314e02, 5.119776353561e02);
      verifyArray[25] = Complex(5.118822370068e02, 5.119794338060e02);
      break;
    }
	    
    // perform actual verification
    double epsilon = 1.0e-12;
    double relError;
    bool verified = true;
	    
    foreach (iter, RECTDOMAIN((1), (numIterations+1))) {
      relError = abs((checkSumArray[iter].real() -
                      verifyArray[iter].real()) /
                     verifyArray[iter].real());
      if (relError > epsilon) {
        verified = false;
      }
      relError = abs((checkSumArray[iter].imag() -
                      verifyArray[iter].imag()) /
                     verifyArray[iter].imag());
      if (relError > epsilon) {
        verified = false;
      }
    }
	    
    // print the results
    if (verified) {
      println("VERIFICATION SUCCESSFUL");
    }
    else {
      println("VERIFICATION FAILED");
      println("Expected results:");
      foreach (iter, RECTDOMAIN((1), (numIterations+1))) {
        println(verifyArray[iter] << ", ");
      }
    }
    println("Check Sum results:");
    foreach (iter, RECTDOMAIN((1), (numIterations+1))) {
      println(checkSumArray[iter] << ", ");
    }
    println("");
  }
}

void Test::printSummary() {
  for (int i=1; i <= 2; i++) {
    if (MYTHREAD == 0) {
      switch (i) {
      case T_EVOLVE:
        println("EVOLVE: ");
        break;
      case T_CHECKSUM:
        println("CHECK SUM: ");
        break;
      }
    }

#ifdef TIMERS_ENABLED
    double totalComponentTime = 0.0;
    foreach (timing, myTimes[i].domain()) {
      totalComponentTime += myTimes[i][timing];
    }

    double minTotalComponentTime = reduce::min(totalComponentTime);
    double sumTotalComponentTime = reduce::add(totalComponentTime);
    double maxTotalComponentTime = reduce::max(totalComponentTime);
	    
    if (MYTHREAD == 0) {
      println("Time: " << minTotalComponentTime << ", " <<
              (sumTotalComponentTime/THREADS) << ", " <<
              maxTotalComponentTime);
    }
#endif
#ifdef COUNTERS_ENABLED
    long totalComponentCount = 0;
    foreach (count, myCounts[i].domain()) {
      totalComponentCount += myCounts[i][count];
    }
	    
    long minTotalComponentCount = reduce::min(totalComponentCount);
    long sumTotalComponentCount = reduce::add(totalComponentCount);
    long maxTotalComponentCount = reduce::max(totalComponentCount);
	    
    if (MYTHREAD == 0) {
      println("Count: " << minTotalComponentCount << ", " <<
              (sumTotalComponentCount/THREADS) << ", " <<
              maxTotalComponentCount);
    }
#endif
  }
}

void Test::printProfile() {
  for (int i=1; i <= 2; i++) {
    if (MYTHREAD == 0) {
      switch (i) {
      case T_EVOLVE:
        println("EVOLVE:");
        break;
      case T_CHECKSUM:
        println("CHECK SUM:");
        break;
      }
    }

    int numReadingsPerProc;
#ifdef TIMERS_ENABLED
    // print timer info
    double lminTime = myTimes[i][myTimes[i].domain().min()];
    double lmaxTime = lminTime;
    double lsumTime = 0;
    numReadingsPerProc = 0;
	    
    foreach (timing, myTimes[i].domain()) {
      double value = myTimes[i][timing];
      if (value < lminTime) {
        lminTime = value;
      }
      if (value > lmaxTime) {
        lmaxTime = value;
      }
      lsumTime += value;
      numReadingsPerProc++;
    }
	    
    double gminTime = reduce::min(lminTime);
    double gmaxTime = reduce::max(lmaxTime);
    double gsumTime = reduce::add(lsumTime);
	    
    if (MYTHREAD == 0) {
      println("Num Readings Per Proc:\t" << numReadingsPerProc);
      println("Min Time Over Procs:\t" << gminTime);
      double gmeanTime = gsumTime/(numReadingsPerProc*THREADS);
      println("Mean Time Over Procs:\t" << gmeanTime);
      println("Max Time Over Procs:\t" << gmaxTime << "\n");
    }
#endif
#ifdef COUNTERS_ENABLED
    // print counter info
    long lminCount = myCounts[i][myCounts[i].domain().min()];
    long lmaxCount = lminCount;
    long lsumCount = 0;
    numReadingsPerProc = 0;
	    
    foreach (count, myCounts[i].domain()) {
      long value = myCounts[i][count];
      if (value < lminCount) {
        lminCount = value;
      }
      if (value > lmaxCount) {
        lmaxCount = value;
      }
      lsumCount += value;
      numReadingsPerProc++;
    }
	    
    long gminCount = reduce::min(lminCount);
    long gmaxCount = reduce::max(lmaxCount);
    long gsumCount = reduce::add(lsumCount);
	    
    if (MYTHREAD == 0) {
      println("Num Readings Per Proc:\t" << numReadingsPerProc);
      println("Min Count Over Procs:\t" << gminCount);
      double gmeanCount = gsumCount/(numReadingsPerProc*THREADS);
      println("Mean Count Over Procs:\t" << gmeanCount);
      println("Max Count Over Procs:\t" << gmaxCount << "\n");
    }

    /*
      if (MYTHREAD == 0) {
        println("Min Count Over Procs:\t" << gminCount <<
                "\nMean Count Over Procs:\t" <<
                (gsumCount/(numReadingsPerProc*THREADS)) <<
                "\nMax Count Over Procs:\t" << gmaxCount << "\n");
      }
    */
#endif
  }
}
