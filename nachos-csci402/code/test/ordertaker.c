/* ordertaker.c
 * A ordertaker for the restaurant simulation
 */

#include "syscall.h"
#include "simulation.c"

 
/* =============================================================	
 * ORDER TAKER
 * =============================================================*/	

 /*-----------------------------
 * Top Level OrderTaker Routine
 * ---------------------------*/
void OrderTaker(int debug)
{
  int ID;

	Acquire(lock_Init_InitializationLock);
	  ID = GetMV(count_NumOrderTakers);
	  SetMV(count_NumOrderTakers,GetMV(count_NumOrderTakers)+1);
	  PrintOutV("OrderTaker", 10);
	  PrintNumberV(ID);
	  PrintOutV("::Created - At your service.\n", 29);	
	Release(lock_Init_InitializationLock);
	
	while(TRUE)
	{
		serviceCustomer(ID);
		bagOrder(FALSE);
		
		/* Exit condition */
		if(shouldExit())
		{
			PrintOutV("OrderTaker", 10);
			PrintOutV("::all customers served.\n", 24);
			Exit(0);
		}
		
		Yield(50);
	}
}


int main()
{
  int i;
  PrintOut("=== OrderTaker ===\n", 19);
  
  StartUserProgram();
  for(i = 0; i < 5000; i++)
  {  
    Yield(1);
  }
  Initialize();  
  OrderTaker(0);
  return 0;
}
