#pragma once

#include <vector>

HRESULT CreateVertexShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11VertexShader** vertexShader);

HRESULT CreatePixelShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11PixelShader** pixelShader);

HRESULT CreateComputeShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11ComputeShader** computeShader);

HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines=nullptr);
