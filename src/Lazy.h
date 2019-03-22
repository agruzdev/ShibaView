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

#ifndef LAZY_H
#define LAZY_H

#include <memory>
#include <functional>

template <typename Ty_>
class Lazy
{
    using ThisType = Lazy<Ty_>;
public:
    using ValueType   = Ty_;
    using BuilderType = std::function<ValueType()>;

    Lazy() = default;

    explicit
    Lazy(BuilderType builder)
        : mBuilder(std::move(builder))
    { }

    ~Lazy() = default;

    Lazy(const Lazy&) = delete;
    Lazy(Lazy&&) = default;

    Lazy& operator=(const Lazy&) = delete;
    Lazy& operator=(Lazy&&) = default;

    void setBuilder(BuilderType builder)
    {
        mBuilder = std::move(builder);
    }

    ValueType & get()
    {
        if (!mValue) {
            if(!mBuilder) {
                throw std::runtime_error("Lazy[get]: No builder.");
            }
            mValue.reset(new ValueType(mBuilder()));
        }
        return *mValue;
    }

    const ValueType & get() const
    {
        return const_cast<ThisType*>(this)->get();
    }

private:
    std::unique_ptr<ValueType> mValue{ nullptr };
    BuilderType mBuilder{ nullptr };
};

#endif // LAZY_H
