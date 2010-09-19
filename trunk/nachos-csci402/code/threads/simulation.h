#ifndef SIMULATION.H
#define SIMULATION.H

/*
 * simulation.h
 *
 * Defines all of the methods used in Simulation.c
 *
 */
 

/* Customer */ 
void Customer(int ID);
void waitInLineToEnterRest(int ID);
void waitInLineToOrderFood(int ID);

/* Manager */
void Manager();
void callWaiter();
void orderInventoryFood();
void manageCook();
void hireCook(int index);
void checkLineToEnterRest();

/* Cook */
void Cook();

/* Order Taker */
void OrderTaker();
void serviceCustomer(int ID);
void bagOrder();

/* Waiter */
void Waiter();

/* Utility */
int randomNumber(int count);
void PrintOut(unsigned int vaddr, int len);
void printNumber(int i);

void RunSimulation(int scenario);

#endif  /* SIMULATION.H */






