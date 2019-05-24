/**
* This file is part of Special K.
*
* Special K is free software : you can redistribute it
* and/or modify it under the terms of the GNU General Public License
* as published by The Free Software Foundation, either version 3 of
* the License, or (at your option) any later version.
*
* Special K is distributed in the hope that it will be useful,
*
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Special K.
*
*   If not, see <http://www.gnu.org/licenses/>.
*
**/

struct IUnknown;
#include <Unknwnbase.h>

#include <SpecialK/SpecialK.h>

SK_INCLUDE_START_CPP (BIDIRECTIONAL_MAP)

#include <SpecialK/utility.h>
#include <SpecialK/log.h>

#include <algorithm>
#include <utility>
#include <unordered_map>
#include <initializer_list>

template <typename _K, typename _V>
class SKTL_BidirectionalHashMap
{
public:
  using       iterator   = typename std::unordered_map <_K, _V>::iterator;
  using const_iterator   = typename std::unordered_map <_K, _V>::const_iterator;

  using       k_iterator = typename std::unordered_map <_V, _K>::iterator;
  using k_const_iterator = typename std::unordered_map <_V, _K>::const_iterator;

  _K dummy_key   = { };
  _V dummy_value = { };

public:
  SKTL_BidirectionalHashMap ( std::initializer_list
                              <
                                std::pair <_K, _V>
                              >     _Ilist
                            )
  {
    for ( auto& it : _Ilist )
    {
      fwd_map [it.first]  = it.second;
      rev_map [it.second] = it.first;
    }
  }

  bool
  addKeyValue ( _K& key,
                _V& value )
  {
    size_t
        fwd_count =
    fwd_map.count (  key),
        rev_count =
    rev_map.count (value);

    SK_ReleaseAssert (fwd_count == 0);
    SK_ReleaseAssert (rev_count == 0);

    fwd_map [key  ] = value;
    rev_map [value] = key;

    if ( fwd_count == 0 &&
         rev_count == 0 )
    {
      return true;
    }

    return false;
  }

  inline
    bool begin_ (typename std::unordered_map <_K, _V>::iterator* pKVIter = nullptr)
    { return begin_ ( pKVIter, nullptr ); };

  inline
    bool begin_ (typename std::unordered_map <_V, _K>::iterator* pVKIter = nullptr)
    { return begin_ ( nullptr, pVKIter ); };

  inline
    bool begin_ (typename std::unordered_map <_K, _V>::iterator* pKVIter = nullptr,
                 typename std::unordered_map <_V, _K>::iterator* pVKIter = nullptr)
  {
    if (pKVIter != nullptr)
    {
      *pKVIter = fwd_map.cbegin ();

      return (*pKVIter != fwd_map.cend ());
    }

    else if (pVKIter != nullptr)
    {
      *pVKIter = rev_map.cbegin ();

      return (*pVKIter != rev_map.cend ());
    }

    else
      return false;
  }

  inline
    bool cbegin_ (typename std::unordered_map <_K, _V>::const_iterator* pKVIter = nullptr) const
    { return cbegin_ ( pKVIter, nullptr ); };

  inline
    bool cbegin_ (typename std::unordered_map <_V, _K>::const_iterator* pVKIter = nullptr) const
    { return cbegin_ ( nullptr, pVKIter ); };

  inline
    const bool cbegin_ ( typename std::unordered_map <_K, _V>::const_iterator* pKVIter = nullptr,
                         typename std::unordered_map <_V, _K>::const_iterator* pVKIter = nullptr ) const
  {
    if (pKVIter != nullptr)
    {
      *pKVIter = fwd_map.cbegin ();

      return (*pKVIter != fwd_map.cend ());
    }

    else if (pVKIter != nullptr)
    {
      *pVKIter = rev_map.cbegin ();

      return (*pVKIter != rev_map.cend ());
    }

    else
      return false;
  }

  inline typename std::unordered_map <_V, _K>::iterator
  begin_v (void)
  {
    std::unordered_map <_V, _K>::iterator it;

    if (! begin_ (&it))
    {
      return
        rev_map.end ();
    }

    return it;
  }

  inline const typename std::unordered_map <_V, _K>::const_iterator
  cbegin_v (void) const
  {
    std::unordered_map <_V, _K>::const_iterator it;

    if (! cbegin_ (&it))
    {
      return
        rev_map.cend ();
    }

    return it;
  }

  inline typename std::unordered_map <_K, _V>::iterator
  begin_k (void)
  {
    std::unordered_map <_K, _V>::iterator it;

    if (! begin_ (&it))
    {
      return
        fwd_map.end ();
    }

    return it;
  }

  inline const typename std::unordered_map <_K, _V>::const_iterator
  cbegin_k (void) const
  {
    std::unordered_map <_K, _V>::const_iterator it;

    if (! cbegin_ (&it))
    {
      return
        rev_map.cend ();
    }

    return it;
  }


  inline
    bool end_ (typename std::unordered_map <_K, _V>::iterator* pKVIter = nullptr)
    { return end_ ( pKVIter, nullptr ); };

  inline
    bool end_ (typename std::unordered_map <_V, _K>::iterator* pVKIter = nullptr)
    { return end_ ( nullptr, pVKIter ); };

  inline
    bool end_ ( typename std::unordered_map <_K, _V>::iterator* pKVIter = nullptr,
                typename std::unordered_map <_V, _K>::iterator* pVKIter = nullptr )
  {
    if (pKVIter != nullptr)
    {
      *pKVIter = fwd_map.end ();
    }

    else if (pVKIter != nullptr)
    {
      *pVKIter = rev_map.end ();
    }

    return true;
  }

