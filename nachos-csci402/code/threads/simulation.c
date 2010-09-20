
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

#define count_MaxNumOrderTakers 10
#define count_MaxNumCustomers 40
/* Number of Agents in the simulation */
int count_NumOrderTakers;
int count_NumCustomers;
int count_NumCooks;
int count_NumManagers;

/* Line To Enter Restaurant */
int count_lineToEnterRestLength;
int lineToEnterRest[count_MaxNumCustomers ];
int lock_MrCr_LineToEnterRest;
int CV_MrCr_LineToEnterRestFromCustomerID[count_MaxNumCustomers ];

/* Line To Order Food */
int count_lineToOrderFoodLength;
int lineToOrderFood[count_MaxNumCustomers ];
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

#define numInventoryItemTypes 5

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
int orderNumCounter;
int lock_OrdersNeedingBagging;
int ordersNeedingBagging[count_MaxNumCustomers ];
int baggedOrders[count_MaxNumCustomers ];
int count_NumOrdersBaggedAndReady;
int lock_OrWr_BaggedOrders;
int CV_OrWr_BaggedOrders;

/* Menu -- size = 2^count_NumFoodChoices
 *         Make sure to update this if you change count_numFoodChoices!!!
 */
#define count_NumFoodChoices = 5;
#define count_MaxNumMenuItems = 32;
int menu[count_numMenuItems];

/* MISC */
int money_Rest;
int count_NumTablesAvailable;
int count_NumOrderTokens;
int lock_Init_InitializationLock;

/* =============================================================	
 * Initialize functions
 * =============================================================*/
 
void Initialize()
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
  for (int i = 0; i < numInventoryItemTypes; ++i)
	lock_MrCk_InventoryLocks[i] = GetLock();
	
  lock_HireCook = GetLock();
  lock_OrdersNeedingBagging = GetLock();
  lock_OrWr_BaggedOrders = GetLock();
  lock_Init_InitializationLock = GetLock();

  /* Initialize CV's */
  CV_OrWr_BaggedOrders = GetCV();
  CV_HireCook = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_MrCk_InventoryLocks[i] = GetCV();
  CV_MrWr = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_CustomerWaitingForFood[i] = GetCV();
  CV_OrCr_OrderReady = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_CustomerSittingFromCustomerID[i] = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_OrderTakerBusy[i] = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_OrCr_LineToOrderFoodFromCustomerID[i] = GetCV();
  for (int i = 0; i < count_MaxNumCustomers; ++i)
    CV_MrCr_LineToEnterRestFromCustomerID[i] = GetCV();
    
  /* Initialize Monitor Variables and Globals */
	count_NumTablesAvailable = 10;
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		minCookedFoodStacks[i] = 2;
		maxCookedFoodStacks[i] = 5;
		cookedFoodStacks[i] = 0;
		cookTime[i] = 50;
		Get_CookIsHiredFromInventoryIndex[i ] = 0;
		Get_CookOnBreakFromInventoryIndex[i ] = 0;
	}
	count_NumOrderTakers = 0;
	count_NumCustomers = 0;
	count_NumCooks = 0;
	count_NumManagers = 0;
	count_lineToEnterRestLength = 0;
	count_lineToOrderFoodLength = 0;
	orderNumCounter = 0;
	for(int i = 0; i < count_MaxNumCustomers ; i += 1)
	{
		ordersNeedingBagging[i ] = 0;
		baggedOrders[i ] =0;
	}
	
	count_NumOrdersBaggedAndReady= 0;

  menu[0] = 1;   /* Soda */
  menu[1] = 2;   /* Fries */
  menu[2] = 3;   /* Fries, Soda */
  menu[3] = 4;   /* Veggie Burger */
  menu[4] = 5;   /* Veggie Burger, Soda */
  menu[5] = 6;   /* Veggie Burger, Fries */
  menu[6] = 7;   /* Veggie Burger, Fries, Soda */
  menu[7] = 8;   /* $3 Burger */
  menu[8] = 9;   /* $3 Burger, Soda */
  menu[9] = 10;  /* $3 Burger, Fries */
  menu[10] = 11; /* $3 Burger, Fries, Soda */
  menu[11] = 16; /* $6 Burger */
  menu[12] = 17; /* $6 Burger, Soda */
  menu[13] = 18; /* $6 Burger, Fries */
  menu[14] = 19; /* $6 Burger, Fries, Soda */
  
	money_Rest = 0;
	count_NumOrderTokens = 0;
}

