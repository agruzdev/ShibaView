/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
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
