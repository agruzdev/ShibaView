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

#include "Controls.h"

#include <QDir>
#include <QFile>
#include "Global.h"

namespace
{
    static const QString kControlsFileName = "Settings.ini";
}

const Controls& Controls::getInstance()
{
    static Controls instance;
    return instance;
}

Controls::Controls()
{
    const QString absSettingsPath = QDir(QApplication::applicationDirPath()).filePath(kControlsFileName);
    const bool fileExists = QFile::exists(absSettingsPath);
    QSettings settings(absSettingsPath, QSettings::Format::IniFormat);
    settings.beginGroup("Controls");
    const auto loadKey = [&](ControlAction action, QString comment, QKeySequence defaultKey, QKeySequence defaultKey2 = QKeySequence(), QKeySequence defaultKey3 = QKeySequence()) {
        assert(!defaultKey.isEmpty());
        const QStringList loadedValues = settings.value(toQString(action)).toStringList();
        QStringList  decodedValues;
        if (!loadedValues.isEmpty()) {
            for (const auto& s : loadedValues) {
                auto inputSequence = QKeySequence(s);
                if (!inputSequence.isEmpty()) {
                    mSeqToAction.emplace(std::move(inputSequence), action);
                    decodedValues.append(inputSequence.toString(QKeySequence::NativeText));
                }
            }
        }
        if (decodedValues.isEmpty()) {
            mSeqToAction.emplace(defaultKey, action);
            decodedValues.append(defaultKey.toString(QKeySequence::NativeText));
            if (!defaultKey2.isEmpty()) {
                mSeqToAction.emplace(defaultKey2, action);
                decodedValues.append(defaultKey2.toString(QKeySequence::NativeText));
            }
            if (!defaultKey3.isEmpty()) {
                mSeqToAction.emplace(defaultKey3, action);
                decodedValues.append(defaultKey3.toString(QKeySequence::NativeText));
            }
        }
        mActionDescriptions.emplace_back(action, comment, decodedValues);
        if (!fileExists) {
            QStringList valuesToStore;
            valuesToStore.append(defaultKey.toString(QKeySequence::PortableText));
            if (!defaultKey2.isEmpty()) {
                valuesToStore.append(defaultKey2.toString(QKeySequence::PortableText));
            }
            if (!defaultKey3.isEmpty()) {
                valuesToStore.append(defaultKey3.toString(QKeySequence::PortableText));
            }
            settings.setValue(toQString(action), valuesToStore);
        }
    };
    loadKey(ControlAction::eAbout, "Show the About page", Qt::Key_F1);
    loadKey(ControlAction::eImageInfo, "Show EXIF data", Qt::Key_F2);
    loadKey(ControlAction::eOverlay, "Show overlay", Qt::Key_Tab);
    loadKey(ControlAction::eOpenFile, "Open file dialog", Qt::ControlModifier | Qt::Key_O);
    loadKey(ControlAction::eReload, "Reload the current image", Qt::ControlModifier | Qt::Key_R);
    loadKey(ControlAction::ePreviousImage, "Previous image", Qt::Key_Left, Qt::KeypadModifier | Qt::Key_Left);
    loadKey(ControlAction::eNextImage, "Next image", Qt::Key_Right, Qt::KeypadModifier | Qt::Key_Right);
    loadKey(ControlAction::eFirstImage, "First image", Qt::Key_Home);
    loadKey(ControlAction::eLastImage, "Last image", Qt::Key_End);
    loadKey(ControlAction::eZoomIn, "Zoom in", Qt::Key_Plus, Qt::KeypadModifier | Qt::Key_Plus);
    loadKey(ControlAction::eZoomOut, "Zoom out", Qt::Key_Minus, Qt::KeypadModifier | Qt::Key_Minus);
    loadKey(ControlAction::eSwitchZoom, "Switch 100%/fit zoom modes", Qt::Key_Asterisk, Qt::KeypadModifier | Qt::Key_Asterisk, Qt::ShiftModifier | Qt::Key_Asterisk);
    loadKey(ControlAction::ePause, "Pause animation playback", Qt::Key_Space);
    loadKey(ControlAction::eNextFrame, "Next animation frame", Qt::Key_PageUp);
    loadKey(ControlAction::ePreviousFrame, "Previous animation frame", Qt::Key_PageDown);
    loadKey(ControlAction::eRotation0, "Toggle rotation 0" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Up);
    loadKey(ControlAction::eRotation90, "Toggle rotation 90" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Right);
    loadKey(ControlAction::eRotation180, "Toggle rotation 180" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Down);
    loadKey(ControlAction::eRotation270, "Toggle rotation 270" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Left);
    loadKey(ControlAction::eColorPicker, "Color picker mode", Qt::ControlModifier | Qt::Key_I);
    loadKey(ControlAction::eQuit, "Quit", Qt::Key_Escape);
}

Controls::~Controls()
{
}

ControlAction Controls::decodeAction(QKeyEvent* event, Qt::KeyboardModifiers modifiers) const
{
    if (event) {
        const QKeySequence inputSequence = QKeySequence(modifiers | event->key());
        //qDebug() << inputSequence;
        const auto it = mSeqToAction.find(inputSequence);
        if (it != mSeqToAction.cend()) {
            return (*it).second;
        }
    }
    return ControlAction::eNone;
}

std::vector<std::tuple<QString, QString>> Controls::printControls() const
{
    std::vector<std::tuple<QString, QString>> result;
    result.reserve(static_cast<size_t>(ControlAction::length_));
    for (const auto& [action, comment, keys] : mActionDescriptions) {
        static_cast<void>(action);
        result.emplace_back(comment, keys.join(","));
    }
    return result;
}