/* =============================================================	
 * Runs the simulation
 * =============================================================*/
void RunSimulation(int scenario)
{
	PrintOut("Starting Simulation\n",20);
	PrintOut("Initializing...\n",16);
	Initialize();
	PrintOut("Initialized\n",12);
	PrintOut("Running Simulation...\n",22);
	
		//todo make menu;
		
	Fork((int)Manager);
	Fork((int)OrderTaker);
	Fork((int)OrderTaker);
	Fork((int)OrderTaker);
	Fork((int)OrderTaker);
	Fork((int)OrderTaker);
	Fork((int)Waiter);
	
	for(int i = 0; i < count_MaxNumCustomers ; i +=1)
	{
		Fork((int)Customer);
	}
	
}

/* =============================================================	
 * CUSTOMER
 * =============================================================*/
int kkk = 1;
/*-----------------------------
 * Top Level Customer Routine
 * ---------------------------*/
void Customer(int debug)
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumCustomers++;
	Release(lock_Init_InitializationLock);

	int eatIn = ID%2;//randomNumber(1);
	
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::I\'M ALIVE!!!\n",15);
	
	if(eatIn)
	{
		PrintOut("Customer",8);
		PrintNumber(ID);
		PrintOut("::Waiting In Line for eatingin\n",31);
		
		WaitInLineToEnterRest(ID);
	}
	
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Waiting In Line for food\n",27);
	WaitInLineToOrderFood(ID);
	/* at this point we are locked with the order taker */
	
	/* randomly pick an order and tell it to order taker */
	Get_CustomerOrderFoodChoiceFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]] = menu[randomNumber(5)];
	/* Tell OrderTaker whether eating in or togo */
	Get_CustomerTogoOrEatinFromCustomerID[ID] = eatIn;
	/* Ordered, Signal Order Taker - this represents talking to ordertaker */
	
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Giving order to OrderTaker #",30);
	PrintNumber(ID_Get_OrderTakerIDFromCustomerID[ID]);
	PrintOut("\n",1);
  
	Signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	
  /*
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Waiting for OrderTaker #",26);
	PrintNumber(ID_Get_OrderTakerIDFromCustomerID[ID]);
	PrintOut("\n",1);
	*/
  
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
	
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Paying OrderTaker #",21);
	PrintNumber(ID_Get_OrderTakerIDFromCustomerID[ID]);
	PrintOut("\n",1);
	
	/* this is the money the customer gives to the orderTaker */
	/* Customer pays double the inventory cost */
	Get_CustomerMoneyPaidFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[ID]] 
				= inventoryCost[Get_CustomerOrderFoodChoiceFromOrderTakerID[					
				ID_Get_OrderTakerIDFromCustomerID[ID]]] * 2; 
	
	Signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]],
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);

  /*
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Giving order to OrderTaker #",32);
	PrintNumber(ID_Get_OrderTakerIDFromCustomerID[ID]);
	PrintOut("\n",1);
	*/
  
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
          
	/* Order Taker got my order, and money */
	/* Order Taker gives me an order number in exchange */
	int token = token_OrCr_OrderNumberFromCustomerID[ID];
	PrintOut("Customer",8);
	PrintNumber(ID);
	PrintOut("::Waiting for order #",21);
  PrintNumber(token);
  PrintOut("\n",1);
  
	if(eatIn)
	{
    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatin - Sitting at table\n",27);

		/* we want the customer to be ready to receive the order */
		/* before we Release the OrderTaker to process the order */
		Acquire(lock_CustomerSittingFromCustomerID[ID]);
		Release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);

    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatin - Waiting for waiter\n",29);		

		/* Wait for order to be ready.  Waiter will deliver it just to me, so I have my own condition variable. */
		Wait(CV_CustomerSittingFromCustomerID[ID], lock_CustomerSittingFromCustomerID[ID]);
    
    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatin - Got order #",21);
    PrintNumber(token);
    PrintOut(" from waiter\n",13);
    
		/* I received my order */
		Release(lock_CustomerSittingFromCustomerID[ID]);
    
    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatin - Eating\n",17);
    
		Yield(10); /* while eating for 1 second */

    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatin - Done eating\n",22);
    
		/* make the table available again. */
		Acquire(lock_MrCr_LineToEnterRest);
		count_NumTablesAvailable += 1;
		Release(lock_MrCr_LineToEnterRest);
		
		PrintOut("Customer",9);
		PrintNumber(ID);
		PrintOut("::Eatin - Leaving Restaurant full and happy =)\n",47);
		
    /* leave restaurant */
	}
	else
	{    
		/*prepare to recieve food before I let the order take go make it. */
		Acquire(lock_OrCr_OrderReady);
		/*let ordertaker go make food. */
		Release(lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[ID]]);
		
    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatout - Ready for food\n",26);
    
		/* check if my order is ready */
    int hasCheckedOrder = 0; /* used for print statement */
		while(bool_ListOrdersReadyFromToken[token] == FALSE)
		{
      if (hasCheckedOrder > 0)
      {
        PrintOut("Customer",8);
        PrintNumber(ID);
        PrintOut("::Eatout - Order called was not #",33);
        PrintNumber(token);
        PrintOut("\n",1);
      }

			/* Wait for an OrderReady Broadcast */
			Wait(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
      hasCheckedOrder = 1;
      
      PrintOut("Customer",8);
      PrintNumber(ID);
      PrintOut("::Eatout - Checking if order called is #",40);
      PrintNumber(token);
      PrintOut("\n",1);
		}
    
    PrintOut("Customer",8);
    PrintNumber(ID);
    PrintOut("::Eatout - Taking order #",25);
    PrintNumber(token);
    PrintOut("\n",1);
    
		bool_ListOrdersReadyFromToken[token] = FALSE;
		Release(lock_OrCr_OrderReady);		
		
		PrintOut("Customer",8);
		PrintNumber(ID);
		PrintOut("::Eatout - Leaving Restaurant with food and happy =)\n",53);
    
		/* leave restaurant */
	}
	Acquire(lock_Init_InitializationLock);
	printf("%d\n",count_NumCustomers-(kkk++));
	Release(lock_Init_InitializationLock);
}

