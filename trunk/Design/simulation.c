
#include "simulation.h"
#include "synch.h"

/*
 * simulation.c
 * Implementation of the Restaurant Simulation
 * CSCI 402 Fall 2010
 *
 */


/* =============================================================	
 * DATA
 * =============================================================*/	

/* conveniences */
int TRUE = 1;
int FALSE = 0;

/* Number of Agents in the simulation */
int count_MaxNumOrderTakers;
int count_NumOrderTakers;
int count_MaxNumCustomers;
int count_NumCustomers;
int count_NumCooks;
int count_NumManagers;

/* Line To Enter Restaurant */
int count_lineToEnterRestLength;
int max_lineToEnterRestLength;
int lineToEnterRest[max_lineToEnterRestLength ];
int lock_MrCr_LineToEnterRest;
int CV_MrCr_LineToEnterRestFromCustomerID[count_MaxNumCustomers ];

/* Line To Order Food */
int count_lineToOrderFoodLength;
int max_lineToOrderFoodLength;
int lineToOrderFood[max_lineToOrderFoodLength ];
int lock_OrCr_LineToOrderFood;
int CV_OrCr_LineToOrderFoodFromCustomerID[count_MaxNumCustomers ];

/* Used by the Customer routine */
int ID_Get_OrderTakerIDFromCustomerID[count_MaxNumCustomers ];
int Get_CustomerOrderFoodChoiceFromOrderTakerID[count_MaxNumOrderTakers ];
int Get_CustomerTogoOrEatinFromCustomerID[count_MaxNumCustomers ];
int Get_CustomerMoneyPaidFromOrderTakerID[count_MaxNumOrderTakers ];
int lock_OrderTakerBusy[count_MaxNumOrderTakers ];
int CV_OrderTakerBusy[count_MaxNumOrderTakers ];
int token_OrCr_OrderNumberFromCustomerID[count_MaxNumCustomers ];
int lock_CustomerSittingFromCustomerID[count_MaxNumCustomers ]; 
int CV_CustomerSittingFromCustomerID[count_MaxNumCustomers ];
/* Eat out customers*/
int lock_OrCr_OrderReady;
int CV_OrCr_OrderReady;
int CV_CustomerWaitingForFood[count_MaxNumCustomers ];
int bool_ListOrdersReadyFromToken[count_MaxNumCustomers ]; /* TO DO: initialize to zero */

/* Used by the Waiter Routine */
int Get_CustomerIDFromToken[count_MaxNumCustomers ];

/* Used by the Manager Routine */
int lock_MrWr;
int CV_MrWr;

int numInventoryItemTypes = 5;
int inventoryCount[numInventoryItemTypes ];
int inventoryCost[numInventoryItemTypes ];

int cookedFoodStacks[numInventoryItemTypes ];
int minCookedFoodStacks[numInventoryItemTypes ];
int maxCookedFoodStacks[numInventoryItemTypes ];
int cookTime[numInventoryItemTypes ];

int Get_CookIsHiredFromInventoryIndex[numInventoryItemTypes ];
int Get_CookOnBreakFromInventoryIndex[numInventoryItemTypes ];
int lock_MrCk_InventoryLocks[numInventoryItemTypes ];
int CV_MrCk_InventoryLocks[numInventoryItemTypes ];

int lock_HireCook;
int CV_HireCook;
int index_Ck_InventoryIndex;

/* Used by the Order Taker Routine */
int orderNumCounter = 0;
int lock_OrdersNeedingBagging;
int ordersNeedingBagging[count_MaxNumCustomers ];
int baggedOrders[count_MaxNumCustomers ];
int count_NumOrdersBaggedAndReady = 0;
int lock_OrWr_BaggedOrders;
int CV_OrWr_BaggedOrders;

/* MISC */
int money_Rest;
int count_NumTablesAvailable;
int count_NumOrderTokens;
int count_NumFoodChoices;
int lock_Init_InitializationLock;

