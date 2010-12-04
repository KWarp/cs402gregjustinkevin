/* networksim.c
 * Launches the network restaurant simulation
 * Project 4
 */

#include "syscall.h"

/* Copy-Pasted code */
#define count_MaxNumOrderTakers 10
#define count_MaxNumCustomers 50
#define count_MaxNumWaiters 10

#define count_defaultNumOrderTakers 3
#define count_defaultNumCustomers 20
#define count_defaultNumWaiters 3

/* new stuff */
#define count_MaxNumCooks 5
#define count_defaultNumCooks 5
#define count_NumTablesAvailable 10

void RunSimulation(int numOrderTakers, int numWaiters, int numCustomers, int numCooks);


/*
 * Starts the main simulation
 */
int main()
{
  int i, numOrderTakers, numWaiters, numCustomers, numCooks;
  
  i = GetConfigArg();
  
  switch(i)
  {
	case 1:
		/* A "large" Carl's Jr implementation that runs on a single instance of Nachos. */
		PrintOut("A \"large\" Carl's Jr implementation that runs on a single instance of Nachos.\n", 77);
		numOrderTakers = 5;
		numWaiters = 5; 
		numCustomers = 20; 
		numCooks = 5;
		break;
	case 2:
		/* A reduced Carl's Jr that runs on at least 2 different Nachos instances. Instance 1 */
		PrintOut("A reduced Carl's Jr that runs on at least 2 different Nachos instances. Instance 1\n", 83);
		numOrderTakers = 2;
		numWaiters = 2; 
		numCustomers = 0; 
		numCooks = 0;
		break;
	case 3:
		/* A reduced Carl's Jr that runs on at least 2 different Nachos instances. Instance 2 */
		PrintOut("A reduced Carl's Jr that runs on at least 2 different Nachos instances. Instance 2\n", 83);
		numOrderTakers = 0;
		numWaiters = 0; 
		numCustomers = 5; 
		numCooks = 5;
		break;
	default:
		/* An arg of zero is invalid */
		PrintOut("Config argument of ", 19); 
		PrintNumber(i); 
		PrintOut(" is invalid\n", 12);
		PrintOut("Use -config 1 for Test 1\n", 25);
		PrintOut("Use -config 2 and -config 3 (on different nachos instances) for Test 2\n", 71);
		Exit(0);
		return 0;
  }
  
  RunSimulation(numOrderTakers, numWaiters, numCustomers, numCooks);
  
  Exit(0);
  return 0;
}


/* =============================================================
 * Runs the simulation
 * =============================================================*/
void RunSimulation(int numOrderTakers, int numWaiters, int numCustomers, int numCooks)
{
  int i;

  if (numCustomers < 0 || numCustomers > count_MaxNumCustomers)
  {
    PrintOut("Setting number of customers to default\n", 39);
    numCustomers = count_defaultNumCustomers;
  }
  if (numOrderTakers < 0 || numOrderTakers > count_MaxNumOrderTakers)
  {
    PrintOut("Setting number of order takers to default\n", 42);
    numOrderTakers = count_defaultNumOrderTakers;
  }
  if (numWaiters < 0 || numWaiters > count_MaxNumWaiters)
  {
    PrintOut("Setting number of waiters to default\n", 37);
    numWaiters = count_defaultNumWaiters;
  }
  if (numCooks < 0 || numCooks > count_MaxNumCooks)
  {
    PrintOut("Setting number of cooks to default\n", 35);
    numCooks = count_defaultNumCooks;
  }
  
	/* Initialize Locks, CVs, and shared data 
	CHANGE: Occurs at the start of each exec
	Initialize();
	*/
	
	PrintOut("\nNumber of Managers = ", 22);
	if(numCooks > 0)
		PrintNumber(1);
	else
		PrintNumber(0);
	
	PrintOut("\nNumber of OrderTakers = ", 25);
	PrintNumber(numOrderTakers);
	PrintOut("\nNumber of Waiters = ", 21);
	PrintNumber(numWaiters);
	PrintOut("\nNumber of Cooks = ", 19);
	PrintNumber(numCooks);
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
	
	/* If 2+ instances of nachos, only make a manager on the instance with the cooks */
	if(numCooks > 0)
		Exec("../test/manager");
	
	for (i = 0; i < numCustomers; ++i)
		Exec("../test/customer");
	for (i = 0; i < numOrderTakers; ++i)
		Exec("../test/ordertaker");
	for (i = 0; i < numWaiters; ++i)
		Exec("../test/waiter");
	for (i = 0; i < numCooks; ++i)
		Exec("../test/cook");	
	
}