/*-----------------------------
 * Customers getting in line for the restaurant
 * ---------------------------*/
void WaitInLineToEnterRest(int ID)
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
	else  /* Should never get here */
	{
      PrintOut("Customer",8);
      PrintNumber(ID);
      PrintOut("::Something is wrong while getting out of line\n",47);
	}	
	Release(lock_MrCr_LineToEnterRest);
}

/*-----------------------------
 * Customers getting in line to take an Order
 * ---------------------------*/
void WaitInLineToOrderFood(int ID)
{
	Acquire(lock_OrCr_LineToOrderFood);
	lineToOrderFood[count_lineToOrderFoodLength] = ID;
	count_lineToOrderFoodLength++;
	Wait(CV_OrCr_LineToOrderFoodFromCustomerID[ID], lock_OrCr_LineToOrderFood);
	
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
void Manager(int debug)
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumOrderTakers++;
	count_NumManagers++;	
	Release(lock_Init_InitializationLock);

  PrintOut("Manager", 7);
  PrintNumber(ID);
  PrintOut("::I\'M ALIVE!!!\n", 15);
  
	while(TRUE)
	{
		int helpOT = 0;

		Acquire(lock_OrCr_LineToOrderFood);

		if(count_lineToOrderFoodLength > 3*(count_NumOrderTakers - count_NumManagers))
    {
      PrintOut("Manager", 7);
      PrintNumber(ID);
      PrintOut("::Helping service customers\n", 28);
			helpOT = 1;
    }
    
		Release(lock_OrCr_LineToOrderFood);

		if(helpOT)
		{
			for(int i = 0 ; i < 2; i++)
			{
				serviceCustomer(ID);
			}
		}

		bagOrder();
		
		callWaiter(); /* Is this necessary? I think this is handled in bagOrder() */
		orderInventoryFood();
		manageCook();
		checkLineToEnterRest();
		Yield(5);
	}
}

/*-----------------------------
 * Manager Broadcasting to Waiters
 * ---------------------------*/
void callWaiter()
{
	Acquire(lock_MrWr);
	
	if(count_NumOrdersBaggedAndReady > 0)	
		Broadcast(CV_MrWr, lock_MrWr);

	Release(lock_MrWr);
}

/*-----------------------------
 * Manager Ordering more food for the Restaurant
 * ---------------------------*/
