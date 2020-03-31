#define TSQRT2 2.828427
#define SQRT2 1.414213
#define ISQRT2 0.707106
#define MIPMAP_HARDCAP 5.4f /* Too high mipmap levels => glitchiness, too low mipmap levels => sharpness. */
#define VOXEL_SIZE (1/256.0) /* Size of a voxel. 128x128x128 => 1/128 = 0.0078125. */
#define DIFFUSE_INDIRECT_FACTOR 1.2f /* Just changes intensity of diffuse indirect lighting. */

#define vec3 float3
#define vec4 float4
// Returns a vector that is orthogonal to u.
float3 orthogonal(float3 u) {
	u = normalize(u);
	float3 v = float3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
	return abs(dot(u, v)) > 0.99999f ? cross(u, float3(0, 1, 0)) : cross(u, v);
}

// Scales and bias a given vector (i.e. from [-1, 1] to [0, 1]).
float3 scaleAndBias(const float3 p) { return 0.5f * p + float3(0.5f,0.5f,0.5f); }

float3 scaleAndBias2(float3 p)
{
	float3 res;
	res.x = 0.5f * p.x + 0.5;
	res.y = -0.5f * p.y + 0.5;
	res.z = 0.5f * p.z + 0.5;
	return res;
}

// Returns true if the point p is inside the unity cube. 
bool isInsideCube(const float3 p, float e) {
	return abs(p.x) < 1 + e && abs(p.y) < 1 + e && abs(p.z) < 1 + e;
}

// Traces a diffuse voxel cone.
vec3 traceDiffuseVoxelCone(const vec3 from, vec3 direction, Texture3D tex, SamplerState samplerState) {
	direction = normalize(direction);

	const float CONE_SPREAD = 3.0325;

	vec4 acc = 0.0f;

	// Controls bleeding from close surfaces.
	// Low values look rather bad if using shadow cone tracing.
	// Might be a better choice to use shadow maps and lower this value.
	float dist = 0.51953125;
	
	// Trace.
	while (dist < SQRT2 && acc.a < 1) {
		vec3 c = from + dist * direction;
		c = scaleAndBias2(from + dist * direction);
		float l = (1 + CONE_SPREAD * dist / VOXEL_SIZE);
		float level = log2(l);
		float ll = (level + 1) * (level + 1);
		//vec4 voxel = textureLod(texture3D, c, min(MIPMAP_HARDCAP, level));
		vec4 voxel = tex.SampleLevel(samplerState, c, min(MIPMAP_HARDCAP, level));
			acc += 0.075 * ll * voxel * pow(1 - voxel.a, 2);
		dist += ll * VOXEL_SIZE * 2;
	}
	return pow(acc.rgb * 2.0, vec3(1.5,1.5,1.5));
}