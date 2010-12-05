/* manager.c
 * A manager for the restaurant simulation
 */

#include "syscall.h"
#include "simulation.c"


/* =============================================================	
 * MANAGER
 * =============================================================*/	

/*-----------------------------
 * Top Level Manager Routine
 * ---------------------------*/
void Manager(int debug)
{
  int i, ID, helpOT;
  
	Acquire(lock_Init_InitializationLock);
	  ID = GetMV(count_NumOrderTakers);
		SetMV(count_NumOrderTakers,GetMV(count_NumOrderTakers)+1);
		SetMV(count_NumManagers,GetMV(count_NumManagers)+1);
	  PrintOutV("Manager", 7);
	  PrintOutV("::Created - I\'m the boss.\n", 26);
	Release(lock_Init_InitializationLock);

  
	while(TRUE)
	{
		helpOT = 0;

		Acquire(lock_OrCr_LineToOrderFood);

			if( GetMV(count_lineToOrderFoodLength) > 3 * ( GetMV(count_NumOrderTakers) -  GetMV(count_NumManagers)))
			{
			  PrintOutV("Manager", 7);
			  PrintOutV("::Helping service customers\n", 28);
					helpOT = 1;
			}
    
		Release(lock_OrCr_LineToOrderFood);

		if (helpOT)
		{
			for (i = 0 ; i < 2; i++)
			{
				serviceCustomer(ID);
			}
		}
		bagOrder(TRUE);
		
		callWaiter(); /* Is this necessary? I think this is handled in bagOrder() */
		orderInventoryFood();
		manageCook();
		checkLineToEnterRest();
		
		/* Exit condition */
		if(shouldExit())
		{
			PrintOutV("Manager", 7);
			PrintOutV("::all customers served.\n", 24);
			Exit(0);
		}
		
		Yield(25);		
	}
}

/*-----------------------------
 * Manager Broadcasting to Waiters
 * ---------------------------*/
void callWaiter()
{
	Acquire(lock_MrWr);
	
	if( GetMV(count_NumOrdersBaggedAndReady )> 0)
	{
		Broadcast(CV_MrWr, lock_MrWr);
	}

	Release(lock_MrWr);
}

/*-----------------------------
 * Manager Ordering more food for the Restaurant
 * ---------------------------*/
void orderInventoryFood()
{
  int i, numBought;

	for(i = 0; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_InventoryLocks[i]);
		
		if(GetMV(inventoryCount[i] )<= 0)
		{
      PrintOutV("Manager",7);
      PrintOutV("::Food item #",13);
      PrintNumberV(i);
      PrintOutV(" out of stock\n",13);
			
			PrintOut("Manager refills the inventory\n", 30);
    
			numBought = 5;
			if(GetMV(money_Rest) < GetMV(inventoryCost[i])*numBought)
			{
        PrintOutV("Manager",7);
        PrintOutV("::Restaurant money ($",21);
        PrintNumberV(GetMV(money_Rest));
        PrintOutV(") too low for food item #",25);
        PrintNumberV(i);
        PrintOutV("\n",1);
				
				PrintOut("Manager goes to bank to withdraw the cash\n", 42);
        
				/* Yield(cookTime[i]*(5)); */ /* Takes forever */
        Yield(5);
				SetMV(money_Rest,GetMV(money_Rest) + GetMV(inventoryCost[i])*2*(numBought+5));
        
        PrintOutV("Manager",7);
        PrintOutV("::Went to bank. Restaurant now has $",36);
        PrintNumberV(GetMV(money_Rest));
        PrintOutV("\n",1);
				
			}

			SetMV(money_Rest,GetMV(money_Rest) - GetMV(inventoryCost[i])*numBought);
			inventoryCount[i] += numBought;

      PrintOutV("Manager",7);
      PrintOutV("::Purchasing ",13);
      PrintNumberV(numBought);
      PrintOutV(" of food item #",15);
      PrintNumberV(i);
      PrintOutV(". Restaurant now has $",22);
      PrintNumberV(GetMV(money_Rest));
      PrintOutV("\n",1);
			PrintOut("Inventory is loaded in the restaurant\n", 38);
		}

		Release(lock_MrCk_InventoryLocks[i]);			
	}
}

