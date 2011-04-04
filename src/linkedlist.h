//
// linkedlist.h
//

// Simple template to track a linked list.  This is a doubly linked list
// written by the 2000-TA Andre Bergholz
// who has spent some effort making the access methods more efficient
// for very long lists.

// This linked list is base 0
// In order to do a traversal through all of the elements:
//   for (int i=0; i<list.GetLength(); i++) {
//       <class T> *pT = list[i]
//   }

#ifndef TEMPL_LINKED_LIST_H
#define TEMPL_LINKED_LIST_H

//
// Definition of Linked list template class
//
template <class T>
class LinkList
{
   public:
      // Simple default Constructor
      LinkList ( );

      // Copy constructor
      LinkList ( const LinkList<T> & sourcell );

      // Destructor
      ~LinkList();

      // Assignment operator
      void operator = ( const LinkList<T> & sourcell );

      // Equality tester
      Boolean operator == ( const LinkList<T> & rhs ) const;

      // Copy the LinkList and convert it to an array.  Caller is
      // responsible for releasing memory.
      operator T * ();

      // Append new item
      void Append ( const T & item );
      void Append ( const LinkList<T> & sourcell );

      // Other, somtimes more intuitive, appending functions
      LinkList<T> operator+(const LinkList<T> &sourcell) const;
      LinkList<T> operator+(const T &element) const;
      void operator+=(const LinkList<T> &sourcell);
      void operator+=(const T &element);

      // Delete current item
      void Delete( int index );

      // Remove all items from the list
      void Erase();

      // Access methods
      //
      // NOTE:  All access methods will return pointers to elements of Type
      // T.  Pointers were chosen since I didn't feel like using
      // exceptions.  The client should *not* delete the pointer returned
      // when done.  You can modify the object in place, it will be
      // reflected within the list.

      // Get current item.  Valid ranges for the index are:
      // 0..(1-GetLength())
      T* Get( int index );

      // A more intuitive way to grab the next element
      T* operator[](int index);

      // Get the length of the linked list
      int  GetLength() const;

   protected:
      // An internal node within the linked list is a single element.  It
      // contains the data that we are storing (of type T defined by the
      // template) and pointers to the previous and next.
      struct InternalNode {
           T Data;
           InternalNode * next;
           InternalNode * previous;
      };

      int iLength;               // # of items in list
      InternalNode *pnHead;      // First item
      InternalNode *pnTail;      // Last item
      InternalNode *pnLastRef;   // Current item
      int iLastRef;              // Which element # last referenced

      // Set the Private members of the class to initial values
      void SetNull();
};

/****************************************************************************/

//
// SetNull
//
// This will set (or reset) the intial values of the private members of
// LinkList
//
template <class T>
inline void LinkList<T>::SetNull()
{
   pnHead = NULL;
   pnTail = NULL;
   pnLastRef = NULL;
   iLength = 0;
   iLastRef = -1;
}

/****************************************************************************/

//
// LinkList
//
// Constructor
//
template <class T>
inline LinkList<T>::LinkList ()
{
   SetNull();
}

/****************************************************************************/

//
// LinkList ( const LinkList<T> &sourcell )
//
// Copy constructor
//
template <class T>
LinkList<T>::LinkList ( const LinkList<T> & sourcell )
{
   // Initialize the new list
   SetNull();

   // And copy all the members of the passed in list
   if (sourcell.iLength == 0)
      return;

   InternalNode *n = sourcell.pnHead;

   while (n != NULL)
   {
      Append(n->Data);
      n = n->next;
   }
   pnLastRef = pnHead;
}

/****************************************************************************/

//
// ~LinkList
//
// Destructor
//
template <class T>
inline LinkList<T>::~LinkList()
{
   Erase();
}

/****************************************************************************/

//
// Operator =
//
// Assignment operator
//
template <class T>
void LinkList<T>::operator = ( const LinkList<T> & sourcell )
{
   // First erase the original list
   Erase();

   // Now, copy the passed in list
   InternalNode *pnTemp = sourcell.pnHead;

   while (pnTemp != NULL)
   {
      Append(pnTemp->Data);
      pnTemp = pnTemp->next;
   }

   pnLastRef = NULL;
   iLastRef = -1;
}

/****************************************************************************/

//
// Operator ==
//
// Test for equality of two link lists
//
template <class T>
Boolean LinkList<T>::operator == ( const LinkList<T> & rhs ) const
{
   if (iLength != rhs.iLength)
      return (FALSE);

   InternalNode *pnLhs = this->pnHead;
   InternalNode *pnRhs = rhs.pnHead;

   while (pnLhs != NULL && pnRhs != NULL)
   {
      // The Data type T set by the template had better define an equality
      // operator for their data type!
      if (!(pnLhs->Data == pnRhs->Data))
         return FALSE;
      pnLhs = pnLhs->next;
      pnRhs = pnRhs->next;
   }

   if (pnLhs==NULL && pnRhs==NULL)
      return TRUE;
   else
      return FALSE;
}
/****************************************************************************/

//
// Conversion to array operator
//
// This returns a copy of the list and the caller must delete it when done.
//
template <class T>
LinkList<T>::operator T * ()
{
   if (iLength == 0)
      return NULL;

   T *pResult = new T[iLength];

   InternalNode *pnCur = pnHead;
   T *pnCopy = pResult;

   while (pnCur != NULL)
   {
      *pnCopy = pnCur->Data;
      ++pnCopy;
      pnCur = pnCur->next;
   }

   // Note:  This is a copy of the list and the caller must delete it when
   // done.
   return pResult;
}

/****************************************************************************/

