/**
 * HashMap.h - hashmap implementation
 *
 * Copyright (c) 2006, Amanjit Singh Gill
 * All rights reserved.
 *
 * Redistribution and use in source and binary  forms, with or without
 * modification, are permitted provided  that the following conditions
 * are met:
 *
 * Redistributions of  source  code must  retain  the  above copyright
 * notice, this  list  of conditions   and the  following  disclaimer.
 * Redistributions in  binary form must  reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or   other     materials   provided  with     the
 * distribution. Neither the name of  Amanjit Singh Gill nor the names
 * of its contributors   may be used  to  endorse or promote  products
 * derived      from this software      without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS"  AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO,  THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND FITNESS
 * FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN  NO  EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL,   SPECIAL,   EXEMPLARY,     OR CONSEQUENTIAL    DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR  PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT  LIABILITY,  OR TORT   (INCLUDING  NEGLIGENCE OR  OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ag_container_hashmap
#define ag_container_hashmap

#include <string>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>

#ifdef __GNUC__
# if (__GNUC__ >= 3 && __GNUC_MINOR__ >= 4)    // 3.3 upwards
#   include <bits/stl_iterator_base_types.h>   // use some library internals
    using std::__iterator_category;
# endif
#endif

// XXX:
//#include "CustomAlloc.h"
//#include "dmalloc.h"

typedef unsigned char byte_t;

enum {
  numPrimes = 28
};

/** our prime list */
static const size_t rgPrimes[numPrimes] = {
  53ul,         97ul,         193ul,       389ul,       769ul,
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
  1610612741ul, 3221225473ul, 4294967291ul
};

/** 
 * retrieve the next prime 
 * @param lMySize: current size 
 */
inline size_t getNextPrime(size_t lMySize) {
  for (int x=0; x<numPrimes;x++) {
    if (rgPrimes[x] >= lMySize)
      return rgPrimes[x];
  }
  assert(false);
  return 0;
}

/** hash func "2" from sedgewick, algorithms in c++ */
inline size_t g_strHashFunc(const std::wstring& sKey) {
  int a = 31415, b = 27183;
  const wchar_t * v = sKey.c_str();
  int h;
  for (h=0; *v !=0; v++, a=a*b)
    h = (a*h + *v);
  return h;
}

/** Functor to halc string hash */
struct StrHashFunc : public std::unary_function<std::wstring, size_t> {
    size_t operator()(const std::wstring& sKey ) const {
      return g_strHashFunc(sKey);
    }
};

/** Alternative Functor to compare two strings */
struct EqFunc : public std::binary_function<std::wstring, std::wstring, bool> {
  bool operator()(const std::wstring s1, const std::wstring s2) const {
    return s1 == s2;
  }
};

/*******************************************************************/

template <bool flag,
  typename T,
  typename U>
struct MySelect {
  typedef T result;
};

template <typename T,
  typename U>
struct MySelect<false, T, U> {
  typedef U result;
};

/*******************************************************************/
 
const int    iHASH_INITSIZE = 16;    /**< initial hash capacity */
const double dGROWTH_FACT   = 1.25;  /**< growth factor */

namespace ag {

/**
 * generic hash_map
 * second edition. NOT documented yet, waiting for code to stabilize
 * NOT PRODUCTION READY
 * @author Amanjit Singh Gill
 */
template <class KEY_, 
  class VAL_, 
  class HASHFUNC_, 
  class EQUAL_ = std::equal_to<KEY_>,
  class ALLOC_ = std::allocator< std::pair<KEY_,VAL_> > >
class hash_map {
public:
	// XXX:
	struct ListEntry;

  /** default constructor */
  hash_map() : 
    m_ulCount(0), m_ulCapacity(0), m_rgBuckets(), m_hashFunc() {
    initBucketsSize_(iHASH_INITSIZE);    
  }

  /** construct with a given capacity */
  hash_map(size_t ulSize) : 
    m_ulCount(0), m_ulCapacity(0), m_rgBuckets(), m_hashFunc() {
    initBucketsSize_(ulSize);    
  }

