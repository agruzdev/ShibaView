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

#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QWidget>
#include <QSettings>
#include <QLineEdit>
#include "QCheckBox2.h"

class TextWidget;

class SettingsWidget final
    : public QWidget
{
    Q_OBJECT
public:
    SettingsWidget();

    ~SettingsWidget() override;

    SettingsWidget(const SettingsWidget&) = delete;

    SettingsWidget(SettingsWidget&&) = delete;

    SettingsWidget& operator=(const SettingsWidget&) = delete;

    SettingsWidget& operator=(SettingsWidget&&) = delete;

public slots:
    void onApply();

signals:
    void changed();

private:
    struct UsageCheckboxes;

    void keyPressEvent(QKeyEvent* event) override;


    std::unique_ptr<QSettings> mSettings;
    std::unique_ptr<QSettings> mPluginsSettings;

    QLineEdit* mEditBackgroundColor{ nullptr };
    QLineEdit* mEditTextColor{ nullptr };
    QCheckBox2* mShowCloseButton{ nullptr };

    std::unique_ptr<UsageCheckboxes> mPluginUsageFlo;
    std::unique_ptr<UsageCheckboxes> mPluginUsageSvg;
    QLineEdit* mEditSvgLibcairo{ nullptr };
    QLineEdit* mEditSvgLibrsvg{ nullptr };
};

#endif // SETTINGS_WIDGET_H
