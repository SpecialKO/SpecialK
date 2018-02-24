/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef FROZEN_LETITGO_MAP_H
#define FROZEN_LETITGO_MAP_H

#include <stdexcept>
#include <utility>

#include <frozen/bits/algorithms.h>
#include <frozen/bits/basic_types.h>

namespace frozen {

namespace impl {

template <class Comparator> class CompareKey {

  Comparator const comparator_;

public:
  constexpr CompareKey(Comparator const &comparator)
      : comparator_(comparator) {}

  template <class Key, class Value>
  constexpr int operator()(std::pair<Key, Value> const &self,
                           std::pair<Key, Value> const &other) const {
    return comparator_(std::get<0>(self), std::get<0>(other));
  }

  template <class Key, class Value>
  constexpr int operator()(Key const &self_key,
                           std::pair<Key, Value> const &other) const {
    return comparator_(self_key, std::get<0>(other));
  }

  template <class Key, class Value>
  constexpr int operator()(std::pair<Key, Value> const &self,
                           Key const &other_key) const {
    return comparator_(std::get<0>(self), other_key);
  }

  template <class Key>
  constexpr int operator()(Key const &self_key, Key const &other_key) const {
    return comparator_(self_key, other_key);
  }
};
} // namespace impl

template <class Key, class Value, std::size_t N, class Compare = std::less<Key>>
class map {
  using container_type = bits::carray<std::pair<Key, Value>, N>;
  impl::CompareKey<Compare> compare_;
  container_type items_;

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  using key_compare = decltype(compare_);
  using const_reference = typename container_type::const_reference;
  using reference = const_reference;
  using const_pointer = typename container_type::const_pointer;
  using pointer = const_pointer;
  using const_iterator = typename container_type::const_iterator;
  using iterator = const_iterator;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;
  using reverse_iterator = const_reverse_iterator;

public:
  /* constructors */
  constexpr map(container_type items, Compare const &compare)
      : compare_{compare}
      , items_{bits::quicksort(items, compare_)} {}
  explicit constexpr map(container_type items)
      : map{items, Compare{}} {}

  constexpr map(std::initializer_list<value_type> items, Compare const &compare)
      : map{container_type {items}, compare} {}
  constexpr map(std::initializer_list<value_type> items)
      : map{items, Compare{}} {}

  /* element access */
  constexpr const_reference at(Key const &key) const {
    auto const where = lower_bound(key);
    if (where != end())
      return *where;
    else
      throw std::out_of_range("invalid key");
  }

  /* iterators */
  const_iterator begin() const { return items_.begin(); }
  const_iterator cbegin() const { return items_.cbegin(); }
  const_iterator end() const { return items_.end(); }
  const_iterator cend() const { return items_.cend(); }

  const_reverse_iterator rbegin() const { return items_.rbegin(); }
  const_reverse_iterator crbegin() const { return items_.crbegin(); }
  const_reverse_iterator rend() const { return items_.rend(); }
  const_reverse_iterator crend() const { return items_.crend(); }

  /* capacity */
  constexpr bool empty() const { return !N; }
  constexpr size_type size() const { return N; }
  constexpr size_type max_size() const { return N; }

  /* lookup */

  constexpr std::size_t count(Key const &key) const {
    return bits::binary_search<N>(items_.begin(), key, compare_);
  }

  const_iterator find(Key const &key) const {
    const_iterator where = lower_bound(key);
    if ((where != end()) && !compare_(key, *where))
      return where;
    else
      return end();
  }

  std::pair<const_iterator, const_iterator> equal_range(Key const &key) const {
    auto const lower = lower_bound(key);
    if (lower == end())
      return {lower, lower};
    else
      return {lower, lower + 1};
  }

  const_iterator lower_bound(Key const &key) const {
    auto const where = bits::lower_bound<N>(items_.begin(), key, compare_);
    if ((where != end()) && !compare_(key, *where))
      return where;
    else
      return end();
  }

  const_iterator upper_bound(Key const &key) const {
    auto const where = bits::lower_bound<N>(items_.begin(), key, compare_);
    if ((where != end()) && !compare_(key, *where))
      return where + 1;
    else
      return end();
  }

  /* observers */
  constexpr key_compare key_comp() const { return compare_; }
  constexpr key_compare value_comp() const { return compare_; }
};

template <class Key, class Value, class Compare>
class map<Key, Value, 0, Compare> {
  using container_type =
      bits::carray<std::pair<Key, Value>, 1>; // just for the type definitions
  impl::CompareKey<Compare> compare_;

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  using key_compare = decltype(compare_);
  using const_reference = typename container_type::const_reference;
  using reference = const_reference;
  using const_pointer = typename container_type::const_pointer;
  using pointer = const_pointer;
  using const_iterator = pointer;
  using iterator = pointer;
  using const_reverse_iterator = pointer;
  using reverse_iterator = pointer;

public:
  /* constructors */
  constexpr map(const map &other) = default;
  constexpr map(std::initializer_list<value_type>, Compare const &compare)
      : compare_{compare} {}
  constexpr map(std::initializer_list<value_type> items)
      : map{items, Compare{}} {}

  /* element access */
  constexpr const_reference at(Key const &) const {
    throw std::out_of_range("invalid key");
  }

  /* iterators */
  constexpr const_iterator begin() const { return nullptr; }
  constexpr const_iterator cbegin() const { return nullptr; }
  constexpr const_iterator end() const { return nullptr; }
  constexpr const_iterator cend() const { return nullptr; }

  constexpr const_reverse_iterator rbegin() const { return nullptr; }
  constexpr const_reverse_iterator crbegin() const { return nullptr; }
  constexpr const_reverse_iterator rend() const { return nullptr; }
  constexpr const_reverse_iterator crend() const { return nullptr; }

  /* capacity */
  constexpr bool empty() const { return true; }
  constexpr size_type size() const { return 0; }
  constexpr size_type max_size() const { return 0; }

  /* lookup */

  constexpr std::size_t count(Key const &) const { return 0; }

  constexpr const_iterator find(Key const &) const { return end(); }

  constexpr std::pair<const_iterator, const_iterator>
  equal_range(Key const &) const {
    return {end(), end()};
  }

  constexpr const_iterator lower_bound(Key const &) const { return end(); }

  constexpr const_iterator upper_bound(Key const &) const { return end(); }

  /* observers */
  constexpr key_compare key_comp() const { return compare_; }
  constexpr key_compare value_comp() const { return compare_; }
};

template <typename T, typename U, std::size_t N>
constexpr auto make_map(std::pair<T, U> const (&items)[N]) {
  return map<T, U, N>{items};
}

} // namespace frozen

#endif
