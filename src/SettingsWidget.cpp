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

#include "SettingsWidget.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QValidator>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include "Global.h"
#include "TextWidget.h"
#include "Settings.h"
#include "PluginManager.h"
#include <iostream>

struct SettingsWidget::UsageCheckboxes
{
    QCheckBox2* useInViewer{ nullptr };
    QCheckBox2* useInThumbnails{ nullptr };

    bool isValid() const {
        return (useInViewer != nullptr) && (useInThumbnails != nullptr);
    }

    bool isModified() const {
        return isValid() && (useInViewer->isModified() || useInThumbnails->isModified());
    }

    void setFromUsage(PluginUsage usage) const {
        if (useInViewer) {
            useInViewer->setChecked((usage & PluginUsage::eViewer) != PluginUsage::eNone);
        }
        if (useInThumbnails) {
            useInThumbnails->setChecked((usage & PluginUsage::eThumbnails) != PluginUsage::eNone);
        }
    }

    PluginUsage toUsage() const {
        PluginUsage usage{ PluginUsage::eNone };
        if (useInViewer && useInViewer->isChecked()) {
            usage = usage | PluginUsage::eViewer;
        }
        if (useInThumbnails && useInThumbnails->isChecked()) {
            usage = usage | PluginUsage::eThumbnails;
        }
        return usage;
    }
};


