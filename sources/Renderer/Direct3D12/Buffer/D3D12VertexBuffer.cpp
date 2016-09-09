/*
 * D3D12VertexBuffer.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "D3D12VertexBuffer.h"


namespace LLGL
{


void D3D12VertexBuffer::UpdateSubResource(
    ID3D12Device* device, ID3D12GraphicsCommandList* gfxCommandList, ComPtr<ID3D12Resource>& bufferUpload,
    const void* data, UINT bufferSize, UINT64 offset)
{
    hwBuffer.UpdateSubResource(
        device, gfxCommandList, bufferUpload,
        data, bufferSize, offset,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
}


} // /namespace LLGL



// ================================================================================
