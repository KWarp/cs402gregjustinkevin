
#include "IPT.h"
#include "system.h"

using namespace std;

// Constructor.
IPT::IPT()
{
  table = new IPTentry[NumPhysPages];
  tableSize = 0;
  lock = new Lock ("IPT Lock");
}

// Destructor.
IPT::~IPT()
{
  delete table;
  delete lock;
}

