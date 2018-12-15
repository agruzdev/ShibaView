#ifndef ZOOMCONTROLLER_H
#define ZOOMCONTROLLER_H

#include <deque>

class ZoomController
{
public:
    ZoomController(int pos100, int posFit, int minValue, int maxValue);
    ~ZoomController();

    int get() const
    {
        return *mPosCurrent;
    }

    int zoomPlus()
    {
        if(mPosCurrent != std::prev(mScales.cend())) {
            ++mPosCurrent;
        }
        return *mPosCurrent;
    }

    int zoomMinus()
    {
        if(mPosCurrent != mScales.cbegin()) {
            --mPosCurrent;
        }
        return *mPosCurrent;
    }

    int moveToPos100()
    {
        mPosCurrent = mPos100;
        return *mPosCurrent;
    }

    int moveToPosFit()
    {
        mPosCurrent = mPosFit;
        return *mPosCurrent;
    }

private:
    std::deque<int> mScales;
    std::deque<int>::const_iterator mPos100;
    std::deque<int>::const_iterator mPosFit;
    std::deque<int>::const_iterator mPosCurrent;
};

#endif // ZOOMCONTROLLER_H