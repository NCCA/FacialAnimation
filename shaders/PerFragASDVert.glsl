#version 330 core
// this is base on http://http.developer.nvidia.com/GPUGems3/gpugems3_ch03.html
layout (location =0) in vec3 baseVert;
layout (location =1) in vec3 baseNormal;

// transform matrix values
uniform mat4 MVP;
uniform mat3 normalMatrix;
uniform mat4 MV;

#define meshOffset 28
#define numWeights 14
uniform float weights[numWeights];



out vec3 position;
out vec3 normal;
out vec4 debugColour;
uniform samplerBuffer TBO;
void main()
{

	vec3  finalN;
	vec3  finalP;
	vec3 weightNorm=vec3(0.0f);
	vec3 weightVert=vec3(0.0f);
	// do the verts first
	int i;
	for (i=0; i<numWeights; ++i)
	{
		weightVert+= (texelFetch(TBO,int(meshOffset*gl_VertexID+i)).xyz*weights[i]);
	}

	finalP= baseVert+weightVert;

	for (; i<meshOffset; ++i)
	{
		weightNorm+= (texelFetch(TBO,int(meshOffset*gl_VertexID+i)).xyz*weights[i-numWeights]);
	}

	finalN= baseNormal+weightNorm;

	// then normalize and mult by normal matrix for shading
	normal = normalize( normalMatrix * finalN);
	// now calculate the eye cord position for the frag stage
	position = vec3(MV * vec4(baseVert,1.0));

	//debugColour=vec4(weight3*poseVert3,1);
	// Convert position to clip coordinates and pass along
	gl_Position = MVP*vec4(finalP,1.0);

}