  /** copy constructor */
  hash_map(const hash_map& srcMap) : 
    m_ulCount(0), m_ulCapacity(0), m_rgBuckets(), m_hashFunc() {
    if (&srcMap == this)
      return;
    initFromCopy_(srcMap);
  }

  /** assignment operator */
  hash_map& operator=(const hash_map& srcMap) {    
    if (&srcMap == this)
      return *this;
    initFromCopy_(srcMap);    
    return *this;
  }  

  /** destructor */
  ~hash_map() {    
    clearPrivate_();        
  }  

  /**
   * Public typedefs, see the sgi hashmap spec for
   * more info.
   */
  typedef ALLOC_                           allocator_type;
  typedef typename ALLOC_::value_type      value_type;
  typedef typename ALLOC_::reference       reference;
  typedef typename ALLOC_::const_reference const_reference;
  typedef typename ALLOC_::pointer         pointer;
  typedef typename ALLOC_::const_pointer   const_pointer;
  typedef typename ALLOC_::size_type       size_type;
  typedef typename ALLOC_::difference_type difference_type;


  typedef KEY_                             key_type;
  typedef VAL_                             data_type;
  typedef HASHFUNC_                        hasher;
  typedef EQUAL_                           key_equal;

  /******************************************************************/

  /**
   * Base iterator template class
   */
  template < bool ISCONST_ >
  class IteratorBase  {
  public:
    typedef typename hash_map<KEY_,VAL_,HASHFUNC_,EQUAL_,ALLOC_>::ListEntry*  
                                                entry_type;
    typedef typename std::vector< entry_type >  bucklist_t;
    typedef std::forward_iterator_tag           iterator_category;
    typedef typename ALLOC_::value_type         value_type;
    typedef typename ALLOC_::difference_type    difference_type;
    typedef typename MySelect< ISCONST_,                            
      const_pointer,
      typename ALLOC_::pointer >::result        pointer;   /**< Select pointer or const_pointer to T. */
    typedef typename MySelect< ISCONST_,
      const_reference,
      typename ALLOC_::reference >::result      reference; /**< Select reference or const_reference to T. */

  private:
    bucklist_t*                                 m_pLstMyBucks; /**< typedef: list of buckets */
    typename bucklist_t::iterator               m_pBuck;   /**< primary buck pointer position */
    entry_type                                  m_pEntry;  /**< secondary pointer position */

  public:    

    /** default constructor */
    IteratorBase() :
      m_pLstMyBucks(0), m_pBuck(), m_pEntry() {
    }
    
    /** construct from list and position (begin/end) */
    IteratorBase(bucklist_t* pList, bool bEnd) :
      m_pLstMyBucks(pList), m_pBuck(m_pLstMyBucks->end()), m_pEntry() {
      if (!bEnd) {
        if (!m_pLstMyBucks->empty()) {
          m_pBuck  = m_pLstMyBucks->begin();
          m_pEntry = *m_pBuck;
        }
      }
    }

    /** construct a specific SecBuck position. the caller must assure that the list is not empty */
    IteratorBase(bucklist_t* pList, typename bucklist_t::iterator pItBuckPos,
      entry_type pItSecBuckPos) :                            
        m_pLstMyBucks(pList), m_pBuck(pItBuckPos), m_pEntry(pItSecBuckPos) {
    }

    /** return designated value. */
    reference operator*() const {
      return (m_pEntry->m_entry);
    }

    /** Return pointer to class object, delegate to operator '*' */
    pointer operator->() const {      
      return &(**this); 
    }

    /** increment operator */
    IteratorBase& operator++() {
      if (m_pEntry && m_pEntry->m_pNext)
        m_pEntry = m_pEntry->m_pNext;
      else {
        for(;;) {
          m_pBuck++;
          if (m_pBuck == m_pLstMyBucks->end()) { // end of list, TODO remove ->end invoke          
            m_pEntry = 0;                 
            break;
          }
          if (*m_pBuck) {
            m_pEntry = (*m_pBuck);
            break;
          }
        }
      }       
      return *this;
    }

