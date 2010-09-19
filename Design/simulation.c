




/* =============================================================	
 * DATA
 * =============================================================*/	

/* conveniences */
int TRUE = 1;
int FALSE = 0;

/* Number of Agents in the simulation */
int count_NumOrderTakers;
int count_NumCustomers;
int count_NumCooks;
int count_NumManagers;


/* Line To Enter Restaurant */
int count_lineToEnterRestLength;
int max_lineToEnterRestLength;
int lineToEnterRest[max_lineToEnterRestLength ];
int lock_MrCr_LineToEnterRest;
int CV_MrCr_LineToEnterRestFromCustomerID[count_NumCustomers ];

/* Line To Order Food */
int count_lineToOrderFoodLength;
int max_lineToOrderFoodLength;
int lineToOrderFood[max_lineToOrderFoodLength ];
int lock_OrCr_LineToOrderFood;
int CV_OrCr_LineToOrderFoodFromCustomerID[count_NumCustomers ];

/* Used by the Customer routine */
int ID_Get_OrderTakerIDFromCustomerID[count_NumCustomers ];
int Get_CustomerOrderFoodChoiceFromOrderTakerID[count_NumOrderTakers ];
int Get_CustomerTogoOrEatinFromCustomerID[count_NumCustomers ];
int Get_CustomerMoneyPaidFromOrderTakerID[count_NumOrderTakers ];
int lock_OrderTakerBusy[count_NumOrderTakers ];
int CV_OrderTakerBusy[count_NumOrderTakers ];
int token_OrCr_OrderNumberFromCustomerID[count_NumCustomers ];
int lock_CustomerSitting[count_NumCustomers ]; 
int CV_CustomerSitting[count_NumCustomers ];
int CV_CustomerWaitingForFood[count_NumCustomers ];
int bool_ListOrdersReadyFromToken[count_NumCustomers ]; /* TO DO: initialize to zero */

/* Used by the Manager Routine */
int lock_MrWr;
int CV_MrWr;

int numInventoryItemTypes = 3;
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


/* MISC */
int money_Rest;
int count_NumTablesAvailable;
int count_NumOrderTokens;
int count_NumOrdersBaggedAndReady;
int lock_OrderReady;
int CV_OrderReady;
int count_NumFoodChoices;


/* =============================================================	
 * CUSTOMER
 * =============================================================*/	

/*-----------------------------
 * Top Level Customer Routine
 * ---------------------------*/
void Customer(int ID)
{
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
	/* wait for order taker to respond */
	wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	
	/*this is the money the customer gives to the orderTaker */
	Get_CustomerMoneyPaidFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]] 
				= Get_CostFromFoodChoice[Get_CustomerOrderFoodChoiceFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]]];
	
	signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]],
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);

	/* wait for order taker to respond */
	wait(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 			
					CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
          
	/* Order Taker got my order, and money */
	/* Order Taker gives me an order number in exchange */
	int token = token_OrCr_OrderNumberFromCustomerID[ID];
					
	if(eatIn)
	{
		/* we want the customer to be ready to receive the order */
		/* before we release the OrderTaker to process the order */
		acquire(lock_CustomerSitting[ID]);
		release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
		
		/* wait for order to be ready.  waiter will deliver it just to me, so I have my own condition variable. */
		wait(CV_CustomerSitting[ID], lock_CustomerSitting[ID]);
		/* I received my order */
		release(lock_CustomerSitting[ID]);
		sleep(1000); /* while eating for 1 second */
		
		/* make the table available again. */
		acquire(lock_MrCr_LineToEnterRest);
		count_NumTablesAvailable += 1;
		release(lock_MrCr_LineToEnterRest);
		
		/* leave store; */
		return;
	}
	else
	{
		/*prepare to recieve food before I let the order take go make it. */
		acquire(lock_OrderReady);
		/*let ordertaker go make food. */
		release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
		
		/* check if my order is ready */
		while(bool_ListOrdersReadyFromToken[token])
		{
			/* wait for an OrderReady broadcast */
			wait(CV_OrderReady, lock_OrderReady);
		}
		bool_ListOrdersReadyFromToken[token] = FALSE;
		release(lock_OrderReady);		
		
		/*leave store; */
		return;
	}
}


/*-----------------------------
 * Customers getting in line for the restaurant
 * ---------------------------*/
void waitInLineToEnterRest(int ID)
{
	acquire(lock_MrCr_LineToEnterRest);
	lineToEnterRest[count_lineToEnterRestLength] = ID;
	count_lineToEnterRestLength++;
	/* wait in line, and pass my ID as CV */
	wait(CV_MrCr_LineToEnterRestFromCustomerID[ID], lock_MrCr_LineToEnterRest);
	
	if (count_lineToEnterRestLength > 0)
	{
		count_lineToEnterRestLength--;
	}
	else
	{
		/* print error */
	}	
	release(lock_MrCr_LineToEnterRest);
}


/*-----------------------------
 * Customers getting in line to take an Order
 * ---------------------------*/
