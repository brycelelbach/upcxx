/**
 * \example test_team.cpp
 *
 * Test teams and collective operations on teams
 *
 */

#include <upcxx.h>

#include <iostream>
#include <stdio.h>

using namespace upcxx;

int test_bcast(team t, size_t nbytes, int root = 0)
{
  if (t.myrank() == 0)
  // t.broadcast();
}

int main(int argc, char **argv)
{
  init(&argc, &argv);

  cout << "team_all: " << team_all << endl;

  if (MYTHREAD == 0)
    cout << "Testing collectives on team_all...\n";
  
  team_all.barrier();
    //test_bcast();
  team_all.barrier();

  if (MYTHREAD == 0)
    cout << "Finished testing team_all\n";

  team row_t, col_t;
  int row_id, col_id;

  if (MYTHREAD == 0)
    cout << "Spliting team_all into row and column teams...\n";
 
  //team_all.split(); // Row teams
  //team_all.split(); // Col teams

  barrier();
  if (MYTHREAD == 0)
    cout << "Finish team split\n";

  if (MYTHREAD == 0)
    cout << "Testing collectives on row teams...\n";

  if (MYTHREAD == 0)
    cout << "Finished testing collectives on row teams...\n";

  if (MYTHREAD == 0)
    cout << "Testing collectives on column teams...\n";

  if (MYTHREAD == 0)
    cout << "Finished testing collectives on column teams...\n";

  finalize();
  return 0;
}
