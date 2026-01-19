/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
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

#include <cassert>
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
    eOpenFile,
    eSaveFile,
    eReload,
    eCopyFrame,
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
    eDisplayPath,
    eHistogram,
    eSettings,
    eLog,
    eQuit,

    length_
};


QString toQString(ControlAction a);



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
