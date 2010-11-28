/*    
 *  teststartuserprogramsub.c
 *	Should be Exec'ed by TestStartUserProgram
 *	it is a subroutine
 */

#include "syscall.h"

int main()
{
  PrintOut("===TestStartUserProgramSub===\n", 30);
  /*Yield(50);*/
  StartUserProgram();
  
  PrintOut("===TestStartUserProgramSub Completed===\n", 40);
}
