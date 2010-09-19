#ifdef CHANGED

#include "simulation.c"

void TestCustomer()
{
  Thread t = new Thread("Customer0");
  for (int i= 0; i < 5; ++i)
    t->Fork((VoidFunctionPtr)Customer, 0);

  // Get the customers into the restaurant.
  for (int i= 0; i < 5; ++i)
    checkLineToEnterRest();
  
  // Take the customers' orders.
  for (int i= 0; i < 5; ++i)
    ServiceCustomer();
  
  // Give the customers their orders
  for (int i = 0; i < maxNumOrders; ++i)
  {
    if (ordersNeedingBagging[i] > 0)
    {
      // If the customer is eat-in.
      if (Get_CustomerTogoOrEatinFromCustomerID[GET_CustomerIDFromOrderNumber[i]] == 1)
      {
        // Pretend we are waiter delivering food and signal the eat-in customer directly.
        acquire(lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[i]]);
        signal(CV_eatinCustomersWaitingForFood[customerNumberFromorderNumber[i]], 
               lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[i]]);
        release(lock_eatinCustomersWaitingForFood[customerNumberFromorderNumber[i]]);
      }
      else // if customer is togo.
      {
        // Tell the waiting togo customers that order i is ready.
        acquire(lock_OrderReady);
          bool_ListOrdersReadyFromToken[i] = 1;
          broadcast(lock_OrderReady, CV_OrderReady);
        release(lock_OrderReady);
      }
    }
  }
}

void TestSimulation()
{
  TestCustomer();
}

#endif