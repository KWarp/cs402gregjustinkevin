#ifndef SIMULATION_H
#define SIMULATION_H

/*
 * simulation.h
 *
 * Defines all of the methods used in Simulation.c
 *
 */
 

/* Customer */ 
void Customer();
void WaitInLineToEnterRest(int ID);
void WaitInLineToOrderFood(int ID);

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

void RunSimulation(int scenario);

#endif  /* SIMULATION.H */






