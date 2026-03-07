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


const QString Global::kApplicationName = "ShibaView";

const QString Global::kOrganizationName = "Alexey Gruzdev";

const QString Global::kDefaultFont = ":/fonts/DejaVuSansCondensed.ttf";

QString Global::makeTitle(const QString& tag)
{
    return tag + " - " + kApplicationName;
}

