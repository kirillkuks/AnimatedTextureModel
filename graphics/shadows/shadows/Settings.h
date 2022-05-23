#pragma once

#include "DeviceResources.h"
#include "ShaderStructures.h"
#include "../../ImGui/imgui.h"

class Settings
{
public:
    enum class SETTINGS_PBR_SHADER_MODE
    {
        REGULAR = 0,
        NORMAL_DISTRIBUTION,
        GEOMETRY,
        FRESNEL
    };

    enum class SETTINGS_SCENE_MODE
    {
        MODEL = 0,
        SPHERE
    };

    Settings(const std::shared_ptr<DeviceResources>& deviceResources);
    ~Settings();

    void CreateResources(HWND hWnd);

    SETTINGS_PBR_SHADER_MODE GetShaderMode() const { return m_shaderMode; };
    SETTINGS_SCENE_MODE GetSceneMode() const { return m_sceneMode; };

    DirectX::XMFLOAT4 GetLightColor(UINT index) const;
    DirectX::XMFLOAT4 GetLightPosition(UINT index) const;
    DirectX::XMFLOAT4 GetLightAttenuation(UINT index) const;

    DirectX::XMFLOAT4 GetAlbedo() const { return DirectX::XMFLOAT4(m_albedo); };
    FLOAT GetMetalness() const { return m_metalRough[0]; };
    FLOAT GetRoughness() const { return m_metalRough[1]; };

    INT GetDepthBias() const { return m_depthBias; };
    FLOAT GetSlopeScaledDepthBias() const { return m_slopeScaledDepthBias; };
    bool GetShadowPCFUsing() const { return m_useShadowPCF; };
    bool GetShadowPSSMUsing() const { return m_useShadowPSSM; };
    bool GetPSSMSplitsShowing() const { return m_showPSSMSplits; };

    void Render();

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    SETTINGS_PBR_SHADER_MODE m_shaderMode;
    SETTINGS_SCENE_MODE      m_sceneMode;

    float m_lightsStrengths[NUM_LIGHTS];
    float m_lightsThetaAngles[NUM_LIGHTS];
    float m_lightsPhiAngles[NUM_LIGHTS];
    float m_lightsDistances[NUM_LIGHTS];
    float m_lightsColors[NUM_LIGHTS][3];
    float m_lightsAttenuations[NUM_LIGHTS][3];
    
    float m_albedo[4] = { 0.25f, 1.0f, 1.0f, 1.0f };
    float m_metalRough[2];

    int   m_depthBias;
    float m_slopeScaledDepthBias;
    bool  m_useShadowPCF;
    bool  m_useShadowPSSM;
    bool  m_showPSSMSplits;
};
