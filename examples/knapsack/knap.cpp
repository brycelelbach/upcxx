// Knapsack
#include "globals.h"

static const int DEFAULT_N_BOOKS = 5000;
static const int DEFAULT_BAG_CAP = 1000;
static const int DEFAULT_MAX_RANDOM_WEIGHT = 2000;
static const int DEFAULT_MAX_RANDOM_PROFIT = 50;

static int n_books = DEFAULT_N_BOOKS;
static int bag_cap = DEFAULT_BAG_CAP;
static ndarray<int, 1 UNSTRIDED> weight;
static ndarray<int, 1 UNSTRIDED> profit;
static ndarray<int, 2 UNSTRIDED> total;
static ndarray<int, 2 UNSTRIDED> total_total;
static ndarray<bool, 1 UNSTRIDED> use_book;

static int cap_per_block;
static int books_per_proc;
static int n_blocks;

static const int STATIC = 0;
static const int READFILE = 1;
static const int RANDOM = 2;
static int init_from = STATIC;
static char *fname;
static int max_random_weight = DEFAULT_MAX_RANDOM_WEIGHT;
static int max_random_profit = DEFAULT_MAX_RANDOM_PROFIT;
static int seedval = -1;

static void copyTotal(ndarray<ndarray<int, 2,
                      global GUNSTRIDED>, 1 UNSTRIDED> totals,
                      ndarray<int, 2 UNSTRIDED> mytotals,
                      int book, int start_cap, int end_cap);
static int solve(int n_books, int bag_cap,
                 ndarray<int, 1 UNSTRIDED> weight,
                 ndarray<int, 1 UNSTRIDED> profit,
                 ndarray<int, 2 UNSTRIDED> total,
                 ndarray<ndarray<int, 2, global GUNSTRIDED>,
                 1 UNSTRIDED> all_totals,
                 int my_books, int my_start_book, int my_end_book,
                 int n_blocks);
static void backtrack(int n_books, int bag_cap,
                      ndarray<int, 1 UNSTRIDED> weight,
                      ndarray<int, 2 UNSTRIDED> total,
                      ndarray<bool, 1 UNSTRIDED> use_book,
                      int my_start_book, int my_end_book);
static void init_books();
static void get_opts(int argc, char **args);
static void dump_usage(char *name, ostream &out, int errcode);
static void read_books(istream &is, int n,
                       ndarray<int, 1 UNSTRIDED> weight,
                       ndarray<int, 1 UNSTRIDED> profit);
static void randgen_books(int seedval, int n,
                          ndarray<int, 1 UNSTRIDED> weight,
                          ndarray<int, 1 UNSTRIDED> profit);
static void staticgen_books(int n,
                            ndarray<int, 1 UNSTRIDED> weight,
                            ndarray<int, 1 UNSTRIDED> profit);

