#include <cstdint>
#include <launch_params.h>
#include <math.h>
#include <optix_device.h>

extern "C" {
__constant__ LaunchParams params;
}

static constexpr uint32_t kMaxBounces = 16;
static constexpr float kSkyIllumination = 1.4f;

struct Ray {
    float3 origin;
    float3 direction;
};

struct Hit {
    float3 normal;
    Material material;
    float distance;
    uint32_t surfaceId;
    bool found;
};

// Basic vector operations used by the ray programs.
static __device__ float3 operator+(float3 a, float3 b) {
    return float3{a.x + b.x, a.y + b.y, a.z + b.z};
}

static __device__ float3 operator-(float3 a, float3 b) {
    return float3{a.x - b.x, a.y - b.y, a.z - b.z};
}

static __device__ float3 operator*(float3 a, float scalar) {
    return float3{a.x * scalar, a.y * scalar, a.z * scalar};
}

static __device__ float3 operator*(float3 a, float3 b) {
    return float3{a.x * b.x, a.y * b.y, a.z * b.z};
}

static __device__ float3 operator*(float scalar, float3 a) {
    return a * scalar;
}

static __device__ float4 operator+(float4 a, float4 b) {
    return float4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static __device__ float4 operator-(float4 a, float4 b) {
    return float4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static __device__ float4 operator*(float4 a, float scalar) {
    return float4{a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar};
}

static __device__ float4 operator*(float scalar, float4 a) {
    return a * scalar;
}

static __device__ float dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static __device__ float3 cross(float3 a, float3 b) {
    return float3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static __device__ float3 unit(float3 a) {
    return a * rsqrtf(fmaxf(dot(a, a), 1e-30f));
}

// Random directions for the material scattering routines.
static __device__ uint32_t randomBits(uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

static __device__ float random01(uint32_t& s) {
    return (randomBits(s) >> 8) * (1.0f / 16777216.0f);
}

static __device__ float3 randomUnit(uint32_t& s) {
    float z = 2 * random01(s) - 1;
    float angle = 6.283185307f * random01(s);
    float r = sqrtf(fmaxf(0, 1 - z * z));
    return float3{r * __cosf(angle), r * __sinf(angle), z};
}

static __device__ float3 cosineHemisphere(float3 n, uint32_t& s) {
    float3 tangent = unit(cross(fabsf(n.y) < .99f ? float3{0, 1, 0} : float3{1, 0, 0}, n));
    float3 bitangent = cross(n, tangent);
    float r = sqrtf(random01(s)), a = 6.283185307f * random01(s);
    return unit(tangent * (r * __cosf(a)) + bitangent * (r * __sinf(a)) + sqrtf(fmaxf(0, 1 - r * r)) * n);
}

// Convert sampled skybox color from sRGB to linear.
static __device__ float3 srgbToLinear(float3 color) {
    return float3{
        color.x <= 0.04045f ? color.x / 12.92f : powf((color.x + 0.055f) / 1.055f, 2.4f),
        color.y <= 0.04045f ? color.y / 12.92f : powf((color.y + 0.055f) / 1.055f, 2.4f),
        color.z <= 0.04045f ? color.z / 12.92f : powf((color.z + 0.055f) / 1.055f, 2.4f),
    };
}

// Read and update the hit payload passed through optixTrace.
static __device__ Hit* payloadHit() {
    uint64_t p = (static_cast<uint64_t>(optixGetPayload_1()) << 32) | optixGetPayload_0();
    return reinterpret_cast<Hit*>(p);
}

static __device__ void storeHit(float3 normal, Material material, uint32_t surfaceId) {
    Hit* h = payloadHit();
    h->normal = unit(normal);
    h->material = material;
    h->distance = optixGetRayTmax();
    h->surfaceId = surfaceId;
    h->found = true;
}

static __device__ Hit trace(Ray const& ray) {
    Hit hit{};
    uint64_t pointer = reinterpret_cast<uint64_t>(&hit);
    uint32_t low = static_cast<uint32_t>(pointer);
    uint32_t high = static_cast<uint32_t>(pointer >> 32);
    optixTrace(
        params.scene,
        ray.origin,
        ray.direction,
        .001f,
        1e16f,
        0,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0,
        1,
        0,
        low,
        high);
    return hit;
}

// Trace one path and incrementally average its radiance per pixel.
extern "C" __global__ void __raygen__render() {
    uint3 index = optixGetLaunchIndex();
    uint32_t pixel = index.y * params.width + index.x;

    // Seed a per-pixel random sequence that changes with the frame.
    uint32_t randomState = (pixel + 1) * 747796405u ^ (params.frameIndex + 1) * 2891336453u;
    if (randomState == 0) {
        // xorshift32 stays at zero if its state starts at zero.
        randomState = 1;
    }

    // Jitter the sample within the pixel before mapping it to the camera plane.
    float const cameraPlaneX = (2 * (index.x + random01(randomState)) / float(params.width) - 1) * float(params.width) /
                               float(params.height) * 0.5773502692f;

    float const cameraPlaneY = (2 * (index.y + random01(randomState)) / float(params.height) - 1) * 0.5773502692f;

    // Construct the primary ray from the camera position and basis vectors.
    Ray ray{params.cameraPosition, unit(params.front + cameraPlaneX * params.right + cameraPlaneY * params.up)};

    // Track the path throughput and the resulting sample radiance.
    float3 pathThroughput{1, 1, 1};
    float3 radiance{0, 0, 0};

    // Follow the path through up to kMaxBounces material bounces.
    for (uint32_t bounceIndex = 0; bounceIndex < kMaxBounces; ++bounceIndex) {
        Hit const hit = trace(ray);
        if (!hit.found) {
            // TODO: Sample the skybox and add the escaping ray's contribution.

            break;
        }

        float3 const hitPoint = ray.origin + ray.direction * hit.distance;

        float3 const surfaceNormal = hit.normal;
        bool const isFrontFace = dot(ray.direction, surfaceNormal) < 0;
        float3 const orientedNormal = isFrontFace ? surfaceNormal : -1.0f * surfaceNormal;

        pathThroughput = pathThroughput * hit.material.albedo;

        float3 scatteredDirection = ray.direction;
        // TODO: Replace the placeholder direction with material scattering.

        ray.origin = hitPoint + scatteredDirection * .001f;
        ray.direction = scatteredDirection;
    }

    // Update the running average and write the current sample.
    float4 old = params.sample ? params.accumulation[pixel] : float4{0, 0, 0, 1};
    float inv = 1.0f / (params.sample + 1);
    float4 current{radiance.x, radiance.y, radiance.z, 1};
    float4 value = old + (current - old) * inv;
    params.accumulation[pixel] = value;
    params.pixels[pixel] = value;
}

extern "C" __global__ void __miss__background() {}

// Store triangle and sphere intersections in the shared payload format.
extern "C" __global__ void __closesthit__triangle() {
    // TODO: Read the hit record, calculate the triangle normal, and store the
    // hit.
}

extern "C" __global__ void __closesthit__sphere() {
    // TODO: Read the hit record, calculate the sphere normal, and store the hit.
}
