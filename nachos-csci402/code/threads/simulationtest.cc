#ifdef CHANGED

#include "simulation.h"
#include "synch.h"
#include "simulationtest.h"

/* Untested */
void TestCustomer()
{
  int numTestCustomers = 1;
  
  for (int i= 0; i < numTestCustomers; ++i)
    Fork((int)Customer);
	
  // Get the customers into the restaurant.
  for (int i= 0; i < numTestCustomers; ++i)
    checkLineToEnterRest();
  
  // Take the customers' orders.
  for (int i= 0; i < numTestCustomers; ++i)
    serviceCustomer(0);
  
  // Give the customers their orders
  for (int token = 0; token < numTestCustomers; ++token)
  {
    if (ordersNeedingBagging[token] > 0)
    {
      // If the customer is eat-in.
      if (Get_CustomerTogoOrEatinFromCustomerID[Get_CustomerIDFromToken[token]] == 1)
      {
        // Pretend we are waiter delivering food and signal the eat-in customer directly.
        Acquire(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
        Signal(CV_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]],
               lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
        Release(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
      }
      else // if customer is togo.
      {
        // Tell the waiting togo customers that order i is ready.
        Acquire(lock_OrCr_OrderReady);
          bool_ListOrdersReadyFromToken[token] = 1;
          Broadcast(lock_OrCr_OrderReady, CV_OrCr_OrderReady);
        Release(lock_OrCr_OrderReady);
      }
    }
  }
}

/* Untested */
void TestWaiter()
{
  int token = 12; // Arbitrary order number.
  Get_CustomerIDFromToken[token] = 1; // Also arbitrary.
  
  Fork((int)Waiter);
  Acquire(lock_OrWr_BaggedOrders);
    baggedOrders[count_NumOrdersBaggedAndReady] = 12;
    count_NumOrdersBaggedAndReady++;
    Signal(CV_OrWr_BaggedOrders, lock_OrWr_BaggedOrders);
    Acquire(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
  Release(lock_OrWr_BaggedOrders);
  
	Wait(CV_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]],
		   lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);

  // If we get here, the waiter signaled us correctly.
}

/* Untested */
void TestCook()
{
  // Hire a cook for each type of food.
  for (int i = 0; i < 5; ++i)
    hireCook(i);

  // Wait to verify all types of food are being cooked in print statements.
  Yield(10000);

  // Send all cooks on break.
  for (int i = 0; i < 5; ++i)
  {
    Acquire(lock_MrCk_InventoryLocks[i]);
    Get_CookOnBreakFromInventoryIndex[i] = TRUE;
    Release(lock_MrCk_InventoryLocks[i]);
  }

  // Wait to verify no food is being cooked in print statements.
  Yield(10000);
    
  // Bring all cooks back from break.
  for (int i = 0; i < 5; ++i)
  {
    Acquire(lock_MrCk_InventoryLocks[i]);
    Get_CookOnBreakFromInventoryIndex[i] = FALSE;
    Release(lock_MrCk_InventoryLocks[i]);
  }
    
  // Wait to verify all types of food are being cooked in print statements.
  Yield(10000);
  
  // Deplete the inventory for all food.
  for (int i = 0; i < 5; ++i)
  {
    Acquire(lock_MrCk_InventoryLocks[i]);
    inventoryCount[i] = 0;
    Release(lock_MrCk_InventoryLocks[i]);
  }
    
  // Wait to verify all types of food are being cooked in print statements.
  Yield(10000);
}

/* Untested / Unfinished */
void TestOrderTaker()
{
  // "Cook" food for the OrderTaker to use.
  
  
  // Test for an eat-in customer.
  int orderCombo = menu[0];
	Get_CustomerOrderFoodChoiceFromOrderTakerID[ID_Get_OrderTakerIDFromCustomerID[0]] = menu[orderCombo];
  Get_CustomerTogoOrEatinFromCustomerID[0] = 1;

 
  // Create an OrderTaker
  Fork((int)OrderTaker);
  
	printf("Customer::Ordering #%d from OrderTaker", orderCombo);
  PrintNumber(orderCombo);
  PrintOut(" from OrderTaker #", 17);
	PrintOut("\n", 1);
  
	Signal(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[0]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[0]]);
  
	// Wait for order taker to respond.
	Wait(CV_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[0]], 
					lock_OrderTakerBusy[ID_Get_OrderTakerIDFromCustomerID[0]]);
	
	PrintOut("Customer", 8);
	PrintNumber(0);
	PrintOut("::Paying OrderTaker #", 21);
	PrintNumber(ID_Get_OrderTakerIDFromCustomerID[0]);
	PrintOut("\n", 1);
}

/* Untested */
void TestManager()
{

}

/* Untested / Unfinished */
void TestSimulation()
{
  Initialize();
  
  TestCustomer();
  TestWaiter();
  TestCook();
  TestOrderTaker();
  TestManager();
}

