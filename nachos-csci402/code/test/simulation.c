
#ifndef CHANGED
  #define CHANGED
#endif

#ifdef CHANGED
#include "simulation.h"
/* #include "synch.h" */
#include "syscall.h"

/*
 * simulation.cc
 * Implementation of the Restaurant Simulation
 * CSCI 402 Fall 2010
 *
 */

 /* =============================================================	
 * DATA
 * =============================================================*/	
 
 /* Number of Agents in the simulation */
int count_NumOrderTakers;
int count_NumCustomers;
int count_NumCooks;
int count_NumManagers;
int count_NumWaiters;

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
int bool_ListOrdersReadyFromToken[count_MaxNumCustomers ];
int count_NumCustomersServed;
int count_WhenAllCustomersServed;
/* Used by the Waiter Routine */
int Get_CustomerIDFromToken[count_MaxNumCustomers ];

/* Used by the Manager Routine */
int lock_MrWr;
int CV_MrWr;


/* Menu -- size = 2^count_MaxNumMenuItems
 */
int menu[count_MaxNumMenuItems];

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

/* MISC */
int money_Rest;
int count_NumTablesAvailable;
int count_NumOrderTokens;
int lock_Init_InitializationLock;

/* TEST */
int test_AllCustomersEatIn;
int test_AllCustomersEatOut;
int test_NoCooks;
int test_AllCustomersOrderThisCombo;
 
/* =============================================================	
 * Initialize functions
 * =============================================================*/

