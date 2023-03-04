/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
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

#ifndef UNIQUETICK_H
#define UNIQUETICK_H

#include <QObject>
#include <QTimer>

class UniqueTick
    : public QObject
{
    Q_OBJECT
public:
    using IdType_ = uint64_t;

    template <typename Duration_, typename Slot_>
    UniqueTick(IdType_ id, Duration_ delay, const QObject* dst, Slot_ && slot, QObject *parent = nullptr)
        : QObject(parent), mId(id)
    {
        using DstType = std::add_pointer_t<std::add_const_t<typename QtPrivate::FunctionPointer<Slot_>::Object>>;
        QTimer::singleShot(delay, this, &UniqueTick::onTimerTick);
        connect(this, &UniqueTick::tick, reinterpret_cast<DstType>(dst), std::forward<Slot_>(slot));
    }
    
    ~UniqueTick() = default;

    UniqueTick(const UniqueTick&) = delete;
    UniqueTick(UniqueTick&&) = delete;

    UniqueTick & operator= (const UniqueTick&) = delete;
    UniqueTick & operator= (UniqueTick&&) = delete;

signals:
    void tick(const IdType_ & id);

public slots:
    void onTimerTick()
    {
        emit tick(mId);
        deleteLater();
    }

private:
    IdType_ mId;

};

#endif // UNIQUETICK_H
