/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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

#include "Image.h"

#include <atomic>
#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

#include "ImageSource.h"

namespace
{
    uint64_t generateId()
    {
        static std::atomic<uint64_t> nextId{ 0 };
        return nextId.fetch_add(1, std::memory_order::memory_order_relaxed);
    }
}

Image::Image(QString name, QString filename) noexcept
    : mId(generateId())
{
#ifdef _MSC_VER
    std::wcout << filename.toStdWString() << std::endl;
#endif

    // Load bitmap. Keep empty on fail.
    try {
        auto source = ImageSource::Load(filename);
        if (!source || 0 == source->pagesCount()) {
            throw std::runtime_error("Failed to open image: " + filename.toStdString());
        }

        mImagePlayer= std::make_unique<Player>(std::move(source));
    }
    catch(std::exception & e) {
        qWarning() << QString(e.what());
        mImagePlayer = nullptr;
    }

    uint32_t width  = 0;
    uint32_t height = 0;
    if (mImagePlayer) {
        width  = mImagePlayer->getWidth();
        height = mImagePlayer->getHeight();
    }

    QFileInfo file(filename);

    mInfo.path     = std::move(filename);
    mInfo.name     = std::move(name);
    mInfo.bytes    = file.size();
    mInfo.modified = file.lastModified();
    mInfo.dims     = { width, height };
    mInfo.animated = (file.suffix().compare("gif", Qt::CaseInsensitive) == 0);
}

Image::~Image() = default;

void Image::addListener(ImageListener* listener)
{
    mListeners.push_back(listener);
}

void Image::removeListener(ImageListener* listener)
{
    const auto it = std::find(mListeners.cbegin(), mListeners.cend(), listener);
    if (it != mListeners.cend()) {
        mListeners.erase(it);
    }
}

uint32_t Image::channels() const
{
    if (mImagePlayer) {
        return FreeImageExt_GetChannelsNumber(getFrame().bmp);
    }
    return 0;
}

void Image::next()
{
    mImagePlayer->next();
    for (auto listener : mListeners) {
        listener->onInvalidated(this);
    }
}

void Image::prev()
{
    mImagePlayer->prev();
    for (auto listener : mListeners) {
        listener->onInvalidated(this);
    }
}