void Initialize()
{
  int i;
  char str[40];

  /* Initialize Locks */
  lock_MrCr_LineToEnterRest = CreateLock("lock_MrCr_LineToEnterRest", 25);
  lock_OrCr_LineToOrderFood = CreateLock("lock_OrCr_LineToOrderFood", 25);
  
  for (i = 0; i < count_MaxNumOrderTakers; ++i)
  {
	str = "lock_OrderTakerBusy";
	str[19] = char(i);
    lock_OrderTakerBusy[i] = CreateLock(str,20);
	}
	
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str = "lock_CustomerSittingFromCustomerID";
	str[34] = char(i);
    lock_CustomerSittingFromCustomerID[i] = CreateLock(str, 34);
  }
  
  lock_OrCr_OrderReady = CreateLock("lock_OrCr_OrderReady", 20);
  
  lock_MrWr = CreateLock("lock_MrWr", 9);
  for (i = 0; i < numInventoryItemTypes; ++i)
  {
	str = "lock_MrCk_InventoryLocks";
	str[24] = char(i);
	lock_MrCk_InventoryLocks[i] = CreateLock(str, 25);
  }
	
  lock_HireCook = CreateLock("lock_HireCook", 13);
  lock_OrdersNeedingBagging = CreateLock("lock_OrdersNeedingBagging", 25);
  lock_OrWr_BaggedOrders = CreateLock("lock_OrWr_BaggedOrders", 22);
  lock_Init_InitializationLock = CreateLock("lock_Init_InitializationLock", 28);

  /* Initialize CV's */
  CV_OrWr_BaggedOrders = CreateCondition("CV_OrWr_BaggedOrders", 20);
  CV_HireCook = CreateCondition("CV_HireCook", 11);
  for (i = 0; i < count_MaxNumCustomers; ++i)
	{
	str = "CV_MrCk_InventoryLocks";
	str[22] = char(i);
    CV_MrCk_InventoryLocks[i] = CreateCondition(str, 23);
	}
  CV_MrWr = CreateCondition("CV_MrWr", 7);
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str= "CV_CustomerWaitingForFood";
	str[25] = char(i);
    CV_CustomerWaitingForFood[i] = CreateCondition(str, 26);
  }
  CV_OrCr_OrderReady = CreateCondition("CV_OrCr_OrderReady", 18);
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str = "CV_CustomerSittingFromCustomerID";
	str[32] = char(i);
    CV_CustomerSittingFromCustomerID[i] = CreateCondition(str, 33);
	}
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str = "CV_OrderTakerBusy";
	str[17] = char(i);
    CV_OrderTakerBusy[i] = CreateCondition(str, 18);
	}
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str = "CV_OrCr_LineToOrderFoodFromCustomerID";
	str[37] = char(i);
    CV_OrCr_LineToOrderFoodFromCustomerID[i] = CreateCondition(str, 38);
  }
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	str= "CV_MrCr_LineToEnterRestFromCustomerID";
	str[37] = char(i);
    CV_MrCr_LineToEnterRestFromCustomerID[i] = CreateCondition(str, 38);
  }
  /* locks and condition variables done here */
  
  int hasBeenInitialized = CreateMV("hasBeenInitialized",18);
  int initializationLock = CreateLock("initializationLock", 18);
  
  /* Initialize Monitor Variables and Globals */
  // create monitor variables
  
  
	count_NumTablesAvailable = CreateMV("count_NumTablesAvailable",24);
	
	for(i = 0; i < numInventoryItemTypes; i += 1)
	{
		str = "inventoryCount";							str[14] = i;		inventoryCount[i] = CreateMV(str,15);
		str = "minCookedFoodStacks";					str[19] = i;		minCookedFoodStacks[i] = CreateMV(str,20);
		str = "maxCookedFoodStacks";					str[19] = i;		maxCookedFoodStacks[i] = CreateMV(str,20);
		str = "cookedFoodStacks";						str[16] = i;		cookedFoodStacks[i] = CreateMV(str,17);
		str = "cookTime";								str[8]  = i;		cookTime[i] = CreateMV(str,9);
		str = "Get_CookIsHiredFromInventoryIndex";		str[33] = i;		Get_CookIsHiredFromInventoryIndex[i] = CreateMV(str,34);
		str = "Get_CookOnBreakFromInventoryIndex";		str[33] = i;		Get_CookOnBreakFromInventoryIndex[i] = CreateMV(str,34);
		str = "inventoryCost";							str[13] = i;		inventoryCost[i] = CreateMV(str,14);
	}
	
	count_NumOrderTakers = CreateMV("count_NumOrderTakers",20);
	count_NumCustomers = CreateMV("count_NumCustomers",18);
	count_NumCooks  = CreateMV("count_NumCooks",14);
	count_NumManagers  = CreateMV("count_NumManagers",17);
	count_lineToEnterRestLength  = CreateMV("count_lineToEnterRestLength",27);
	count_lineToOrderFoodLength  = CreateMV("count_lineToOrderFoodLength",27);
	orderNumCounter  = CreateMV("orderNumCounter",15);
	
	for(i = 0; i < count_MaxNumCustomers ; i += 1)
	{
		str = "ordersNeedingBagging";					str[20] = i;		ordersNeedingBagging[i] = CreateMV(str,21);
		str = "baggedOrders";							str[12] = i;		baggedOrders[i] = CreateMV(str,13);
		str = "bool_ListOrdersReadyFromToken";			str[29] = i;		bool_ListOrdersReadyFromToken[i] = CreateMV(str,30);
	}
	
	count_NumOrdersBaggedAndReady=CreateMV("count_NumOrdersBaggedAndReady",29);

	for (i = 0; i < count_MaxNumMenuItems; ++i)
		str = "menu";					str[4] = i;		menu[i] = CreateMV(str,5);
  
	money_Rest = CreateMV("money_Rest",10);
	count_NumOrderTokens = CreateMV("count_NumOrderTokens",20);
	count_NumCustomersServed =  CreateMV("count_NumCustomersServed",24);
  
	/* Initialize Test Variables as off */
	test_AllCustomersEatIn = CreateMV("test_AllCustomersEatIn",22);
	test_AllCustomersEatOut = CreateMV("test_AllCustomersEatOut",23);
	test_NoCooks = CreateMV("test_NoCooks",12);
	test_AllCustomersOrderThisCombo = CreateMV("test_AllCustomersOrderThisCombo",31);
  
	  
	count_NumCustomersServed = CreateMV("count_NumCustomersServed",24);
	count_WhenAllCustomersServed = CreateMV("count_WhenAllCustomersServed",28);
  
  index_Ck_InventoryIndex = CreateMV("index_Ck_InventoryIndex",23);
  
	for (i = 0; i < count_MaxNumMenuItems; ++i)
	{
		str = "ID_Get_OrderTakerIDFromCustomerID";						str[33] = i;		ID_Get_OrderTakerIDFromCustomerID[i] = CreateMV(str,34);
		str = "Get_CustomerOrderFoodChoiceFromOrderTakerID";			str[43] = i;		Get_CustomerOrderFoodChoiceFromOrderTakerID[i] = CreateMV(str,44);
		str = "Get_CustomerTogoOrEatinFromCustomerID";					str[37] = i;		Get_CustomerTogoOrEatinFromCustomerID[i] = CreateMV(str,38);
		str = "Get_CustomerMoneyPaidFromOrderTakerID";					str[37] = i;		Get_CustomerMoneyPaidFromOrderTakerID[i] = CreateMV(str,38);
		str = "token_OrCr_OrderNumberFromCustomerID";					str[36] = i;		token_OrCr_OrderNumberFromCustomerID[i] = CreateMV(str,37);
		str = "bool_ListOrdersReadyFromToken";							str[29] = i;		bool_ListOrdersReadyFromToken[i] = CreateMV(str,30);
		str = "Get_CustomerIDFromToken";								str[23] = i;		Get_CustomerIDFromToken[i] = CreateMV(str,24);
	}
  
  
  
  Acquire(initializationLock);
  if(GetMV(hasBeenInitialized) != 1)
  {
	setMV(hasBeenInitialized,1);
	
	
  //initializeMonitorVariables
	setMV(count_NumTablesAvailable,10);
	
	for(i = 0; i < numInventoryItemTypes; i += 1)
	{
		setMV(count_NumTablesAvailable,10);
	    setMV(inventoryCount[i] ,0);
		setMV(minCookedFoodStacks[i] , 2);
		setMV(maxCookedFoodStacks[i] , 5);
		setMV(cookedFoodStacks[i] , 0);
		setMV(cookTime[i] , 50);
		setMV(Get_CookIsHiredFromInventoryIndex[i ] , 0);
		setMV(Get_CookOnBreakFromInventoryIndex[i ] , 0);
		setMV(inventoryCost[i] , i*5 + 5);
	}
	
	setMV(count_NumOrderTakers , 0);
	setMV(count_NumCustomers , 0);
	setMV(count_NumCooks  , 0);
	setMV(count_NumManagers , 0);
	setMV(count_lineToEnterRestLength , 0);
	setMV(count_lineToOrderFoodLength , 0);
	setMV(orderNumCounter , 0);
	
	for(i = 0; i < count_MaxNumCustomers ; i += 1)
	{
		setMV(ordersNeedingBagging[i] , 0);
		setMV(baggedOrders[i] , 0);
		setMV(bool_ListOrdersReadyFromToken[i] , 0);
	}
	
	setMV(count_NumOrdersBaggedAndReady , 0);
	
	setMV(menu[0] , 1);   /* Soda */
	setMV(menu[1] ,2);   /* Fries */
	setMV(menu[2] , 3);   /* Fries, Soda */
	setMV(menu[3] , 4);   /* Veggie Burger */
	setMV(menu[4] , 5);   /* Veggie Burger, Soda */
	setMV(menu[5] , 6);   /* Veggie Burger, Fries */
	setMV(menu[6] , 7);   /* Veggie Burger, Fries, Soda */
	setMV(menu[7] , 8);   /* $3 Burger */
	setMV(menu[8] , 9);   /* $3 Burger, Soda */
	setMV(menu[9] , 10);  /* $3 Burger, Fries */
	setMV(menu[10] , 11); /* $3 Burger, Fries, Soda */
	setMV(menu[11] , 16); /* $6 Burger */
	setMV(menu[12] , 17); /* $6 Burger, Soda */
	setMV(menu[13] , 18); /* $6 Burger, Fries */
	setMV(menu[14] ,19); /* $6 Burger, Fries, Soda */
	
	setMV(money_Rest , 0);
	setMV(count_NumOrderTokens , 0);
	setMV(count_NumCustomersServed , 0);
	
	/* Initialize Test Variables as off */
	setMV(test_AllCustomersEatIn , 0);
	setMV(test_AllCustomersEatOut , 0);
	setMV(test_NoCooks , 0);
	setMV(test_AllCustomersOrderThisCombo , -1);
  }
  Release(initializationLock);
}

