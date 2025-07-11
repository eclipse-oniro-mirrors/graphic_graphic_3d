#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_compatibility_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"

#include "3d/shaders/common/3d_dm_vert_layout_common.h"

#include "common/dotfield_common.h"
#include "common/dotfield_struct_common.h"

// sets

// in
layout(location = 0) in vec4 inFieldData;


// out
layout(location = 0) out vec4 outColor;

/*
vertex shader for dot field effect
*/
void main(void)
{
    // Tunable parameters
    const float animationSpeed = 5.f;
    const float positionPhaseFactor = 0.17f;
    const float waveAnimationSpeed = 0.3f;
    const float waveAnimationAmplitude = 0.1f;
    const float gradientAnimationSpeed = 0.1f;

    const uint sizeX = uMeshMatrix.mesh[0].customData[0].x;
    const uint sizeY = uMeshMatrix.mesh[0].customData[0].y;
    int grid_x = int(gl_InstanceIndex % sizeX);
    int grid_y = int(gl_InstanceIndex / sizeX);

    const uint matInstanceIdx = 0U;
    const vec4 time = uMaterialData.material[matInstanceIdx].factors[CORE_MATERIAL_FACTOR_MATERIAL_IDX];

    float phase = (grid_x + grid_y) * positionPhaseFactor + (time.x * animationSpeed);

    vec3 position = inFieldData.xyz;
    position.x += grid_x / (sizeX / 2.0f) - 1.0f;
    position.y += grid_y / (sizeY / 2.0f) - 1.0f;

    position.x += waveAnimationAmplitude * sin(phase * waveAnimationSpeed);
    position.y += waveAnimationAmplitude * cos(phase * waveAnimationSpeed);
    position.z += waveAnimationAmplitude * cos(phase * waveAnimationSpeed);

    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);

    const mat4 worldMatrix = uMeshMatrix.mesh[0].world;
    const vec4 worldPos = worldMatrix * vec4(position, 1.0);
    const vec4 viewPos = uCameras[cameraIdx].view * worldPos;

    CORE_VERTEX_OUT(uCameras[cameraIdx].proj * viewPos);

    // Gradient
    float colorPhase = mod(phase * gradientAnimationSpeed, 4); // number of colors
    int idx0 = int(floor(colorPhase));
    int idx1 = int(ceil(colorPhase)) & 3; // number of colors - 1
    float fr = fract(colorPhase);
    const vec4 colors = uMaterialData.material[matInstanceIdx].factors[CORE_MATERIAL_FACTOR_NORMAL_IDX];
    vec4 color0 = unpackUnorm4x8(floatBitsToUint(colors[idx0]));
    vec4 color1 = unpackUnorm4x8(floatBitsToUint(colors[idx1]));
    outColor = mix(color0, color1, fr);

    gl_PointSize = abs(uCameras[cameraIdx].proj[1][1]) * time.y  / gl_Position.w;
}


// Some useful functions
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }


/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(vec3 p) {
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/

	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));

	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);

	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;

	 /* 2. find four surflets and store them in d */
	 vec4 w, d;

	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);

	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);

	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);

	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;

	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

/* const matrices for 3d rotation */
const mat3 rot1 = mat3(-0.37, 0.36, 0.85,-0.14,-0.93, 0.34,0.92, 0.01,0.4);
const mat3 rot2 = mat3(-0.55,-0.39, 0.74, 0.33,-0.91,-0.24,0.77, 0.12,0.63);
const mat3 rot3 = mat3(-0.71, 0.52,-0.47,-0.08,-0.72,-0.68,-0.7,-0.45,0.56);

/* directional artifacts can be reduced by rotating each octave */
float simplex3d_fractal(vec3 m) {
    return   0.5333333*simplex3d(m*rot1)
			+0.2666667*simplex3d(2.0*m*rot2)
			+0.1333333*simplex3d(4.0*m*rot3)
			+0.0666667*simplex3d(8.0*m);
}