/*-----------------------------
 * Manager managing Cook
 * ---------------------------*/
void manageCook()
{
  int i;
  
  /* Start at i = 1 because we never want to hire a cook for Soda!!! */
	for(i = 1; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_InventoryLocks[i]);
		if(GetMV(cookedFoodStacks[i] )<GetMV( minCookedFoodStacks[i] )&& test_NoCooks == FALSE)
		{
			if(GetMV(Get_CookIsHiredFromInventoryIndex[i] )== 0)
			{
        
				/* Cook is not hired so hire new cook */
				hireCook(i);
			}
			else
			{
				if(GetMV(Get_CookOnBreakFromInventoryIndex[i]))
				{
					  PrintOutV("Manager",7);
					  PrintOutV("::Bringing cook #",14);
					  PrintNumberV(i);
					  PrintOutV(" off break\n",11);
        
					/* Cook is hired so Signal him to make sure he is not on break */
					SetMV(Get_CookOnBreakFromInventoryIndex[i] , FALSE);
				}
				Signal(CV_MrCk_InventoryLocks[i], lock_MrCk_InventoryLocks[i]);
			}
		}
		else if(GetMV(cookedFoodStacks[i] )> GetMV(maxCookedFoodStacks[i]))
		{
			if(!GetMV(Get_CookOnBreakFromInventoryIndex[i]))
			{
				PrintOutV("Manager",7);
				PrintOutV("::Sending cook #",16);
				PrintNumberV(i);
				PrintOutV(" on break\n",10);
        
				SetMV(Get_CookOnBreakFromInventoryIndex[i] , TRUE);
			}
		}
		Release(lock_MrCk_InventoryLocks[i]);
	}
}

/*-----------------------------
 * Manager hiring a new Cook
 * ---------------------------*/
void hireCook(int index)
{
	/* Acquire(lock_MrCk_InventoryLocks[index])  <-- this is already done! */
	
	Acquire(lock_HireCook);
	
	SetMV(Get_CookIsHiredFromInventoryIndex[index] , -1);
	SetMV(index_Ck_InventoryIndex , index);

	/*Fork((void *)Cook);*/
	/*Exec("../test/cook");*/

	Wait(CV_HireCook, lock_HireCook); /* don't continue until the new cook says he knows what he is cooking.*/
	

	Release(lock_HireCook);

	/*Release(lock_MrCk_InventoryLocks[index])  <-- this will be done! */
}

/*-----------------------------
 * Manager Opening Spaces in line
 * ---------------------------*/
void checkLineToEnterRest()
{
  int custID;
  
	Acquire(lock_MrCr_LineToEnterRest);
	
	custID = GetMV(lineToEnterRest[GetMV(count_lineToEnterRestLength)-1]);
	
	if (GetMV(count_lineToEnterRestLength) > 0)
	{
	
		if (GetMV(count_NumTablesAvailable) > 0)
		{
		
			PrintOut("Customer ", 9);
			PrintNumber(custID);
			PrintOut(" is informed by the Manager-the restaurant is ", 46);
			if (GetMV(count_NumTablesAvailable )> 0)
				PrintOut("not full\n", 9);
			else
				PrintOut("full\n", 5);
			
			if (GetMV(count_NumTablesAvailable )<= 0)
			{
				PrintOut("Customer ", 8);
				PrintNumber(custID);
				PrintOut(" is waiting to sit on the table\n", 32);
			}  
		
			SetMV(count_NumTablesAvailable ,GetMV(count_NumTablesAvailable )- 1);
			/* Signal to the customer to enter the restaurant */
			Signal(CV_MrCr_LineToEnterRestFromCustomerID[custID],
						lock_MrCr_LineToEnterRest);
		}
	}
	Release(lock_MrCr_LineToEnterRest);
}


int main()
{
  int i;
  PrintOut("=== Manager ===\n", 16);
  
  StartUserProgram();
  Initialize();  
  Manager(0);
  return 0;
}
