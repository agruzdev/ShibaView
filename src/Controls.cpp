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

#include "Controls.h"

#include <QDir>
#include <QFile>
#include "Global.h"
#include "Settings.h"


QString toQString(ControlAction a)
{
    switch (a) {
    case ControlAction::eQuit:
        return "Quit";
    case ControlAction::eAbout:
        return "About";
    case ControlAction::eImageInfo:
        return "ImageInfo";
    case ControlAction::eOverlay:
        return "Overlay";
    case ControlAction::eOpenFile:
        return "OpenFile";
    case ControlAction::eSaveFile:
        return "SaveFile";
    case ControlAction::eReload:
        return "Reload";
    case ControlAction::eCopyFrame:
        return "CopyFrame";
    case ControlAction::ePreviousImage:
        return "PreviousImage";
    case ControlAction::eNextImage:
        return "NextImage";
    case ControlAction::eFirstImage:
        return "FirstImage";
    case ControlAction::eLastImage:
        return "LastImage";
    case ControlAction::eZoomIn:
        return "ZoomIn";
    case ControlAction::eZoomOut:
        return "ZoomOut";
    case ControlAction::eSwitchZoom:
        return "SwitchZoom";
    case ControlAction::ePause:
        return "Pause";
    case ControlAction::eNextFrame:
        return "NextFrame";
    case ControlAction::ePreviousFrame:
        return "PreviousFrame";
    case ControlAction::eRotation0:
        return "Rotation0";
    case ControlAction::eRotation90:
        return "Rotation90";
    case ControlAction::eRotation180:
        return "Rotation180";
    case ControlAction::eRotation270:
        return "Rotation270";
    case ControlAction::eColorPicker:
        return "ColorPicker";
    case ControlAction::eDisplayPath:
        return "DisplayPath";
    case ControlAction::eHistogram:
        return "Histogram";
    case ControlAction::eSettings:
        return "Settings";
    case ControlAction::eLog:
        return "Log";
    default:
        assert(false);
        return "None";
    }
}


const Controls& Controls::getInstance()
{
    static Controls instance;
    return instance;
}

Controls::Controls()
{
    auto settings = Settings::getSettings(Settings::Group::eControls);
    const auto loadKey = [&](ControlAction action, QString comment, QKeySequence defaultKey, QKeySequence defaultKey2 = QKeySequence(), QKeySequence defaultKey3 = QKeySequence()) {
        assert(!defaultKey.isEmpty());
        const QStringList loadedValues = settings->value(toQString(action)).toStringList();
        QStringList  decodedValues;
        if (!loadedValues.isEmpty()) {
            for (const auto& s : loadedValues) {
                auto inputSequence = QKeySequence(s);
                if (!inputSequence.isEmpty()) {
                    decodedValues.append(inputSequence.toString(QKeySequence::NativeText));
                    mSeqToAction.emplace(std::move(inputSequence), action);
                }
            }
        }
        if (decodedValues.isEmpty()) {
            decodedValues.append(defaultKey.toString(QKeySequence::NativeText));
            mSeqToAction.emplace(defaultKey, action);
            if (!defaultKey2.isEmpty()) {
                decodedValues.append(defaultKey2.toString(QKeySequence::NativeText));
                mSeqToAction.emplace(defaultKey2, action);
            }
            if (!defaultKey3.isEmpty()) {
                decodedValues.append(defaultKey3.toString(QKeySequence::NativeText));
                mSeqToAction.emplace(defaultKey3, action);
            }
        }
        mActionDescriptions.emplace_back(action, comment, decodedValues);
        if (loadedValues.isEmpty()) {
            QStringList valuesToStore;
            valuesToStore.append(defaultKey.toString(QKeySequence::PortableText));
            if (!defaultKey2.isEmpty()) {
                valuesToStore.append(defaultKey2.toString(QKeySequence::PortableText));
            }
            if (!defaultKey3.isEmpty()) {
                valuesToStore.append(defaultKey3.toString(QKeySequence::PortableText));
            }
            settings->setValue(toQString(action), valuesToStore);
        }
    };
    loadKey(ControlAction::eAbout, "Show the About page", Qt::Key_F1);
    loadKey(ControlAction::eImageInfo, "Show EXIF data", Qt::Key_F2);
    loadKey(ControlAction::eOverlay, "Show overlay", Qt::Key_Tab);
    loadKey(ControlAction::eOpenFile, "Open file dialog", Qt::ControlModifier | Qt::Key_O);
    loadKey(ControlAction::eSaveFile, "Save file dialog", Qt::ControlModifier | Qt::Key_S);
    loadKey(ControlAction::eReload, "Reload current image", Qt::ControlModifier | Qt::Key_R);
    loadKey(ControlAction::eCopyFrame, "Copy current frame to clipboard", Qt::ControlModifier | Qt::Key_C);
    loadKey(ControlAction::ePreviousImage, "Previous image", Qt::Key_Left, Qt::KeypadModifier | Qt::Key_Left);
    loadKey(ControlAction::eNextImage, "Next image", Qt::Key_Right, Qt::KeypadModifier | Qt::Key_Right);
    loadKey(ControlAction::eFirstImage, "First image", Qt::Key_Home);
    loadKey(ControlAction::eLastImage, "Last image", Qt::Key_End);
    loadKey(ControlAction::eZoomIn, "Zoom in", Qt::Key_Plus, Qt::KeypadModifier | Qt::Key_Plus);
    loadKey(ControlAction::eZoomOut, "Zoom out", Qt::Key_Minus, Qt::KeypadModifier | Qt::Key_Minus);
    loadKey(ControlAction::eSwitchZoom, "Switch 100%/fit zoom modes", Qt::Key_Asterisk, Qt::KeypadModifier | Qt::Key_Asterisk, Qt::ShiftModifier | Qt::Key_Asterisk);
    loadKey(ControlAction::ePause, "Pause animation playback", Qt::Key_Space);
    loadKey(ControlAction::eNextFrame, "Next animation frame", Qt::Key_PageUp, Qt::KeypadModifier | Qt::Key_PageUp);
    loadKey(ControlAction::ePreviousFrame, "Previous animation frame", Qt::Key_PageDown, Qt::KeypadModifier | Qt::Key_PageDown);
    loadKey(ControlAction::eRotation0, "Toggle rotation 0" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Up);
    loadKey(ControlAction::eRotation90, "Toggle rotation 90" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Right);
    loadKey(ControlAction::eRotation180, "Toggle rotation 180" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Down);
    loadKey(ControlAction::eRotation270, "Toggle rotation 270" UTF8_DEGREE, Qt::ControlModifier | Qt::Key_Left);
    loadKey(ControlAction::eColorPicker, "Color picker mode", Qt::ControlModifier | Qt::Key_I);
    loadKey(ControlAction::eDisplayPath, "Display full path", Qt::ControlModifier | Qt::Key_P);
    loadKey(ControlAction::eHistogram, "Display/hide histogram", Qt::ControlModifier | Qt::Key_H);
    loadKey(ControlAction::eSettings, "Open settings window", Qt::Key_F9);
    loadKey(ControlAction::eLog, "Display/hide log", Qt::Key_F10);
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