void waitInLineToOrderFood(int ID)
{
	acquire(lock_OrCr_LineToOrderFood);
	lineToOrderFood[count_lineToOrderFoodLength] = ID;
	count_lineToOrderFoodLength++;
	wait(CV_OrCr_LineToOrderFoodFromCustomerID[ID], lock_OrCr_LineToOrderFood);
	
	if (count_lineToOrderFoodLength > 0)
	{
		count_lineToOrderFoodLength--;
	}
	else
	{
		/* print error */
	}	
	/* make sure OrderTaker is ready to serve me */
	acquire(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	release(lock_OrCr_LineToOrderFood);
}

/* =============================================================	
 * MANAGER
 * =============================================================*/	


/*-----------------------------
 * Top Level Manager Routine
 * ---------------------------*/

void Manager()
{
	int ID = count_numOrderTakers++;
	count_numManagers++;	

	while(TRUE)
	{
		int helpOT = 0;

		acquire(lock_OrCr_LineToOrderFood);

		if(count_lineToOrderFoodLength > 3*)
			helpOT = 1;

		release(lock_OrCr_LineToOrderFood);

		if(helpOT)
		{
			for(int i = 0 ; i < 2; i++)
			{
				ServiceCustomer(ID);
			}
		}

		BagOrder(ID);
		
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
	acquire(lock_MrWr);
	
	if(count_NumOrdersBaggedAndReady > 0)	
		broadcast(CV_MrWr, lock_MrWr);

	release(lock_MrWr);
}

/*-----------------------------
 * Manager Ordering more food for the Restaurant
 * ---------------------------*/
void orderInventoryFood()
{
	
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		acquire(lock_MrCk_inventoryLocks[i]);
		
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

		release(lock_MrCk_inventoryLocks[i]);			
	}

}

/*-----------------------------
 * Manager managing Cook
 * ---------------------------*/
void manageCook()
{
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		acquire(lock_MrCk_InventoryLocks[i]);
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
		
		release(lock_MrCk_InventoryLocks[i]);
	}
}


/*-----------------------------
 * Manager hiring a new Cook
 * ---------------------------*/
void hireCook(index)
{
	/* acquire(lock_MrCk_InventoryLocks[index])  <-- this is already done! */

	acquire(lock_HireCook[index]);
	
	Get_CookIsHiredFromInventoryIndex[index] = -1;
	index_Ck_InventoryIndex = index;
	newThread(Cook);
	wait(CV_HireCook, lock_HireCook); /* don't continue until the new cook says he knows what he is cooking.*/
	
	release(lock_HireCook[index]);

	/*release(lock_MrCk_InventoryLocks[index])  <-- this will be done! */
}


/*-----------------------------
 * Manager Opening Spaces in line
 * ---------------------------*/
void checkLineToEnterRest()
{
	acquire(lock_MrCr_LineToEnterRest);
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
	release(lock_MrCr_LineToEnterRest);
}

/* =============================================================	
 * COOK
 * =============================================================*/	
 
 /*-----------------------------
 * Top Level Manager Routine
 * ---------------------------*/
void Cook()
{
	acquire(lock_HireCook);
	
	int ID = index_Ck_InventoryIndex;
	Get_CookIsHiredFromInventoryIndex[ID] = 1;

	signal(CV_HireCook, lock_HireCook); 
	
	release(lock_HireCook);

	/* I am Alive!!! */

	while(TRUE)
	{
		acquire(lock_MrCk_InventoryLocks[ID]);

		if(Get_CookOnBreakFromInventoryIndex[ID])
			wait(CV_MrCk_InventoryLocks[ID], lock_MrCk_InventoryLocks[ID])
		
		/* Cook something */
		if(inventoryCount[ID] > 0)
		{
			inventoryCount[ID]--;
		
			release(lock_MrCk_InventoryLocks[ID]);

			Yield(cookTime[ID]);
		
			acquire(lock_MrCk_InventoryLocks[ID]);
		
			cookedFoodStacks[ID]++;
		
		}

		release(lock_MrCk_InventoryLocks[ID]);
	}

}
 

/* =============================================================	
 * ORDER TAKER
 * =============================================================*/	





 
/* =============================================================	
 * WAITER
 * =============================================================*/	
void Waiter()
{
  while (true)
  {
    acquire(lock_BaggedOrders);
    wait(CV_BaggedOrders, lock_BaggedOrders);
    if (numTokens > 0)
    {
      order = waiterOrderTokens[numTokens - 1];
      numTokens--;
    }
    release(lock_BaggedOrders);
    acquire(CV_eatinCustomersWaitingForFood[customerNumberFromorderNumber[order]],
            lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[order]]);
    signal(lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[order]]);
    release(lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[order]]);
  }
}













 
/* =============================================================	
 * UTILITY METHODS
 * =============================================================*/	

int randomNumber(int count)
{
	return 4;
}

void printString(char[] s, int length)
{
	/* ??? */
}

void printNumber(int i)
{
	/* ??? */
}