/*
Customers who wants to eat-in, must wait if the restaurant is full.
*/
void Test1CustomerWaitsIfRestaurantFull()
{
  PrintOut("Test1::Customers who wants to eat-in, must wait if the restaurant is full.\n",75);
  PrintOut("Starting Simulation\n",20);  
	PrintOut("Initializing...\n",16);
	Initialize();
  
  // Set the restaurant as full for test.
  count_NumTablesAvailable = 0;
  test_AllCustomersEatIn = TRUE;
  
  int numOrderTakers = 0;
  int numWaiters = 0;
  int numCustomers = 1;
  
	PrintOut("Initialized\n",12);
  
	PrintOut("Running Simulation with:\n",25);
  PrintNumber(numOrderTakers);
  PrintOut(" OrderTaker(s)\n",15);
  PrintNumber(numWaiters);
  PrintOut(" Waiter(s)\n",11);
  PrintNumber(numCustomers);
  PrintOut(" Customer(s)\n",13);
  PrintOut("===================================================\n",52);
		
	Fork((int)Manager);
  for (int i = 0; i < numOrderTakers; ++i)
    Fork((int)OrderTaker);
  for (int i = 0; i < numWaiters; ++i)
    Fork((int)Waiter);
	for (int i = 0; i < numCustomers; ++i)
		Fork((int)Customer);
}

/*
OrderTaker/Manager gives order number to the Customer when the food is not ready.
*/
void Test2FoodNotReady()
{
  PrintOut("Test2::OrderTaker/Manager gives order number to the Customer when the food is not ready.\n",89);
  PrintOut("Starting Simulation\n",20);  
	PrintOut("Initializing...\n",16);
	Initialize();
  
  // Disable cooks so food will never be ready.
  test_NoCooks = TRUE;
  test_AllCustomersOrderThisCombo = 3;
  
  int numOrderTakers = 1;
  int numWaiters = 1;
  int numCustomers = 1;
  
	PrintOut("Initialized\n",12);
  
	PrintOut("Running Simulation with:\n",25);
  PrintNumber(numOrderTakers);
  PrintOut(" OrderTaker(s)\n",15);
  PrintNumber(numWaiters);
  PrintOut(" Waiter(s)\n",11);
  PrintNumber(numCustomers);
  PrintOut(" Customer(s)\n",13);
  PrintOut("===================================================\n",52);
	
	Fork((int)Manager);
  for (int i = 0; i < numOrderTakers; ++i)
    Fork((int)OrderTaker);
  for (int i = 0; i < numWaiters; ++i)
    Fork((int)Waiter);
	for (int i = 0; i < numCustomers; ++i)
		Fork((int)Customer);
}
/*
Customers who have chosen to eat-in, must leave after they have their food and Customers 
who have chosen to-go, must leave the restaurant after the OrderTaker/Manager has given the food.
*/
void Test3CustomerLeavingRestaurant()
{
  PrintOut("Test3::Customers who have chosen to eat-in, must leave after they have their food and Customers\nwho have chosen to-go, must leave the restaurant after the OrderTaker/Manager has given the food.\n",194);
  RunSimulation(3, 3, 4);
}

/*
Manager maintains the track of food inventory. Inventory is refilled when it goes below order level.
*/
void Test4InventoryTracking()
{
  PrintOut("Test4::Manager maintains the track of food inventory. Inventory is refilled when it goes below order level.\n",108);
  RunSimulation(3, 3, 20);
}

/*
A Customer who orders only soda need not wait.
*/
void Test5SodaWait()
{
  PrintOut("Test5::A Customer who orders only soda need not wait.\n",54);
  PrintOut("Starting Simulation\n",20);  
	PrintOut("Initializing...\n",16);
	Initialize();
  
  // Have all customers order a soda.
  test_AllCustomersOrderThisCombo = 0;
  test_AllCustomersEatOut = TRUE;
  
  int numOrderTakers = 1;
  int numWaiters = 1;
  int numCustomers = 4;
  
	PrintOut("Initialized\n",12);
  
	PrintOut("Running Simulation with:\n",25);
  PrintNumber(numOrderTakers);
  PrintOut(" OrderTaker(s)\n",15);
  PrintNumber(numWaiters);
  PrintOut(" Waiter(s)\n",11);
  PrintNumber(numCustomers);
  PrintOut(" Customer(s)\n",13);
  PrintOut("===================================================\n",52);
	
	Fork((int)Manager);
  for (int i = 0; i < numOrderTakers; ++i)
    Fork((int)OrderTaker);
  for (int i = 0; i < numWaiters; ++i)
    Fork((int)Waiter);
	for (int i = 0; i < numCustomers; ++i)
		Fork((int)Customer);
}

/*
The OrderTaker and the Manager both somethimes bag the food.
*/
void Test6ManagerBagsFood()
{
  PrintOut("Test6::The OrderTaker and the Manager both somethimes bag the food.\n",68);
  RunSimulation(1, 1, 40);
}

/*
Manager goes to the bank for cash when inventory is to be refilled and there is no cash in the restaurant.
*/
void Test7BankTrip()
{
  PrintOut("Test7::Manager goes to the bank for cash when inventory is to be refilled and there is no cash in the restaurant.\n",114);
  RunSimulation(3, 3, 40);
}

/*
Cooks goes on break when informed by manager.
*/
void Test8CookBreak()
{
  PrintOut("Test8::Cooks goes on break when informed by manager.\n",53);
  RunSimulation(1, 1, 1);
}

#endif
