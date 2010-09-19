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
void waitInLineToOrderFood(int ID)

/* Manager */
void Manager();
void callWaiter();
void orderInventoryFood();
void manageCook();
void hireCook(index);
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
void printString(char[] s, int length);
void printNumber(int i);



#endif  /* SIMULATION.H */