/* Used to print more messages (prints a lot more if you enable). */
#undef DEBUGV
void PrintOutV(char * input, int len)
{
  #ifdef DEBUGV
    PrintOut(input, len);
  #endif
}

void PrintNumberV(int input)
{
  #ifdef DEBUGV
    PrintNumber(input);
  #endif
}

/* =============================================================
 * Runs the simulation
 * =============================================================*/
void RunSimulation(int numOrderTakers, int numWaiters, int numCustomers)
{
  int i;

  if (numCustomers < 0 || numCustomers > count_MaxNumCustomers)
  {
    PrintOut("Setting number of customers to default\n",39);
    numCustomers = count_defaultNumCustomers;
    count_WhenAllCustomersServed = numCustomers;
  }
  if (numOrderTakers < 0 || numOrderTakers > count_MaxNumOrderTakers)
  {
    PrintOut("Setting number of order takers to default\n",42);
    numOrderTakers = count_defaultNumOrderTakers;
  }
  if (numWaiters < 0 || numWaiters > count_MaxNumWaiters)
  {
    PrintOut("Setting number of waiters to default\n",37);
    numWaiters = count_defaultNumWaiters;
  }
  
	/* Initialize Locks, CVs, and shared data */
	Initialize();
	
	PrintOut("\nNumber of OrderTakers = ", 25);
	PrintNumber(numOrderTakers);
	PrintOut("\nNumber of Waiters = ", 21);
	PrintNumber(numWaiters);
	PrintOut("\nNumber of Cooks = ", 19);
	PrintNumber(0);
	PrintOut("\nNumber of Customers = ", 23);
	PrintNumber(numCustomers);
	PrintOut("\nTotal Number of tables in the Restaurant = ", 44);
	PrintNumber(count_NumTablesAvailable);
	PrintOut("\nMinimum number of cooked 6-dollar burger = ", 44);
	PrintNumber(2);
	PrintOut("\nMinimum number of cooked 3-dollar burger = ", 44);
	PrintNumber(2);
	PrintOut("\nMinimum number of cooked veggie burger = ", 42);
	PrintNumber(2);
	PrintOut("\nMinimum number of cooked french fries burger = ", 48);
	PrintNumber(2);
	PrintOut("\n", 1);
  
	PrintOut("===================================================\n", 52);
		
	Fork((void *)Manager);
	for (i = 0; i < numCustomers; ++i)
		Fork((void *)Customer);
	for (i = 0; i < numOrderTakers; ++i)
		Fork((void *)OrderTaker);
	for (i = 0; i < numWaiters; ++i)
		Fork((void *)Waiter);
}

