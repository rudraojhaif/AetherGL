#include "ControlPanel.h"
#include "TerrainViewer.h"
#include <QScrollArea>
#include <iostream>

ControlPanel::ControlPanel(TerrainViewer* viewer, QWidget *parent)
    : QWidget(parent)
    , m_viewer(viewer)
{
    setFixedWidth(300);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setupUI();
    updateFromViewer();
}

void ControlPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Create single scroll area for all controls
    auto* scrollArea = new QScrollArea(this);
    auto* scrollWidget = new QWidget();
    auto* scrollLayout = new QVBoxLayout(scrollWidget);
    
    setupPresetControls();
    setupLightingControls();
    setupTerrainControls();  // This now includes POM controls
    setupPostProcessingControls();
    
    // Add all group boxes to scroll layout
    for (int i = 0; i < layout()->count(); ++i) {
        auto item = layout()->takeAt(0);
        if (item && item->widget()) {
            scrollLayout->addWidget(item->widget());
        }
        delete item;
    }
    
    scrollLayout->addStretch();
    
    // Set size policies for proper scrolling
    scrollWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    scrollWidget->setMinimumHeight(600);  // Reasonable minimum height
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    mainLayout->addWidget(scrollArea);
}

void ControlPanel::setupPresetControls()
{
    auto* presetsGroup = new QGroupBox("Lighting Presets", this);
    auto* presetsLayout = new QVBoxLayout(presetsGroup);
    
    auto* defaultBtn = new QPushButton("Default Daytime", this);
    auto* sunsetBtn = new QPushButton("Sunset", this);
    auto* nightBtn = new QPushButton("Night Scene", this);
    
    connect(defaultBtn, &QPushButton::clicked, this, &ControlPanel::onDefaultLightingClicked);
    connect(sunsetBtn, &QPushButton::clicked, this, &ControlPanel::onSunsetLightingClicked);
    connect(nightBtn, &QPushButton::clicked, this, &ControlPanel::onNightLightingClicked);
    
    presetsLayout->addWidget(defaultBtn);
    presetsLayout->addWidget(sunsetBtn);
    presetsLayout->addWidget(nightBtn);
    
    layout()->addWidget(presetsGroup);
}

void ControlPanel::setupLightingControls()
{
    auto* lightingGroup = new QGroupBox("Lighting", this);
    auto* lightingLayout = new QVBoxLayout(lightingGroup);
    
    // Sun intensity
    lightingLayout->addWidget(createSlider("Sun Intensity", 0.0, 5.0, 3.0, 0.1, &m_sunIntensity));
    connect(m_sunIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onSunIntensityChanged);
    
    // Sun direction
    auto* sunDirGroup = new QGroupBox("Sun Direction", this);
    auto* sunDirLayout = new QVBoxLayout(sunDirGroup);
    sunDirLayout->addWidget(createSlider("X", -1.0, 1.0, 0.3, 0.1, &m_sunDirX));
    sunDirLayout->addWidget(createSlider("Y", -1.0, 1.0, -0.7, 0.1, &m_sunDirY));
    sunDirLayout->addWidget(createSlider("Z", -1.0, 1.0, -0.2, 0.1, &m_sunDirZ));
    lightingLayout->addWidget(sunDirGroup);
    
    connect(m_sunDirX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onSunDirectionChanged);
    connect(m_sunDirY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onSunDirectionChanged);
    connect(m_sunDirZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onSunDirectionChanged);
    
    // IBL intensity
    lightingLayout->addWidget(createSlider("IBL Intensity", 0.0, 2.0, 0.8, 0.1, &m_iblIntensity));
    connect(m_iblIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onIBLIntensityChanged);
    
    // Time of day
    lightingLayout->addWidget(createSlider("Time of Day", 0.0, 1.0, 0.3, 0.01, &m_timeOfDay));
    connect(m_timeOfDay, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onTimeOfDayChanged);
    
    // Animate time checkbox
    m_animateTime = new QCheckBox("Animate Time of Day", this);
    m_animateTime->setChecked(true);
    connect(m_animateTime, &QCheckBox::toggled, this, &ControlPanel::onAnimateTimeToggled);
    lightingLayout->addWidget(m_animateTime);
    
    // Fog density
    lightingLayout->addWidget(createSlider("Fog Density", 0.0, 0.1, 0.015, 0.001, &m_fogDensity));
    connect(m_fogDensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onFogDensityChanged);
    
    // Atmospheric perspective
    lightingLayout->addWidget(createSlider("Atmospheric Perspective", 0.0, 1.0, 0.7, 0.1, &m_atmosphericPerspective));
    connect(m_atmosphericPerspective, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onAtmosphericPerspectiveChanged);
    
    layout()->addWidget(lightingGroup);
}