    /** increment const operator */
    const IteratorBase operator++(int iNum) {
      for (int x=0; x<iNum; x++)
        this++;         // Increment
      return m_pEntry;  // Return copy of original
    }

    //
    // TODO cleanup operators. 
    //

    /** operator == (non const) */
    bool operator ==(IteratorBase<ISCONST_>& itR) const {
      assert(itR.m_pLstMyBucks->size() != 0);
      assert(m_pLstMyBucks->size() != 0);
      if (itR.m_pBuck == itR.m_pLstMyBucks->end()) {
        return ( (m_pBuck != m_pLstMyBucks->end()) ? false : true );
      }
      if (m_pBuck == m_pLstMyBucks->end()) {
        return ( (itR.m_pBuck != itR.m_pLstMyBucks->end()) ? false : true );
      }
      // normal case, no end() situation
      return ( (m_pEntry == itR.m_pEntry) &&
              (m_pBuck == itR.m_pBuck) );
    }

    /** operator == (const) */
    bool operator ==(const IteratorBase<ISCONST_>& itR) const { // const
      assert(itR.m_pLstMyBucks->size() != 0);
      assert(m_pLstMyBucks->size() != 0);
      if (itR.m_pBuck == itR.m_pLstMyBucks->end()) {
        return ( (m_pBuck != m_pLstMyBucks->end()) ? false : true );
      }
      if (m_pBuck == m_pLstMyBucks->end()) {
        return ( (itR.m_pBuck != itR.m_pLstMyBucks->end()) ? false : true );
      }
      // normal case, no end() situation
      return ( (m_pEntry == itR.m_pEntry) &&
              (m_pBuck == itR.m_pBuck) );
    }

    /** operator != (non const) */
    bool operator !=(IteratorBase<ISCONST_>& itR) const {  
      assert(itR.m_pLstMyBucks->size() != 0);
      assert(m_pLstMyBucks->size() != 0);
      if (itR.m_pBuck == itR.m_pLstMyBucks->end()) {
        return ( (m_pBuck == m_pLstMyBucks->end()) ? false : true );
      }
      if (m_pBuck == m_pLstMyBucks->end()) {
        return ( (itR.m_pBuck == itR.m_pLstMyBucks->end()) ? false : true );
      }
      // normal case, no end() situation
      return ( (m_pEntry == itR.m_pEntry) &&
              (m_pBuck == itR.m_pBuck) );
    }

    /** operator != (const) */
    bool operator !=(const IteratorBase<ISCONST_>& itR) const {  // const
      assert(itR.m_pLstMyBucks->size() != 0);
      assert(m_pLstMyBucks->size() != 0);
      if (itR.m_pBuck == itR.m_pLstMyBucks->end()) {
        return ( (m_pBuck == m_pLstMyBucks->end()) ? false : true );
      }
      if (m_pBuck == m_pLstMyBucks->end()) {
        return ( (itR.m_pBuck == itR.m_pLstMyBucks->end()) ? false : true );
      }
      // normal case, no end() situation
      return ( (m_pEntry == itR.m_pEntry) &&
              (m_pBuck == itR.m_pBuck) );
    }

    #ifdef _WIN32
      friend class IteratorBase; // gcc 3.x doesn't really like this
    #endif
    friend class hash_map;
  };

  /******************************************************************/

  typedef IteratorBase< false > iterator;       /**< the normal mutable iterator */
  typedef IteratorBase< true >  const_iterator; /**< the const operator */

  key_equal						m_eqFunc;
 
  /** invoke the hasher */
  size_t doHashFunc(const KEY_& key) const {
    return m_hashFunc(key);
  }

