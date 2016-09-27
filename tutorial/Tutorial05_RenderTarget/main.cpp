/*
 * main.cpp (Tutorial05_RenderTarget)
 *
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "../tutorial.h"


class Tutorial05 : public Tutorial
{

    LLGL::ShaderProgram*    shaderProgram   = nullptr;

    LLGL::GraphicsPipeline* pipeline        = nullptr;

    LLGL::VertexBuffer*     vertexBuffer    = nullptr;
    LLGL::IndexBuffer*      indexBuffer     = nullptr;
    LLGL::ConstantBuffer*   constantBuffer  = nullptr;

    LLGL::Texture*          colorMap        = nullptr;
    LLGL::Sampler*          samplerState    = nullptr;

    LLGL::RenderTarget*     renderTarget    = nullptr;
    LLGL::Texture*          renderTargetTex = nullptr;

    Gs::Matrix4f            renderTargetProj;

    const Gs::Vector2i      renderTargetSize { 512, 512 };

    struct Settings
    {
        Gs::Matrix4f        wvpMatrix;
    }
    settings;

public:

    Tutorial05() :
        Tutorial( "Direct3D11", L"LLGL Tutorial 05: RenderTarget")
    {
        // Create all graphics objects
        auto vertexFormat = CreateBuffers();
        shaderProgram = LoadStandardShaderProgram(vertexFormat);
        CreatePipelines();
        CreateColorMap();
        CreateRenderTarget();
    }

    LLGL::VertexFormat CreateBuffers()
    {
        // Specify vertex format
        LLGL::VertexFormat vertexFormat;
        vertexFormat.AddAttribute("position", LLGL::DataType::Float, 3);

        // Create vertex, index, and constant buffer
        vertexBuffer = CreateVertexBuffer(GenerateCubeVertices(), vertexFormat);
        indexBuffer = CreateIndexBuffer(GenerateCubeTriangelIndices(), LLGL::DataType::UInt32);
        constantBuffer = CreateConstantBuffer(settings);

        return vertexFormat;
    }

    void CreatePipelines()
    {
        // Create common graphics pipeline for scene rendering
        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        {
            pipelineDesc.shaderProgram                  = shaderProgram;

            pipelineDesc.depth.testEnabled              = true;
            pipelineDesc.depth.writeEnabled             = true;

            pipelineDesc.rasterizer.multiSampleEnabled  = true;
            pipelineDesc.rasterizer.samples             = 8;
        }
        pipeline = renderer->CreateGraphicsPipeline(pipelineDesc);
    }

    void CreateColorMap()
    {
        // Load color map texture from file
        colorMap = LoadTexture("colorMap.jpg");

        // Create common sampler state with default descriptor
        samplerState = renderer->CreateSampler({});
    }

    void CreateRenderTarget()
    {
        // Create render-target with multi-sampling
        renderTarget = renderer->CreateRenderTarget(8);

        // Create empty render-target texture
        renderTargetTex = renderer->CreateTexture();
        renderer->SetupTexture2D(*renderTargetTex, LLGL::TextureFormat::RGBA, renderTargetSize);

        // Generate all MIP-map levels
        context->GenerateMips(*renderTargetTex);

        // Attach depth buffer to render-target
        renderTarget->AttachDepthBuffer(renderTargetSize);

        // Attach texture (first MIP-map level) to render-target
        renderTarget->AttachTexture2D(*renderTargetTex);

        // Initialize projection matrix for render-target scene rendering
        renderTargetProj = Gs::ProjectionMatrix4f::Perspective(1.0f, 0.1f, 100.0f, Gs::Deg2Rad(45.0f)).ToMatrix4();
    }

private:

    void UpdateModelTransform(const Gs::Matrix4f& proj, float rotation)
    {
        settings.wvpMatrix = proj;
        Gs::Translate(settings.wvpMatrix, { 0, 0, 5 });
        Gs::RotateFree(settings.wvpMatrix, { 0, 1, 0 }, rotation);

        UpdateConstantBuffer(constantBuffer, settings);
    }

    void OnDrawFrame() override
    {
        static const auto shaderStages = LLGL::ShaderStageFlags::VertexStage | LLGL::ShaderStageFlags::FragmentStage;

        // Update scene animation (simple rotation)
        static float anim;
        anim += 0.01f;

        // Set common buffers and sampler states
        context->SetVertexBuffer(*vertexBuffer);
        context->SetIndexBuffer(*indexBuffer);
        context->SetConstantBuffer(*constantBuffer, 0, shaderStages);
        context->SetSampler(*samplerState, 0, shaderStages);

        // Set graphics pipeline state
        context->SetGraphicsPipeline(*pipeline);

        // Set graphics API dependent state.
        // This is important for OpenGL to be uniform with Direct3D.
        // The viewport and scissor origin will be flipped vertically.
        LLGL::GraphicsAPIDependentStateDescriptor apiState;
        
        apiState.stateOpenGL.flipViewportVertical = true;
        context->SetGraphicsAPIDependentState(apiState);

        // Draw scene into render-target
        context->SetRenderTarget(*renderTarget);
        {
            // Set viewport for render target
            context->SetViewports({ { 0, 0, static_cast<float>(renderTargetSize.x), static_cast<float>(renderTargetSize.y) } });

            // Clear color and depth buffers of active framebuffer (i.e. the render target)
            context->SetClearColor({ 0, 1, 0 });
            context->ClearBuffers(LLGL::ClearBuffersFlags::Color | LLGL::ClearBuffersFlags::Depth);

            // Set color map texture
            context->SetTexture(*colorMap, 0, shaderStages);

            // Update model transformation with render-target projection
            UpdateModelTransform(renderTargetProj, -anim*2);

            // Draw scene
            context->DrawIndexed(36, 0);
        }
        context->UnsetRenderTarget();

        // Generate MIP-maps again after texture has been written by the render-target
        context->GenerateMips(*renderTargetTex);

        // Reset renderer specific state
        apiState.stateOpenGL.flipViewportVertical = false;
        context->SetGraphicsAPIDependentState(apiState);

        // Reset viewport for the screen
        auto resolution = context->GetVideoMode().resolution.Cast<float>();
        context->SetViewports({ { 0, 0, resolution.x, resolution.y } });

        // Clear color and depth buffers of active framebuffer (i.e. the screen)
        context->SetClearColor(defaultClearColor);
        context->ClearBuffers(LLGL::ClearBuffersFlags::Color | LLGL::ClearBuffersFlags::Depth);

        // Set render-target texture
        context->SetTexture(*renderTargetTex, 0, shaderStages);

        // Update model transformation with standard projection
        UpdateModelTransform(projection, anim);

        // Draw scene
        context->DrawIndexed(36, 0);

        // Present result on the screen
        context->Present();
    }

};

LLGL_IMPLEMENT_TUTORIAL(Tutorial05);