void ControlPanel::setupTerrainControls()
{
    auto* terrainGroup = new QGroupBox("Terrain & POM Settings", this);
    auto* terrainLayout = new QVBoxLayout(terrainGroup);
    
    // Biome height thresholds
    terrainLayout->addWidget(createSlider("Grass Height", 0.0, 5.0, 1.0, 0.1, &m_grassHeight));
    connect(m_grassHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onGrassHeightChanged);
    
    terrainLayout->addWidget(createSlider("Rock Height", 0.0, 15.0, 6.0, 0.5, &m_rockHeight));
    connect(m_rockHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onRockHeightChanged);
    
    terrainLayout->addWidget(createSlider("Snow Height", 0.0, 20.0, 10.0, 0.5, &m_snowHeight));
    connect(m_snowHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onSnowHeightChanged);
    
    // POM (Parallax Occlusion Mapping) controls
    auto* pomGroup = new QGroupBox("Parallax Occlusion Mapping (POM)", this);
    auto* pomLayout = new QVBoxLayout(pomGroup);
    
    m_enablePOM = new QCheckBox("Enable POM", this);
    m_enablePOM->setChecked(true);
    connect(m_enablePOM, &QCheckBox::toggled, this, &ControlPanel::onPOMToggled);
    pomLayout->addWidget(m_enablePOM);
    
    pomLayout->addWidget(createSlider("POM Scale", 0.0, 0.5, 0.1, 0.01, &m_pomScale));
    connect(m_pomScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onPOMScaleChanged);
    
    // POM sample controls using spinboxes
    auto* minSamplesWidget = new QWidget(this);
    auto* minSamplesLayout = new QHBoxLayout(minSamplesWidget);
    minSamplesLayout->setContentsMargins(0, 0, 0, 0);
    auto* minSamplesLabel = new QLabel("Min Samples", this);
    minSamplesLabel->setMinimumWidth(100);
    m_pomMinSamples = new QSpinBox(this);
    m_pomMinSamples->setRange(8, 64);
    m_pomMinSamples->setValue(16);
    connect(m_pomMinSamples, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::onPOMMinSamplesChanged);
    minSamplesLayout->addWidget(minSamplesLabel);
    minSamplesLayout->addWidget(m_pomMinSamples);
    pomLayout->addWidget(minSamplesWidget);
    
    auto* maxSamplesWidget = new QWidget(this);
    auto* maxSamplesLayout = new QHBoxLayout(maxSamplesWidget);
    maxSamplesLayout->setContentsMargins(0, 0, 0, 0);
    auto* maxSamplesLabel = new QLabel("Max Samples", this);
    maxSamplesLabel->setMinimumWidth(100);
    m_pomMaxSamples = new QSpinBox(this);
    m_pomMaxSamples->setRange(16, 128);
    m_pomMaxSamples->setValue(64);
    connect(m_pomMaxSamples, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::onPOMMaxSamplesChanged);
    maxSamplesLayout->addWidget(maxSamplesLabel);
    maxSamplesLayout->addWidget(m_pomMaxSamples);
    pomLayout->addWidget(maxSamplesWidget);
    
    terrainLayout->addWidget(pomGroup);
    
    layout()->addWidget(terrainGroup);
}