  /** set a @key via operator[] and return a reference to it */
  inline VAL_& operator[](const KEY_& key) {    
    size_t ulHash = doHashFunc(key);
    if (++m_ulCount*dGROWTH_FACT > m_ulCapacity)
      initBucketsSize_((unsigned long)(m_ulCount*dGROWTH_FACT*1.25));
    size_t lIter = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    if (!pEntry) {
      *pCurr = new ListEntry(key, VAL_());
      return (*pCurr)->m_entry.second;
    }
    ListEntry* pLastValEntry = 0;
    for (;pEntry;pEntry = pEntry->m_pNext) {
      if (pEntry->m_entry.first == key) {
        pEntry->m_entry.second = VAL_();
        --m_ulCount;
        return pEntry->m_entry.second;
      }               
      pLastValEntry = pEntry;
    }
    ListEntry* pNew = new ListEntry(key, VAL_(), pLastValEntry);    
    return pNew->m_entry.second;
  }  

  /** whether a value @val exist */
  inline bool hasValue(const VAL_& val) const {
    typename std::vector<ListEntry*>::const_iterator pCurr = m_rgBuckets.begin();
    size_t lRuns = m_ulCount;
    while(pCurr != m_rgBuckets.end() && lRuns > 0) {
      ListEntry* pEntry = *pCurr;
      while(pEntry) {
        if (pEntry->m_entry.second == val) 
          return true;
        pEntry = pEntry->m_pNext;
        lRuns--;
      }
      pCurr++;
    }
    return false;
  }

  /** delete a key @key */
  inline bool delKey_(const KEY_& key) {
    size_t ulHash = doHashFunc(key);
    size_t lIter  = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    while (pEntry) {
      if (pEntry->m_entry.first == key) {
        if (!pEntry->m_pPrev) {                         // first entry
          if (!pEntry->m_pNext)                         // first entry, alone
            *pCurr = 0;
          else {
            pEntry->m_pNext->m_pPrev = 0;
            *pCurr = pEntry->m_pNext;
          }
        } else {                                        // normal entry 
          pEntry->m_pPrev->m_pNext = pEntry->m_pNext;
          if (pEntry->m_pNext)                          // normal entry, not the last one  
            pEntry->m_pNext->m_pPrev = pEntry->m_pPrev;
        }
        delete pEntry;
        m_ulCount--;
        return true;
      }
      pEntry = pEntry->m_pNext;
    }
    return false;    
  }

  /** put an entry {@key/@val} */
  void put(const KEY_& key, const VAL_& val) {
    size_t ulHash = doHashFunc(key);    
    if (++m_ulCount*dGROWTH_FACT >= m_ulCapacity)
      initBucketsSize_((size_t)(m_ulCapacity*2.5));
    size_t lIter = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    if (!pEntry) {
      *pCurr = new ListEntry(key, val);
      return;   
    }
    ListEntry* pLastValEntry = 0;
    for (;pEntry;pEntry = pEntry->m_pNext) {
      if (pEntry->m_entry.first == key) {
        pEntry->m_entry.second = val;
        --m_ulCount;
        return;
      }               
      pLastValEntry = pEntry;
    }
    new ListEntry(key, val, pLastValEntry);    
  }  

  /** dump a map to std::cout */
  void dumpHash_() const {
    ListEntry* pEntry;        
    for (int x=0; x < m_ulCapacity; x++) {
      pEntry = m_rgBuckets[x];
      while (pEntry) {
        std::cout << std::endl << x <<": {" << pEntry->m_entry.first << " : " 
          << pEntry->m_entry.second << " }" << std::flush;
        pEntry = pEntry->m_pNext;
      }    
    }
  }

  /** get an entry (@key / @outval) from the map */
  bool get(const KEY_& key, VAL_& outVal) const {    
    size_t ulHash = doHashFunc(key);
    size_t lIter  = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::const_iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    while (pEntry) {
      if (pEntry->m_entry.first == key) {
        outVal = pEntry->m_entry.second;
        return true;
      }
      pEntry = pEntry->m_pNext;
    }
    return false;
  }

  /** retrieve amount of entries */
  inline size_t getCount() const {
    return m_ulCount;
  }

  /** grow our capacity */
  inline void growCapacity(size_t ulSize) {    
    assert(ulSize > 0);
    if (ulSize*dGROWTH_FACT >= m_ulCapacity)
      initBucketsSize_((size_t)(ulSize*dGROWTH_FACT*2.5));
  }

