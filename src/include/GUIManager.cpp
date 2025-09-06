#include "GUIManager.h"
#include "TerrainRenderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

GUIManager::GUIManager() {
}

GUIManager::~GUIManager() {
    shutdown();
}

bool GUIManager::initialize(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Customize colors for a more professional look
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.30f, 0.35f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.40f, 0.80f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.20f, 0.25f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.30f, 0.80f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.30f, 0.35f, 0.80f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.55f, 0.65f, 1.00f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_initialized = true;
    std::cout << "ImGui initialized successfully!" << std::endl;
    return true;
}

void GUIManager::shutdown() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void GUIManager::beginFrame() {
    if (!m_initialized) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUIManager::endFrame() {
    if (!m_initialized) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUIManager::render(TerrainRenderer* renderer) {
    if (!m_initialized || !renderer) return;

    // Main control panel
    if (m_showSettings) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 700), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AetherGL Terrain Controls", &m_showSettings)) {
            if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Separator();
                
                if (ImGui::Button("Toggle Demo Window")) {
                    m_showDemo = !m_showDemo;
                }
                ImGui::SameLine(); helpMarker("Shows the ImGui demo window with examples");
            }
            
            renderMainSettings(renderer);
            renderLightingSettings(renderer);
            renderPostProcessingSettings(renderer);
            renderTerrainSettings(renderer);
        }
        ImGui::End();
    }

    // ImGui Demo Window
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

void GUIManager::renderMainSettings(TerrainRenderer* renderer) {
    if (ImGui::CollapsingHeader("Camera & Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Current Camera Position:");
        auto pos = renderer->getCameraPosition();
        ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", pos.x, pos.y, pos.z);
        
        ImGui::Separator();
        ImGui::TextWrapped("Controls:");
        ImGui::BulletText("WASD - Move camera");
        ImGui::BulletText("Left Click + Drag - Look around");
        ImGui::BulletText("Mouse Wheel - Zoom");
        ImGui::BulletText("1/2/3 - Toggle post-processing effects");
        ImGui::BulletText("ESC - Exit application");
    }
}

