#pragma once
/*************************  DynamicQueue.cpp  *********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-07-24
* Description:
* Defines First-In-First-Out queue of dynamic size.
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* DynamicQueue is a container class defining a First-In-First-Out queue  
* that can contain any number of objects of the same type.
*
* The objects stored can be of any type that does not require a constructor
* or destructor. 
*
* DynamicQueue is not thread safe if shared between threads. If your program 
* needs storage in multiple threads then each thread must have its own 
* private instance of DynamicQueue, or you must prevent multiple threads from
* accessing DynamicQueue at the same time.
*
* Note that you should never store a pointer to an object in DynamicQueue
* because the pointer will become invalid in case a subsequent addition of 
* another object causes the memory to be re-allocated.
*
* Attempts to read from an empty queue will cause an error message to the 
* standard error output. You may change the DynamicQueue::Error function 
* to produce a message box if the program has a graphical user interface.
*
* It is possible, but not necessary, to allocate a certain amount of 
* memory before adding any objects. This can reduce the risk of having
* to re-allocate memory if the first allocated memory block turns out
* to be too small. Use Reserve(n) to reserve space for a dynamically
* growing queue.
*
* At the end of this file you will find a working example of how to use 
* DynamicQueue.
*
* The first part of this file containing declarations may be placed in a 
* header file. The second part containing examples should be removed from your
* final application.
*
******************************************************************************/

#include <memory.h>                              // For memcpy and memset
#include <stdlib.h>                              // For exit in Error function
#include <stdio.h>                               // Needed for example only

#include <headers.hpp>


// Class DynamicQueue makes a dynamic array which can grow as new data are added
template <typename TX>
class DynamicQueue
{
public:
   DynamicQueue();                               // Constructor
   ~DynamicQueue();                              // Destructor
   void Reserve(int num);                        // Allocate buffer for num objects
   int GetNum(){return NumEntries;};             // Get number of objects stored
   bool IsEmpty() {return GetNum() == 0; };             // Get number of objects stored
   inline bool empty() { return IsEmpty(); }
   int GetMaxNum(){return MaxNum;};              // Get number of objects that can be stored without re-allocating memory
   void Put(TX const & obj);                     // Add object to head of queue
   inline TX push_back(TX const & obj) { Put(obj); }
   TX Get();                                     // Take object out from tail of queue
   inline TX pop_front() { return Get(); }
   TX & operator[] (int i);                      // Access object with index i from the tail
   inline TX front() { return operator[](GetNum() - 1); }
   // Define desired allocation size
   enum DefineSize {
      AllocateSpace = 4 * 1024                       // Minimum size, in bytes, of automatic re-allocation done by Put
   };
private:
   TX * Buffer;                                  // Buffer containing data
   TX * OldBuffer;                               // Old buffer before re-allocation
   int head;
   int tail;
   int MaxNum;                                   // Maximum number of objects that currently allocated buffer can contain
   int NumEntries;                               // Number of objects stored
   void ReAllocate(int num);                     // Increase size of memory buffer
   void Error(int e, int n);                     // Make fatal error message
   DynamicQueue(DynamicQueue const&){};          // Make private copy constructor to prevent copying
   void operator = (DynamicQueue const&){};      // Make private assignment operator to prevent copying
};


// Members of class DynamicQueue
template <typename TX>
DynamicQueue<TX>::DynamicQueue() {  
   // Constructor
   Buffer = OldBuffer = 0;
   MaxNum = NumEntries = head = tail = 0;
}


template <typename TX>
DynamicQueue<TX>::~DynamicQueue() {
   // Destructor
   Reserve(0);                                   // De-allocate buffer
}


template <typename TX>
void DynamicQueue<TX>::Reserve(int num) {
   // Allocate buffer of the specified size
   // Setting num > current MaxNum will allocate a larger buffer and 
   // move all data to the new buffer.
   // Setting num <= current MaxNum will do nothing. The buffer will 
   // only grow, not shrink.
   // Setting num = 0 will discard all data and de-allocate the buffer.
   if (num <= MaxNum) {
      if (num <= 0) {
         if (num < 0) Error(1, num);
         // num = 0. Discard data and de-allocate buffer
         if (Buffer) nb_free(Buffer);            // De-allocate buffer
         Buffer = 0;                             // Reset everything
         MaxNum = NumEntries = head = tail = 0;
         return;
      }
      // Request to reduce size. Ignore
      return;
   }
   // num > MaxNum. Increase Buffer
   ReAllocate(num);
   // OldBuffer must be deleted after calling ReAllocate
   if (OldBuffer) {nb_free(OldBuffer);  OldBuffer = 0;}
}


