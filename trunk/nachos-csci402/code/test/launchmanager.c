/*    
 *  launchmanager.c
 *	Launches the manager for the restaurant simulation
 *	  and also the cooks
 */
 
#include "syscall.h"

int main()
{
  int i;
  
  PrintOut("Will Exec Manager 1 time \n", 26);
  Exec("../test/manager");  
  
  PrintOut("Will Exec Cook 5 times \n", 24);
  for(i=0; i < 5; i++)
  {
    Exec("../test/cook");  
  }

  Exit(0);
}
