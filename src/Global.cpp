/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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

#include "Global.h"

#include <QDir>
#include <QFile>

#include "FreeImageExt.h"

namespace
{
    QStringList cvtExtensionsToFilters(const QStringList& exts)
    {
        QStringList filters;
        filters.reserve(exts.size());
        for (const auto& ext : exts) {
            filters.emplace_back("*" + ext);
        }
        return filters;
    }
}

QStringList Global::getSupportedExtensionFilters()
{
    return cvtExtensionsToFilters(getSupportedExtensions());
}

QString Global::getSupportedExtensionsFilterString()
{
    return "Images (" + getSupportedExtensionFilters().join(" ") + ")";
}

const QString Global::kApplicationName = "ShibaView";

const QString Global::kOrganizationName = "Alexey Gruzdev";

const QString Global::kDefaultFont = ":/fonts/DejaVuSansCondensed.ttf";

QString Global::makeTitle(const QString& tag)
{
    return tag + " - " + kApplicationName;
}

const QStringList& Global::getSupportedExtensions() noexcept
{
    static QStringList extensions = []() {
        QStringList extensions{};
        for (int fifIdx = 0; fifIdx < FreeImage_GetFIFCount2(); ++fifIdx) {
            const auto fif = FreeImage_GetFIFFromIndex(fifIdx);
            if (fif == FIF_UNKNOWN) {
                continue;
            }
            if (const char* extsString = FreeImage_GetFIFExtensionList(fif)) {
                extensions.append(QString(extsString).split(',', Qt::SplitBehaviorFlags::SkipEmptyParts));
            }
        }
        return extensions;
    }();
    return extensions;
}