SettingsWidget::SettingsWidget()
    : QWidget(nullptr)
{
    constexpr qreal kTitleFontSize = 14.0;
    constexpr qreal kLabelFontSize = 12.0;

    mSettings = Settings::getSettings(Settings::Group::eGlobal);
    assert(mSettings);

    setWindowTitle(Global::makeTitle("Settings"));
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);

    auto vlayout = new QVBoxLayout(this);

    uint32_t lineIndex = 0;
    auto appendOption = [&](QGridLayout* grid, QString labelText, auto elemPtr) -> auto {
        auto label = std::make_unique<TextWidget>(nullptr, std::nullopt, kLabelFontSize);
        label->setText(labelText);
        grid->addWidget(label.release(), lineIndex, 0);
        auto ptr = elemPtr.release();
        grid->addWidget(ptr, lineIndex, 1);
        ++lineIndex;
        return ptr;
    };

    if (mSettings) {

        // [Global]
        auto gridWidget = new QWidget(this);
        auto gridGlobal = new QGridLayout(gridWidget);
        gridGlobal->setSpacing(8);

        auto title = std::make_unique<TextWidget>(nullptr, std::nullopt, kTitleFontSize);
        title->setText("Global");
        gridGlobal->addWidget(title.release(), lineIndex++, 0);

        //
        mEditBackgroundColor = appendOption(gridGlobal, "Background color", std::make_unique<QLineEdit>(nullptr));
        mEditBackgroundColor->setText(mSettings->value(Settings::kParamBackgroundKey, Settings::kParamBackgroundDefault).toString());
        mEditBackgroundColor->setValidator(new QRegularExpressionValidator(QRegularExpression("#[0-9a-fA-F]{6}")));

        //
        mEditTextColor = appendOption(gridGlobal, "Text color", std::make_unique<QLineEdit>(nullptr));
        mEditTextColor->setText(mSettings->value(Settings::kParamTextColorKey, Settings::kParamTextColorDefault).toString());
        mEditTextColor->setValidator(new QRegularExpressionValidator(QRegularExpression("#[0-9a-fA-F]{6}")));

        //
        mShowCloseButton = appendOption(gridGlobal, "Show Close button", std::make_unique<QCheckBox2>(nullptr));
        mShowCloseButton->setChecked(mSettings->value(Settings::kParamShowCloseButtonKey, Settings::kParamShowCloseButtonDefault).toBool());

        //
        mInvertZoom = appendOption(gridGlobal, "Invert zoom direction", std::make_unique<QCheckBox2>(nullptr));
        mInvertZoom->setChecked(mSettings->value(Settings::kParamInvertZoom, Settings::kParamInvertZoomDefault).toBool());

        vlayout->addWidget(gridWidget);

    } // [Global]

    vlayout->addItem(new QSpacerItem(4, 4, QSizePolicy::Fixed, QSizePolicy::Fixed));
    lineIndex = 0;

    mPluginsSettings = Settings::getSettings(Settings::Group::ePlugins);
    if (mPluginsSettings) {
        auto appendPluginUsage = [&](QGridLayout* grid, const QString& labelText, PluginUsage usageMask) {
            auto checkboxes = std::make_unique<UsageCheckboxes>();

            auto label = std::make_unique<TextWidget>(nullptr, std::nullopt, kLabelFontSize);
            label->setText(labelText);

            auto checkBoxViewer = std::make_unique<QCheckBox2>(nullptr);
            checkBoxViewer->setChecked(testFlag(usageMask, PluginUsage::eViewer));
            checkboxes->useInViewer = checkBoxViewer.get();

            auto checkBoxThumbnails = std::make_unique<QCheckBox2>(nullptr);
            checkBoxThumbnails->setChecked(testFlag(usageMask, PluginUsage::eThumbnails));
            checkboxes->useInThumbnails = checkBoxThumbnails.get();

            grid->addWidget(label.release(), lineIndex, 0);
            grid->addWidget(checkBoxViewer.release(), lineIndex, 1);
            grid->addWidget(checkBoxThumbnails.release(), lineIndex, 2);
            ++lineIndex;

            return checkboxes;
        };

        // [Plguins]
        auto gridWidget = new QWidget(this);
        auto gridPlugins = new QGridLayout(gridWidget);
        gridPlugins->setSpacing(8);

        auto title = std::make_unique<TextWidget>(nullptr, std::nullopt, kTitleFontSize);
        title->setText("Plugins");
        gridPlugins->addWidget(title.release(), lineIndex++, 0);

        auto title1 = std::make_unique<TextWidget>(nullptr, std::nullopt, kLabelFontSize);
        title1->setText("in Viewer");
        gridPlugins->addWidget(title1.release(), lineIndex, 1);

        auto title2 = std::make_unique<TextWidget>(nullptr, std::nullopt, kLabelFontSize);
        title2->setText("in Thumbnails");
        gridPlugins->addWidget(title2.release(), lineIndex, 2);

        ++lineIndex;

        //
        mPluginUsageFlo = appendPluginUsage(gridPlugins, "FLO", PluginUsage::eNone);
        mPluginUsageSvg = appendPluginUsage(gridPlugins, "SVG", PluginUsage::eNone);

        //
        vlayout->addWidget(gridWidget);

        //
        auto gridExtraWidget = new QWidget(this);
        auto gridExtra = new QGridLayout(gridExtraWidget);
        gridExtra->setSpacing(8);

        mEditSvgLibcairo = appendOption(gridExtra, "SVG: libcairo-2", std::make_unique<QLineEdit>(nullptr));
        mEditSvgLibrsvg  = appendOption(gridExtra, "SVG: librsvg",    std::make_unique<QLineEdit>(nullptr));

        //
        vlayout->addWidget(gridExtraWidget);

    } // [Plugins]

    vlayout->addItem(new QSpacerItem(4, 4, QSizePolicy::Fixed, QSizePolicy::Expanding));

    //
    auto buttonsBox = std::make_unique<QDialogButtonBox>();
    buttonsBox->addButton("Apply", QDialogButtonBox::AcceptRole);
    buttonsBox->addButton("Close", QDialogButtonBox::RejectRole);
    connect(buttonsBox.get(), &QDialogButtonBox::accepted, this, &SettingsWidget::onApply);
    connect(buttonsBox.get(), &QDialogButtonBox::rejected, this, &SettingsWidget::close);
    vlayout->addWidget(buttonsBox.release());
}

SettingsWidget::~SettingsWidget() = default;