  /** retrieve the capacity */
  inline size_t getCapacity() const {
    return m_ulCapacity;
  }
  
  /** rehash our map */
  inline void reHash() {    
    if (m_ulCount <= 1) // TODO NOW stupid - the map actually is empty!
      return;    
    size_t iSize = m_rgBuckets.size();
    typename std::vector<ListEntry*> tmpBuckets;
    tmpBuckets.reserve(iSize);
	size_t x; // XXX:
    for (x=0; x<iSize; x++) {
      ListEntry* pEntry = m_rgBuckets[x];
      while(pEntry) {
        tmpBuckets.push_back(pEntry);
        pEntry = pEntry->m_pNext;
      }
    }
    assert(tmpBuckets.size() == m_ulCount-1); // TODO NOW : see above
    m_rgBuckets.clear();
    m_rgBuckets.resize(iSize,0);
    assert(m_ulCapacity == iSize);
    for (x=0; x<m_ulCount-1; x++) {
      ListEntry* pEntry = tmpBuckets[x];
      size_t ulHash     = doHashFunc(pEntry->m_entry.first);        
      size_t lIter      = ulHash % m_ulCapacity;
      typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);      
      if (!*pCurr) {
        *pCurr = pEntry;
        pEntry->m_pNext = 0;
        pEntry->m_pPrev = 0;
        continue;
      }
      ListEntry* pDestEntry    = *pCurr;
      ListEntry* pLastValEntry = 0;
      while(pDestEntry) {  
        assert(pDestEntry->m_entry.first != pEntry->m_entry.first); // TODO remove: could be expensive
        pLastValEntry = pDestEntry;
        pDestEntry = pDestEntry->m_pNext;
      }
      assert(pLastValEntry != 0);
      pLastValEntry->m_pNext = pEntry;
      pEntry->m_pPrev = pLastValEntry;
      pEntry->m_pNext = 0; 
    }
  }    

  /** put an entry but do not handle duplicate keys and do not incr. m_ulCount */
  inline void putPrivate(const KEY_& key, const VAL_& val) {    
    size_t ulHash = doHashFunc(key);        
    size_t lIter  = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    if (!pEntry) {
      *pCurr = new ListEntry(key, val);
      return;   
    }
    ListEntry* pLastValEntry = 0;
    for (;pEntry;pEntry = pEntry->m_pNext) {
      if (pEntry->m_entry.first == key) {
        assert(false); // should never happen, since our entry points are rehashing/copy con/assg. op
        return;
      }
      pLastValEntry = pEntry;
    }
    new ListEntry(key, val, pLastValEntry);
  } 

  /** clear the hashmap */
  inline void clear() {
    clearPrivate_();
  }

  /** resize the hashmap */
  void resize(size_type n) {
    growCapacity(n);
  }

  /** return the hasher */
  hasher hash_funct() const {
    return m_hashFunc;
  }

  /** return the key comparison function */
  key_equal key_eq() const {
    return m_eqFunc;
  }

  /** return amount of buckets */
  inline size_type bucket_count() const {
    return size();
  }

  /** return the capacity */
  inline size_type max_size() const {
    return m_ulCapacity;
  }

  /** return amount of entries */
  inline size_type size() const {
    return m_ulCount;
  }

  /** is the map empty ? */
  inline bool empty() const {
    return m_ulCount == 0 ? true : false;
  }

  /** return amount of entries for a key */
  inline size_type count(const KEY_& key) const {
    return hasKey_(key) ? 1 : 0;
  }

  /** return begin() */
  iterator begin() {
    if (m_ulCount == 0)
      return end();
    return IteratorBase<false>(&m_rgBuckets, false);
  }

  /** return const begin() */
  const_iterator begin() const {
    if (m_ulCount == 0)
      return end();
    return IteratorBase<true>(
      const_cast< std::vector<ListEntry* >* >(&m_rgBuckets),
      false);
  }

  /** return end() */
  iterator end() {
    return IteratorBase<false>(
      const_cast< std::vector<ListEntry* >* >(&m_rgBuckets),
      true);
  }