int main()
{
  StartUserProgram();
  RunSimulation(-1, -1, -1);
  return 0;
}

/* =============================================================	
 * CUSTOMER
 * =============================================================*/

/*-----------------------------
 * Top Level Customer Routine
 * ---------------------------*/
void Customer(int debug)
{
  int ID, eatIn, orderCombo, token, hasCheckedOrder;
  
	Acquire(lock_Init_InitializationLock);
	ID = GetMV(count_NumCustomers);
	SetMV(count_NumCustomers,ID+1);
	Release(lock_Init_InitializationLock);

	if (GetMV(test_AllCustomersEatIn) == TRUE)
		eatIn = 1;
	else if (GetMV(test_AllCustomersEatOut) == TRUE)
		eatIn = 0;
	else
		eatIn = RandomNumber(2);
	
	if(eatIn)
	{
		PrintOut("Customer ", 9);
		PrintNumber(ID);
		PrintOut(" wants to eat-in the food\n", 26);

		WaitInLineToEnterRest(ID);
	}
	else
	{
		PrintOut("Customer ", 9);
		PrintNumber(ID);
		PrintOut(" wants to to-go the food\n", 25);
	}
	
	PrintOutV("Customer", 8);
	PrintNumberV(ID);
	PrintOutV("::Waiting in line for food\n", 27);
	WaitInLineToOrderFood(ID);
	/* at this point we are locked with the order taker */
	
	/* randomly pick an order and tell it to order taker if not testing */
	if (GetMV(test_AllCustomersOrderThisCombo) != -1)
		orderCombo = GetMV(test_AllCustomersOrderThisCombo);
	else
		orderCombo = 2 * (ID % numInventoryItemTypes); /*RandomNumber(count_MaxNumMenuItems);*/
	
	SetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV( ID_Get_OrderTakerIDFromCustomerID[ID])] , GetMV(menu[orderCombo]));
	/* Tell OrderTaker whether eating in or togo */
	SetMV(Get_CustomerTogoOrEatinFromCustomerID[ID]) = eatIn;
	/* Ordered, Signal Order Taker - this represents talking to ordertaker */
	
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is giving order to ", 20);  
	if(GetMV(ID_Get_OrderTakerIDFromCustomerID)[ID] > 0)
	{
		PrintOut("OrderTaker ", 11);
		PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
	}
	else
	{
		PrintOut("the Manager", 11);
	}
	PrintOut("\n", 1);

	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is ", 4);
	if (((GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]) >> 4) & 1) == 0)
		PrintOut("not ", 4);
	PrintOut("ordering 6-dollar burger\n", 25);
  
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is ", 4);
	if (((GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]) >> 3) & 1) == 0)
		PrintOut("not ", 4);
	PrintOut("ordering 3-dollar burger\n", 25);
  
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is ", 4);
	if (((GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]) >> 2) & 1) == 0)
		PrintOut("not ", 4);
	PrintOut("ordering veggie burger\n", 23);
  
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is ", 4);
	if (((GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]) >> 1) & 1) == 0)
		PrintOut("not ", 4);
	PrintOut("ordering french fries\n", 22);
  
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is ", 4);
	if ((GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]) & 1) == 0)
		PrintOut("not ", 4);
	PrintOut("ordering soda\n", 14);
  
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" chooses to ", 12);
	if(eatIn)
		PrintOut("eat-in", 7);
	else
		PrintOut(" to-go", 7);
	PrintOut(" the food\n", 10);
  
  
	Signal(CV_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])], 
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]);
  
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])], 
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]]));
	
	PrintOutV("Customer", 8);
	PrintNumberV(ID);
	PrintOutV("::Paying OrderTaker #", 21);
	PrintNumberV(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
	PrintOutV("\n", 1);
	
	/* this is the money the customer gives to the orderTaker */
	/* Customer pays double the inventory cost */
	SetMV(Get_CustomerMoneyPaidFromOrderTakerID[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])] 
				, inventoryCost[GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[					
				GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])])] * 2); 
	
	Signal(CV_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])],
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]]));
  
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])], 
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]);
   
	/* Order Taker got my order, and money */
	/* Order Taker gives me an order number in exchange */
	token = GetMV(token_OrCr_OrderNumberFromCustomerID[ID]);
  
	if (orderCombo != 0)
	{
		PrintOutV("Customer", 8);
		PrintNumberV(ID);
		PrintOutV("::My order is order #", 21);
		PrintNumberV(token);
		PrintOutV("\n", 1);
	}
  
	if(eatIn)
	{
		PrintOut("Customer ", 9);
		PrintNumber(ID);
		PrintOut(" is seated at table. ", 21);
		PrintNumber(GetMV(count_NumTablesAvailable));
		PrintOut(" tables available\n", 18);

		/* we want the customer to be ready to receive the order */
		/* before we Release the OrderTaker to process the order */
		Acquire(lock_CustomerSittingFromCustomerID[ID]);
		Release(lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]);

		PrintOut("Customer ", 9);
		PrintNumber(ID);
		PrintOut(" is waiting for the waiter to serve the food\n", 45);		
	
		/* Wait for order to be ready.  
		 * Waiter will deliver it just to me, so I have my own condition variable. 
		 * Signal - Wait - Signal insures correct sequencing
		 */
		Signal(CV_CustomerSittingFromCustomerID[ID], lock_CustomerSittingFromCustomerID[ID]);
		Wait(CV_CustomerSittingFromCustomerID[ID], lock_CustomerSittingFromCustomerID[ID]);
		Signal(CV_CustomerSittingFromCustomerID[ID], lock_CustomerSittingFromCustomerID[ID]);
		
		PrintOut("Customer ", 9);
		PrintNumber(ID);
		PrintOut(" is served by waiter 0\n", 23);
    
		/* I received my order */
		Release(lock_CustomerSittingFromCustomerID[ID]);
    
		PrintOutV("Customer", 8);
		PrintNumberV(ID);
		PrintOutV("::Eatin - Eating\n",17);
    
		/* make the table available again. */
		Acquire(lock_MrCr_LineToEnterRest);
		  count_NumTablesAvailable += 1;
		Release(lock_MrCr_LineToEnterRest);
		
		PrintOutV("Customer", 8);
		PrintNumberV(ID);
		PrintOutV("::Eatin - Leaving Restaurant full and happy =)\n", 47);
		
    /* leave restaurant */
	}
	else
	{    
		/*prepare to recieve food before I let the order take go make it. */
		Acquire(lock_OrCr_OrderReady);
		/*let ordertaker go make food. */
		Release(lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]);
		
		PrintOutV("Customer", 8);
		PrintNumberV(ID);
		PrintOutV("::Eatout - Ready for food\n",26);
    
		/* check if my order is ready */
		hasCheckedOrder = 0; /* used for print statement */
		while(GetMV(bool_ListOrdersReadyFromToken[token]) == FALSE)
		{
			if (hasCheckedOrder > 0)
			{
				PrintOutV("Customer", 8);
				PrintNumberV(ID);
				PrintOutV("::Eatout - Order called was not #",33);
				PrintNumberV(token);
				PrintOutV("\n",1);
			}
			else
			{
				if(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]) > 0)
				{
					PrintOut("OrderTaker ", 11);
					PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
				}
				else
				{
					PrintOut("Manager", 7);
				}
				PrintOut(" gives token number ", 20);
				PrintNumber(token);
				PrintOut(" to Customer ", 13);
				PrintNumber(ID);
				PrintOut("\n", 1);
			  
				PrintOut("Customer ", 9);
				PrintNumber(ID);
				PrintOut(" is given token number ", 23);
				PrintNumber(token);
				PrintOut(" by ", 4);
				if(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]) > 0)
				{
				  PrintOut("OrderTaker ", 11);
				  PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
				}
				else
				{
					PrintOut("the Manager", 11);
				}
				PrintOut("\n", 1);
			}
      
			/* Wait for an OrderReady Broadcast */
			Wait(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
			hasCheckedOrder = 1;
      
			PrintOutV("Customer", 8);
			PrintNumberV(ID);
			PrintOutV("::Eatout - Checking if order called is #", 40);
			PrintNumberV(token);
			PrintOutV("\n", 1);
		}
    
		if (hasCheckedOrder == 0)
		{
			if(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]) > 0)
			{
				PrintOut("OrderTaker ", 11);
				PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
			}
			else
			{
				PrintOut("Manager", 7);
			}
			PrintOut(" gives food to Customer ", 24);
			PrintNumber(ID);
			PrintOut("\n", 1);
    
			PrintOut("Customer ", 9);
			PrintNumber(ID);
			PrintOut(" receives food from ", 20);
			if(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]) > 0)
			{
				PrintOut("OrderTaker ", 11);
				PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
			}
			else
			{
				PrintOut("the Manager", 11);
			}
			PrintOut("\n", 1);
		}
		else
		{
			PrintOut("Customer ", 9);
			PrintNumber(ID);
			PrintOut(" receives token number ", 23);
			PrintNumber(token);
			PrintOut(" from the OrderTaker ", 21);
			PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
			PrintOut("\n", 1);
		}
    
		PrintOutV("Customer", 8);
		PrintNumberV(ID);
		PrintOutV("::Eatout - Taking order #", 25);
		PrintNumberV(token);
		PrintOutV("\n",1);
    
		SetMV(bool_ListOrdersReadyFromToken[token] , FALSE);
		Release(lock_OrCr_OrderReady);
    
		if (hasCheckedOrder > 0)
		{
			PrintOut("Customer ", 9);
			PrintNumber(ID);
			PrintOut(" is leaving the restaurant after OrderTaker ", 44);
			PrintNumber(GetMV(ID_Get_OrderTakerIDFromCustomerID[ID]));
			PrintOut(" packed the food\n", 17);
		}
    
		/* leave restaurant */
	}
	
	Acquire(lock_Init_InitializationLock);
	  PrintOut("~~~ Total Customers left to be served: ", 39);
	  SetMV(count_NumCustomersServed,GetMV(count_NumCustomersServed)+1);
	  PrintNumber(GetMV(count_NumCustomers)-(GetMV(count_NumCustomersServed)));
      PrintOut(" ~~~\n", 5);
	Release(lock_Init_InitializationLock);
	
	
	Exit(0);
}