/* =============================================================	
 * Initializes and runs the simulation
 * =============================================================*/
void RunSimulation()
{
  /* Initialize Locks */
  lock_MrCr_LineToEnterRest = GetLock();
  lock_OrCr_LineToOrderFood = GetLock();
  for (int i = 0; i < count_MaxNumOrderTakers; ++i)
    lock_OrderTakerBusy[i] = GetLock();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    lock_CustomerSittingFromCustomerID[i] = GetLock();
  lock_OrCr_OrderReady = GetLock();
  lock_MrWr = GetLock();
  lock_MrCk_InventoryLocks = GetLock();
  lock_HireCook = GetLock();
  lock_OrdersNeedingBagging = GetLock();
  lock_OrWr_BaggedOrders = GetLock();
  lock_Init_InitializationLock = GetLock();
}

/* =============================================================	
 * CUSTOMER
 * =============================================================*/

/*-----------------------------
 * Top Level Customer Routine
 * ---------------------------*/
void Customer(int ID)
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumCustomers++;
	Release(lock_Init_InitializationLock);

	int eatIn = randomNumber(1);
	
	if(eatIn)
	{
		waitInLineToEnterRest(ID);
	}
	
	waitInLineToOrderFood(ID);
	/* at this point we are locked with the order taker */
	
	
	/* randomly pick an order and tell it to order taker */
	Get_CustomerOrderFoodChoiceFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]] = randomNumber(count_NumFoodChoices);
	/* Tell OrderTaker whether eating in or togo */
	Get_CustomerTogoOrEatinFromCustomerID[ID] = eatIn;
	/* Ordered, signal Order Taker - this represents talking to ordertaker */
	signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	
	/* this is the money the customer gives to the orderTaker */
	Get_CustomerMoneyPaidFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]] 
				= Get_CostFromFoodChoice[Get_CustomerOrderFoodChoiceFromOrderTakerID[					
				ID_Get_OrderTakerIDFromCustomerID[ID]]];
	
	signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]],
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);

	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
          
	/* Order Taker got my order, and money */
	/* Order Taker gives me an order number in exchange */
	int token = token_OrCr_OrderNumberFromCustomerID[ID];
					
	if(eatIn)
	{
		/* we want the customer to be ready to receive the order */
		/* before we Release the OrderTaker to process the order */
		Acquire(lock_CustomerSittingFromCustomerID[ID]);
		Release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
		
		/* Wait for order to be ready.  waiter will deliver it just to me, so I have my own condition variable. */
		Wait(CV_CustomerSittingFromCustomerID[ID], lock_CustomerSittingFromCustomerID[ID]);
		/* I received my order */
		Release(lock_CustomerSittingFromCustomerID[ID]);
		sleep(1000); /* while eating for 1 second */
		
		/* make the table available again. */
		Acquire(lock_MrCr_LineToEnterRest);
		count_NumTablesAvailable += 1;
		Release(lock_MrCr_LineToEnterRest);
		
		/* leave store; */
		return;
	}
	else
	{
		/*prepare to recieve food before I let the order take go make it. */
		Acquire(lock_OrCr_OrderReady);
		/*let ordertaker go make food. */
		Release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
		
		/* check if my order is ready */
		while(bool_ListOrdersReadyFromToken[token])
		{
			/* Wait for an OrderReady broadcast */
			Wait(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
		}
		bool_ListOrdersReadyFromToken[token] = FALSE;
		Release(lock_OrCr_OrderReady);		
		
		/*leave store; */
		return;
	}
}


/*-----------------------------
 * Customers getting in line for the restaurant
 * ---------------------------*/