  /** return const end() */
  const_iterator end() const {
    return IteratorBase<true>(
      const_cast< std::vector<ListEntry* >* >(&m_rgBuckets),
      true);
  }

  /** find the entry @key and return an iterator */
  iterator find(const KEY_& key) {
    size_t ulHash = doHashFunc(key);
    size_t lIter  = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    while (pEntry) {
      if (pEntry->m_entry.first == key) 
        return iterator(&m_rgBuckets, pCurr, pEntry);      
      pEntry = pEntry->m_pNext;
    }
    return end();
  }

  /** find the entry @key and return a const iterator */
  const_iterator find(const KEY_& key) const {
    size_t ulHash = doHashFunc(key);
    size_t lIter  = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::const_iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    while (pEntry) {
      if (pEntry->m_entry.first == key) 
        return iterator(&m_rgBuckets, pCurr, pEntry);      
      pEntry = pEntry->m_pNext;
    }
    return end();
  }

  /** insert an entry @x and return iterator/has existed pair */
  inline std::pair<iterator, bool> insert(const value_type& x) {
    std::pair<iterator, bool> pairRet;
    size_t ulHash = doHashFunc(x.first);    
    if (++m_ulCount*dGROWTH_FACT >= m_ulCapacity)
      initBucketsSize_((size_t)(m_ulCapacity*2.5));
    size_t lIter = ulHash % m_ulCapacity;
    typename std::vector<ListEntry*>::iterator pCurr = (m_rgBuckets.begin() + lIter);
    ListEntry* pEntry = *pCurr;
    if (!pEntry) {
      *pCurr = new ListEntry(x.first, x.second); 
      pairRet.second = false;      
      pairRet.first  = IteratorBase<false>(&m_rgBuckets, pCurr, *pCurr); 
      return pairRet;
    }
    ListEntry* pLastValEntry = 0;
    for (;pEntry;pEntry = pEntry->m_pNext) {
      if (pEntry->m_entry.first == x.first) {
        pEntry->m_entry.second = x.second;
        --m_ulCount;        
        pairRet.second = true;
        pairRet.first  = IteratorBase<false>(&m_rgBuckets, pCurr, pEntry); 
        return pairRet;
      }          
      pLastValEntry = pEntry;
    }
    new ListEntry(x.first, x.second, pLastValEntry);    
    pairRet.second = false;
    pairRet.first  = IteratorBase<false>(&m_rgBuckets, pCurr, pLastValEntry);
    return pairRet;
  }

  /** erase a key */
  void erase(iterator pos) {
    delKey_(pos->first);
  }  

private:

  /** initialize our bucket size (note:presently we can only grow) */
  void initBucketsSize_(size_t lEstimSize) {    
    assert(lEstimSize >= m_ulCapacity);
    m_ulCapacity = getNextPrime(lEstimSize);        
    m_rgBuckets.resize(m_ulCapacity, 0); 
    assert(m_rgBuckets.capacity() >= m_ulCapacity);
    if (m_ulCount > 0)
      reHash();
  }  

  /** initialized our map from another one */
  void initFromCopy_(const hash_map& srcMap) {    
    clear();
    if (!srcMap.getCount())
      return;
    initBucketsSize_(srcMap.getCapacity());
    ListEntry* pEntry;        
    for (size_t x=0; x < srcMap.getCapacity(); x++) {
      pEntry = srcMap.m_rgBuckets[x];
      while (pEntry) {
        this->putPrivate(pEntry->m_entry.first, pEntry->m_entry.second);
        m_ulCount++;
        pEntry = pEntry->m_pNext;
      }    
    }
    assert(m_ulCount == srcMap.m_ulCount);
    assert(getCapacity() == srcMap.getCapacity());    
  }

