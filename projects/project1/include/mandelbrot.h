#pragma once

#include <cuda_runtime.h>

#include <complex.hpp>
#include <render_paramters.h>

void renderMandelbrotSet(float4* output, RenderParameters const& parameters
                         /* add additional arguments if necessary */
);
