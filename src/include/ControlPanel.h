#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

#include "LightingConfig.h"
#include "PostProcessor.h"

class TerrainViewer;

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(TerrainViewer* viewer, QWidget *parent = nullptr);

private slots:
    // Lighting controls
    void onSunIntensityChanged(double value);
    void onSunDirectionChanged();
    void onIBLIntensityChanged(double value);
    void onTimeOfDayChanged(double value);
    void onAnimateTimeToggled(bool enabled);
    void onFogDensityChanged(double value);
    void onAtmosphericPerspectiveChanged(double value);
    
    // Terrain biome controls
    void onGrassHeightChanged(double value);
    void onRockHeightChanged(double value);
    void onSnowHeightChanged(double value);
    
    // POM (Parallax Occlusion Mapping) controls
    void onPOMToggled(bool enabled);
    void onPOMScaleChanged(double value);
    void onPOMMinSamplesChanged(int value);
    void onPOMMaxSamplesChanged(int value);
    
    // Post-processing controls
    void onBloomToggled(bool enabled);
    void onBloomIntensityChanged(double value);
    void onBloomThresholdChanged(double value);
    void onDOFToggled(bool enabled);
    void onFocusDistanceChanged(double value);
    void onFocusRangeChanged(double value);
    void onChromaticAberrationToggled(bool enabled);
    void onAberrationStrengthChanged(double value);
    void onExposureChanged(double value);
    void onGammaChanged(double value);
    
    // Lighting presets
    void onDefaultLightingClicked();
    void onSunsetLightingClicked();
    void onNightLightingClicked();

private:
    void setupUI();
    void setupPresetControls();
    void setupLightingControls();
    void setupTerrainControls();
    void setupPostProcessingControls();
    void updateFromViewer();
    
    // Helper to create labeled slider
    QWidget* createSlider(const QString& label, double min, double max, double value, 
                         double step, QDoubleSpinBox** spinBox = nullptr);
    
    TerrainViewer* m_viewer;
    
    // Lighting controls
    QDoubleSpinBox* m_sunIntensity;
    QDoubleSpinBox* m_sunDirX;
    QDoubleSpinBox* m_sunDirY;
    QDoubleSpinBox* m_sunDirZ;
    QDoubleSpinBox* m_iblIntensity;
    QDoubleSpinBox* m_timeOfDay;
    QCheckBox* m_animateTime;
    QDoubleSpinBox* m_fogDensity;
    QDoubleSpinBox* m_atmosphericPerspective;
    
    // Terrain controls
    QDoubleSpinBox* m_grassHeight;
    QDoubleSpinBox* m_rockHeight;
    QDoubleSpinBox* m_snowHeight;
    
    // POM controls
    QCheckBox* m_enablePOM;
    QDoubleSpinBox* m_pomScale;
    QSpinBox* m_pomMinSamples;
    QSpinBox* m_pomMaxSamples;
    
    // Post-processing controls
    QCheckBox* m_enableBloom;
    QDoubleSpinBox* m_bloomIntensity;
    QDoubleSpinBox* m_bloomThreshold;
    QCheckBox* m_enableDOF;
    QDoubleSpinBox* m_focusDistance;
    QDoubleSpinBox* m_focusRange;
    QCheckBox* m_enableChromaticAberration;
    QDoubleSpinBox* m_aberrationStrength;
    QDoubleSpinBox* m_exposure;
    QDoubleSpinBox* m_gamma;
};