int main(int argc, char **argv) {
  init(&argc, &argv);

  int total_profit, total_weight, book_i, n_sold;
  timer solve_timer, backtrack_timer;
  ndarray<ndarray<int, 2, global GUNSTRIDED>,
          1 UNSTRIDED> all_totals(RECTDOMAIN((0), ((int)THREADS)));
  ndarray<ndarray<bool, 1, global GUNSTRIDED>,
          1 UNSTRIDED> all_use_book(RECTDOMAIN((0), ((int)THREADS)));
  ndarray<int, 1, global GUNSTRIDED> weight0, profit0;
  int my_books, my_start_book, my_end_book;
  n_blocks = THREADS;

  get_opts(argc, argv);
  books_per_proc = (n_books + THREADS - 1) / THREADS;
  my_books = ((THREADS - 1 == MYTHREAD) &&
              (n_books % books_per_proc != 0)) ?
    (n_books % books_per_proc) : books_per_proc;
  my_start_book = books_per_proc * MYTHREAD;
  my_end_book = my_start_book + my_books - 1;
  debug("" << my_books, 1);

  cap_per_block = (bag_cap + 1) / n_blocks;
  if (MYTHREAD == 0)
    println("cap per block: " << cap_per_block);

  weight = ndarray<int, 1 UNSTRIDED>(RECTDOMAIN((0), (n_books)));
  profit = ndarray<int, 1 UNSTRIDED>(RECTDOMAIN((0), (n_books)));
  if (MYTHREAD != 0) {
    use_book =
      ndarray<bool, 1 UNSTRIDED>(RECTDOMAIN((my_start_book),
                                            (my_end_book+1)));
  } else {
    use_book =
      ndarray<bool, 1 UNSTRIDED>(RECTDOMAIN((0), (n_books)));
  }

  if (MYTHREAD == 0) {
    // solve_timer = new Timer();
    // backtrack_timer = new Timer();
    init_books();
  }

  total =
    ndarray<int, 2 UNSTRIDED>(RECTDOMAIN((my_start_book-1, 0),
                                         (my_end_book+1,
                                          bag_cap+1)));

  // Send data out to all processors.
  all_totals.exchange(total);
  all_use_book.exchange(use_book);
  // weight0 = broadcast(weight, 0);
  // profit0 = broadcast(profit, 0);
  // if (MYTHREAD != 0) {
  //   weight.copy(weight0);
  //   profit.copy(profit0);
  // }
  broadcast(weight, weight, 0);
  broadcast(profit, profit, 0);

  barrier();
  if (MYTHREAD == 0) {
    solve_timer.reset();
    backtrack_timer.reset();
  }

  if (MYTHREAD == 0) {
    solve_timer.start();
  }
  total_profit = solve(n_books, bag_cap, weight, profit, total,
                       all_totals, my_books, my_start_book,
                       my_end_book, n_blocks);
  if (MYTHREAD == 0) {
    solve_timer.stop();
  }

  barrier();
  debug("G", 1);

  if (MYTHREAD == 0) {
    backtrack_timer.start();
  }
  backtrack(n_books, bag_cap , weight, total, use_book,
            my_start_book, my_end_book);
  if (MYTHREAD == 0) {
    ndarray<bool, 1, global GUNSTRIDED> puse_book;
    for (int p = 1; p < THREADS; p++) {
      puse_book = all_use_book[p];
      use_book.copy(puse_book);
    }
    backtrack_timer.stop();
  }

  if (MYTHREAD == 0) {
    println("Total profit: " << total_profit);
    println("Using books:");
    total_weight = n_sold = 0;
    for (book_i = 0; book_i < n_books; ++book_i)
      if (use_book[book_i]) {
        if (n_sold < 10)
          println("\t" << book_i << " (w " << weight[book_i] << "; "
                  << "p " << profit[book_i] << ")");
        else if (n_sold == 10)
          println("\t...");
        total_weight += weight[book_i];
        total_profit -= profit[book_i];
        ++n_sold;
      }
    if (total_profit != 0)
      println("Profit didn't balance!");
    println("Number of processors: " << THREADS);
    println("Total number of books: " << n_books);
    println("Max book weight: " << max_random_weight);
    println("Max book profit: " << max_random_profit);
    println("Capacity used: " << total_weight << " of " << bag_cap
            << " with " << n_sold << " books");
    println("Times: solve " << solve_timer.secs() << ", backtrack "
            << backtrack_timer.secs());
  }

  finalize();
  return 0;
}

static inline void copyTotal(ndarray<ndarray<int, 2,
                             global GUNSTRIDED>, 1 UNSTRIDED> totals,
                             ndarray<int, 2 UNSTRIDED> mytotals,
                             int book, int start_cap, int end_cap) {
  int proc = book / books_per_proc;
  mytotals.copy(totals[proc],
                RECTDOMAIN((book,start_cap), (book+1,end_cap+1)));
}

