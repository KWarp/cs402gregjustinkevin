/* customer.c
 * A customer for the restaurant simulation
 */

#include "syscall.h"
#include "simulation.c"



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
	SetMV(count_WhenAllCustomersServed, ID+1);
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
	SetMV(Get_CustomerTogoOrEatinFromCustomerID[ID], eatIn);
	/* Ordered, Signal Order Taker - this represents talking to ordertaker */
	
	PrintOut("Customer ", 9);
	PrintNumber(ID);
	PrintOut(" is giving order to ", 20);  
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
					lock_OrderTakerBusy[GetMV( ID_Get_OrderTakerIDFromCustomerID[ID])] );
	
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
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])] );
  
	/* Wait for order taker to respond */
	Wait(CV_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])], 
					lock_OrderTakerBusy[GetMV(ID_Get_OrderTakerIDFromCustomerID[ID])] );
   
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


int main()
{
  int i;
  PrintOut("=== Customer ===\n", 17);
  
  StartUserProgram();
  Initialize();
  for(i = 0; i < 5000; i++)
  {  
    Yield(1);
  }
  Customer(0);
  return 0;
}
