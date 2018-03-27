#version 330 compatibility
#extension GL_ARB_gpu_shader5 : enable
in vec4 xfb_position;
//in vec4 xfb_data;
out vec2 ov_tex_coord0;
out vec3 ov_normal;
out mat3 ov_normal_matrix;
out float ov_depth;
out float ov_intensity;


float fakeRand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float fakeRandRange(float minValue, float maxValue, vec2 co)
{
	float t = fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
	return minValue + t*(maxValue - minValue);
}

mat4 rotationMat4(vec3 axis, float angle)
{
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat4(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
		oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
		oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat3 rotationMat3(vec3 axis, float angle)
{
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat3(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s,
		oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s,
		oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
}

void main(void) {
	float rand_val = fakeRand(xfb_position.xy);
	float rand_angle = rand_val*3.14;
	float rand_scale = 0.5 + rand_val * (1.5 - 0.5);
	mat4 rand_rot = rotationMat4(vec3(0, 0, 1), rand_angle);
	vec4 position = rand_rot * gl_Vertex;
	position.xyz *= rand_scale;
	position.xyz += xfb_position.xyz;
	//position.xyz += xfb_data.x;
	
	gl_Position = gl_ModelViewProjectionMatrix * position;
	ov_normal_matrix = gl_NormalMatrix * mat3(rand_rot);
	ov_normal = normalize(ov_normal_matrix * gl_Normal);
	vec4 mvm_pos = gl_ModelViewMatrix * position;
	ov_depth = length(mvm_pos.xyz);
	ov_tex_coord0 = gl_MultiTexCoord0.xy;
	ov_intensity = xfb_position.w;
}