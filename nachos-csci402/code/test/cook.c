/* cook.c
 * A cook for the restaurant simulation
 */

#include "syscall.h"
#include "simulation.c"

/* =============================================================	
 * COOK
 * =============================================================*/	
 
 /*-----------------------------
 * Top Level Cook Routine
 * ---------------------------*/
void Cook(int debug)
{
	int ID, t;
  
	Acquire(lock_HireCook);
	
	ID = GetMV(index_Ck_InventoryIndex);
	SetMV(Get_CookIsHiredFromInventoryIndex[ID] , 1);
  
	Signal(CV_HireCook, lock_HireCook); 
	
	Release(lock_HireCook);

	/* I am Alive!!! */
	
	PrintOut("Manager informs Cook ", 21);
	PrintNumber(ID);
	PrintOut(" to cook ", 9);
	switch(ID)
			{
			/* [6-dollar burger/3-dollar burger/veggie burger/french fries] */
				case 1: PrintOut("french fries\n", 13); break;
				case 2: PrintOut("veggie burger\n", 14); break;
				case 3: PrintOut("3-dollar burger\n", 16); break;
				case 4: PrintOut("6-dollar burger\n", 16); break;
			}

	while(TRUE)
	{
		Acquire(lock_MrCk_InventoryLocks[ID]);
		t = 0;
		
		if(GetMV(Get_CookOnBreakFromInventoryIndex[ID]))
		{
			t = 1;
			PrintOut("Cook ", 5);
			PrintNumber(ID);
			PrintOut(" is going on break\n", 19);
			  
			Wait(CV_MrCk_InventoryLocks[ID], lock_MrCk_InventoryLocks[ID]);
			
			PrintOut("Cook ", 5);
			PrintNumber(ID);
			PrintOut(" returned from break\n", 21);
		}
		
		if(t == 1)
		{
			PrintOut("Cook ", 5);
			PrintNumber(ID);
			PrintOut(" is going on break\n", 19);
		}
		
		/* Cook something */
		if(GetMV(inventoryCount[ID]) > 0)
		{
			PrintOut("Cook ", 5);
			PrintNumber(ID);
			PrintOut(" is going to cook ", 18);
			switch(ID)
			{
			/* [6-dollar burger/3-dollar burger/veggie burger/french fries] */
				case 1: PrintOut("french fries\n", 13); break;
				case 2: PrintOut("veggie burger\n", 14); break;
				case 3: PrintOut("3-dollar burger\n", 16); break;
				case 4: PrintOut("6-dollar burger\n", 16); break;
			}
			
			SetMV(inventoryCount[ID],GetMV(inventoryCount[ID])-1);
			Release(lock_MrCk_InventoryLocks[ID]);
			
			Yield(GetMV(cookTime[ID]));
      
			Acquire(lock_MrCk_InventoryLocks[ID]);
			SetMV(cookedFoodStacks[ID],GetMV(cookedFoodStacks[ID])+1);
		}
		
		Release(lock_MrCk_InventoryLocks[ID]);
		
		/* Exit condition */
		if(shouldExit())
		{
			PrintOutV("Cook", 5);
			PrintOutV("::all customers served.\n", 24);
			Exit(0);
		}
		Yield(5);
	}

}


int main()
{
  PrintOut("=== Cook ===\n", 17);
  
  StartUserProgram();
  
  Cook(0);
  
}