void GUIManager::renderLightingSettings(TerrainRenderer* renderer) {
    if (ImGui::CollapsingHeader("Lighting System")) {
        
        // Sun/Directional Light
        if (ImGui::TreeNode("Directional Light (Sun)")) {
            auto& lighting = renderer->getLightingConfig();
            
            bool enabled = lighting.directionalLight.enabled;
            if (ImGui::Checkbox("Enable Directional Light", &enabled)) {
                lighting.directionalLight.enabled = enabled;
            }
            
            float intensity = lighting.directionalLight.intensity;
            if (ImGui::SliderFloat("Sun Intensity", &intensity, 0.0f, 10.0f, "%.2f")) {
                lighting.directionalLight.intensity = intensity;
            }
            
            float dir[3] = {lighting.directionalLight.direction.x, lighting.directionalLight.direction.y, lighting.directionalLight.direction.z};
            if (ImGui::SliderFloat3("Sun Direction", dir, -1.0f, 1.0f, "%.2f")) {
                lighting.directionalLight.direction = glm::vec3(dir[0], dir[1], dir[2]);
            }
            
            float color[3] = {lighting.directionalLight.color.r, lighting.directionalLight.color.g, lighting.directionalLight.color.b};
            if (ImGui::ColorEdit3("Sun Color", color)) {
                lighting.directionalLight.color = glm::vec3(color[0], color[1], color[2]);
            }
            
            ImGui::TreePop();
        }
        
        // Point Lights
        if (ImGui::TreeNode("Point Lights")) {
            auto& lighting = renderer->getLightingConfig();
            
            int numLights = static_cast<int>(lighting.pointLights.size());
            if (ImGui::SliderInt("Number of Point Lights", &numLights, 0, 8)) {
                lighting.pointLights.resize(numLights);
            }
            
            for (int i = 0; i < numLights; i++) {
                if (ImGui::TreeNode(("Point Light " + std::to_string(i + 1)).c_str())) {
                    auto& light = lighting.pointLights[i];
                    
                    float pos[3] = {light.position.x, light.position.y, light.position.z};
                    if (ImGui::SliderFloat3("Position", pos, -50.0f, 50.0f, "%.1f")) {
                        light.position = glm::vec3(pos[0], pos[1], pos[2]);
                    }
                    
                    float color[3] = {light.color.r, light.color.g, light.color.b};
                    if (ImGui::ColorEdit3("Color", color)) {
                        light.color = glm::vec3(color[0], color[1], color[2]);
                    }
                    
                    ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 100.0f, "%.1f");
                    ImGui::SliderFloat("Range", &light.range, 1.0f, 100.0f, "%.1f");
                    
                    ImGui::TreePop();
                }
            }
            
            ImGui::TreePop();
        }
        
        // IBL Settings
        if (ImGui::TreeNode("Image-Based Lighting (IBL)")) {
            auto& lighting = renderer->getLightingConfig();
            
            bool enabled = lighting.ibl.enabled;
            if (ImGui::Checkbox("Enable IBL", &enabled)) {
                lighting.ibl.enabled = enabled;
            }
            
            ImGui::SliderFloat("IBL Intensity", &lighting.ibl.intensity, 0.0f, 2.0f, "%.2f");
            
            ImGui::TreePop();
        }
        
        // Time of Day
        if (ImGui::TreeNode("Time of Day")) {
            auto& lighting = renderer->getLightingConfig();
            
            bool animate = lighting.timeOfDay.animateTimeOfDay;
            if (ImGui::Checkbox("Animate Time of Day", &animate)) {
                lighting.timeOfDay.animateTimeOfDay = animate;
            }
            
            ImGui::SliderFloat("Time of Day", &lighting.timeOfDay.timeOfDay, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Animation Speed", &lighting.timeOfDay.daySpeed, 0.0f, 0.2f, "%.3f");
            
            ImGui::TreePop();
        }
        
        // Atmospheric Fog
        if (ImGui::TreeNode("Atmospheric Effects")) {
            auto& lighting = renderer->getLightingConfig();
            
            bool fogEnabled = lighting.atmosphere.enableAtmosphericFog;
            if (ImGui::Checkbox("Enable Atmospheric Fog", &fogEnabled)) {
                lighting.atmosphere.enableAtmosphericFog = fogEnabled;
            }
            
            ImGui::SliderFloat("Fog Density", &lighting.atmosphere.fogDensity, 0.0f, 0.1f, "%.4f");
            ImGui::SliderFloat("Height Falloff", &lighting.atmosphere.fogHeightFalloff, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Atmospheric Perspective", &lighting.atmosphere.atmosphericPerspective, 0.0f, 1.0f, "%.2f");
            
            float fogColor[3] = {lighting.atmosphere.fogColor.r, lighting.atmosphere.fogColor.g, lighting.atmosphere.fogColor.b};
            if (ImGui::ColorEdit3("Fog Color", fogColor)) {
                lighting.atmosphere.fogColor = glm::vec3(fogColor[0], fogColor[1], fogColor[2]);
            }
            
            ImGui::TreePop();
        }
    }
}

void GUIManager::renderPostProcessingSettings(TerrainRenderer* renderer) {
    if (ImGui::CollapsingHeader("Post-Processing Effects")) {
        
        bool ppEnabled = renderer->isPostProcessingEnabled();
        if (ImGui::Checkbox("Enable Post-Processing", &ppEnabled)) {
            renderer->setPostProcessingEnabled(ppEnabled);
        }
        
        if (!ppEnabled) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "Post-processing is disabled");
            return;
        }
        
        auto config = renderer->getPostProcessorConfig();
        
        // Bloom
        if (ImGui::TreeNode("Bloom")) {
            bool changed = false;
            
            changed |= ImGui::Checkbox("Enable Bloom", &config.enableBloom);
            changed |= ImGui::SliderFloat("Bloom Threshold", &config.bloomThreshold, 0.0f, 3.0f, "%.2f");
            changed |= ImGui::SliderFloat("Bloom Intensity", &config.bloomIntensity, 0.0f, 3.0f, "%.2f");
            changed |= ImGui::SliderInt("Blur Passes", &config.bloomBlurPasses, 1, 10);
            
            if (changed) {
                renderer->setPostProcessorConfig(config);
            }
            
            ImGui::TreePop();
        }
        
        // Depth of Field
        if (ImGui::TreeNode("Depth of Field")) {
            bool changed = false;
            
            changed |= ImGui::Checkbox("Enable DOF", &config.enableDOF);
            changed |= ImGui::SliderFloat("Focus Distance", &config.focusDistance, 1.0f, 100.0f, "%.1f");
            changed |= ImGui::SliderFloat("Focus Range", &config.focusRange, 1.0f, 50.0f, "%.1f");
            changed |= ImGui::SliderFloat("Bokeh Radius", &config.bokehRadius, 1.0f, 10.0f, "%.1f");
            
            if (changed) {
                renderer->setPostProcessorConfig(config);
            }
            
            ImGui::TreePop();
        }
        
        // Chromatic Aberration
        if (ImGui::TreeNode("Chromatic Aberration")) {
            bool changed = false;
            
            changed |= ImGui::Checkbox("Enable Chromatic Aberration", &config.enableChromaticAberration);
            changed |= ImGui::SliderFloat("Aberration Strength", &config.aberrationStrength, 0.0f, 2.0f, "%.2f");
            
            if (changed) {
                renderer->setPostProcessorConfig(config);
            }
            
            ImGui::TreePop();
        }
        
        // Tone Mapping
        if (ImGui::TreeNode("Tone Mapping")) {
            bool changed = false;
            
            changed |= ImGui::SliderFloat("Exposure", &config.exposure, 0.1f, 5.0f, "%.2f");
            changed |= ImGui::SliderFloat("Gamma", &config.gamma, 1.0f, 3.0f, "%.2f");
            
            if (changed) {
                renderer->setPostProcessorConfig(config);
            }
            
            ImGui::TreePop();
        }
    }
}

