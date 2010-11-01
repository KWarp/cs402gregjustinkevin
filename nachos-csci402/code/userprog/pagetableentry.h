
#ifndef PAGETABLEENTRY_H
#define PAGETABLEENTRY_H

#include "../filesys/openfile.h"

enum Location {IN_EXECUTABLE, IN_SWAP_FILE, NEITHER};

class PageTableEntry
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
  Location location;  // Used to keep track of where to find the page
                      // in the event of a page fault.
  OpenFile* executable; // Used to store the executable so we can read from 
                        // it later.
  int byteOffset;     // The offset of the page on disk in the executable.
  int swapPageIndex;  // the index of the page in the swap file
};

#endif
