/*
 * main.cpp (Tutorial02_Tessellation)
 *
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "../tutorial.h"


// Automatically rotate the model
//#define AUTO_ROTATE

// Enable multi-sampling anti-aliasing
#define ENABLE_MULTISAMPLING

// Test constant buffer array
//#define _TEST_BUFFER_ARRAY_


class Tutorial02 : public Tutorial
{

    LLGL::ShaderProgram*    shaderProgram       = nullptr;
    LLGL::GraphicsPipeline* pipeline[2]         = { nullptr };

    LLGL::Buffer*           vertexBuffer        = nullptr;
    LLGL::Buffer*           indexBuffer         = nullptr;
    LLGL::Buffer*           constantBuffer      = nullptr;

    #ifdef _TEST_BUFFER_ARRAY_
    LLGL::BufferArray*      constantBufferArray = nullptr;
    #endif

    unsigned int            constantBufferIndex = 0;

    bool                    showWireframe       = false;

    struct Settings
    {
        Gs::Matrix4f    wvpMatrix;
        float           tessLevelInner  = 5.0f;
        float           tessLevelOuter  = 5.0f;
        float           twist           = 0.0f;
        float           _pad0;                  // <-- padding for 16 byte pack alignment of constant buffers
    }
    settings;

public:

    Tutorial02() :
        Tutorial( L"LLGL Tutorial 02: Tessellation" )//, { 800, 600 }, 0 )
    {
        // Check if constant buffers and tessellation shaders are supported
        const auto& renderCaps = renderer->GetRenderingCaps();

        if (!renderCaps.hasConstantBuffers)
            throw std::runtime_error("constant buffers are not supported by this renderer");
        if (!renderCaps.hasTessellationShaders)
            throw std::runtime_error("tessellation shaders are not supported by this renderer");

        // Create graphics object
        auto vertexFormat = CreateBuffers();
        LoadShaders(vertexFormat, (renderCaps.shadingLanguage >= LLGL::ShadingLanguage::HLSL_2_0));
        CreatePipelines();

        // Print some information on the standard output
        std::cout << "press LEFT MOUSE BUTTON and move mouse on X axis to increase/decrease inner tessellation" << std::endl;
        std::cout << "press RIGHT MOUSE BUTTON and move mouse on X axis to increase/decrease outer tessellation" << std::endl;
        std::cout << "press MIDDLE MOUSE BUTTON and move mouse on X axis to increase/decrease twist" << std::endl;
        std::cout << "press TAB KEY to switch between wireframe modes" << std::endl;
        ShowTessLevel();
    }

    LLGL::VertexFormat CreateBuffers()
    {
        // Specify vertex format
        LLGL::VertexFormat vertexFormat;
        vertexFormat.AppendAttribute({ "position", LLGL::VectorType::Float3 });

        // Create buffers for a simple 3D cube model
        vertexBuffer = CreateVertexBuffer(GenerateCubeVertices(), vertexFormat);
        indexBuffer = CreateIndexBuffer(GenerateCubeQuadlIndices(), LLGL::DataType::UInt32);
        constantBuffer = CreateConstantBuffer(settings);

        #ifdef _TEST_BUFFER_ARRAY_

        // Create constant buffer array
        constantBufferArray = renderer->CreateBufferArray(1, &constantBuffer);

        #endif

        return vertexFormat;
    }

    void LoadShaders(const LLGL::VertexFormat& vertexFormat, bool loadHLSL)
    {
        // Load shader program
        if (loadHLSL)
        {
            shaderProgram = LoadShaderProgram(
                {
                    { LLGL::ShaderType::Vertex, "shader.hlsl", "VS", "vs_5_0" },
                    { LLGL::ShaderType::TessControl, "shader.hlsl", "HS", "hs_5_0" },
                    { LLGL::ShaderType::TessEvaluation, "shader.hlsl", "DS", "ds_5_0" },
                    { LLGL::ShaderType::Fragment, "shader.hlsl", "PS", "ps_5_0" }
                },
                vertexFormat
            );
        }
        else
        {
            shaderProgram = LoadShaderProgram(
                {
                    { LLGL::ShaderType::Vertex, "vertex.glsl" },
                    { LLGL::ShaderType::TessControl, "tesscontrol.glsl" },
                    { LLGL::ShaderType::TessEvaluation, "tesseval.glsl" },
                    { LLGL::ShaderType::Fragment, "fragment.glsl" }
                },
                vertexFormat
            );
        }

        // Bind constant buffer location to the index we use later with the render context
        shaderProgram->BindConstantBuffer("Settings", constantBufferIndex);
    }

    void CreatePipelines()
    {
        // Setup graphics pipeline descriptors
        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        {
            // Set shader program
            pipelineDesc.shaderProgram              = shaderProgram;

            // Set input-assembler state (draw pachtes with 4 control points)
            pipelineDesc.primitiveTopology          = LLGL::PrimitiveTopology::Patches4;

            // Enable multi-sample anti-aliasing
            #ifdef ENABLE_MULTISAMPLING
            pipelineDesc.rasterizer.multiSampling   = LLGL::MultiSamplingDescriptor(8);
            #endif

            // Enable depth test and writing
            pipelineDesc.depth.testEnabled          = true;
            pipelineDesc.depth.writeEnabled         = true;

            // Enable back-face culling
            pipelineDesc.rasterizer.cullMode        = LLGL::CullMode::Back;
            pipelineDesc.rasterizer.frontCCW        = true;
        }

        // Create graphics pipelines
        pipeline[0] = renderer->CreateGraphicsPipeline(pipelineDesc);

        pipelineDesc.rasterizer.polygonMode = LLGL::PolygonMode::Wireframe;
        pipeline[1] = renderer->CreateGraphicsPipeline(pipelineDesc);
    }

    void ShowTessLevel()
    {
        //std::cout << "tessellation level (inner = " << settings.tessLevelInner << ", outer = " << settings.tessLevelOuter << ")      \r";
        //std::flush(std::cout);
    }

private:

    void UpdateUserInput()
    {
        // Tessellation level-of-detail limits
        static const float tessLevelMin = 1.0f, tessLevelMax = 64.0f;

        // Update tessellation levels by user input
        auto motion = input->GetMouseMotion().x;
        auto motionScaled = static_cast<float>(motion)*0.1f;

        if (input->KeyPressed(LLGL::Key::LButton))
        {
            settings.tessLevelInner += motionScaled;
            settings.tessLevelInner = Gs::Clamp(settings.tessLevelInner, tessLevelMin, tessLevelMax);
        }

        if (input->KeyPressed(LLGL::Key::RButton))
        {
            settings.tessLevelOuter += motionScaled;
            settings.tessLevelOuter = Gs::Clamp(settings.tessLevelOuter, tessLevelMin, tessLevelMax);
        }

        if ( motion != 0 && ( input->KeyPressed(LLGL::Key::LButton) || input->KeyPressed(LLGL::Key::RButton) ) )
            ShowTessLevel();

        if (input->KeyPressed(LLGL::Key::MButton))
            settings.twist += Gs::Deg2Rad(motionScaled);

        if (input->KeyDown(LLGL::Key::Tab))
            showWireframe = !showWireframe;
        
        // Update matrices
        Gs::Matrix4f worldMatrix;
        Gs::Translate(worldMatrix, Gs::Vector3f(0, 0, 5));

        settings.wvpMatrix = projection * worldMatrix;

        #ifdef AUTO_ROTATE
        static float rotation;
        rotation += 0.0025f;
        Gs::RotateFree(settings.worldMatrix, Gs::Vector3f(1, 1, 1).Normalized(), rotation);
        #endif
    }

    void DrawScene()
    {
        // Set the render context as the initial render target
        commands->SetRenderTarget(*context);

        // Clear color- and depth buffers
        commands->Clear(LLGL::ClearFlags::Color | LLGL::ClearFlags::Depth);

        // Update constant buffer
        UpdateBuffer(constantBuffer, settings);

        // Set graphics pipeline with the shader
        commands->SetGraphicsPipeline(*pipeline[showWireframe ? 1 : 0]);

        // Set hardware buffers to draw the model
        commands->SetVertexBuffer(*vertexBuffer);
        commands->SetIndexBuffer(*indexBuffer);

        // Set constant buffer only to tessellation shader stages
        #ifdef _TEST_BUFFER_ARRAY_
        commands->SetConstantBufferArray(*constantBufferArray, constantBufferIndex, LLGL::ShaderStageFlags::AllTessStages);
        #else
        commands->SetConstantBuffer(*constantBuffer, constantBufferIndex, LLGL::ShaderStageFlags::AllTessStages);
        #endif

        // Draw tessellated quads with 24=4*6 vertices from patches of 4 control points
        commands->DrawIndexed(24, 0);

        // Present result on the screen
        context->Present();
    }

    void OnDrawFrame() override
    {
        UpdateUserInput();
        DrawScene();
    }

};

LLGL_IMPLEMENT_TUTORIAL(Tutorial02);



