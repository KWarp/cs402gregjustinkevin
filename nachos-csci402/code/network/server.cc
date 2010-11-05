// server.cc 
//      Test out lock-related message delivery between a server Nachos and multiple client nachos (user programs),
//      using the Post Office to coordinate delivery.

#include "netcall.h"
#include "list.h"

void Server()
{
        printf("Running Server.  Server count: %i;  Server id: %i\n", serverCount, postOffice->GetID());

   while(1)
   {
                if (!(HandleRequest())){
                        //something went wrong in handling request
                        /*sendError();*/
                }
   }
   // Then we're done!
    //interrupt->Halt();
}