  /** clear and dealloc our entries */
  void clearPrivate_() {
    ListEntry* pEntry;    
    size_t lToDel = m_ulCount;
    for (size_t x=0; x<m_ulCapacity && lToDel>0; x++) {
      pEntry = m_rgBuckets[x];
      if(!pEntry)
        continue;      
      while(pEntry->m_pNext)
        pEntry = pEntry->m_pNext;
      for(;;) {
        if (pEntry->m_pPrev) {
          pEntry = pEntry->m_pPrev; 
          delete pEntry->m_pNext;   
          lToDel--;
        } else
          break;
      }      
      lToDel--;
      delete pEntry;           
    }
    assert(lToDel == 0);
    m_rgBuckets.clear();
    m_ulCount = 0;
    m_ulCapacity = 0;
  }

public:

  /**
   * ListEntry 
   */
  struct ListEntry {    
    ListEntry() :
        m_entry(), m_pPrev(0), m_pNext(0) {
    }

    ListEntry(const KEY_& key, const VAL_& val) :
        m_entry(key,val), m_pPrev(0), m_pNext(0) {
    }
    
    ListEntry(const KEY_& key, const VAL_& val, ListEntry* pos) :
        m_entry(key,val), m_pPrev(pos), m_pNext(pos->m_pNext) {
      pos->m_pNext = this;
    }    
    
    inline void* operator new (size_t size) {
      assert(size == sizeof(ListEntry));
      return(::malloc(size));
    }
    
    inline void operator delete(void * mem) {
      ::free(mem);
    };

    std::pair<KEY_,VAL_>  m_entry;    
    ListEntry*            m_pPrev;
    ListEntry*            m_pNext;    
  };  

private:

  size_t                  m_ulCount;
  size_t                  m_ulCapacity;
  std::vector<ListEntry*> m_rgBuckets; /**< dim is always m_ulCapacity */
  HASHFUNC_               m_hashFunc;
};


/**
 * compare two hashmaps
 */
template <class KEY_,
  class VAL_,
  class HASHFUNC_,
  class EQUAL_,
  class ALLOC_ >
bool operator ==(hash_map<KEY_, VAL_, HASHFUNC_, EQUAL_, ALLOC_>& map1,       // cannot be const because of iterator comparison bug!
    hash_map<KEY_,VAL_,HASHFUNC_,EQUAL_,ALLOC_>& map2) {
  if (map1.size() != map2.size())
    return false;
  typename hash_map<KEY_, VAL_, HASHFUNC_, EQUAL_, ALLOC_>::iterator pBegin, pEnd, pCurr;
  for (pBegin = map1.begin(), pEnd = map1.end();
       pBegin != pEnd;
       ++pBegin) {
    pCurr = map2.find(pBegin->first);
    if (pCurr == map2.end())
      return false;
    if (pCurr->second != pBegin->second)
      return false;
  }
  return true;
}

/**
 * swap two hashmaps
 */
template <class KEY_,
  class VAL_,
  class HASHFUNC_,
  class EQUAL_,
  class ALLOC_ >
void swap(hash_map<KEY_, VAL_, HASHFUNC_, EQUAL_, ALLOC_>& a,
    hash_map<KEY_, VAL_, HASHFUNC_, EQUAL_, ALLOC_>& b) {
  hash_map<KEY_, VAL_, HASHFUNC_, EQUAL_, ALLOC_> aCopy = a;
  a = b;
  b = aCopy;
}

/*
 * some other string hash functions you may want to experiment with
 */
#if 0
inline size_t g_strHashFunc(const std::string& sKey) { // classic sedgewick
  int h=0, a =127;
  const char* v = sKey.c_str();
  for (; *v != 0; v++)
    h = (a*h + *v);
  return h;
}

inline size_t g_strHashFunc(const std::string& sKey) { // sgi stl
  const char* __s = sKey.c_str();
  size_t __h = 0;
  for ( ; *__s; ++__s)
    __h = 5*__h + *__s;
  return __h;
}

inline size_t g_strHashFunc(const std::string& sKey) { // stroustrup
  const char* psBytes = sKey.c_str();
  size_t retVal       = 0;
  size_t lLen         = sKey.length();
  while (lLen--)
    retVal = (retVal<<1) ^ *psBytes++;
  return (size_t) retVal;
}
#endif

}

#endif
