// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "AdvancedGraphics.h"

#include "../configuration/configuration.h"
#include "../display/MapCanvasConfig.h"
#include "../global/FixedPoint.h"
#include "../global/RuleOf5.h"
#include "../global/SignalBlocker.h"
#include "../global/utils.h"

#include <cassert>
#include <memory>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class NODISCARD FpSlider final : public QSlider
{
private:
    FixedPoint<1> &m_fp;

public:
    explicit FpSlider(FixedPoint<1> &fp)
        : QSlider(Qt::Orientation::Horizontal)
        , m_fp{fp}
    {
        setRange(m_fp.min, m_fp.max);
        setValue(m_fp.get());
    }
    ~FpSlider() final;
};

FpSlider::~FpSlider() = default;

class NODISCARD FpSpinBox final : public QDoubleSpinBox
{
private:
    using FP = FixedPoint<1>;
    FP &m_fp;

public:
    explicit FpSpinBox(FixedPoint<1> &fp)
        : QDoubleSpinBox()
        , m_fp{fp}
    {
        const double fraction = std::pow(10.0, -FP::digits);
        setRange(static_cast<double>(m_fp.min) * fraction, static_cast<double>(m_fp.max) * fraction);
        setValue(m_fp.getDouble());
        setDecimals(FP::digits);
        setSingleStep(fraction);
    }
    ~FpSpinBox() final;

public:
    NODISCARD int getIntValue() const
    {
        return static_cast<int>(std::lround(std::clamp(value() * std::pow(10.0, FP::digits),
                                                       static_cast<double>(m_fp.min),
                                                       static_cast<double>(m_fp.max))));
    }
    void setIntValue(int value)
    {
        setValue(static_cast<double>(value) * std::pow(10.0, -FP::digits));
    }
};

FpSpinBox::~FpSpinBox() = default;

class NODISCARD SliderSpinboxButton final
{
private:
    using FP = FixedPoint<1>;
    AdvancedGraphicsGroupBox &m_group;
    FP &m_fp;
    FpSlider m_slider;
    FpSpinBox m_spin;
    QPushButton m_reset;
    QHBoxLayout m_horizontal;
    QVector<QWidget *> m_blockable;

public:
    explicit SliderSpinboxButton(AdvancedGraphicsGroupBox &in_group,
                                 QVBoxLayout &vbox,
                                 const QString &name,
                                 FP &in_fp)
        : m_group{in_group}
        , m_fp{in_fp}
        , m_slider(m_fp)
        , m_spin(m_fp)
        , m_reset("Reset")
    {
        auto &group = m_group;
        QObject::connect(&m_slider, &QSlider::valueChanged, &group, [this](int value) {
            const SignalBlocker block_slider{m_slider};
            const SignalBlocker block_spin{m_spin};
            m_fp.set(value);
            m_spin.setIntValue(value);
            m_group.graphicsSettingsChanged();
        });

        QObject::connect(&m_spin,
                         QOverload<double>::of(&FpSpinBox::valueChanged),
                         &group,
                         [this](double /*value*/) {
                             const SignalBlocker block_slider{m_slider};
                             const SignalBlocker block_spin{m_spin};
                             const int value = m_spin.getIntValue();
                             m_fp.set(value);
                             m_slider.setValue(value);
                             m_group.graphicsSettingsChanged();
                         });

        QObject::connect(&m_reset, &QPushButton::clicked, &group, [this](bool) {
            m_slider.setValue(m_fp.defaultValue);
        });

        vbox.addWidget(new QLabel(name));

        auto &horizontal = m_horizontal;
        horizontal.addSpacing(20);
        horizontal.addWidget(&m_slider);
        horizontal.addWidget(&m_spin);
        horizontal.addWidget(&m_reset);
        vbox.addLayout(&horizontal, 0);
    }
    ~SliderSpinboxButton() = default;

    void setEnabled(bool enabled)
    {
        this->m_slider.setEnabled(enabled);
        this->m_spin.setEnabled(enabled);
        this->m_reset.setEnabled(enabled);
    }