template <typename TX>
void DynamicQueue<TX>::ReAllocate(int num) {
   // Increase size of memory buffer. 
   // This function is used only internally. 
   // Note: ReAllocate leaves OldBuffer to be deleted by the calling function
   if (OldBuffer) nb_free(OldBuffer);            // Should not occur in single-threaded applications

   TX * Buffer2 = 0;                             // New buffer
   Buffer2 = static_cast<TX *>(nb_malloc(sizeof(TX) * num));                        // Allocate new buffer
   if (Buffer2 == 0) {Error(3,num); return;}     // Error can't allocate
   if (Buffer) {
      // A smaller buffer is previously allocated
      // Copy queue from old to new buffer
      if (tail < head) {
         // trail is unbroken, copy as one block
         memcpy(Buffer2, Buffer + tail, NumEntries*sizeof(TX));
      }
      else if (NumEntries) {
         // trail is wrapping around or full, copy two blocks
         memcpy(Buffer2, Buffer + tail, (MaxNum - tail)*sizeof(TX));
         if (head) {
            memcpy(Buffer2 + (MaxNum - tail), Buffer, head*sizeof(TX));
         }
      }
      // Reset head and tail
      tail = 0;  head = NumEntries;
   }
   OldBuffer = Buffer;                           // Save old buffer. Deleted by calling function
   Buffer = Buffer2;                             // Save pointer to buffer
   MaxNum = num;                                 // Save new size
}


template <typename TX>
void DynamicQueue<TX>::Put(const TX & obj) {
   // Add object to buffer, return index

   if (NumEntries >= MaxNum) {
      // buffer too small or no buffer. Allocate more memory
      // Determine new size = 2 * current size + the number of objects that correspond to AllocateSpace
      int NewSize = MaxNum * 2 + (AllocateSpace+sizeof(TX)-1)/sizeof(TX);

      ReAllocate(NewSize);
   }
   // Insert new object at head
   Buffer[head] = obj;
   // Delete old buffer after copying object, in case obj was in old buffer
   if (OldBuffer) {nb_free(OldBuffer);  OldBuffer = 0;}
   // Make head point to next vacant slot
   if (++head >= MaxNum) head = 0;
   // Count entries
   NumEntries++;
}


template <typename TX>
TX DynamicQueue<TX>::Get() {
   // Remove last object and return it
   if (NumEntries <= 0) {
      // buffer is empty. Make error message
      Error(2, 0);
      // Return empty object
      TX temp;
      memset(&temp, 0, sizeof(temp));
      return temp;
   }
   // Pointer to object at tail
   TX * p = Buffer + tail;
   // Advance tail
   if (++tail >= MaxNum) tail = 0;
   // Count down entries
   NumEntries--;
   // Return object
   return *p;
}


template <typename TX>
TX & DynamicQueue<TX>::operator[] (int i) {
   // Access object at position i from tail
   if ((unsigned int)i >= (unsigned int)NumEntries) {
      // Index i does not exist
      Error(1, i);  i = 0;
   }
   // Calculate position
   i += tail;
   if (i >= MaxNum) i -= MaxNum; // Wrap around
   return Buffer[i];
}


// Produce fatal error message. Used internally.
// Note: If your program has a graphical user interface (GUI) then you
// must rewrite this function to produce a message box with the error message.
template <typename TX>
void DynamicQueue<TX>::Error(int e, int n) {
   // Define error texts
   static const char * ErrorTexts[] = {
      "Unknown error",                 // 0
      "Index out of range",            // 1
      "Queue is empty",                // 2
      "Memory allocation failed"       // 3
   };
   // Number of texts in ErrorTexts
   const unsigned int NumErrorTexts = sizeof(ErrorTexts) / sizeof(*ErrorTexts);

   // check that index is within range
   if ((unsigned int)e >= NumErrorTexts) e = 0;

   // Replace this with your own error routine, possibly with a message box:
   // fprintf(stderr, "\nDynamicArray error: %s (%i)\n", ErrorTexts[e], n);
   assert(false || ErrorTexts[e]);

   // Terminate execution
   // exit(1);
}
