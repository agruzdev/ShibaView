#include "ZoomController.h"

#include <algorithm>

ZoomController::ZoomController(int initValue, int minLimit, int maxLimit)
{
    const float step = 0.075f;
    mScales.push_back(initValue);
    float v = initValue;
    for(;;) {
        v = v * (1.0f + step);
        const int s = std::min(static_cast<int>(std::ceil(v)), maxLimit);
        if(s != mScales.back()) {
            mScales.push_back(s);
        }
        if (s == maxLimit) {
            break;
        }
    }
    int offset = 0;
    v = initValue;
    for(;;) {
        v = v * (1.0f - step);
        const int s = std::max(static_cast<int>(std::floor(v)), minLimit);
        if(s != mScales.front()) {
            mScales.push_front(s);
            ++offset;
        }
        if (s == minLimit) {
            break;
        }
    }
    mPos = mScales.cbegin() + offset;
}

ZoomController::~ZoomController()
{
    
}