    void forcedUpdate()
    {
        const SignalBlocker block_slider{m_slider};
        const SignalBlocker block_spin{m_spin};

        const auto value = m_fp.get();
        m_spin.setIntValue(value);
        m_slider.setValue(value);
        if ((false)) {
            m_group.graphicsSettingsChanged();
        }
    }

    DELETE_CTORS_AND_ASSIGN_OPS(SliderSpinboxButton);
};

static void addLine(QLayout &layout)
{
    auto *const line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout.addWidget(line);
}

AdvancedGraphicsGroupBox::AdvancedGraphicsGroupBox(QGroupBox &groupBox)
    : QObject{&groupBox}
    , m_groupBox{&groupBox}
{
    auto *vertical = new QVBoxLayout(m_groupBox);

    using FP = FixedPoint<1>;

    auto makeSsb = [this, vertical](const QString name, FP &fp) {
        addLine(*vertical);
        m_ssbs.emplace_back(std::make_unique<SliderSpinboxButton>(*this, *vertical, name, fp));
    };

    // Background Image Controls (at top of Advanced Settings)
    auto &advanced = setConfig().canvas.advanced;

    auto *bgImageCheckbox = new QCheckBox("Enable Background Image");
    bgImageCheckbox->setChecked(advanced.useBackgroundImage);
    vertical->addWidget(bgImageCheckbox);

    auto *bgImageLayout = new QHBoxLayout();
    bgImageLayout->addSpacing(20);

    auto *bgImageButton = new QPushButton("Select Image...");
    bgImageLayout->addWidget(bgImageButton);

    auto *bgImageLabel = new QLabel(advanced.backgroundImagePath.isEmpty()
        ? "No image selected"
        : QFileInfo(advanced.backgroundImagePath).fileName());
    bgImageLabel->setWordWrap(true);
    bgImageLayout->addWidget(bgImageLabel, 1);

    vertical->addLayout(bgImageLayout);

    // Fit mode dropdown
    auto *fitModeLayout = new QHBoxLayout();
    fitModeLayout->addSpacing(20);
    fitModeLayout->addWidget(new QLabel("Fit Mode:"));

    auto *fitModeCombo = new QComboBox();
    fitModeCombo->addItem("Fit (Letterbox)", 0);
    fitModeCombo->addItem("Fill (Crop)", 1);
    fitModeCombo->addItem("Stretch", 2);
    fitModeCombo->addItem("Center", 3);
    fitModeCombo->addItem("Tile", 4);
    fitModeCombo->addItem("Focused (Follow Player)", 5);
    fitModeCombo->setCurrentIndex(advanced.backgroundFitMode);
    fitModeLayout->addWidget(fitModeCombo);
    fitModeLayout->addStretch();

    vertical->addLayout(fitModeLayout);

    // Opacity slider
    auto *opacityLayout = new QHBoxLayout();
    opacityLayout->addSpacing(20);
    opacityLayout->addWidget(new QLabel("Opacity:"));

    auto *opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(static_cast<int>(advanced.backgroundOpacity * 100.0f));
    opacitySlider->setTickPosition(QSlider::TicksBelow);
    opacitySlider->setTickInterval(10);
    opacityLayout->addWidget(opacitySlider);

    auto *opacityLabel = new QLabel(QString::number(static_cast<int>(advanced.backgroundOpacity * 100.0f)) + "%");
    opacityLabel->setMinimumWidth(40);
    opacityLayout->addWidget(opacityLabel);

    vertical->addLayout(opacityLayout);

    // Focused mode scale slider
    auto *scaleLayout = new QHBoxLayout();
    scaleLayout->addSpacing(20);
    scaleLayout->addWidget(new QLabel("Focus Scale:"));

    auto *scaleSlider = new QSlider(Qt::Horizontal);
    scaleSlider->setRange(10, 10000);  // 0.1x to 100.0x (stored as integer * 100)
    scaleSlider->setValue(static_cast<int>(advanced.backgroundFocusedScale * 100.0f));
    scaleSlider->setTickPosition(QSlider::TicksBelow);
    scaleSlider->setTickInterval(1000);
    scaleSlider->setSingleStep(10);  // 0.1x increment for fine control
    scaleLayout->addWidget(scaleSlider);

    auto *scaleSpinBox = new QDoubleSpinBox();
    scaleSpinBox->setRange(0.1, 100.0);
    scaleSpinBox->setValue(advanced.backgroundFocusedScale);
    scaleSpinBox->setDecimals(1);
    scaleSpinBox->setSingleStep(0.1);
    scaleSpinBox->setSuffix("x");
    scaleSpinBox->setMinimumWidth(80);
    scaleLayout->addWidget(scaleSpinBox);

    vertical->addLayout(scaleLayout);

    // Focused mode X offset slider
    auto *offsetXLayout = new QHBoxLayout();
    offsetXLayout->addSpacing(20);
    offsetXLayout->addWidget(new QLabel("X Offset:"));

    auto *offsetXSlider = new QSlider(Qt::Horizontal);
    offsetXSlider->setRange(-1000, 1000);  // -1000 to 1000
    offsetXSlider->setValue(static_cast<int>(advanced.backgroundFocusedOffsetX));
    offsetXSlider->setTickPosition(QSlider::TicksBelow);
    offsetXSlider->setTickInterval(200);
    offsetXSlider->setSingleStep(1);  // Fine control with 1-unit steps
    offsetXLayout->addWidget(offsetXSlider);

    auto *offsetXSpinBox = new QSpinBox();
    offsetXSpinBox->setRange(-1000, 1000);
    offsetXSpinBox->setValue(static_cast<int>(advanced.backgroundFocusedOffsetX));
    offsetXSpinBox->setMinimumWidth(70);
    offsetXLayout->addWidget(offsetXSpinBox);

    vertical->addLayout(offsetXLayout);

    // Focused mode Y offset slider
    auto *offsetYLayout = new QHBoxLayout();
    offsetYLayout->addSpacing(20);
    offsetYLayout->addWidget(new QLabel("Y Offset:"));

    auto *offsetYSlider = new QSlider(Qt::Horizontal);
    offsetYSlider->setRange(-1000, 1000);  // -1000 to 1000
    offsetYSlider->setValue(static_cast<int>(advanced.backgroundFocusedOffsetY));
    offsetYSlider->setTickPosition(QSlider::TicksBelow);
    offsetYSlider->setTickInterval(200);
    offsetYSlider->setSingleStep(1);  // Fine control with 1-unit steps
    offsetYLayout->addWidget(offsetYSlider);

    auto *offsetYSpinBox = new QSpinBox();
    offsetYSpinBox->setRange(-1000, 1000);
    offsetYSpinBox->setValue(static_cast<int>(advanced.backgroundFocusedOffsetY));
    offsetYSpinBox->setMinimumWidth(70);
    offsetYLayout->addWidget(offsetYSpinBox);

    vertical->addLayout(offsetYLayout);

    // Initially hide all focused mode controls unless Focused mode is selected
    scaleLayout->setEnabled(advanced.backgroundFitMode == 5);
    offsetXLayout->setEnabled(advanced.backgroundFitMode == 5);
    offsetYLayout->setEnabled(advanced.backgroundFitMode == 5);

    addLine(*vertical); // Separator line

    auto *const checkboxDiag = new QCheckBox("Show Performance Stats");
    checkboxDiag->setChecked(MapCanvasConfig::getShowPerfStats());
    vertical->addWidget(checkboxDiag);

    auto *const checkbox3d = new QCheckBox("3d Mode");
    const bool is3dAtInit = MapCanvasConfig::isIn3dMode();
    checkbox3d->setChecked(is3dAtInit);
    vertical->addWidget(checkbox3d);

    auto *const autoTilt = new QCheckBox("Auto tilt with zoom");
    autoTilt->setChecked(MapCanvasConfig::isAutoTilt());
    vertical->addWidget(autoTilt);

    // Layer Transparency slider
    addLine(*vertical);
    auto *layerTransLabel = new QLabel("Layer Transparency:");
    vertical->addWidget(layerTransLabel);

    auto *layerTransLayout = new QHBoxLayout();
    layerTransLayout->addSpacing(20);

    auto *layerTransSlider = new QSlider(Qt::Horizontal);
    layerTransSlider->setRange(0, 100);
    layerTransSlider->setValue(static_cast<int>(getConfig().canvas.layerTransparency * 100.0f));
    layerTransSlider->setTickPosition(QSlider::TicksBelow);
    layerTransSlider->setTickInterval(10);
    layerTransLayout->addWidget(layerTransSlider);

    auto *layerTransValueLabel = new QLabel(QString::number(static_cast<int>(getConfig().canvas.layerTransparency * 100.0f)) + "%");
    layerTransValueLabel->setMinimumWidth(40);
    layerTransLayout->addWidget(layerTransValueLabel);

    vertical->addLayout(layerTransLayout);

    // Radial Transparency checkbox
    auto *radialTransCheckbox = new QCheckBox("Radial Transparency (distance-based layer visibility)");
    radialTransCheckbox->setChecked(getConfig().canvas.enableRadialTransparency);
    vertical->addWidget(radialTransCheckbox);

    {
        // NOTE: This is a slight abuse of the interface, because we're taking a persistent reference.
        auto &advanced = setConfig().canvas.advanced;
        makeSsb("Field of View (fovy)", advanced.fov);
        makeSsb("Vertical Angle (pitch up from straight down)", advanced.verticalAngle);
        makeSsb("Horizontal Angle (yaw)", advanced.horizontalAngle);
        makeSsb("Layer height (in rooms)", advanced.layerHeight);
    }

    enableSsbs(is3dAtInit);
    autoTilt->setEnabled(is3dAtInit);

    m_groupBox->setLayout(vertical);

    // Background image connections
    connect(bgImageCheckbox, &QCheckBox::stateChanged, this, [this, bgImageCheckbox](int) {
        setConfig().canvas.advanced.useBackgroundImage = bgImageCheckbox->isChecked();
        graphicsSettingsChanged();
    });

    connect(bgImageButton, &QPushButton::clicked, this, [this, bgImageLabel]() {
        const QString filter = "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff)";
        const QString currentPath = getConfig().canvas.advanced.backgroundImagePath;
        const QString startDir = currentPath.isEmpty()
            ? QDir::currentPath()
            : QFileInfo(currentPath).absolutePath();

        const QString fileName = QFileDialog::getOpenFileName(
            m_groupBox,
            "Select Background Image",
            startDir,
            filter);

        if (!fileName.isEmpty()) {
            setConfig().canvas.advanced.backgroundImagePath = fileName;
            bgImageLabel->setText(QFileInfo(fileName).fileName());
            graphicsSettingsChanged();
        }
    });

    connect(fitModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, scaleLayout, offsetXLayout, offsetYLayout](int index) {
        setConfig().canvas.advanced.backgroundFitMode = index;
        // Enable focused mode controls only when Focused mode (5) is selected
        const bool isFocused = (index == 5);
        scaleLayout->setEnabled(isFocused);
        offsetXLayout->setEnabled(isFocused);
        offsetYLayout->setEnabled(isFocused);
        graphicsSettingsChanged();
    });

    connect(opacitySlider, &QSlider::valueChanged, this, [this, opacityLabel](int value) {
        const float opacity = static_cast<float>(value) / 100.0f;
        setConfig().canvas.advanced.backgroundOpacity = opacity;
        opacityLabel->setText(QString::number(value) + "%");
        graphicsSettingsChanged();
    });

    // Connect scale slider and spinbox
    connect(scaleSlider, &QSlider::valueChanged, this, [this, scaleSpinBox](int value) {
        const float scale = static_cast<float>(value) / 100.0f;
        scaleSpinBox->blockSignals(true);
        scaleSpinBox->setValue(scale);
        scaleSpinBox->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedScale = scale;
        graphicsSettingsChanged();
    });

    connect(scaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this, scaleSlider](double value) {
        scaleSlider->blockSignals(true);
        scaleSlider->setValue(static_cast<int>(value * 100.0f));
        scaleSlider->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedScale = static_cast<float>(value);
        graphicsSettingsChanged();
    });

    // Connect X offset slider and spinbox
    connect(offsetXSlider, &QSlider::valueChanged, this, [this, offsetXSpinBox](int value) {
        offsetXSpinBox->blockSignals(true);
        offsetXSpinBox->setValue(value);
        offsetXSpinBox->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedOffsetX = static_cast<float>(value);
        graphicsSettingsChanged();
    });

    connect(offsetXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this, offsetXSlider](int value) {
        offsetXSlider->blockSignals(true);
        offsetXSlider->setValue(value);
        offsetXSlider->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedOffsetX = static_cast<float>(value);
        graphicsSettingsChanged();
    });

    // Connect Y offset slider and spinbox
    connect(offsetYSlider, &QSlider::valueChanged, this, [this, offsetYSpinBox](int value) {
        offsetYSpinBox->blockSignals(true);
        offsetYSpinBox->setValue(value);
        offsetYSpinBox->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedOffsetY = static_cast<float>(value);
        graphicsSettingsChanged();
    });

    connect(offsetYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this, offsetYSlider](int value) {
        offsetYSlider->blockSignals(true);
        offsetYSlider->setValue(value);
        offsetYSlider->blockSignals(false);
        setConfig().canvas.advanced.backgroundFocusedOffsetY = static_cast<float>(value);
        graphicsSettingsChanged();
    });

    connect(checkbox3d, &QCheckBox::stateChanged, this, [this, checkbox3d, autoTilt](int) {
        const bool is3d = checkbox3d->isChecked();
        MapCanvasConfig::set3dMode(is3d);
        enableSsbs(is3d);
        autoTilt->setEnabled(is3d);
        graphicsSettingsChanged();
    });

    connect(autoTilt, &QCheckBox::stateChanged, this, [this, autoTilt](int) {
        const bool val = autoTilt->isChecked();
        MapCanvasConfig::setAutoTilt(val);
        graphicsSettingsChanged();
    });

    connect(checkboxDiag, &QCheckBox::stateChanged, this, [this, checkboxDiag](int) {
        const bool show = checkboxDiag->isChecked();
        MapCanvasConfig::setShowPerfStats(show);
        graphicsSettingsChanged();
    });

    connect(layerTransSlider, &QSlider::valueChanged, this, [this, layerTransValueLabel](int value) {
        const float transparency = static_cast<float>(value) / 100.0f;
        setConfig().canvas.layerTransparency = transparency;
        layerTransValueLabel->setText(QString::number(value) + "%");
        graphicsSettingsChanged();
    });

    connect(radialTransCheckbox, &QCheckBox::stateChanged, this, [this, radialTransCheckbox](int) {
        setConfig().canvas.enableRadialTransparency = radialTransCheckbox->isChecked();
        graphicsSettingsChanged();
    });

    MapCanvasConfig::registerChangeCallback(m_lifetime,
                                            [this, checkboxDiag, checkbox3d, autoTilt]() -> void {
                                                SignalBlocker sb1{*checkboxDiag};
                                                SignalBlocker sb2{*checkbox3d};
                                                SignalBlocker sb3{*autoTilt};
                                                for (auto &ssb : m_ssbs) {
                                                    ssb->forcedUpdate();
                                                }
                                                checkboxDiag->setChecked(
                                                    MapCanvasConfig::getShowPerfStats());
                                                checkbox3d->setChecked(
                                                    MapCanvasConfig::isIn3dMode());
                                                autoTilt->setChecked(MapCanvasConfig::isAutoTilt());
                                            });
}

AdvancedGraphicsGroupBox::~AdvancedGraphicsGroupBox() = default;

void AdvancedGraphicsGroupBox::enableSsbs(bool enabled)
{
    for (auto &ssb : m_ssbs) {
        ssb->setEnabled(enabled);
    }
}