void ControlPanel::setupPostProcessingControls()
{
    auto* postGroup = new QGroupBox("Post-Processing Effects", this);
    auto* postLayout = new QVBoxLayout(postGroup);
    
    // Bloom controls
    auto* bloomGroup = new QGroupBox("Bloom", this);
    auto* bloomLayout = new QVBoxLayout(bloomGroup);
    
    m_enableBloom = new QCheckBox("Enable Bloom", this);
    m_enableBloom->setChecked(true);
    connect(m_enableBloom, &QCheckBox::toggled, this, &ControlPanel::onBloomToggled);
    bloomLayout->addWidget(m_enableBloom);
    
    bloomLayout->addWidget(createSlider("Bloom Intensity", 0.0, 2.0, 0.6, 0.1, &m_bloomIntensity));
    connect(m_bloomIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onBloomIntensityChanged);
    
    bloomLayout->addWidget(createSlider("Bloom Threshold", 0.5, 3.0, 1.2, 0.1, &m_bloomThreshold));
    connect(m_bloomThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onBloomThresholdChanged);
    
    postLayout->addWidget(bloomGroup);
    
    // DOF controls
    auto* dofGroup = new QGroupBox("Depth of Field", this);
    auto* dofLayout = new QVBoxLayout(dofGroup);
    
    m_enableDOF = new QCheckBox("Enable DOF", this);
    m_enableDOF->setChecked(false);
    connect(m_enableDOF, &QCheckBox::toggled, this, &ControlPanel::onDOFToggled);
    dofLayout->addWidget(m_enableDOF);
    
    dofLayout->addWidget(createSlider("Focus Distance", 1.0, 50.0, 15.0, 1.0, &m_focusDistance));
    connect(m_focusDistance, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onFocusDistanceChanged);
    
    dofLayout->addWidget(createSlider("Focus Range", 1.0, 20.0, 8.0, 0.5, &m_focusRange));
    connect(m_focusRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onFocusRangeChanged);
    
    postLayout->addWidget(dofGroup);
    
    // Chromatic Aberration
    auto* caGroup = new QGroupBox("Chromatic Aberration", this);
    auto* caLayout = new QVBoxLayout(caGroup);
    
    m_enableChromaticAberration = new QCheckBox("Enable Chromatic Aberration", this);
    m_enableChromaticAberration->setChecked(true);
    connect(m_enableChromaticAberration, &QCheckBox::toggled, this, &ControlPanel::onChromaticAberrationToggled);
    caLayout->addWidget(m_enableChromaticAberration);
    
    caLayout->addWidget(createSlider("Aberration Strength", 0.0, 2.0, 0.3, 0.1, &m_aberrationStrength));
    connect(m_aberrationStrength, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onAberrationStrengthChanged);
    
    postLayout->addWidget(caGroup);
    
    // Tone mapping
    auto* toneGroup = new QGroupBox("Tone Mapping", this);
    auto* toneLayout = new QVBoxLayout(toneGroup);
    
    toneLayout->addWidget(createSlider("Exposure", 0.1, 3.0, 1.1, 0.1, &m_exposure));
    connect(m_exposure, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onExposureChanged);
    
    toneLayout->addWidget(createSlider("Gamma", 1.0, 3.0, 2.2, 0.1, &m_gamma));
    connect(m_gamma, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ControlPanel::onGammaChanged);
    
    postLayout->addWidget(toneGroup);
    
    layout()->addWidget(postGroup);
}

QWidget* ControlPanel::createSlider(const QString& label, double min, double max, double value, double step, QDoubleSpinBox** spinBox)
{
    auto* widget = new QWidget(this);
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    auto* labelWidget = new QLabel(label, this);
    labelWidget->setMinimumWidth(100);
    layout->addWidget(labelWidget);
    
    auto* spin = new QDoubleSpinBox(this);
    spin->setRange(min, max);
    spin->setValue(value);
    spin->setSingleStep(step);
    spin->setDecimals(3);
    layout->addWidget(spin);
    
    if (spinBox) {
        *spinBox = spin;
    }
    
    return widget;
}

