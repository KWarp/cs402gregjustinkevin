/* testmultisim.c
 *	Makes 2 copies of the restaurant simulation
 */
 
#include "syscall.h"


int main()
{
  PrintOut("\n\n******** MULTI SIMULATION TEST ********\n\n", 43);
  Exec("simulation");
  Exec("simulation");
  
  Exit(0);  
  return 0;
}