static int solve(int n_books, int bag_cap,
                 ndarray<int, 1 UNSTRIDED> weight,
                 ndarray<int, 1 UNSTRIDED> profit,
                 ndarray<int, 2 UNSTRIDED> total,
                 ndarray<ndarray<int, 2, global GUNSTRIDED>,
                 1 UNSTRIDED> all_totals,
                 int my_books, int my_start_book, int my_end_book,
                 int n_blocks) {
  int book_i;
  int cap_j;
  int iter;
  int start_iter, end_iter;
  int block_no;
  int my_start_cap, my_end_cap;
  int total_profit;
  debug("books: " << my_start_book << " " << my_end_book, 1);
  if (MYTHREAD == 0) {
    for (cap_j = 0; cap_j <= bag_cap; ++cap_j) {
      if (weight[0] > cap_j)
        /* Doesn't fit. */
        writeTotal(total, 0, cap_j, 0);
      else
        writeTotal(total, 0, cap_j, profit[0]);
    }
    my_start_book++;
  }
  debug("A", 1);
  start_iter = MYTHREAD;
  end_iter = MYTHREAD + n_blocks - 1;
  debug(start_iter << " " << end_iter, 1);
  for (iter = 0; iter < THREADS + n_blocks - 1; ++iter) {
    barrier();
    debug("B " << iter, 2);
    if (start_iter <= iter && end_iter >= iter) {
      debug("D", 2);
      block_no = iter - MYTHREAD;
      my_start_cap = block_no * cap_per_block;
      my_end_cap = (block_no == n_blocks - 1 ? bag_cap :
                    my_start_cap + cap_per_block - 1);
      if (MYTHREAD != 0) {
        copyTotal(all_totals, total, my_start_book-1,
                  my_start_cap, my_end_cap);
      }
      for (book_i = my_start_book; book_i <= my_end_book; book_i++) {
        int wi = weight[book_i];
        for (cap_j = my_start_cap; cap_j <= my_end_cap; cap_j++) {
          int new_profit;
          debug("E " << iter << " " << cap_j << " " << book_i, 3);
          if (cap_j < wi) {
            /* Doesn't fit. */
            writeTotal(total, book_i, cap_j,
                       getTotal(total, book_i - 1, cap_j));
            continue;
          }
          new_profit = profit[book_i] +
            getTotal(total, book_i-1, cap_j-wi);
          writeTotal(total, book_i, cap_j,
                     max(getTotal(total, book_i-1, cap_j),
                         new_profit));
        }
      }
    }
  }
  barrier();
  debug("C", 1);
  if (MYTHREAD == THREADS - 1) {
    total_profit = getTotal(total, n_books-1, bag_cap);
  }
  total_profit = broadcast(total_profit, THREADS - 1);
  return total_profit;
}

static void backtrack(int n_books, int bag_cap,
                      ndarray<int, 1 UNSTRIDED> weight,
                      ndarray<int, 2 UNSTRIDED> total,
                      ndarray<bool, 1 UNSTRIDED> use_book,
                      int my_start_book, int my_end_book) {
  int book_i;
  ndarray<int, 1 UNSTRIDED> off(RECTDOMAIN((0), (1)));
  ndarray<ndarray<int, 1, global GUNSTRIDED>,
          1 UNSTRIDED> offs(RECTDOMAIN((0), ((int)THREADS)));
  int my_start_book2 = my_start_book;
  if (MYTHREAD == 0)
    my_start_book2++;
  offs.exchange(off);
  off[0] = bag_cap;
  for (int proc = THREADS-1; proc >= 0; proc--) {
    if (proc == MYTHREAD) {
      for (book_i = my_end_book; book_i >= my_start_book2; --book_i) {
        if (getTotal(total, book_i, off[0]) !=
            getTotal(total, book_i-1, off[0])) {
          use_book[book_i] = true;
          off[0] -= weight[book_i];
        }
        else use_book[book_i] = false;
      }
      if (MYTHREAD != 0) {
        offs[MYTHREAD-1][0] = off[0];
      }
    }
    barrier();
  }
}

static void init_books() {
  ifstream ifs;
  switch (init_from) {
  case STATIC:
    staticgen_books(n_books, weight, profit);
    break;
  case RANDOM:
    randgen_books(seedval, n_books, weight, profit);
    break;
  case READFILE:
    ifs.open(fname);
    read_books(ifs, n_books, weight, profit);
    break;
  default:
    println("Unknown initialization routine.");
    exit(1);
  }
}

