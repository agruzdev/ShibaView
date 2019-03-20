/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ENUMARRAY_H
#define ENUMARRAY_H

#include <array>
#include <cassert>

/**
 * Array using enum as index.
 * Enum_ must have a value length_.
 */
template <typename Ty_, typename Enum_>
class EnumArray
{
public:
    using ValueType = Ty_;
    using IndexType = Enum_;
    using SizeType  = size_t;
    static constexpr size_t Length = static_cast<SizeType>(Enum_::length_);

    std::array<ValueType, Length> data;

    constexpr
    SizeType size() const
    {
        return Length;
    }

    ValueType & operator[](IndexType idx)
    {
        assert(static_cast<SizeType>(idx) < Length);
        return data[static_cast<SizeType>(idx)];
    }

    const ValueType & operator[](IndexType idx) const
    {
        assert(static_cast<SizeType>(idx) < Length);
        return data[static_cast<SizeType>(idx)];
    }
};

#endif // ENUMARRAY_H