/*-----------------------------
 * Customers getting in line for the restaurant
 * ---------------------------*/
void WaitInLineToEnterRest(int ID)
{
	Acquire(lock_MrCr_LineToEnterRest);
  
	SetMV(lineToEnterRest[GetMV(count_lineToEnterRestLength)] ,ID);
	SetMV(count_lineToEnterRestLength,GetMV(count_lineToEnterRestLength)+1);
	/* Wait in line, and pass my ID as CV */
	Wait(CV_MrCr_LineToEnterRestFromCustomerID[ID], lock_MrCr_LineToEnterRest);

	if (GetMV(count_lineToEnterRestLength) > 0)
	{
		SetMV(count_lineToEnterRestLength,GetMV(count_lineToEnterRestLength)-1);
	}
	else  /* Should never get here */
	{
      PrintOutV("Customer", 8);
      PrintNumberV(ID);
      PrintOutV("::Something is wrong while customer getting out of order line\n",47);
	}	
	Release(lock_MrCr_LineToEnterRest);
}

/*-----------------------------
 * Customers getting in line to take an Order
 * ---------------------------*/
void WaitInLineToOrderFood(int ID)
{
	Acquire(lock_OrCr_LineToOrderFood);
	SetMV(lineToOrderFood[count_lineToOrderFoodLength] , ID);
	SetMV(count_lineToOrderFoodLength,GetMV(count_lineToOrderFoodLength)+1);
	Wait(CV_OrCr_LineToOrderFoodFromCustomerID[ID], lock_OrCr_LineToOrderFood);
	
	/* make sure OrderTaker is ready to serve me */
	Acquire(lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])]);
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
				SetMV(money_Rest,GetMV(money_Rest) + GetMV(inventoryCost[i])*2*(numBought+5);
        
        PrintOutV("Manager",7);
        PrintOutV("::Went to bank. Restaurant now has $",36);
        PrintNumberV(GetMV(money_Rest));
        PrintOutV("\n",1);
				
			}

			SetMV(money_Rest,GetMV(money_Rest) - GetMV(inventoryCost[i])*numBought;
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

	Fork((void *)Cook);

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

/*-----------------------------
 * If there are any customers Waiting in line, 
 * get one out of line and take his order.
 * ---------------------------*/
void serviceCustomer(int ID)
{
  int custID, token;

	Acquire(lock_OrCr_LineToOrderFood);
	if (GetMV(count_lineToOrderFoodLength) <= 0)
	{
		/* no one in line */
		Release(lock_OrCr_LineToOrderFood);
		return;
	}

	/* Get a customer out of line and tell him he's our bitch so he can order. */
	custID = GetMV(lineToOrderFood[GetMV(count_lineToOrderFoodLength) - 1]);
	  SetMV(count_lineToOrderFoodLength,GetMV(count_lineToOrderFoodLength)-1);
	SetMV(ID_Get_OrderTakerIDFromCustomerID[custID] , ID);
	Signal(CV_OrCr_LineToOrderFoodFromCustomerID[custID], lock_OrCr_LineToOrderFood); 
	Acquire(lock_OrderTakerBusy[ID]);
	Release(lock_OrCr_LineToOrderFood);
	
	/* Ask the customer what he would like to order */
	if(ID > 0)
	{
		PrintOut("OrderTaker ", 11);
		PrintNumber(ID);
	}
	else
	{
		PrintOut("Manager", 7);
	}
	PrintOut(" is taking order of Customer ", 29);
	PrintNumber(custID);
	PrintOut("\n", 1);
	
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	Signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	Wait(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);

	/* Customer has paid by the time we get here. */

	/* Give the customer's order an order number. */
	token = GetMV(orderNumCounter)
	SetMV(orderNumCounter,GetMV(orderNumCounter)+1);
	SetMV(token_OrCr_OrderNumberFromCustomerID[custID] , token);
	
  if (GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[ID]) != 1)
  {
    PrintOutV("OrderTaker", 10);
    PrintNumberV(ID);
    PrintOutV("::Thanks for the order. Your token number is: ", 47);
    PrintNumberV(token);
    PrintOutV("\n", 1);
  }
	
	/* Tell all Waiters the token too */
	SetMV(Get_CustomerIDFromToken[token] , custID);
	/* Tell the customer to pick up the order number. */
	Signal(CV_OrderTakerBusy[ID], lock_OrderTakerBusy[ID]);
	/* Add order list of orders needing to get bagged. */
	Acquire(lock_OrdersNeedingBagging);
	SetMV(ordersNeedingBagging[token] , GetMV(Get_CustomerOrderFoodChoiceFromOrderTakerID[ID]));
	PrintOutV("OrderTaker", 10);
	PrintNumberV(ID);
	PrintOutV("::Order #", 9);
	PrintNumberV(token);
	PrintOutV(" needs to be bagged.\n", 21);
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
void bagOrder(int isManager)
{
    int i, j;

    Acquire(lock_OrdersNeedingBagging);
    for (i = 0; i < count_MaxNumCustomers; i++)
    {
      /* If order i still needs food to be fully bagged. */
      if (GetMV(ordersNeedingBagging[i]) > 0)
      {
        /* Check if each item the order needs has been added to the bag. */
        /* Start at 1 because FoodType 0 is Soda (which we can assume will be always immediately available). */
        for (j = 1; j < numInventoryItemTypes; j++)
        {
          /* // For Debugging.
          PrintOut("order=", 6);
          PrintNumber(ordersNeedingBagging[i]);
          PrintOut("\norder>>", 7);
          PrintNumber(j);
          PrintOut("=", 1);
          PrintNumber(ordersNeedingBagging[i] >> j);
          PrintOut("\n", 1);
          */
          
          if ((((GetMV(ordersNeedingBagging[i]) >> j) & 1) > 0) && (GetMV(cookedFoodStacks[j]) > 0))
          {
            SetMV(ordersNeedingBagging[i] , GetMV(ordersNeedingBagging[i]) ^ (1 << j));
            SetMV(cookedFoodStacks[j],GetMV(cookedFoodStacks[j])-1);
      
            if (isManager)
			{
              PrintOutV("Manager",7);
            }
			else
			{
              PrintOutV("OrderTaker",10);
            }
						PrintOutV("::Bagging item #", 14);
            PrintNumberV(j);
            PrintOutV(" for order #", 12);
            PrintNumberV(i);
            PrintOutV("\n", 1);
          }
        }
        
        /* If we just completely bagged order i. */
        if (GetMV(ordersNeedingBagging[i] )<= 1)
        {
          SetMV(ordersNeedingBagging[i] , 0);
          /* If the customer for order i is an eatin customer (i.e. he is Waiting at a table). */
          if (GetMV(Get_CustomerTogoOrEatinFromCustomerID[GetMV(Get_CustomerIDFromToken[i])]) != 0)
          {
            /* Tell Waiters there is a bagged order to be delivered. */
            Acquire(lock_OrWr_BaggedOrders);
            /* Store the token */
              SetMV(baggedOrders[count_NumOrdersBaggedAndReady] , i);
              SetMV(count_NumOrdersBaggedAndReady,GetMV(count_NumOrdersBaggedAndReady)+1);
							
							if(isManager)
								PrintOut("Manager calls back all Waiters from break\n", 42);
							
              Broadcast(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
              
              PrintOutV("Eatin order #", 13);
              PrintNumberV(i);
              PrintOutV(" is ready\n", 10);
            Release(lock_OrWr_BaggedOrders);
          }
          else /* if the customer or order i is a togo customer. */
          {
            /* Tell the Waiting togo customers that order i is ready. */
            Acquire(lock_OrCr_OrderReady);
              SetMV(bool_ListOrdersReadyFromToken[i] , 1);
              Broadcast(CV_OrCr_OrderReady, lock_OrCr_OrderReady);
              
              PrintOutV("Eatout order #", 14);
              PrintNumberV(i);
              PrintOutV(" is ready\n", 10);
            Release(lock_OrCr_OrderReady);
          }
        }
      }
    }
	Release(lock_OrdersNeedingBagging);
}
 
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
			Acquire(lock_CustomerSittingFromCustomerID[GetMV(Get_CustomerIDFromToken[token]))]);
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

/* =============================================================	
 * Utility Functions
 * =============================================================*/	
 
/*
 * Returns true if should exit
 */
int shouldExit()
{
	int exit;
	
	Acquire(lock_Init_InitializationLock);
	  exit = (GetMV(count_NumCustomersServed) == GetMV(count_WhenAllCustomersServed));
	Release(lock_Init_InitializationLock);
	
	return exit;
}
 
#endif /* CHANGED */
