#include "pch.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "Settings.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"

Settings::Settings(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_pDeviceResources(deviceResources),
    m_shaderMode(SETTINGS_PBR_SHADER_MODE::REGULAR),
    m_sceneMode(SETTINGS_SCENE_MODE::MODEL),
    m_lightsStrengths(),
    m_lightsThetaAngles(),
    m_lightsPhiAngles(),
    m_lightsDistances(),
    m_lightsColors(),
    m_lightsAttenuations(),
    m_metalRough(),
    m_depthBias(10),
    m_slopeScaledDepthBias(2 * static_cast<float>(sqrt(2))),
    m_useShadowPCF(true),
    m_useShadowPSSM(false),
    m_showPSSMSplits(false)
{
    for (UINT i = 0; i < NUM_LIGHTS; ++i)
    {
        m_lightsColors[i][0] = m_lightsColors[i][1] = m_lightsColors[i][2] = 1.0f;
        m_lightsAttenuations[i][0] = 1.0f;
        m_lightsAttenuations[i][1] = 0.01f;
    }

    m_lightsAttenuations[0][1] = 0.001f;
    m_lightsStrengths[0] = 500.0f;
    m_lightsThetaAngles[0] = 1.1f;
    m_lightsPhiAngles[0] = 2.1f;
    m_lightsDistances[0] = 200.0f;
};

void Settings::CreateResources(HWND hWnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetDeviceContext());
    ImGui::StyleColorsDark();
}

void Settings::Render()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(410, 90), ImGuiCond_Once);

    ImGui::Begin("Settings");

    static const char* shaderModes[] = { "Regular", "Normal distribution", "Geometry", "Fresnel" };
    ImGui::Combo("Shader mode", reinterpret_cast<int*>(&m_shaderMode), shaderModes, IM_ARRAYSIZE(shaderModes));

    static const char* sceneModes[] = { "Model", "Sphere" };
    ImGui::Combo("Scene content", reinterpret_cast<int*>(&m_sceneMode), sceneModes, IM_ARRAYSIZE(sceneModes));

    ImGui::End();

    for (UINT i = 0; i < NUM_LIGHTS; ++i)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 90 + 175 * static_cast<float>(i)), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(410, 175), ImGuiCond_Once);

        ImGui::Begin((std::string("Light ") + std::to_string(i)).c_str());

        ImGui::SliderFloat("Distance", m_lightsDistances + i, 0.001f, 1000.0f);

        ImGui::SliderFloat("Theta", m_lightsThetaAngles + i, 0.0f, static_cast<float>(M_PI));

        ImGui::SliderFloat("Phi", m_lightsPhiAngles + i, 0.0f, static_cast<float>(2 * M_PI));

        ImGui::ColorEdit3("Color", m_lightsColors[i]);

        ImGui::SliderFloat("Strength", m_lightsStrengths + i, 0.0f, 500.0f);

        ImGui::SliderFloat2("Attenuation", m_lightsAttenuations[i], 0.001f, 1.0f);

        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(0, 90 + 175 * NUM_LIGHTS), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(410, 150), ImGuiCond_Once);

    ImGui::Begin("Shadows");

    ImGui::SliderInt("Depth bias", &m_depthBias, 0, 32);

    ImGui::SliderFloat("Slope scaled depth bias", &m_slopeScaledDepthBias, 0.0f, 10.0f);

    ImGui::Checkbox("Use PCF", &m_useShadowPCF);

    ImGui::Checkbox("Use PSSM", &m_useShadowPSSM);

    if (m_useShadowPSSM)
        ImGui::Checkbox("Show splits", &m_showPSSMSplits);
    else
        m_showPSSMSplits = false;

    ImGui::End();

    if (m_sceneMode == SETTINGS_SCENE_MODE::SPHERE)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 240 + 175 * NUM_LIGHTS), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(410, 100), ImGuiCond_Once);

        ImGui::Begin("Material");

        ImGui::SliderFloat("Metalness", m_metalRough, 0.0f, 1.0f);

        ImGui::SliderFloat("Roughness", m_metalRough + 1, 0.0f, 1.0f);

        ImGui::ColorEdit3("Albedo", m_albedo);

        ImGui::End();
    }

    ImGui::Render();
    
    ID3D11RenderTargetView* renderTarget = m_pDeviceResources->GetRenderTarget();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();
    m_pDeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &renderTarget, depthStencil);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

DirectX::XMFLOAT4 Settings::GetLightColor(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    float color[4] = {};
    for (UINT i = 0; i < 3; ++i)
        color[i] = m_lightsColors[index][i];
    color[3] = m_lightsStrengths[index];
    return DirectX::XMFLOAT4(color);
}

DirectX::XMFLOAT4 Settings::GetLightPosition(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    float position[4] = {};
    position[0] = m_lightsDistances[index] * static_cast<float>(sin(m_lightsThetaAngles[index])) * static_cast<float>(cos(m_lightsPhiAngles[index]));
    position[2] = m_lightsDistances[index] * static_cast<float>(sin(m_lightsThetaAngles[index])) * static_cast<float>(sin(m_lightsPhiAngles[index]));
    position[1] = m_lightsDistances[index] * static_cast<float>(cos(m_lightsThetaAngles[index]));
    position[3] = 0.0f;
    return DirectX::XMFLOAT4(position);
}

DirectX::XMFLOAT4 Settings::GetLightAttenuation(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
    return DirectX::XMFLOAT4(m_lightsAttenuations[index][0], m_lightsAttenuations[index][1], 0.0f, 0.0f);
}

Settings::~Settings()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
