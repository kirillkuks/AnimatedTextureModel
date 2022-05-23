#include "pch.h"

#include <fstream>

#include "Utils.h"

HRESULT ReadCompiledShader(const WCHAR* szFileName, std::vector<BYTE>& bytes)
{
    std::ifstream csoFile(szFileName, std::ios::in | std::ios::binary | std::ios::ate);

    if (csoFile.is_open())
    {
        size_t bufferSize = static_cast<size_t>(csoFile.tellg());
        bytes.clear();
        bytes.resize(bufferSize);
        csoFile.seekg(0, std::ios::beg);
        csoFile.read(reinterpret_cast<char*>(bytes.data()), bufferSize);
        csoFile.close();

        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT CreateVertexShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11VertexShader** vertexShader)
{
    HRESULT hr = S_OK;

    // Read the vertex shader
    hr = ReadCompiledShader(szFileName, bytes);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = device->CreateVertexShader(bytes.data(), bytes.size(), nullptr, vertexShader);
    
    return hr;
}

HRESULT CreatePixelShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11PixelShader** pixelShader)
{
    HRESULT hr = S_OK;

    // Read the pixel shader
    hr = ReadCompiledShader(szFileName, bytes);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = device->CreatePixelShader(bytes.data(), bytes.size(), nullptr, pixelShader);

    return hr;
}

HRESULT CreateComputeShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11ComputeShader** computeShader)
{
    HRESULT hr = S_OK;

    // Read the compute shader
    hr = ReadCompiledShader(szFileName, bytes);
    if (FAILED(hr))
        return hr;

    // Create the compute shader
    hr = device->CreateComputeShader(bytes.data(), bytes.size(), nullptr, computeShader);

    return hr;
}

HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> err;
    hr = D3DCompileFromFile(szFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &err);
    if (FAILED(hr) && err)
        OutputDebugStringA(reinterpret_cast<const char*>(err->GetBufferPointer()));

    return hr;
}
