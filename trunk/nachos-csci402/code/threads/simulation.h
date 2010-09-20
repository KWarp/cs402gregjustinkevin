#ifndef SIMULATION_H
#define SIMULATION_H

/*
 * simulation.h
 *
 * Defines all of the methods used in Simulation.c
 *
 */
 
void RunSimulation(int numOrderTakers, int numWaiters, int numCustomers);
 
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
void bagOrder();

/* Waiter */
void Waiter(int debug);

#endif  /* SIMULATION_H */