  inline
    bool cend_ (typename std::unordered_map <_K, _V>::const_iterator* pKVIter = nullptr) const
    { return cend_ ( pKVIter, nullptr ); };

  inline
    bool cend_ (typename std::unordered_map <_V, _K>::const_iterator* pVKIter = nullptr) const
    { return cend_ ( nullptr, pVKIter ); };

  inline
    const bool cend_ ( typename std::unordered_map <_K, _V>::const_iterator* pKVIter = nullptr,
                       typename std::unordered_map <_V, _K>::const_iterator* pVKIter = nullptr ) const
  {
    if (pKVIter != nullptr)
    {
      *pKVIter = fwd_map.cend ();
    }

    else if (pVKIter != nullptr)
    {
      *pVKIter = rev_map.cend ();
    }

    return true;
  }

  inline typename std::unordered_map <_V, _K>::iterator
  end_v (void)
  {
    std::unordered_map <_V, _K>::iterator it;

    if (! end_ (&it))
    {
      return
        rev_map.end ();
    }

    return it;
  }

  inline const typename std::unordered_map <_V, _K>::const_iterator
  cend_v (void) const
  {
    std::unordered_map <_V, _K>::const_iterator it;

    if (! cend_ (&it))
    {
      return
        rev_map.cend ();
    }

    return it;
  }


  inline typename std::unordered_map <_K, _V>::iterator
  end_k (void)
  {
    std::unordered_map <_K, _V>::iterator it;

    if (! end_ (&it))
    {
      return
        fwd_map.end ();
    }

    return it;
  }

  inline const typename std::unordered_map <_K, _V>::const_iterator
  cend_k (void) const
  {
    std::unordered_map <_K, _V>::const_iterator it;

    if (! cend_ (&it))
    {
      return
        fwd_map.cend ();
    }

    return it;
  }




  inline
    typename std::unordered_map <_K, _V>::const_iterator
      cbegin (void) const
      {
        return fwd_map.cbegin ();
      }

  inline
    typename std::unordered_map <_K, _V>::iterator
      begin (void)
      {
        return fwd_map.begin ();
      }

  inline
    typename std::unordered_map <_K, _V>::const_iterator
      cend (void) const
      {
        return fwd_map.cend ();
      }

  inline
    typename std::unordered_map <_K, _V>::iterator
      end (void)
      {
        return fwd_map.end ();
      }



  inline const_iterator find (const _K& key)   const { return fwd_map.find (  key); }
  inline const_iterator find (const _V& value) const { return rev_map.find (value); }

  inline iterator       find (const _K& key)         { return fwd_map.find (  key); }
  inline iterator       find (const _V& value)       { return rev_map.find (value); }

  inline       _V& operator [] (_K& key)         { return        getValue (key); }
  inline const _V& operator [] (_K& key)   const { return        getValue (key); }
  inline       _K& operator [] (_V& value)       { return        getKey (value); }
  inline const _K& operator [] (_V& value) const { return        getKey (value); }

  inline size_t    count       (_K&   key)       { return fwd_map.count (  key); }
  inline size_t    count       (_V& value)       { return rev_map.count (value); }

  inline bool      contains    (_K&   key)       { return   0 !=  count (  key); }
  inline bool      contains    (_V& value)       { return   0 !=  count (value); }

  inline _K&       get         (_V& value)       { return        getKey (value); }
  inline _V&       get         (_K&   key)       { return        getValue (key); }

  inline bool      erase       (_K&   key)       { return eraseKey   (     key); }
  inline bool      erase       (_V& value)       { return eraseValue (   value); }

  inline void      clear       (void)            { fwd_map.clear ();
                                                   rev_map.clear (); }


protected:
  inline
  const
    std::unordered_map <_K, _V>&
      getFwd (void) const
      {
        return fwd_map;
      }

  inline
  const
    std::unordered_map <_V, _K>&
      getRev (void) const
      {
        return rev_map;
      }

  inline
  bool
    eraseKey (_K& key)
    {
      size_t
        fwd_count =
    fwd_map.count (key);

      SK_ReleaseAssert (fwd_count == 1);

      rev_map.erase (fwd_map [key]);
      fwd_map.erase (         key);

      if (fwd_count != 0)
      {
        return true;
      }

      return false;
    }

  inline
  bool
    eraseValue (_V& value)
    {
      size_t
        rev_count =
    rev_map.count (value);

      SK_ReleaseAssert (rev_count == 1);

      fwd_map.erase (rev_map [value]);
      rev_map.erase (         value);

      if (rev_count != 0)
      {
        return true;
      }

      return false;
    }



  inline
  _K&
  getKey (_V& value)
    {
      auto it  = rev_map.find (value);
      if ( it != rev_map.end  (     ))
      {
        return (*it).second;
      }

      return
        dummy_key;
    }

  inline
  const _K&
    getKey (const _V& value) const
    {
      auto it  = rev_map.find (value);
      if ( it != rev_map.cend (     ))
      {
        return (*it).second;
      }

      return
        dummy_key;
    }


  inline
  _V&
    getValue (_K& key)
    {
      auto it  = fwd_map.find (key);
      if ( it != fwd_map.end  (   ))
      {
        return (*it).second;
      }

      return
        dummy_value;
    }

  inline
  const _V&
    getValue (const _K& key) const
    {
      auto it  = fwd_map.find (key);
      if ( it != fwd_map.cend (   ))
      {
        return (*it).second;
      }

      return
        dummy_value;
    }

private:
  std::unordered_map <_K, _V> fwd_map = { };
  std::unordered_map <_V, _K> rev_map = { };
};

SK_INCLUDE_END_CPP   (BIDIRECTIONAL_MAP)
