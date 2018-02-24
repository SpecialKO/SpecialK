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

#ifndef FROZEN_LETITGO_STRING_H
#define FROZEN_LETITGO_STRING_H

#include <functional>

#include "frozen/bits/elsa.h"

namespace frozen {

class wstring {
  wchar_t const *data_;
  std::size_t size_;

public:
  template <std::size_t N>
  constexpr wstring(wchar_t const (&data)[N])
      : data_(data), size_(N - 1) {}
  constexpr wstring(wchar_t const *data, std::size_t size)
      : data_(data), size_(size) {}

  constexpr wstring(const wstring &) noexcept = default;
  constexpr wstring &operator=(const wstring &) noexcept = default;

  constexpr std::size_t size() const { return size_; }

  constexpr wchar_t operator[](std::size_t i) const { return data_[i]; }

  constexpr bool operator==(wstring other) const {
    if (size_ != other.size_)
      return false;
    for (std::size_t i = 0; i < size_; ++i)
      if (data_[i] != other.data_[i])
        return false;
    return true;
  }

  constexpr bool operator<(const wstring &other) const {
    unsigned i = 0;
    while (i < size() && i < other.size()) {
      if ((*this)[i] < other[i]) {
        return true;
      }
      if ((*this)[i] > other[i]) {
        return false;
      }
      ++i;
    }
    return size() < other.size();
  }

  constexpr const wchar_t *data() const { return data_; }
};

template <> struct elsa<wstring> {
  constexpr std::size_t operator()(wstring value) const {
    std::size_t d = 5381;
    for (std::size_t i = 0; i < value.size(); ++i)
      d = d * 33 + value[i];
    return d;
  }
  constexpr std::size_t operator()(wstring value, std::size_t seed) const {
    std::size_t d = seed;
    for (std::size_t i = 0; i < value.size(); ++i)
      d = (d * 0x01000193) ^ value[i];
    return d;
  }
};

namespace wstring_literals {

constexpr wstring operator"" _s(const wchar_t *data, std::size_t size) {
  return {data, size};
}

} // namespace wstring_literals

} // namespace frozen

namespace std {
template <> struct hash<frozen::wstring> {
  size_t operator()(frozen::wstring s) const {
    return frozen::elsa<frozen::wstring>{}(s);
  }
};
} // namespace std

#endif
