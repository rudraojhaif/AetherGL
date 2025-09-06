#pragma once

#include <QOpenGLWidget>
// Don't include QOpenGLFunctions to avoid header conflicts
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "LightingConfig.h"
#include "HDRLoader.h"
#include "PostProcessor.h"

class TerrainViewer : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit TerrainViewer(QWidget *parent = nullptr);
    ~TerrainViewer();

    // Public interface for control panel
    void setLightingConfig(const LightingConfig& config) { m_lighting = config; }
    void setPostProcessorConfig(const PostProcessor::Config& config);
    LightingConfig& getLightingConfig() { return m_lighting; }
    PostProcessor::Config getPostProcessorConfig() const;

protected:
    // QOpenGLWidget interface
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    // Input handling
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void updateScene();

private:
    void setupTerrain();
    void setupShaders();
    void setupLighting();
    void renderScene();

    // Core rendering components
    std::unique_ptr<Camera> m_camera;
    std::shared_ptr<Shader> m_terrainShader;
    std::shared_ptr<Shader> m_skyboxShader;
    std::shared_ptr<Mesh> m_terrainMesh;
    std::unique_ptr<PostProcessor> m_postProcessor;

    // Lighting and environment
    LightingConfig m_lighting;
    HDRLoader::IBLTextures m_iblTextures;

    // Update timer
    QTimer* m_updateTimer;
    float m_deltaTime = 0.0f;
    qint64 m_lastFrameTime = 0;

    // Mouse interaction
    QPoint m_lastMousePos;
    bool m_mousePressed = false;
    bool m_mouseCaptured = false;  // Track mouse capture state
    bool m_isFullscreen = false;   // Track fullscreen state

    // Viewport settings
    int m_viewportWidth = 800;
    int m_viewportHeight = 600;
};