void waitInLineToEnterRest(int ID)
{
	Acquire(lock_MrCr_LineToEnterRest);
	lineToEnterRest[count_lineToEnterRestLength] = ID;
	count_lineToEnterRestLength++;
	/* Wait in line, and pass my ID as CV */
	Wait(CV_MrCr_LineToEnterRestFromCustomerID[ID], lock_MrCr_LineToEnterRest);
	
	if (count_lineToEnterRestLength > 0)
	{
		count_lineToEnterRestLength--;
	}
	else
	{
		/* print error */
	}	
	Release(lock_MrCr_LineToEnterRest);
}


/*-----------------------------
 * Customers getting in line to take an Order
 * ---------------------------*/
void waitInLineToOrderFood(int ID)
{
	Acquire(lock_OrCr_LineToOrderFood);
	lineToOrderFood[count_lineToOrderFoodLength] = ID;
	count_lineToOrderFoodLength++;
	Wait(CV_OrCr_LineToOrderFoodFromCustomerID[ID], lock_OrCr_LineToOrderFood);
	
	if (count_lineToOrderFoodLength > 0)
	{
		count_lineToOrderFoodLength--;
	}
	else
	{
		/* print error */
	}	
	/* make sure OrderTaker is ready to serve me */
	Acquire(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	Release(lock_OrCr_LineToOrderFood);
}

/* =============================================================	
 * MANAGER
 * =============================================================*/	


/*-----------------------------
 * Top Level Manager Routine
 * ---------------------------*/

void Manager()
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumOrderTakers++;
	count_numManagers++;	
	Release(lock_Init_InitializationLock);

	while(TRUE)
	{
		int helpOT = 0;

		Acquire(lock_OrCr_LineToOrderFood);

		if(count_lineToOrderFoodLength > 3*)
			helpOT = 1;

		Release(lock_OrCr_LineToOrderFood);

		if(helpOT)
		{
			for(int i = 0 ; i < 2; i++)
			{
				ServiceCustomer(ID);
			}
		}

		bagOrder(ID);
		
		callWaiter();
		orderInventoryFood();
		manageCook();
		checkLineToEnterRest();
	}
}


/*-----------------------------
 * Manager Broadcasting to Waiters
 * ---------------------------*/
void callWaiter()
{
	Acquire(lock_MrWr);
	
	if(count_NumOrdersBaggedAndReady > 0)	
		broadcast(CV_MrWr, lock_MrWr);

	Release(lock_MrWr);
}

/*-----------------------------
 * Manager Ordering more food for the Restaurant
 * ---------------------------*/
void orderInventoryFood()
{
	
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_inventoryLocks[i]);
		
		if(inventoryCount[i] <= 0)
		{
			int numBought = 5;
			if(money_Rest < inventoryCost[i]*numBought)
			{
				Yield(cookTime[i]*(5));
				money_Rest += inventoryCost[i]*2*(numBought+5);
			}

			money_Rest -= inventoryCost[i]*numBought;
			inventoryCount[i] += numBought;
			
		}

		Release(lock_MrCk_inventoryLocks[i]);			
	}

}

/*-----------------------------
 * Manager managing Cook
 * ---------------------------*/
void manageCook()
{
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_InventoryLocks[i]);
		if(cookedFoodStacks[i] < minCookedFoodStacks[i])
		{
			if(Get_CookIsHiredFromInventoryIndex[i] == 0)
			{
				//Cook is not hired so hire new cook
				hireCook(i);
			}
			else
			{
				//cook is hired so signal him to make sure he is not on break;
				Get_CookOnBreakFromInventoryIndex[i] = FALSE;
				signal(CV_MrCk_InventoryLocks[i], lock_MrCk_InventoryLocks[i]);
			}
		}
		else if(cookedFoodStacks[i] > maxCookedFoodStacks[i])
		{
			Get_CookOnBreakFromInventoryIndex[i] = TRUE;
		}
		
		Release(lock_MrCk_InventoryLocks[i]);
	}
}


/*-----------------------------
 * Manager hiring a new Cook
 * ---------------------------*/