//
// Append
//
// Append new item to the end of the linked list
//
template <class T>
inline void LinkList<T>::Append ( const T & item )
{
   InternalNode *pnNew = new InternalNode;

   pnNew->Data = item;
   pnNew->next = NULL;
   pnNew->previous = pnTail;

   // If it is the first then set the head to this element
   if (iLength == 0)
   {
      pnHead = pnNew;
      pnTail = pnNew;
      pnLastRef = pnNew;
   }
   else
   {
      // Set the tail to be this new element
      pnTail->next = pnNew;
      pnTail = pnNew;
   }

   ++iLength;
}

/****************************************************************************/

template <class T>
inline LinkList<T>
LinkList<T>::operator+(const LinkList<T> &sourcell) const
{
    LinkList<T> pTempLL(*this);
    pTempLL += sourcell;
    return pTempLL;
}

/****************************************************************************/

template <class T>
inline LinkList<T>
LinkList<T>::operator+(const T &element) const
{
    LinkList<T> pTempLL(*this);
    pTempLL += element;
    return pTempLL;
}

/****************************************************************************/

template <class T>
void
LinkList<T>::operator+=(const LinkList<T> &list)
{
    const InternalNode *pnTemp;
    const int iLength = list.iLength;
    int i;

    // Must use size as stopping condition in case *this == list.
    for (pnTemp = list.pnHead, i=0; i < iLength; pnTemp = pnTemp->next, i++)
        *this += pnTemp->Data;
}

/****************************************************************************/

template <class T>
void
LinkList<T>::operator+=(const T &element)
{
    InternalNode *pnNew = new InternalNode;
    pnNew->next = NULL;
    pnNew->Data = element;
    if (iLength++ == 0) {
        pnHead = pnNew;
        pnNew->previous = NULL;
    }
    else {
        pnTail->next = pnNew;
        pnNew->previous = pnTail;
    }
    pnTail = pnNew;
}

/****************************************************************************/

template <class T>
void LinkList<T>::Append ( const LinkList<T> & sourcell )
{
   const InternalNode *pnCur = sourcell.pnHead;

   while (pnCur != NULL)
   {
      Append(pnCur->Data);
      pnCur = pnCur->next;
   }
}

/****************************************************************************/

//
// Delete
//
// Delete the specified element
//
template <class T>
inline void LinkList<T>::Delete(int which)
{
   if (which>iLength || which == 0)
      return;

   InternalNode *pnDeleteMe = pnHead;

   for (int i=1; i<which; i++)
      pnDeleteMe = pnDeleteMe->next;

   if (pnDeleteMe == pnHead)
   {
      if (pnDeleteMe->next == NULL)
      {
         delete pnDeleteMe;
         SetNull();
      }
      else
      {
         pnHead = pnDeleteMe->next;
         pnHead->previous = NULL;
         delete pnDeleteMe;
         pnLastRef = pnHead;
      }
   }
   else
   {
      if (pnDeleteMe == pnTail)
      {
         if (pnDeleteMe->previous == NULL)
         {
            delete pnDeleteMe;
            SetNull();
         }
         else
         {
            pnTail = pnDeleteMe->previous;
            pnTail->next = NULL;
            delete pnDeleteMe;
            pnLastRef = pnTail;
         }
      }
      else
      {
         pnLastRef = pnDeleteMe->next;
         pnDeleteMe->previous->next = pnDeleteMe->next;
         pnDeleteMe->next->previous = pnDeleteMe->previous;
         delete pnDeleteMe;
      }
   }

   if (iLength!=0)
      --iLength;
}

/****************************************************************************/

template <class T>
inline T* LinkList<T>::operator[](int index)
{
	return (Get(index));
}

/****************************************************************************/

//
// Erase
//
// remove all items from the list
//
template <class T>
inline void LinkList<T>::Erase()
{
   pnLastRef = pnHead;

   while (pnLastRef != NULL)
   {
      pnHead = pnLastRef->next;
      delete pnLastRef;
      pnLastRef = pnHead;
   }
   SetNull();
}

/****************************************************************************/

// Get
//
// Get a specified item.  Notice here that index can be between 0 and
// 1-iLength.  Once we determine this I add 1 to the index in order to make
// the get function easier.
//
template <class T>
inline T* LinkList<T>::Get(int index)
{
	int iCur;               // Position to start search from
	InternalNode *pnTemp;   // Node to start search from
	int iRelToMiddle;       // Position asked for relative to last ref

   // Make sure that item is within bounds
	if (index < 0 || index >= iLength)
		return NULL;

   // Having the index be base 1 makes this procedure much easier.
   index++;

	if (iLastRef==-1)
		if (index < (iLength-index)) {
			iCur = 1;
			pnTemp = pnHead;
		} else {
			iCur = iLength;
			pnTemp = pnTail;
		}
	else
		{
			if (index < iLastRef)
				iRelToMiddle = iLastRef - index;
			else
				iRelToMiddle = index - iLastRef;

			if (index < iRelToMiddle) {
				// The head is closest to requested element
				iCur = 1;
				pnTemp = pnHead;
			}
			else
				if (iRelToMiddle < (iLength - index)) {
					iCur = iLastRef;
					pnTemp = pnLastRef;
				} else {
					iCur = iLength;
					pnTemp = pnTail;
				}
		}

	// Now starting from the decided upon first element
	// find the desired element
	while (iCur != index)
		if (iCur < index) {
			iCur++;
			pnTemp = pnTemp->next;
		} else {
			iCur--;
			pnTemp = pnTemp->previous;
		}

	iLastRef = index;
	pnLastRef = pnTemp;

   return &(pnLastRef->Data);
}

/****************************************************************************/

template <class T>
inline int LinkList<T>::GetLength() const
{
   return iLength;
}

#endif
