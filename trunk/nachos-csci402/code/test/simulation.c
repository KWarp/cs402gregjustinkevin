
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

#define MAX_STR_LEN 20

/*
 * Utility to copy string over
 */
void copyChar(char* str, const char* literal, int len)
{
	int i;
	for (i = 0; i < MAX_STR_LEN; i++)
	{
		if( i < len )
			str[i] = literal[i];
		else
			str[i] = '\0';
	}	
}
 
/* =============================================================	
 * Initialize functions
 * =============================================================*/

void Initialize()
{
  int i, hasBeenInitialized, initializationLock;
  char* str;
  str = "12345678901234567890\n"; /* 21 length */
  
  /* Initialize Locks */
  lock_MrCr_LineToEnterRest = CreateLock("lock_MrCr_EnterRest", 19);
  lock_OrCr_LineToOrderFood = CreateLock("lock_OrCr_OrderFood", 19);
  
  for (i = 0; i < count_MaxNumOrderTakers; ++i)
  {
	copyChar(str, "lock_OrderTakerBusy", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    lock_OrderTakerBusy[i] = CreateLock(str, MAX_STR_LEN);
	}
	
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "lock_CuSitFromCustID", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    lock_CustomerSittingFromCustomerID[i] = CreateLock(str, MAX_STR_LEN);
  }
  
  lock_OrCr_OrderReady = CreateLock("lock_OrCr_OrderReady", MAX_STR_LEN);
  
  lock_MrWr = CreateLock("lock_MrWr", 9);
  for (i = 0; i < numInventoryItemTypes; ++i)
  {
	copyChar(str, "lock_MrCk_InvenLocks", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
	lock_MrCk_InventoryLocks[i] = CreateLock(str, MAX_STR_LEN);
  }
	
  lock_HireCook = CreateLock("lock_HireCook", 13);
  lock_OrdersNeedingBagging = CreateLock("lock_OrdersNeBagging", MAX_STR_LEN);
  lock_OrWr_BaggedOrders = CreateLock("lock_OrWr_BaggOrders", MAX_STR_LEN);
  lock_Init_InitializationLock = CreateLock("lock_Init_InitiaLock", MAX_STR_LEN);

  /* Initialize CV's */
  CV_OrWr_BaggedOrders = CreateCondition("CV_OrWr_BaggedOrders", MAX_STR_LEN);
  CV_HireCook = CreateCondition("CV_HireCook", 11);
  for (i = 0; i < count_MaxNumCustomers; ++i)
	{
	copyChar(str, "CV_MrCk_InventLocks", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    CV_MrCk_InventoryLocks[i] = CreateCondition(str, MAX_STR_LEN);
	}
  CV_MrWr = CreateCondition("CV_MrWr", 7);
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "CV_CusWaitingForFood", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    CV_CustomerWaitingForFood[i] = CreateCondition(str, MAX_STR_LEN);
  }
  CV_OrCr_OrderReady = CreateCondition("CV_OrCr_OrderReady", 18);
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "CV_CuSittingFromCuID", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    CV_CustomerSittingFromCustomerID[i] = CreateCondition(str, MAX_STR_LEN);
	}
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "CV_OrderTakerBusy", 18);
	str[17] = (char)i;
    CV_OrderTakerBusy[i] = CreateCondition(str, 18);
	}
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "CV_OrCr_LineFoodCuID", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    CV_OrCr_LineToOrderFoodFromCustomerID[i] = CreateCondition(str, MAX_STR_LEN);
  }
  for (i = 0; i < count_MaxNumCustomers; ++i)
  {
	copyChar(str, "CV_MrCr_LineEnteCuID", MAX_STR_LEN);
	str[MAX_STR_LEN-1] = (char)i;
    CV_MrCr_LineToEnterRestFromCustomerID[i] = CreateCondition(str, MAX_STR_LEN);
  }
  /* locks and condition variables done here */
  
  hasBeenInitialized = CreateMV("hasBeenInitialized",18);
  initializationLock = CreateLock("initializationLock", 18);
  
  /* Initialize Monitor Variables and Globals
     create monitor variables */
  
  
	count_NumTablesAvailable = CreateMV("count_NumTablesAvail", MAX_STR_LEN);
	
	for(i = 0; i < numInventoryItemTypes; i += 1)
	{
		copyChar(str, "inventoryCount", 15);					str[14] = i;		inventoryCount[i] = CreateMV(str,15);
		copyChar(str, "minCookedFoodStacks", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		minCookedFoodStacks[i] = CreateMV(str,MAX_STR_LEN);
		copyChar(str, "maxCookedFoodStacks", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		maxCookedFoodStacks[i] = CreateMV(str,MAX_STR_LEN);
		copyChar(str, "cookedFoodStacks", 17);					str[16] = i;		cookedFoodStacks[i] = CreateMV(str,17);
		copyChar(str, "cookTime", 9);							str[8]  = i;		cookTime[i] = CreateMV(str,9);
		copyChar(str, "Get_CokHiredInvIndex", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		Get_CookIsHiredFromInventoryIndex[i] = CreateMV(str,MAX_STR_LEN);
		copyChar(str, "Get_CokBreakInvIndex", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		Get_CookOnBreakFromInventoryIndex[i] = CreateMV(str,MAX_STR_LEN);
		copyChar(str, "inventoryCost", 14);						str[13] = i;		inventoryCost[i] = CreateMV(str,14);
	}
	
	count_NumOrderTakers = CreateMV("count_NumOrderTakers", MAX_STR_LEN);
	count_NumCustomers = CreateMV("count_NumCustomers",18);
	count_NumCooks  = CreateMV("count_NumCooks",14);
	count_NumManagers  = CreateMV("count_NumManagers",17);
	count_lineToEnterRestLength  = CreateMV("count_lineRestLength", MAX_STR_LEN);
	count_lineToOrderFoodLength  = CreateMV("count_lineFoodLength", MAX_STR_LEN);
	orderNumCounter  = CreateMV("orderNumCounter",15);
	
	for(i = 0; i < count_MaxNumCustomers ; i += 1)
	{
		copyChar(str, "ordersNeedingBagging", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		ordersNeedingBagging[i] = CreateMV(str,MAX_STR_LEN);
		copyChar(str, "baggedOrders", 13);						str[12] = i;		baggedOrders[i] = CreateMV(str,13);
		copyChar(str, "bool_ListOrdersToken", MAX_STR_LEN);		str[MAX_STR_LEN-1] = i;		bool_ListOrdersReadyFromToken[i] = CreateMV(str,MAX_STR_LEN-1);
	}
	
	count_NumOrdersBaggedAndReady=CreateMV("count_NumOrdersBagge", MAX_STR_LEN);

	for (i = 0; i < count_MaxNumMenuItems; ++i)
		copyChar(str, "menu", 5);				str[4] = i;		menu[i] = CreateMV(str,5);
  
	money_Rest = CreateMV("money_Rest",10);
	count_NumOrderTokens = CreateMV("count_NumOrderTokens", MAX_STR_LEN);
	count_NumCustomersServed =  CreateMV("count_NumCustoServed", MAX_STR_LEN);
  
	/* Initialize Test Variables as off */
	test_AllCustomersEatIn = CreateMV("test_AllCustomeEatIn", MAX_STR_LEN);
	test_AllCustomersEatOut = CreateMV("test_AllCustomEatOut", MAX_STR_LEN);
	test_NoCooks = CreateMV("test_NoCooks",12);
	test_AllCustomersOrderThisCombo = CreateMV("test_AllCOrderTCombo", MAX_STR_LEN);
  
	  
	count_NumCustomersServed = CreateMV("count_NumCustoServed", MAX_STR_LEN);
	count_WhenAllCustomersServed = CreateMV("count_WhenAllCServed", MAX_STR_LEN);
  
	index_Ck_InventoryIndex = CreateMV("index_Ck_InventIndex", MAX_STR_LEN);
  
	for (i = 0; i < count_MaxNumMenuItems; ++i)
	{
		copyChar(str, "ID_Get_OTIDFromCusID", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		ID_Get_OrderTakerIDFromCustomerID[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "Get_COrderFoodChOTID", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		Get_CustomerOrderFoodChoiceFromOrderTakerID[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "Get_CTogoOrEatinFCID", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		Get_CustomerTogoOrEatinFromCustomerID[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "Get_CMoneyPaidFrOTID", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		Get_CustomerMoneyPaidFromOrderTakerID[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "token_OrCr_ONumFrCID", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		token_OrCr_OrderNumberFromCustomerID[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "bool_ListORdyFrToken", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		bool_ListOrdersReadyFromToken[i] = CreateMV(str, MAX_STR_LEN);
		copyChar(str, "Get_CustoIDFromToken", MAX_STR_LEN);			str[MAX_STR_LEN-1] = i;		Get_CustomerIDFromToken[i] = CreateMV(str, MAX_STR_LEN);
	}
  
  
  
  Acquire(initializationLock);
  if(GetMV(hasBeenInitialized) != 1)
  {
	SetMV(hasBeenInitialized,1);
	
	
	/* Initialize Monitor Variables */
	SetMV(count_NumTablesAvailable,10);
	
	for(i = 0; i < numInventoryItemTypes; i += 1)
	{
		SetMV(count_NumTablesAvailable,10);
	    SetMV(inventoryCount[i] ,0);
		SetMV(minCookedFoodStacks[i] , 2);
		SetMV(maxCookedFoodStacks[i] , 5);
		SetMV(cookedFoodStacks[i] , 0);
		SetMV(cookTime[i] , 50);
		SetMV(Get_CookIsHiredFromInventoryIndex[i ] , 0);
		SetMV(Get_CookOnBreakFromInventoryIndex[i ] , 0);
		SetMV(inventoryCost[i] , i*5 + 5);
	}
	
	SetMV(count_NumOrderTakers , 0);
	SetMV(count_NumCustomers , 0);
	SetMV(count_NumCooks  , 0);
	SetMV(count_NumManagers , 0);
	SetMV(count_lineToEnterRestLength , 0);
	SetMV(count_lineToOrderFoodLength , 0);
	SetMV(orderNumCounter , 0);
	
	for(i = 0; i < count_MaxNumCustomers ; i += 1)
	{
		SetMV(ordersNeedingBagging[i] , 0);
		SetMV(baggedOrders[i] , 0);
		SetMV(bool_ListOrdersReadyFromToken[i] , 0);
	}
	
	SetMV(count_NumOrdersBaggedAndReady , 0);
	
	SetMV(menu[0] , 1);   /* Soda */
	SetMV(menu[1] ,2);   /* Fries */
	SetMV(menu[2] , 3);   /* Fries, Soda */
	SetMV(menu[3] , 4);   /* Veggie Burger */
	SetMV(menu[4] , 5);   /* Veggie Burger, Soda */
	SetMV(menu[5] , 6);   /* Veggie Burger, Fries */
	SetMV(menu[6] , 7);   /* Veggie Burger, Fries, Soda */
	SetMV(menu[7] , 8);   /* $3 Burger */
	SetMV(menu[8] , 9);   /* $3 Burger, Soda */
	SetMV(menu[9] , 10);  /* $3 Burger, Fries */
	SetMV(menu[10] , 11); /* $3 Burger, Fries, Soda */
	SetMV(menu[11] , 16); /* $6 Burger */
	SetMV(menu[12] , 17); /* $6 Burger, Soda */
	SetMV(menu[13] , 18); /* $6 Burger, Fries */
	SetMV(menu[14] ,19); /* $6 Burger, Fries, Soda */
	
	SetMV(money_Rest , 0);
	SetMV(count_NumOrderTokens , 0);
	SetMV(count_NumCustomersServed , 0);
	
	/* Initialize Test Variables as off */
	SetMV(test_AllCustomersEatIn , 0);
	SetMV(test_AllCustomersEatOut , 0);
	SetMV(test_NoCooks , 0);
	SetMV(test_AllCustomersOrderThisCombo , -1);
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
#if 0
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
#endif
}



#if 0
int main()
{
  int i;
  StartUserProgram();
  RunSimulation(-1, -1, -1);
  return 0;
}
#endif





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
	token = GetMV(orderNumCounter);
	SetMV(orderNumCounter, GetMV(orderNumCounter)+1);
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
