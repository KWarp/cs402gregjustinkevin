/* waiter.c
 * A waiter for the restaurant simulation
 */

#include "syscall.h"
#include "simulation.c"

 
/* =============================================================	
 * Waiter
 * =============================================================*/	
void Waiter(int debug)
{
	int ID, token;
  
	Acquire(lock_Init_InitializationLock);
		ID = GetMV(count_NumWaiters);
		SetMV(count_NumWaiters,GetMV(count_NumWaiters)+1);
	Release(lock_Init_InitializationLock);
	
	while(TRUE)
	{
		token = -1;
		Acquire(lock_OrWr_BaggedOrders);
		/* Wait to get Signaled by the Order Taker*/
		while(GetMV(count_NumOrdersBaggedAndReady) <= 0)
		{
			PrintOut("Waiter ", 7);
			PrintNumber(ID);
			PrintOut(" is going on break\n", 19);
			Wait(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
			PrintOut("Waiter ", 7);
			PrintNumber(ID);
			PrintOut(" returned from break\n", 21);
		}
		
		/* Grab the order that is ready */
		token = GetMV(baggedOrders[GetMV(count_NumOrdersBaggedAndReady) - 1]);
		SetMV(count_NumOrdersBaggedAndReady,GetMV(count_NumOrdersBaggedAndReady)-1);
		
		Release(lock_OrWr_BaggedOrders);
		
		if (token != -1)
		{
			PrintOut("Manager gives Token number ", 27);
			PrintNumber(token);
			PrintOut(" to Waiter ", 11);
			PrintNumber(ID);
			PrintOut(" for Customer ", 14);
			PrintNumber(GetMV(Get_CustomerIDFromToken[token]));
			PrintOut("\n", 1);
			
			PrintOut("Waiter ", 7);
			PrintNumber(ID);
			PrintOut(" got token number ", 18);
			PrintNumber(token);
			PrintOut(" for Customer ", 14);
			PrintNumber(GetMV(Get_CustomerIDFromToken[token]));
			PrintOut("\n", 1);
		  
			PrintOut("Waiter ", 7);
			PrintNumber(ID);
			PrintOut(" validates the token number for Customer ", 41);
			PrintNumber(GetMV(Get_CustomerIDFromToken[token]));
			PrintOut("\n", 1);
		  
			PrintOut("Waiter ", 7);
			PrintNumber(ID);
			PrintOut(" serves food to Customer ", 25);
			PrintNumber(GetMV(Get_CustomerIDFromToken[token]));
			PrintOut("\n", 1);
		  
			/* Signal to the customer that the order is ready 
			 * Signal - Wait - Signal insures correct sequencing
			 */
			Acquire(lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])]);
				Signal(CV_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])],
						lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])]);
				Wait(CV_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])],
						lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])]);
				Signal(CV_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])],
						lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])]);

			Release(lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token])]);
		}
		Yield(5);
	}
}

int main()
{
  PrintOut("=== Waiter ===\n", 15);
  
  StartUserProgram();
  Initialize();  
  Waiter(0);
  return 0;  
}