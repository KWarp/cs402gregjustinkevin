/*    
 *  teststartuserprogram.c
 *	Simple program to test whether running a user program works in the network directory.
 *	
 */

#include "syscall.h"

int main()
{
  PrintOut("===TestStartUserProgram===\n", 27);
  
  StartUserProgram();
  
  PrintOut("===TestStartUserProgram Completed===\n", 37);
}
