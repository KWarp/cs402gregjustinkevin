#ifndef SIMULATION_H
#define SIMULATION_H

/*
 * simulation.h
 *
 * Defines all of the methods used in Simulation.c
 *
 */
 
 /* =============================================================	
 * DATA
 * =============================================================*/	

#define count_MaxNumOrderTakers 10
#define count_MaxNumCustomers 50
#define count_MaxNumWaiters 10

#define count_defaultNumOrderTakers 3
#define count_defaultNumCustomers 20
#define count_defaultNumWaiters 3

/* Number of Agents in the simulation */
extern int count_NumOrderTakers;
extern int count_NumCustomers;
extern int count_NumCooks;
extern int count_NumManagers;
extern int count_NumWaiters;

/* Line To Enter Restaurant */
extern int count_lineToEnterRestLength;
extern int lineToEnterRest[count_MaxNumCustomers ];
extern int lock_MrCr_LineToEnterRest;
extern int CV_MrCr_LineToEnterRestFromCustomerID[count_MaxNumCustomers ];

/* Line To Order Food */
extern int count_lineToOrderFoodLength;
extern int lineToOrderFood[count_MaxNumCustomers ];
extern int lock_OrCr_LineToOrderFood;
extern int CV_OrCr_LineToOrderFoodFromCustomerID[count_MaxNumCustomers ];

/* Used by the Customer routine */
extern int ID_Get_OrderTakerIDFromCustomerID[count_MaxNumCustomers ];
extern int Get_CustomerOrderFoodChoiceFromOrderTakerID[count_MaxNumOrderTakers ];
extern int Get_CustomerTogoOrEatinFromCustomerID[count_MaxNumCustomers ];
extern int Get_CustomerMoneyPaidFromOrderTakerID[count_MaxNumOrderTakers ];
extern int lock_OrderTakerBusy[count_MaxNumOrderTakers ];
extern int CV_OrderTakerBusy[count_MaxNumOrderTakers ];
extern int token_OrCr_OrderNumberFromCustomerID[count_MaxNumCustomers ];
extern int lock_CustomerSittingFromCustomerID[count_MaxNumCustomers ]; 
extern int CV_CustomerSittingFromCustomerID[count_MaxNumCustomers ];
/* Eat out customers*/
extern int lock_OrCr_OrderReady;
extern int CV_OrCr_OrderReady;
extern int CV_CustomerWaitingForFood[count_MaxNumCustomers ];
extern int bool_ListOrdersReadyFromToken[count_MaxNumCustomers ]; /* TO DO: initialize to zero */
extern int count_NumCustomersServed;

/* Used by the Waiter Routine */
extern int Get_CustomerIDFromToken[count_MaxNumCustomers ];

/* Used by the Manager Routine */
extern int lock_MrWr;
extern int CV_MrWr;


/* Menu -- size = 2^numInventoryItemTypes
 *         Make sure to update this if you change numInventoryItemTypes!!!
 */
#define numInventoryItemTypes 5
#define count_MaxNumMenuItems 32
extern int menu[count_MaxNumMenuItems];

extern int inventoryCount[numInventoryItemTypes ];
extern int inventoryCost[numInventoryItemTypes ];

extern int cookedFoodStacks[numInventoryItemTypes ];
extern int minCookedFoodStacks[numInventoryItemTypes ];
extern int maxCookedFoodStacks[numInventoryItemTypes ];
extern int cookTime[numInventoryItemTypes ];

extern int Get_CookIsHiredFromInventoryIndex[numInventoryItemTypes ];
extern int Get_CookOnBreakFromInventoryIndex[numInventoryItemTypes ];
extern int lock_MrCk_InventoryLocks[numInventoryItemTypes ];
extern int CV_MrCk_InventoryLocks[numInventoryItemTypes ];

extern int lock_HireCook;
extern int CV_HireCook;
extern int index_Ck_InventoryIndex;

/* Used by the Order Taker Routine */
extern int orderNumCounter;
extern int lock_OrdersNeedingBagging;
extern int ordersNeedingBagging[count_MaxNumCustomers ];
extern int baggedOrders[count_MaxNumCustomers ];
extern int count_NumOrdersBaggedAndReady;
extern int lock_OrWr_BaggedOrders;
extern int CV_OrWr_BaggedOrders;

/* MISC */
extern int money_Rest;
extern int count_NumTablesAvailable;
extern int count_NumOrderTokens;
extern int lock_Init_InitializationLock;

/* TEST */
extern int test_AllCustomersEatIn;
extern int test_AllCustomersEatOut;
extern int test_NoCooks;
extern int test_AllCustomersOrderThisCombo;
 
/* =============================================================	
 * Function Declarations
 * =============================================================*/	
 
void RunSimulation(int numOrderTakers, int numWaiters, int numCustomers);
void Initialize();
 
/* Customer */ 
void Customer(int debug);
void WaitInLineToEnterRest(int ID);
void WaitInLineToOrderFood(int ID);

/* Manager */
void Manager(int debug);
void callWaiter();
void orderInventoryFood();
void manageCook();
void hireCook(int index);
void checkLineToEnterRest();

/* Cook */
void Cook(int debug);

/* Order Taker */
void OrderTaker(int debug);
void serviceCustomer(int ID);
void bagOrder(int isManager);

/* Waiter */
void Waiter(int debug);

#endif  /* SIMULATION_H */