static void get_opts(int argc, char **args) {
  int i = 1;
  char *opt, *optarg;
  char *name = args[0];

  while (i < argc) {
    opt = args[i];
    if (!strcmp(opt, "-h") || !strcmp(opt, "-?")) {
      dump_usage(name, cout, 0);
    } else if (!strcmp(opt, "-B")) {
      optarg = args[++i];
      n_blocks = atoi(optarg);
      if (n_blocks <= 0 || n_blocks > bag_cap + 1) {
        debug("hello", 1);
        dump_usage(name, cerr, 1);
      }
    } else if (!strcmp(opt, "-b")) {
      optarg = args[++i];
      n_books = atoi(optarg);
      if (n_books <= 0)
        dump_usage(name, cerr, 1);
    } else if (!strcmp(opt, "-c")) {
      optarg = args[++i];
      bag_cap = atoi(optarg);
      if (bag_cap <= 0)
        dump_usage(name, cerr, 1);
    } else if (!strcmp(opt, "-g")) {
      if (init_from != STATIC)
        dump_usage(name, cerr, 1);
      init_from = STATIC;
    } else if (!strcmp(opt, "-f")) {
      fname = args[++i];
      if (init_from != STATIC)
        dump_usage(name, cerr, 1);
      init_from = READFILE;
    } else if (!strcmp(opt, "-r")) {
      if (init_from != STATIC)
        dump_usage(name, cerr, 1);
      init_from = RANDOM;
    } else if (!strcmp(opt, "-W")) {
      optarg = args[++i];
      max_random_weight = atoi(optarg);
      if (max_random_weight <= 0)
        dump_usage(name, cerr, 1);
    } else if (!strcmp(opt, "-P")) {
      optarg = args[++i];
      max_random_profit = atoi(optarg);
      if (max_random_profit <= 0)
        dump_usage(name, cerr, 1);
    } else if (!strcmp(opt, "-s")){
      seedval = atoi(args[++i]);
    } else {
      if (MYTHREAD == 0)
        cerr << "Unknown option: " << opt << endl;
      dump_usage(name, cerr, 1);
    }
    i++;
  }
  return;
}

static void dump_usage(char *name, ostream &out, int errcode) {
  /*h?b:c:f:rW:P:s:*/
  if (MYTHREAD != 0) exit(errcode);
  out << "Usage:   " << name << " [options]\n" <<
    "Options: " <<
    "  \t-h or -?      this message\n" <<
    "\t\t-B            number of pipeline stages (default: THREADS)\n" <<
    "\t\t-b <int>      number of books (default: " << DEFAULT_N_BOOKS << ")\n" <<
    "\t\t-c <int>      bag capacity (default: " << DEFAULT_BAG_CAP << ")\n" <<
    "\t\t-g            generate weights and profits statically (default)\n" <<
    "\t\t-f <filename> file to read weights and profits\n" <<
    "\t\t-r            generate weights and profits randomly\n" <<
    "\t\t-W <int>      max random weight (default: " << DEFAULT_MAX_RANDOM_WEIGHT << ")\n" <<
    "\t\t-P <int>      max random profit (default: " << DEFAULT_MAX_RANDOM_PROFIT << ")\n" <<
    "\t\t-s <int>      random seed\n";
  exit(errcode);
}

static void read_books(istream &is, int n,
                       ndarray<int, 1 UNSTRIDED> weight,
                       ndarray<int, 1 UNSTRIDED> profit) {
  for (int k = 0; k < n; ++k) {
    int w, p;
    is >> w >> p;
    weight[k] = w;
    profit[k] = p;
  }
}

static void randgen_books_(int n, ndarray<int, 1 UNSTRIDED> weight,
                           ndarray<int, 1 UNSTRIDED> profit) {
  int k;
  double x;

  for (k = 0; k < n; ++k) {
    weight[k] = 1 + (rand() % max_random_weight);
    profit[k] = 1 + (rand() % max_random_profit);
  }
}

static void randgen_books(int seedval, int n,
                          ndarray<int, 1 UNSTRIDED> weight,
                          ndarray<int, 1 UNSTRIDED> profit) {
  if (seedval < 0)
    seedval = 1010101;
  // seedval = (long) clock();
  // srand48(seedval);
  srand(seedval);
  randgen_books_(n, weight, profit);
}

static void staticgen_books(int n,
                            ndarray<int, 1 UNSTRIDED> weight,
                            ndarray<int, 1 UNSTRIDED> profit) {
  srand(1010101);
  randgen_books_(n, weight, profit);
}
