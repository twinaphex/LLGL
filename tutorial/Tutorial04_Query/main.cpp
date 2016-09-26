/*
 * main.cpp (Tutorial04_Query)
 *
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "../tutorial.h"


class Tutorial04 : public Tutorial
{

    LLGL::ShaderProgram*    shaderProgram       = nullptr;

    LLGL::GraphicsPipeline* occlusionPipeline   = nullptr;
    LLGL::GraphicsPipeline* scenePipeline       = nullptr;

    LLGL::VertexBuffer*     vertexBuffer        = nullptr;
    LLGL::IndexBuffer*      indexBuffer         = nullptr;
    LLGL::ConstantBuffer*   constantBuffer      = nullptr;

    LLGL::Query*            occlusionQuery      = nullptr;
    LLGL::Query*            geometryQuery       = nullptr;

    struct Settings
    {
        Gs::Matrix4f        wvpMatrix;
        LLGL::ColorRGBAf    color;
    }
    settings;

public:

    Tutorial04() :
        Tutorial( "OpenGL", L"LLGL Tutorial 04: Query")
    {
        // Create all graphics objects
        auto vertexFormat = CreateBuffers();
        shaderProgram = LoadStandardShaderProgram(vertexFormat);
        CreatePipelines();
        CreateQueries();
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
        // Create graphics pipeline for occlusion query
        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        {
            pipelineDesc.shaderProgram                  = shaderProgram;

            pipelineDesc.depth.testEnabled              = true;

            pipelineDesc.rasterizer.multiSampleEnabled  = true;
            pipelineDesc.rasterizer.samples             = 8;

            LLGL::BlendTargetDescriptor blendDesc;
            {
                blendDesc.colorMask = LLGL::ColorRGBAb(false);
            }
            pipelineDesc.blend.targets.push_back(blendDesc);
        }
        occlusionPipeline = renderer->CreateGraphicsPipeline(pipelineDesc);

        // Create graphics pipeline for scene rendering
        {
            pipelineDesc.depth.testEnabled          = true;
            pipelineDesc.depth.writeEnabled         = true;
            pipelineDesc.blend.targets[0].colorMask = LLGL::ColorRGBAb(true);
        }
        scenePipeline = renderer->CreateGraphicsPipeline(pipelineDesc);
    }

    void CreateQueries()
    {
        // Create query to determine if any samples passed the depth test (occlusion query)
        LLGL::QueryDescriptor queryDesc;
        {
            queryDesc.type              = LLGL::QueryType::AnySamplesPassed;
            queryDesc.renderCondition   = true;
        }
        occlusionQuery = renderer->CreateQuery(queryDesc);

        // Create query to determine number of primitives that are sent to the rasterizer
        {
            queryDesc.type              = LLGL::QueryType::PrimitivesGenerated;
            queryDesc.renderCondition   = false;
        }
        geometryQuery = renderer->CreateQuery(queryDesc);
    }

    std::uint64_t GetAndSyncQueryResult(LLGL::Query* query)
    {
        // Wait until query result is available and return result
        std::uint64_t result = 0;
        while (!context->QueryResult(*query, result)) { /* wait */ }
        return result;
    }

    void PrintQueryResult()
    {
        std::cout
            << "occluded: " << (GetAndSyncQueryResult(occlusionQuery) != 0 ? "yes" : "no")
            << ", primitives generated: " << GetAndSyncQueryResult(geometryQuery)
            << "         \r";
        std::flush(std::cout);
    }

    void SetBoxColor(const LLGL::ColorRGBAf& color)
    {
        settings.color = color;
        UpdateConstantBuffer(constantBuffer, settings);
    }

private:

    void OnDrawFrame() override
    {
        // Update matrices in constant buffer
        static float anim;
        anim += 0.01f;

        settings.wvpMatrix = projection;
        Gs::RotateFree(settings.wvpMatrix, { 0, 1, 0 }, Gs::Deg2Rad(std::sin(anim)*55.0f));
        Gs::Translate(settings.wvpMatrix, { 0, 0, 5 });
        Gs::RotateFree(settings.wvpMatrix, Gs::Vector3f(1).Normalized(), anim*3);

        SetBoxColor({ 1, 1, 1 });

        // Clear color and depth buffers
        context->ClearBuffers(LLGL::ClearBuffersFlags::Color | LLGL::ClearBuffersFlags::Depth);

        // Set buffers
        context->SetVertexBuffer(*vertexBuffer);
        context->SetIndexBuffer(*indexBuffer);
        context->SetConstantBuffer(*constantBuffer, 0);

        // Start with qeometry query
        context->BeginQuery(*geometryQuery);
        {
            // Draw box for occlusion query
            context->SetGraphicsPipeline(*occlusionPipeline);

            context->BeginQuery(*occlusionQuery);
            {
                context->DrawIndexed(36, 0);
            }
            context->EndQuery(*occlusionQuery);

            // Draw scene
            context->SetGraphicsPipeline(*scenePipeline);

            SetBoxColor({ 0, 1, 0 });

            context->BeginRenderCondition(*occlusionQuery, LLGL::RenderConditionMode::Wait);
            {
                context->DrawIndexed(36, 0);
            }
            context->EndRenderCondition();
        }
        context->EndQuery(*geometryQuery);

        PrintQueryResult();

        // Present result on the screen
        context->Present();
    }

};

LLGL_IMPLEMENT_TUTORIAL(Tutorial04);


