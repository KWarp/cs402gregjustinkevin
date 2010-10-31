
#ifndef IPT_H
#define IPT_H

#include "../threads/synch.h"     // For Lock.
#include "../machine/translate.h" //For TranslationEntry.
#include "addrspace.h"

class IPTEntry
{
 public:
  int virtualPage;  	// The page number in virtual memory.
  int physicalPage;  	// The page number in real memory (relative to the
                      //  start of "mainMemory"
  bool valid;         // If this bit is set, the translation is ignored.
                      // (In other words, the entry hasn't been initialized.)
  bool readOnly;	    // If this bit is set, the user program is not allowed
                      // to modify the contents of the page.
  bool use;           // This bit is set by the hardware every time the
                      // page is referenced or modified.
  bool dirty;         // This bit is set by the hardware every time the
                      // page is modified.
  AddrSpace* processID; // The processID of the vpn.
};

class IPT
{
 public:
  IPT();
  ~IPT();
 
 private:
  IPTEntry* ipt;
  int tableSize;
  Lock* lock;
};

#endif
