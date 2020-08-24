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

#ifndef RAWFORMAT_H
#define RAWFORMAT_H

#include <cassert>
#include <cstdint>

enum class RawPixelColor
{
    eY = 0,
    eRGB,
    eRGBA,

    length_
};

inline
QString toQString(RawPixelColor color)
{
    switch(color) {
    case RawPixelColor::eY:
        return "Y";
    case RawPixelColor::eRGB:
        return "RGB";
    case RawPixelColor::eRGBA:
        return "RGBA";
    default:
        assert(false);
        return "Invalid color";
    }
}

inline
uint32_t getChannelsNumber(RawPixelColor color)
{
    switch (color) {
    case RawPixelColor::eY:
        return 1;
    case RawPixelColor::eRGB:
        return 3;
    case RawPixelColor::eRGBA:
        return 4;
    default:
        assert(false);
        return 0;
    }
}



enum class RawDataType
{
    eFloat64 = 0,
    eFloat32,
    eUInt32,
    eUInt16,
    eUInt8,

    length_
};

template<RawDataType T> struct RawDataTypeToCType { };
template<> struct RawDataTypeToCType<RawDataType::eFloat64> { using type = double;   };
template<> struct RawDataTypeToCType<RawDataType::eFloat32> { using type = float;    };
template<> struct RawDataTypeToCType<RawDataType::eUInt32>  { using type = uint32_t; };
template<> struct RawDataTypeToCType<RawDataType::eUInt16>  { using type = uint16_t; };
template<> struct RawDataTypeToCType<RawDataType::eUInt8>   { using type = uint8_t;  };

inline
QString toQString(RawDataType pt)
{
    switch(pt) {
    case RawDataType::eFloat64:
        return "Float64";
    case RawDataType::eFloat32:
        return "Float32";
    case RawDataType::eUInt32:
        return "UInt32";
    case RawDataType::eUInt16:
        return "UInt16";
    case RawDataType::eUInt8:
        return "UInt8";
    default:
        assert(false);
        return "Invalid type";
    }
}

inline
size_t getSizeOfDataType(RawDataType pt)
{
    switch(pt) {
    case RawDataType::eFloat64:
        return 8;
    case RawDataType::eFloat32:
    case RawDataType::eUInt32:
        return 4;
    case RawDataType::eUInt16:
        return 2;
    case RawDataType::eUInt8:
        return 1;
    default:
        assert(false);
        return 1;
    }
}

struct RawFormat
{
    RawPixelColor colorSpace = RawPixelColor::eY;
    RawDataType   dataType   = RawDataType::eFloat32;
    uint32_t height = 0;
    uint32_t width  = 0;
    //size_t   lineStride = 0;
};



#endif // RAWFORMAT_H