void ControlPanel::updateFromViewer()
{
    if (!m_viewer) return;
    
    // Update controls from viewer's current state
    auto& lighting = m_viewer->getLightingConfig();
    auto postConfig = m_viewer->getPostProcessorConfig();
    
    // Disconnect signals temporarily to avoid loops
    blockSignals(true);
    
    m_sunIntensity->setValue(lighting.directionalLight.intensity);
    m_sunDirX->setValue(lighting.directionalLight.direction.x);
    m_sunDirY->setValue(lighting.directionalLight.direction.y);
    m_sunDirZ->setValue(lighting.directionalLight.direction.z);
    m_iblIntensity->setValue(lighting.ibl.intensity);
    m_timeOfDay->setValue(lighting.timeOfDay.timeOfDay);
    m_animateTime->setChecked(lighting.timeOfDay.animateTimeOfDay);
    m_fogDensity->setValue(lighting.atmosphere.fogDensity);
    m_atmosphericPerspective->setValue(lighting.atmosphere.atmosphericPerspective);
    
    m_grassHeight->setValue(lighting.terrain.grassHeight);
    m_rockHeight->setValue(lighting.terrain.rockHeight);
    m_snowHeight->setValue(lighting.terrain.snowHeight);
    
    m_enablePOM->setChecked(lighting.pom.enabled);
    m_pomScale->setValue(lighting.pom.scale);
    m_pomMinSamples->setValue(lighting.pom.minSamples);
    m_pomMaxSamples->setValue(lighting.pom.maxSamples);
    
    m_enableBloom->setChecked(postConfig.enableBloom);
    m_bloomIntensity->setValue(postConfig.bloomIntensity);
    m_bloomThreshold->setValue(postConfig.bloomThreshold);
    m_enableDOF->setChecked(postConfig.enableDOF);
    m_focusDistance->setValue(postConfig.focusDistance);
    m_focusRange->setValue(postConfig.focusRange);
    m_enableChromaticAberration->setChecked(postConfig.enableChromaticAberration);
    m_aberrationStrength->setValue(postConfig.aberrationStrength);
    m_exposure->setValue(postConfig.exposure);
    m_gamma->setValue(postConfig.gamma);
    
    blockSignals(false);
}

// Lighting control slots
void ControlPanel::onSunIntensityChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.directionalLight.intensity = static_cast<float>(value);
}

void ControlPanel::onSunDirectionChanged()
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.directionalLight.direction = glm::vec3(
        static_cast<float>(m_sunDirX->value()),
        static_cast<float>(m_sunDirY->value()),
        static_cast<float>(m_sunDirZ->value())
    );
}

void ControlPanel::onIBLIntensityChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.ibl.intensity = static_cast<float>(value);
}

void ControlPanel::onTimeOfDayChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.timeOfDay.timeOfDay = static_cast<float>(value);
}

void ControlPanel::onAnimateTimeToggled(bool enabled)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.timeOfDay.animateTimeOfDay = enabled;
}

void ControlPanel::onFogDensityChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.atmosphere.fogDensity = static_cast<float>(value);
}

void ControlPanel::onAtmosphericPerspectiveChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.atmosphere.atmosphericPerspective = static_cast<float>(value);
}

// Terrain control slots
void ControlPanel::onGrassHeightChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.terrain.grassHeight = static_cast<float>(value);
}

void ControlPanel::onRockHeightChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.terrain.rockHeight = static_cast<float>(value);
}

void ControlPanel::onSnowHeightChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.terrain.snowHeight = static_cast<float>(value);
}

// POM control slots
void ControlPanel::onPOMToggled(bool enabled)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.pom.enabled = enabled;
}

void ControlPanel::onPOMScaleChanged(double value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.pom.scale = static_cast<float>(value);
}

void ControlPanel::onPOMMinSamplesChanged(int value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.pom.minSamples = value;
}

void ControlPanel::onPOMMaxSamplesChanged(int value)
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.pom.maxSamples = value;
}

// Post-processing control slots - Industry standard implementation
void ControlPanel::onBloomToggled(bool enabled)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.enableBloom = enabled;
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onBloomIntensityChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.bloomIntensity = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onBloomThresholdChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.bloomThreshold = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onDOFToggled(bool enabled)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.enableDOF = enabled;
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onFocusDistanceChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.focusDistance = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onFocusRangeChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.focusRange = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onChromaticAberrationToggled(bool enabled)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.enableChromaticAberration = enabled;
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onAberrationStrengthChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.aberrationStrength = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onExposureChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.exposure = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

void ControlPanel::onGammaChanged(double value)
{
    auto config = m_viewer->getPostProcessorConfig();
    config.gamma = static_cast<float>(value);
    m_viewer->setPostProcessorConfig(config);
}

// Preset slots
void ControlPanel::onDefaultLightingClicked()
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.setupDefaultLights();
    updateFromViewer();
}

void ControlPanel::onSunsetLightingClicked()
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.setupSunsetLighting();
    updateFromViewer();
}

void ControlPanel::onNightLightingClicked()
{
    auto& lighting = m_viewer->getLightingConfig();
    lighting.setupNightLighting();
    updateFromViewer();
}

