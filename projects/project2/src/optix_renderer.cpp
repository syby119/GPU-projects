#include <array>
#include <fstream>
#include <gpu/utility.h>
#include <iterator>
#include <optix/module.h>
#include <optix/program_group.h>
#include <optix_renderer.h>
#include <optix_stack_size.h>
#include <stdexcept>
#include <string>

OptixRenderer::OptixRenderer(
    optix::DeviceContext const& context,
    Scene const& scene,
    std::filesystem::path const& modulePath,
    uint32_t width,
    uint32_t height)
    : m_context(context)
    , m_scene(scene)
    , m_denoiser(
          context,
          OPTIX_DENOISER_MODEL_KIND_TEMPORAL_AOV,
          OptixDenoiserOptions{.guideAlbedo = 1, .guideNormal = 1}) {
    createPipelineAndShaderBindingTable(modulePath);
    resize(width, height);
}

void OptixRenderer::createPipelineAndShaderBindingTable(std::filesystem::path const& modulePath) {
    std::ifstream file(modulePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open OptiX module: " + modulePath.string());
    }
    std::string moduleData(std::istreambuf_iterator<char>{file}, {});

    // Create the pipeline.
    OptixPipelineCompileOptions pipelineOptions{
        .traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY,
        .numPayloadValues = 2,   // Two 32-bit values carry the Hit pointer through optixTrace.
        .numAttributeValues = 2, // Two triangle barycentric coordinates are used
                                 // as intersection attributes.
        .pipelineLaunchParamsVariableName = "params",
        .usesPrimitiveTypeFlags =
            static_cast<unsigned int>(OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE | OPTIX_PRIMITIVE_TYPE_FLAGS_SPHERE),
    };

    OptixModuleCompileOptions moduleOptions{
        .optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT,
        .debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE,
    };

    optix::Module module(m_context, moduleOptions, pipelineOptions, moduleData);

    OptixBuiltinISOptions sphereOptions{
        .builtinISModuleType = OPTIX_PRIMITIVE_TYPE_SPHERE,
        .buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE,
    };

    OptixModule sphereModuleHandle{};
    CHECK_OPTIX(optixBuiltinISModuleGet(
        m_context.handle(), &moduleOptions, &pipelineOptions, &sphereOptions, &sphereModuleHandle));

    optix::Module sphereModule = optix::Module::adopt(sphereModuleHandle);

    std::array<OptixProgramGroupDesc, 4> desc{};

    desc[0].kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    desc[0].raygen.module = module.handle();
    desc[0].raygen.entryFunctionName = "__raygen__render";

    desc[1].kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    desc[1].miss.module = module.handle();
    desc[1].miss.entryFunctionName = "__miss__background";

    desc[2].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    desc[2].hitgroup.moduleCH = module.handle();
    desc[2].hitgroup.entryFunctionNameCH = "__closesthit__triangle";

    desc[3].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    desc[3].hitgroup.moduleCH = module.handle();
    desc[3].hitgroup.entryFunctionNameCH = "__closesthit__sphere";
    desc[3].hitgroup.moduleIS = sphereModule.handle();

    OptixProgramGroupOptions groupOptions{};
    std::array<optix::ProgramGroup, 4> programGroups{};
    for (size_t i = 0; i < programGroups.size(); ++i) {
        programGroups[i] = optix::ProgramGroup(m_context, desc[i], groupOptions);
    }

    OptixPipelineLinkOptions linkOptions{.maxTraceDepth = 1};
    m_pipeline = optix::Pipeline(m_context, pipelineOptions, linkOptions, programGroups);

    OptixStackSizes stackSizes{};
    for (optix::ProgramGroup const& group : programGroups) {
        CHECK_OPTIX(optixUtilAccumulateStackSizes(group.handle(), &stackSizes, m_pipeline.handle()));
    }
    uint32_t traversal = 0, state = 0, continuation = 0;
    CHECK_OPTIX(optixUtilComputeStackSizes(&stackSizes, 1, 0, 0, &traversal, &state, &continuation));
    m_pipeline.setStackSize(traversal, state, continuation, 3);

    // Create the shader binding table.
    optix::SbtRecord<> raygen{}, miss{};
    optix::packHeader(programGroups[0].handle(), raygen);
    optix::packHeader(programGroups[1].handle(), miss);

    std::array<optix::SbtRecord<HitRecordData>, 4> hits{};
    auto const& modelMaterials = m_scene.modelMaterials();
    for (int i = 0; i < 3; ++i) {
        optix::packHeader(programGroups[2].handle(), hits[i]);
        hits[i].data.normals = m_scene.triangleNormals();
        hits[i].data.material = modelMaterials[i];
    }

    optix::packHeader(programGroups[3].handle(), hits[3]);
    hits[3].data.sphereMaterials = m_scene.sphereMaterials();

    m_sbt.setRaygen(&raygen, sizeof(raygen));
    m_sbt.setMiss(&miss, sizeof(miss), 1);
    m_sbt.setHitgroup(hits.data(), sizeof(hits[0]), static_cast<uint32_t>(hits.size()));
}

void OptixRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || (width == m_width && height == m_height)) {
        return;
    }

    // TODO: Resize the per-pixel rendering resources.
    m_width = width;
    m_height = height;
}

void OptixRenderer::resetTemporalHistory() {
    // TODO (bonus): Reset the temporal denoiser history.
}

void OptixRenderer::render(
    float4* pixels,
    uint32_t firstSampleIndex,
    Camera const& camera,
    bool denoise,
    uint32_t samplesPerFrame) {
    (void)firstSampleIndex;
    (void)camera;
    (void)denoise;
    (void)samplesPerFrame;

    // TODO: Launch OptiX for the requested samples and write the rendered image.
    // Until the TODO is complete, keep the output initialized for online and
    // offline mode.
    CHECK_CUDA(cudaMemset(pixels, 0, static_cast<size_t>(m_width) * m_height * sizeof(float4)));

    // TODO (bonus): Denoise the accumulated image when requested.
}
