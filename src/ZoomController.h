#ifndef ZOOMCONTROLLER_H
#define ZOOMCONTROLLER_H

#include <deque>

class ZoomController
{
public:
    ZoomController(int initValue, int minLimit, int maxLimit);
    ~ZoomController();

    int get() const
    {
        return *mPos;
    }

    int zoomPlus()
    {
        if(mPos != std::prev(mScales.cend())) {
            ++mPos;
        }
        return *mPos;
    }

    int zoomMinus()
    {
        if(mPos != mScales.cbegin()) {
            --mPos;
        }
        return *mPos;
    }

private:
    std::deque<int> mScales;
    std::deque<int>::const_iterator mPos;
};

#endif // ZOOMCONTROLLER_H