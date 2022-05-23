#pragma once

#include "DeviceResources.h"
#include "Renderer.h"
#include "Camera.h"
#include "Settings.h"

class App
{
public:
    App();
    ~App();

    HRESULT CreateDesktopWindow(HINSTANCE hInstance, int nCmdShow, WNDPROC pWndProc);
    HRESULT CreateDeviceResources();

    HWND GetWindowHandle() const { return m_hWnd; };

    void    Run();
    HRESULT OnResize();
    LRESULT KeyHandler(WPARAM wParam, LPARAM lParam);
    LRESULT MouseHandler(UINT uMsg, WPARAM wParam);

private:
    HRESULT Render();

    HWND m_hWnd;
    POINT m_cursor;

    std::shared_ptr<DeviceResources> m_pDeviceResources;
    std::shared_ptr<Renderer>        m_pRenderer;
    std::shared_ptr<Camera>          m_pCamera;
    std::shared_ptr<Settings>        m_pSettings;
};