void hireCook(index)
{
	/* Acquire(lock_MrCk_InventoryLocks[index])  <-- this is already done! */

	Acquire(lock_HireCook[index]);
	
	Get_CookIsHiredFromInventoryIndex[index] = -1;
	index_Ck_InventoryIndex = index;
	Fork(Cook);
	Wait(CV_HireCook, lock_HireCook); /* don't continue until the new cook says he knows what he is cooking.*/
	
	Release(lock_HireCook[index]);

	/*Release(lock_MrCk_InventoryLocks[index])  <-- this will be done! */
}


/*-----------------------------
 * Manager Opening Spaces in line
 * ---------------------------*/
void checkLineToEnterRest()
{
	Acquire(lock_MrCr_LineToEnterRest);
	if (count_lineToEnterRestLength > 0)
	{
		if (count_NumTablesAvailable > 0)
		{
			count_NumTablesAvailable -= 1;
			/* signal to the customer to enter the restaurant */
			signal(CV_MrCr_LineToEnterRestFromCustomerID[lineToEnterRest[count_lineToEnterRestLength-1]],
						lock_MrCr_LineToEnterRest);
		}
	}
	Release(lock_MrCr_LineToEnterRest);
}

/* =============================================================	
 * COOK
 * =============================================================*/	
 
 /*-----------------------------
 * Top Level Cook Routine
 * ---------------------------*/
void Cook()
{
	Acquire(lock_HireCook);
	
	int ID = index_Ck_InventoryIndex;
	Get_CookIsHiredFromInventoryIndex[ID] = 1;

	signal(CV_HireCook, lock_HireCook); 
	
	Release(lock_HireCook);

	/* I am Alive!!! */

	while(TRUE)
	{
		Acquire(lock_MrCk_InventoryLocks[ID]);

		if(Get_CookOnBreakFromInventoryIndex[ID])
			Wait(CV_MrCk_InventoryLocks[ID], lock_MrCk_InventoryLocks[ID])
		
		/* Cook something */
		if(inventoryCount[ID] > 0)
		{
			inventoryCount[ID]--;
		
			Release(lock_MrCk_InventoryLocks[ID]);

			Yield(cookTime[ID]);
		
			Acquire(lock_MrCk_InventoryLocks[ID]);
		
			cookedFoodStacks[ID]++;
		
		}

		Release(lock_MrCk_InventoryLocks[ID]);
	}

}
 

/* =============================================================	
 * ORDER TAKER
 * =============================================================*/	

 /*-----------------------------
 * Top Level OrderTaker Routine
 * ---------------------------*/
void OrderTaker()
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumOrderTakers++;
	Release(lock_Init_InitializationLock);
	
	while(TRUE)
	{
		serviceCustomer(ID);
		bagOrder(ID);
	}
}


/*-----------------------------
 * If there are any customers waiting in line, 
 * get one out of line and take his order.
 * ---------------------------*/
void serviceCustomer(int ID)
{
	Acquire(lock_OrCr_LineToOrderFood);
	if (count_lineToOrderFoodLength <= 0)
	{
		/* no one in line */
		Release(lock_OrCr_LineToOrderFood);
		return;
	}
	/* Get a customer out of line and tell him he's our bitch so he can order. */
	int custID = lineToOrderFood[count_lineToOrderFoodLength - 1];
	ID_Get_OrderTakerIDFromCustomerID[custID] = ID;
	signal(CV_OrCr_LineToOrderFoodFromCustomerID[custID], lock_OrCr_LineToOrderFood); 
	Acquire(lock_OrderTakerBusy[ID]);
	Release(lock_OrCr_LineToOrderFood);
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	/* Customer has placed order by the time we get here. */

	signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	/* Customer has paid by the time we get here. */

	/* Give the customer's order an order number. */
	int token = orderNumCounter++;
	token_OrCr_OrderNumberFromCustomerID[custID] = token;
	/* Tell all Waiters the token too */
	Get_CustomerIDFromToken[token] = custID;
	/* Tell the customer to pick up the order number. */
	signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	
	/* Add order list of orders needing to get bagged. */
	Acquire(lock_OrdersNeedingBagging);
	ordersNeedingBagging[token] = Get_CustomerOrderFoodChoiceFromOrderTakerID[ID];
	Release(lock_OrdersNeedingBagging);

	Release(lock_OrderTakerBusy[ID]);
	}

}

