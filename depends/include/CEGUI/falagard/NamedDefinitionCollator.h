/***********************************************************************
    created:    Mon Jun 11 2012
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUINamedDefinitionCollator_h_
#define _CEGUINamedDefinitionCollator_h_

#include "CEGUI/Base.h"
#include <vector>
#include <algorithm>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

namespace CEGUI
{
/*!
\brief
    Helper container used to implement inherited collections of component
    definitions specified in a WidgetLook.
*/
template<typename K, typename V>
class NamedDefinitionCollator
{
public:
    typedef V value_type;

    //! Return total number of values in the collection.
    size_t size() const
    { return d_values.size(); }

    //! return reference to value at given index.
    V& at(size_t idx)
    { return d_values.at(idx).second; }

    //! return const reference to value at given index.
    const V& at(size_t idx) const
    { return d_values.at(idx).second; }

    /*!
    \brief
        Set value for a given key.  If a value is already associated with the
        given key, it is replaced with the new value and the value is moved to
        the end of the collection.
    */
    void set(const K& key, const V& val)
    {
        typename ValueArray::iterator i =
            std::find_if(d_values.begin(), d_values.end(), pred(key));

        if (i != d_values.end())
            d_values.erase(i);

        d_values.push_back(std::make_pair(key, val));
    }

protected:
    typedef std::pair<K, V> Entry;
    typedef std::vector<Entry CEGUI_VECTOR_ALLOC(Entry)> ValueArray;

    struct pred
    {
        const K& d_k;
        pred(const K& k) : d_k(k) {}
        bool operator()(const Entry& e)
        { return e.first == d_k; }
    };

    ValueArray d_values;

public:
    /** This is not intended to provide full std::iterator support, it is here
     * so that we can provide access via CEGUI::ConstVectorIterator.
     */
    class const_iterator
    {
    public:
        const_iterator(typename ValueArray::const_iterator i) :
            d_iter(i) {}

        const_iterator(const const_iterator& iter) :
            d_iter(iter.d_iter) {}

        const_iterator()
        {}

        const V& operator*() const
        { return d_iter->second; }

        const V* operator->() const
        { return &**this; }

        bool operator==(const const_iterator& iter) const
        { return d_iter == iter.d_iter; }

        bool operator!=(const const_iterator& iter) const
        { return !operator==(iter); }

        const_iterator& operator++()
        {
            ++d_iter;
            return *this;
        }

        const_iterator& operator++(int)
        {
            const_iterator& tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator& operator--()
        {
            --d_iter;
            return *this;
        }

        const_iterator& operator--(int)
        {
            const_iterator& tmp = *this;
            --*this;
            return tmp;
        }

        const_iterator& operator=(const const_iterator& iter)
        {
            d_iter = iter.d_iter;
            return *this;
        }

    protected:
        typename ValueArray::const_iterator d_iter;
    };

    const_iterator begin() const
    { return const_iterator(d_values.begin()); }

    const_iterator end() const
    { return const_iterator(d_values.end()); }

    const_iterator find(const K& key) const
    {
        return const_iterator(std::find_if(d_values.begin(),
                                           d_values.end(),
                                           pred(key)));
    }

};

}

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif

