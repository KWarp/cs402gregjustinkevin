#ifdef CHANGED

#include "simulation.c"

void TestCustomer()
{
  for (int i= 0; i < 5; ++i)
	Fork(Customer);
	
  // Get the customers into the restaurant.
  for (int i= 0; i < 5; ++i)
    checkLineToEnterRest();
  
  // Take the customers' orders.
  for (int i= 0; i < 5; ++i)
    ServiceCustomer();
  
  // Give the customers their orders
  for (int token = 0; token < count_NumCustomers; ++token)
  {
    if (ordersNeedingBagging[token] > 0)
    {
      // If the customer is eat-in.
      if (Get_CustomerTogoOrEatinFromCustomerID[GET_CustomerIDFromOrderNumber[token]] == 1)
      {
        // Pretend we are waiter delivering food and signal the eat-in customer directly.
        Acquire(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
        signal(CV_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]],
               lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
        Release(lock_CustomerSittingFromCustomerID[Get_CustomerIDFromToken[token]]);
      }
      else // if customer is togo.
      {
        // Tell the waiting togo customers that order i is ready.
        Acquire(lock_OrderReady);
          bool_ListOrdersReadyFromToken[token] = 1;
          Broadcast(lock_OrderReady, CV_OrderReady);
        Release(lock_OrderReady);
      }
    }
  }
}

void TestWaiter()
{
  int token = 12; // Arbitrary order number.
  Get_CustomerIDFromToken[token] = 1; // Also arbitrary.
  
  Fork((VoidFunctionPtr)Waiter);
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

void TestOrderTaker()
{

}

void TestManager()
{

}

void TestSimulation()
{
  // Initialize();
  TestCustomer();
  TestWaiter();
  TestCook();
  TestOrderTaker();
  TestManager();
}

#endif