void GUIManager::renderTerrainSettings(TerrainRenderer* renderer) {
    if (ImGui::CollapsingHeader("Terrain & POM Settings")) {
        auto& lighting = renderer->getLightingConfig();
        
        // Biome Heights
        if (ImGui::TreeNode("Biome Heights")) {
            ImGui::SliderFloat("Grass Height", &lighting.terrain.grassHeight, 0.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("Rock Height", &lighting.terrain.rockHeight, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Snow Height", &lighting.terrain.snowHeight, 0.0f, 25.0f, "%.1f");
            
            ImGui::TreePop();
        }
        
        // Parallax Occlusion Mapping
        if (ImGui::TreeNode("Parallax Occlusion Mapping (POM)")) {
            bool enabled = lighting.pom.enabled;
            if (ImGui::Checkbox("Enable POM", &enabled)) {
                lighting.pom.enabled = enabled;
            }
            
            ImGui::SliderFloat("POM Scale", &lighting.pom.scale, 0.0f, 0.5f, "%.3f");
            ImGui::SliderInt("Min Samples", &lighting.pom.minSamples, 8, 32);
            ImGui::SliderInt("Max Samples", &lighting.pom.maxSamples, 32, 128);
            
            ImGui::TreePop();
        }
        
        // Terrain Info
        if (ImGui::TreeNode("Terrain Information")) {
            ImGui::Text("Vertices: 10,201");
            ImGui::Text("Size: 80x80 units");
            ImGui::Text("Height Scale: 15 units");
            ImGui::Text("Noise Scale: 0.08");
            ImGui::Text("Materials: Procedural (no textures)");
            
            ImGui::TreePop();
        }
    }
}

void GUIManager::helpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