void orderInventoryFood()
{
	
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_InventoryLocks[i]);
		
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

		Release(lock_MrCk_InventoryLocks[i]);			
	}

}

/*-----------------------------
 * Manager managing Cook
 * ---------------------------*/
void manageCook()
{
	//printf("manageCook");
	for(int i = 0; i < numInventoryItemTypes; i += 1)
	{
		Acquire(lock_MrCk_InventoryLocks[i]);
		if(cookedFoodStacks[i] < minCookedFoodStacks[i])
		{
			if(Get_CookIsHiredFromInventoryIndex[i] == 0)
			{
				printf("hireCook\n");
				//Cook is not hired so hire new cook
				hireCook(i);
			}
			else
			{
				if(Get_CookOnBreakFromInventoryIndex[i])
				{
					printf("wakeUpCook\n");
					//cook is hired so Signal him to make sure he is not on break;
					Get_CookOnBreakFromInventoryIndex[i] = FALSE;
				}
				Signal(CV_MrCk_InventoryLocks[i], lock_MrCk_InventoryLocks[i]);
			}
		}
		else if(cookedFoodStacks[i] > maxCookedFoodStacks[i])
		{
		
			if(!Get_CookOnBreakFromInventoryIndex[i])
			{
				printf("sleepCook\n");
				Get_CookOnBreakFromInventoryIndex[i] = TRUE;
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
	
	Get_CookIsHiredFromInventoryIndex[index] = -1;
	index_Ck_InventoryIndex = index;
	Fork((int)Cook);
	Wait(CV_HireCook, lock_HireCook); /* don't continue until the new cook says he knows what he is cooking.*/
	
	Release(lock_HireCook);

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
			/* Signal to the customer to enter the restaurant */
			Signal(CV_MrCr_LineToEnterRestFromCustomerID[lineToEnterRest[count_lineToEnterRestLength-1]],
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
void Cook(int debug)
{
	Acquire(lock_HireCook);
	
	int ID = index_Ck_InventoryIndex;
	Get_CookIsHiredFromInventoryIndex[ID] = 1;

  PrintOut("Cook", 4);
  PrintNumber(ID);
  PrintOut("::I\'M ALIVE!!!\n", 15);
  
	Signal(CV_HireCook, lock_HireCook); 
	
	Release(lock_HireCook);

	/* I am Alive!!! */

	while(TRUE)
	{
		Acquire(lock_MrCk_InventoryLocks[ID]);

		if(Get_CookOnBreakFromInventoryIndex[ID])
    {
      PrintOut("Cook", 4);
      PrintNumber(ID);
      PrintOut("::Going on break\n", 17);
      
			Wait(CV_MrCk_InventoryLocks[ID], lock_MrCk_InventoryLocks[ID]);
    }
		
		/* Cook something */
		if(inventoryCount[ID] > 0)
		{
      //PrintOut("Cook", 4);
      //PrintNumber(ID);
      //PrintOut("::Cooking...\n", 13);
      
			inventoryCount[ID]--;
			Release(lock_MrCk_InventoryLocks[ID]);
			
      Yield(cookTime[ID]);
      
			Acquire(lock_MrCk_InventoryLocks[ID]);
			cookedFoodStacks[ID]++;
		
      PrintOut("Cook", 4);
      PrintNumber(ID);
      PrintOut("::Food Ready +1\n", 13);
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
void OrderTaker(int debug)
{
	Acquire(lock_Init_InitializationLock);
	int ID = count_NumOrderTakers++;
	PrintOut("OrderTaker ", 11);
	PrintNumber(ID);
	PrintOut(":: Created\n", 11);	
	Release(lock_Init_InitializationLock);
	printf("OrderTaker %d",ID);
	
	while(TRUE)
	{
		serviceCustomer(ID);
		bagOrder();
		Yield(100);
	}
}

/*-----------------------------
 * If there are any customers Waiting in line, 
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
	count_lineToOrderFoodLength--;
	ID_Get_OrderTakerIDFromCustomerID[custID] = ID;
	Signal(CV_OrCr_LineToOrderFoodFromCustomerID[custID], lock_OrCr_LineToOrderFood); 
	Acquire(lock_OrderTakerBusy[ID]);
	Release(lock_OrCr_LineToOrderFood);
	
	/* Ask the customer what he would like to order */
	PrintOut("OrderTaker ", 11);
	PrintNumber(ID);
	PrintOut(":: What would you like to order Customer ", 41);
	PrintNumber(custID);
	PrintOut("?\n", 2);
	
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	/* Customer has placed order by the time we get here. */

	Signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	/* Customer has paid by the time we get here. */

	/* Give the customer's order an order number. */
	int token = orderNumCounter++;
	token_OrCr_OrderNumberFromCustomerID[custID] = token;
	
	PrintOut("OrderTaker ", 11);
	PrintNumber(ID);
	PrintOut(":: Thanks for the order. Your token number is: ", 47);
	PrintNumber(token);
	PrintOut("\n", 1);
	
	/* Tell all Waiters the token too */
	Get_CustomerIDFromToken[token] = custID;
	/* Tell the customer to pick up the order number. */
	Signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	
	/* Add order list of orders needing to get bagged. */
	Acquire(lock_OrdersNeedingBagging);
	ordersNeedingBagging[token] = Get_CustomerOrderFoodChoiceFromOrderTakerID[ID];
	PrintOut("OrderTaker ", 11);
	PrintNumber(ID);
	PrintOut(":: Order ", 9);
	PrintNumber(token);
	PrintOut(" needs to be bagged.\n", 21);
	Release(lock_OrdersNeedingBagging);

	Release(lock_OrderTakerBusy[ID]);

}

/*-----------------------------
 * If there are any orders Waiting to be bagged:
 * - add the required food (if it's cooked)
 * - when an order becomes fully bagged:
 *   - if the customer is eat-in, Broadcast the Waiters
 *   - if the customer is togo, Broadcast the Waiting togo customers
 * ---------------------------*/
void bagOrder()
{
    Acquire(lock_OrdersNeedingBagging);
    for (int i = 0; i < count_MaxNumCustomers; i++)
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
			  
			  PrintOut("Bagging Order #", 15);
			  PrintNumber(i);
            }
        }
        
        /* If we just completely bagged order i. */
        if (ordersNeedingBagging[i] <= 1)
        {
          /* If the customer for order i is an eatin customer (i.e. he is Waiting at a table). */
          if (Get_CustomerTogoOrEatinFromCustomerID[Get_CustomerIDFromToken[i]] == 1)
          {
            /* Tell Waiters there is a bagged order to be delivered. */
            Acquire(lock_OrWr_BaggedOrders);
            /* Store the token */
              baggedOrders[count_NumOrdersBaggedAndReady] = i;
              count_NumOrdersBaggedAndReady++;
              Broadcast(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
			  printf("EatInOrderReady\n");
            Release(lock_OrWr_BaggedOrders);
          }
          else /* if the customer or order i is a togo customer. */
          {
            /* Tell the Waiting togo customers that order i is ready. */
            Acquire(lock_OrCr_OrderReady);
              bool_ListOrdersReadyFromToken[i] = 1;
              Broadcast(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
			printf("Eat Out Order Up\n");
			Release(lock_OrCr_OrderReady);
          }
        }
      }
    }
	Release(lock_OrdersNeedingBagging);
}
 
/* =============================================================	
 * WaitER
 * =============================================================*/	
void Waiter(int debug)
{
	while (TRUE)
	{
		int token = -1;
		Acquire(lock_OrWr_BaggedOrders);
		/* Wait to get Signaled by the Order Taker*/
		PrintOut("Waiter::Waiting for order to deliver\n", 37);
		
		while(count_NumOrdersBaggedAndReady <= 0)
		{
			Wait(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
		}
		
		PrintOut("Waiter::Received broadcast\n", 27);
		
		/* Grab the order that is ready */
		token = baggedOrders[count_NumOrdersBaggedAndReady - 1];
		PrintOut("Waiter::Grabbing order number: ", 31);
		PrintNumber(token);
		PrintOut("\n", 1);
		count_NumOrdersBaggedAndReady--;
		
		Release(lock_OrWr_BaggedOrders);
		
		if (token != -1)
		{
		  /* Signal to the customer that the order is ready */
		  PrintOut("Waiter::Giving order number: ", 31);
		  PrintNumber(token);
		  PrintOut("to customer number: ", 20);
		  PrintNumber(Get_CustomerIDFromToken[token]);
		  
		  Acquire(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
		  Signal(CV_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]],
				lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
		  Release(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
		}
	}
}