/*-----------------------------
 * If there are any orders waiting to be bagged:
 * - add the required food (if it's cooked)
 * - when an order becomes fully bagged:
 *   - if the customer is eat-in, broadcast the waiters
 *   - if the customer is togo, broadcast the waiting togo customers
 * ---------------------------*/
void bagOrder()
{
    Acquire(lock_OrdersNeedingBagging);
    for (int i = 0; i < maxNumOrders; i++)
    {
      /* If order i still needs food to be fully bagged. */
      if (ordersNeedingBagging[i] > 0)
      {
        /* Check if each item the order needs has been added to the bag. */
        /* Start at 1 because FoodType 0 is Soda (which we can assume will be always immediately available). */
        for (int j = 1; j < numInventoryItemTypes; j++)
        {
            if (((ordersNeedingBagging[i] >> j) > 0) && (cookedFoodStacks[j] > 0))
            {
              ordersNeedingBagging[i] ^= (1 << j);
              cookedFoodStacks[j]--;
            }
        }
        
        /* If we just completely bagged order i. */
        if (ordersNeedingBagging[i] <= 1)
        {
          /* If the customer for order i is an eatin customer (i.e. he is waiting at a table). */
          if (Get_CustomerTogoOrEatinFromCustomerID[GET_CustomerIDFromOrderNumber[i]] == 1)
          {
            /* Tell waiters there is a bagged order to be delivered. */
            Acquire(lock_OrWr_BaggedOrders);
			  /* Store the token */
              baggedOrders[count_NumOrdersBaggedAndReady] = i;
              count_NumOrdersBaggedAndReady++;
              broadcast(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
            Release(lock_OrWr_BaggedOrders);
          }
          else /* if the customer or order i is a togo customer. */
          {
            /* Tell the waiting togo customers that order i is ready. */
            Acquire(lock_OrCr_OrderReady);
              bool_ListOrdersReadyFromToken[i] = 1;
              broadcast(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
            Release(lock_OrCr_OrderReady);
          }
        }
      }
    }
	Release(lock_OrdersNeedingBagging);
}


 
/* =============================================================	
 * WAITER
 * =============================================================*/	
void Waiter()
{
	while (TRUE)
	{
		int token;
		Acquire(lock_OrWr_BaggedOrders);
		/* Wait to get signaled by the Order Taker*/
		Wait(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
		if (count_NumOrdersBaggedAndReady > 0)
		{
			/* Grab the order that is ready */
			token = baggedOrders[count_NumOrdersBaggedAndReady - 1];
			count_NumOrdersBaggedAndReady--;
		}
		Release(lock_OrWr_BaggedOrders);
		
		/* Signal to the customer that the order is ready */
		Acquire(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
		signal(CV_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]],
			   lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
		Release(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
	}
}
 
/* =============================================================	
 * UTILITY METHODS
 * =============================================================*/	

int randomNumber(int count)
{
	return 4;
}

void PrintOut(unsigned int vaddr, int len)
{
	char *buf;	/* Kernel buffer for output */

	if ( !(buf = new char[len]) ) 
	{
		printf("%s","Error allocating kernel buffer for write!\n");
		return;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to write: data not written\n");
			delete[] buf;
			return;
		}
	}

	for (int ii=0; ii<len; ii++) 
	{
		printf("%c",buf[ii]);
	}

	delete[] buf;
}


void PrintNumber(int number)
{
	printf("%d", number);
}