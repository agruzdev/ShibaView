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

#ifndef QCHECKBOX2_H_
#define QCHECKBOX2_H_

#include <QCheckBox>


class QCheckBox2:
    public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)

public:
    explicit
    QCheckBox2(QWidget* parent = nullptr)
        : QCheckBox(parent)
    { }

    explicit
    QCheckBox2(const QString& text, QWidget* parent = nullptr)
        : QCheckBox(text, parent)
    { }

    ~QCheckBox2() override = default;


    bool isModified() const {
        return modified;
    }

    void setModified(bool value) {
        modified = value;
    }

protected:
    void nextCheckState() override {
        QCheckBox::nextCheckState();
        setModified(true);
    }

    bool modified{ false };

private:
    Q_DISABLE_COPY(QCheckBox2)
};


#endif // QCHECKBOX2_H_
