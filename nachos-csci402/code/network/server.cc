#include "netcall.h"
#include "list.h"

void Server()
{
        printf("Running Server.  Server count: %i;  Server id: %i\n", serverCount, postOffice->GetID());

   while(1)
   {
                if (!(HandleRequest()))
				{
                }
   }
}
