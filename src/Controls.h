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

#ifndef CONTROLS_H
#define CONTROLS_H

#include <QApplication>
#include <QSettings>
#include <QKeyEvent>
#include <QKeySequence>

enum class ControlAction
{
    eNone,
    eAbout,
    eImageInfo,
    eOverlay,
    eReload,
    ePreviousImage,
    eNextImage,
    eFirstImage,
    eLastImage,
    eZoomIn,
    eZoomOut,
    eSwitchZoom,
    ePause,
    eNextFrame,
    ePreviousFrame,
    eRotation0,
    eRotation90,
    eRotation180,
    eRotation270,
    eColorPicker,
    eQuit,

    length_
};

inline
QString toQString(ControlAction a)
{
    switch(a) {
    case ControlAction::eQuit:
        return "Quit";
    case ControlAction::eAbout:
        return "About";
    case ControlAction::eImageInfo:
        return "ImageInfo";
    case ControlAction::eOverlay:
        return "Overlay";
    case ControlAction::eReload:
        return "Reload";
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
    default:
        return "None";
    }
}

class Controls
{
public:
    static
    const Controls& getInstance();

    Controls(const Controls&) = delete;

    Controls(Controls&&) = delete;

    Controls& operator=(const Controls&) = delete;

    Controls& operator=(Controls&&) = delete;


    ControlAction decodeAction(QKeyEvent* event, Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers()) const;

    std::vector<std::tuple<QString, QString>> printControls() const;

private:
    Controls();

    ~Controls();

    std::vector<std::tuple<ControlAction, QString, QStringList>> mActionDescriptions;
    std::map<QKeySequence, ControlAction> mSeqToAction;
};


#endif // CONTROLS_H
