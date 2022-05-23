#include "pch.h"

#include "Camera.h"

Camera::Camera() :
    m_eye(DirectX::XMVectorSet(0.0f, 9.0f, 0.0f, 1.0f)),
    m_viewDir(DirectX::XMVector3Normalize(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f))),
    m_up(DirectX::XMVector3Normalize(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))),
    m_vertAngle(0.0f),
    m_horzAngle(0.0f),
	m_dist(30.0f)
{
	SetupCamera();
};

void Camera::MoveVertical(float delta)
{
	m_eye = DirectX::XMVectorAdd(m_eye, DirectX::XMVectorSet(0, delta, 0, 1));
}

void Camera::MoveDirection(float delta)
{
    m_eye = DirectX::XMVectorAdd(m_eye, DirectX::XMVectorScale(CalcProjectedDir(), delta));
}

void Camera::MovePerpendicular(float delta)
{
	DirectX::FXMVECTOR right = DirectX::XMVector3Rotate(CalcProjectedDir(), DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0,1,0,1), -DirectX::XM_PIDIV2));

    m_eye = DirectX::XMVectorAdd(m_eye, DirectX::XMVectorScale(right, delta));
}

void Camera::Rotate(float horisontalAngle, float verticalAngle)
{
    m_vertAngle += verticalAngle;
	m_horzAngle -= horisontalAngle;

	SetupCamera();
}

void Camera::Zoom(float delta)
{
	m_dist -= delta;

	SetupCamera();
}

void Camera::SetupCamera()
{
	if (m_dist < 1.0f)
	{
		m_dist = 1.0f;
	}
	if (m_dist > 100.0f)
	{
		m_dist = 100.0f;
	}

	if (m_vertAngle > DirectX::XM_PIDIV2)
	{
		m_vertAngle = DirectX::XM_PIDIV2;
	}
	if (m_vertAngle < -DirectX::XM_PIDIV2)
	{
		m_vertAngle = -DirectX::XM_PIDIV2;
	}

	if (m_horzAngle >= DirectX::XM_2PI)
	{
		m_horzAngle -= DirectX::XM_2PI;
	}
	if (m_horzAngle < 0)
	{
		m_horzAngle += DirectX::XM_2PI;
	}

	static const DirectX::FXMVECTOR initialRight = DirectX::XMVectorSet(1, 0, 0, 1);
	static const DirectX::FXMVECTOR initialUp = DirectX::XMVectorSet(0, 1, 0, 1);
	static const DirectX::FXMVECTOR initialDir = DirectX::XMVectorSet(0, 0, 1, 1);

	m_viewDir = DirectX::XMVector3Rotate(initialDir, DirectX::XMQuaternionRotationAxis(initialRight, m_vertAngle));
	m_viewDir = DirectX::XMVector3Rotate(m_viewDir, DirectX::XMQuaternionRotationAxis(initialUp, m_horzAngle));

	m_up = DirectX::XMVector3Rotate(initialUp, DirectX::XMQuaternionRotationAxis(initialRight, m_vertAngle));
	m_up = DirectX::XMVector3Rotate(m_up, DirectX::XMQuaternionRotationAxis(initialUp, m_horzAngle));
}

DirectX::FXMVECTOR Camera::CalcProjectedDir() const
{
	DirectX::FXMVECTOR dir = DirectX::XMVector3Normalize(DirectX::XMVectorSet(
		DirectX::XMVectorGetX(m_viewDir),
		0.0f,
		DirectX::XMVectorGetZ(m_viewDir),
		1.0f
	)); // "Projection"

	return dir;
}

DirectX::XMVECTOR Camera::GetPerpendicular() const
{
    return DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_viewDir, m_up));
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    DirectX::FXMVECTOR poi = m_eye;
    DirectX::FXMVECTOR eye = DirectX::XMVectorSubtract(m_eye, DirectX::XMVectorScale(m_viewDir, m_dist));

    return DirectX::XMMatrixLookAtRH(eye, poi, m_up);
}

Camera::~Camera()
{}