void SettingsWidget::showEvent(QShowEvent* event)
{
    if (mSettings) {
        if (mEditBackgroundColor) {
            mEditBackgroundColor->setText(mSettings->value(Settings::kParamBackgroundKey, Settings::kParamBackgroundDefault).toString());
        }
        if (mEditTextColor) {
            mEditTextColor->setText(mSettings->value(Settings::kParamTextColorKey, Settings::kParamTextColorDefault).toString());
        }
        if (mShowCloseButton) {
            mShowCloseButton->setChecked(mSettings->value(Settings::kParamShowCloseButtonKey, Settings::kParamShowCloseButtonDefault).toBool());
        }
        if (mInvertZoom) {
            mInvertZoom->setChecked(mSettings->value(Settings::kParamInvertZoom, Settings::kParamInvertZoomDefault).toBool());
        }
    }

    if (mPluginsSettings) {
        if (mPluginUsageFlo) {
            mPluginUsageFlo->setFromUsage(static_cast<PluginUsage>(mPluginsSettings->value(Settings::kPluginFloUsage, Settings::kPluginFloUsageDefault).toUInt()));
        }
        if (mPluginUsageSvg) {
            mPluginUsageSvg->setFromUsage(static_cast<PluginUsage>(mPluginsSettings->value(Settings::kPluginSvgUsage, Settings::kPluginSvgUsageDefault).toUInt()));
        }
        if (mEditSvgLibcairo) {
            mEditSvgLibcairo->setText(mPluginsSettings->value(Settings::kPluginSvgLibcairo, QString{}).toString());
        }
        if (mEditSvgLibrsvg) {
            mEditSvgLibrsvg->setText(mPluginsSettings->value(Settings::kPluginSvgLibrsvg, QString{}).toString());
        }
    }
}

void SettingsWidget::onApply()
{
    bool globalsChanged{ false };
    if (mSettings) {
        if (mEditBackgroundColor && mEditBackgroundColor->isModified() && mEditBackgroundColor->hasAcceptableInput()) {
            mSettings->setValue(Settings::kParamBackgroundKey, mEditBackgroundColor->text());
            globalsChanged = true;
        }
        if (mEditTextColor && mEditTextColor->isModified() && mEditTextColor->hasAcceptableInput()) {
            mSettings->setValue(Settings::kParamTextColorKey, mEditTextColor->text());
            globalsChanged = true;
        }
        if (mShowCloseButton && mShowCloseButton->isModified()) {
            mSettings->setValue(Settings::kParamShowCloseButtonKey, mShowCloseButton->isChecked());
            globalsChanged = true;
        }
        if (mInvertZoom && mInvertZoom->isModified()) {
            mSettings->setValue(Settings::kParamInvertZoom, mInvertZoom->isChecked());
            globalsChanged = true;
        }
    }

    bool pluginsChanged{ false };
    if (mPluginsSettings) {
        if (mPluginUsageFlo && mPluginUsageFlo->isModified()) {
            mPluginsSettings->setValue(Settings::kPluginFloUsage, static_cast<uint32_t>(mPluginUsageFlo->toUsage()));
            pluginsChanged = true;
        }
        if (mPluginUsageSvg && mPluginUsageSvg->isModified()) {
            mPluginsSettings->setValue(Settings::kPluginSvgUsage, static_cast<uint32_t>(mPluginUsageSvg->toUsage()));
            pluginsChanged = true;
        }
        if (mEditSvgLibcairo && mEditSvgLibcairo->isModified()) {
            mPluginsSettings->setValue(Settings::kPluginSvgLibcairo, mEditSvgLibcairo->text());
            pluginsChanged = true;
        }
        if (mEditSvgLibrsvg && mEditSvgLibrsvg->isModified()) {
            mPluginsSettings->setValue(Settings::kPluginSvgLibrsvg, mEditSvgLibrsvg->text());
            pluginsChanged = true;
        }
    }

    if (pluginsChanged) {
        PluginManager::getInstance().reload();
    }

    if (globalsChanged || pluginsChanged) {
        emit changed();
    }
}


void SettingsWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}
