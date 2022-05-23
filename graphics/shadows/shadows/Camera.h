#pragma once

class Camera 
{
public:
    Camera();
    ~Camera();

    DirectX::XMVECTOR GetPosition() const { return m_eye; };
    DirectX::XMVECTOR GetDirection() const { return m_viewDir; };
    DirectX::XMVECTOR GetUp() const { return m_up; };
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMVECTOR GetPerpendicular() const;

    void MoveVertical(float delta);
    void MoveDirection(float delta);
    void MovePerpendicular(float delta);
    void Rotate(float horisontalAngle, float verticalAngle);
    void Zoom(float delta);

private:
    void SetupCamera();
    DirectX::FXMVECTOR CalcProjectedDir() const;

private:
    DirectX::XMVECTOR m_eye;
    DirectX::XMVECTOR m_viewDir;
    DirectX::XMVECTOR m_up;

    float m_vertAngle;
    float m_horzAngle;
    float m_dist;
};
