// Shaders are written in a language called the high level shading language (HLSL)
// which is similar syntax to C++, so ti's easy to learn.
// Appendix b provides a concise reference to the HLSL.

// The vertex shader is the funcction called VS.
// note that you can give the vertex shader any valid function name.
// This vertex shader has four parameters:
// first 2 input parameters, the last 2 are output parameters.
// The HLSL does not have references or pointers, so to return multiple values
// from a function, you need to either use structure or out parameters.
// In HLSL, functions are always inlined.

// The first 2 input parameters form the input signature of the vertex shader
// and correspond to data members in our custom vertex strucutre we are using for
// the draw. The parameter semenatics ":POSITION" and ":COLOR" are used for
// mapping the elements in the vertex structure to the vertex shader input parameters.
// The output parameters also have attached semantics (":SV_POSITION" and ":COLOR").
// These are used to map vertex shader outputs to the corresponding inputs of the next stage.
// Note that the SV_POSITION semantic has a special SV prefix which stands for
// System Value. it is used to denote the vertex shader output element that holds
// the vertex position in homogeneous clip space.
// We must attach the SV_POSITION sematnic to the position output because the GPU
// needs to be aware of this value because it is involved in operations the other
// attributes are not involved in, such as clipping, depth testing and rasterization.
// the semantic name for output parameters that are not system values can be any valid
// semantic name.

// A constant buffer is an example of a GPU resource whose data contents can 
// be refrerenced in shader programs.
// Unlike vertex and index buffers, constant buffers are usually updated once
// per frame by the CPU. For example, if the camera is moving every frame,
// the constant buffer would need to be updated with the new view matrix every frame, 
// Therefore, we create constant buffers in an upload heap rather than a default heap
// so that we can update the contents from the CPU.

// Constant buffers also have the special hardware requirement that their size
// must be a multiple of the minimum hardware allocation size 256.

// Often we will need multiple constant buffers of the same type. For example,
// the below constant buffer cbPerObject sotres constants that vary per object,
// so if we have n objects, then we will need n constant buffers of this type.


// This is a constant buffer
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
}

// Direct3D 12 introduced shader model 5.1
// Shader model 5.1 has introduced an alternative HLSL syntax for defining
// a constant buffer which looks like this:
/*
struct ObjectConstants
{
	float4x4 gWorldViewProj;
	uint matIndex;
};
ConstantBuffer<ObjectConstants> gObjConstant : register(b0);
*/

// Here the data elements of the constant buffer are just defined in a
// separate structure and then a constant buffer is created from that structure.
// Fields of the constant buffer are then acces in the shader using data member syntax.
// uint index = gObjConstants.matIndex;

struct VertexIn
{
	float3 PosL : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn input)
{
	VertexOut vout;

	//Transform to homogeneous clip space.
	vout.PosH = mul(float4(input.PosL, 1.0f), gWorldViewProj);

	//Just pass the vertex color into the pixel shader
	vout.Color = input.Color;

	return vout;
}

// Note: if there is no geometry shader, then the vertex shader must output 
// the vertex position in homogenous clip space with the SV_POSITION semantic
// because this is the space the hardware expects the vertices to be in when
// leaving the vertex shader.
// If there is a geometry shader, the job of outputting the homogenous clip
// space position can be deferred to the geometry shader.

// Note: a vertex shader (or geometry shader) does not do the perspective divide.
// it just does the projection matrix part. The perspective divide will be done
// later by the hardware.

/*
both do the same thing!

void VS(
float3 iPosL : POSITION,
float4 iColor : COLOR,
out float4 oPosH : SV_POSITION,
out float4 oColor : COLOR
)
{
	//Transform to homgeneous clip space.
	oPosH = mul(float4(iPosL, 1.0f), gWorldViewProj);

	//Just pass vertex color into the pixel shader
	oColor = iColor;
}
*/

// SV_TARGET --> the return value type should match the render target format
float4 PS(VertexOut input) : SV_TARGET 
{
	return input.Color;
}