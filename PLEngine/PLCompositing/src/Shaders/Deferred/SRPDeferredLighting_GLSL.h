/*********************************************************\
 *  File: SRPDeferredLighting_GLSL.h                     *
 *
 *  Copyright (C) 2002-2011 The PixelLight Team (http://www.pixellight.org/)
 *
 *  This file is part of PixelLight.
 *
 *  PixelLight is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  PixelLight is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with PixelLight. If not, see <http://www.gnu.org/licenses/>.
\*********************************************************/


// OpenGL 3.0 ("#version 130") GLSL vertex shader source code, "#version" is added by "PLRenderer::ProgramGenerator"
static const PLGeneral::String sDeferredLighting_GLSL_VS = "\
// Attributes\n\
in  vec4 VertexPosition;				// Clip space vertex position, lower/left is (-1,-1) and upper/right is (1,1)\n\
in  vec2 VertexTexCoord0;				// Vertex texture coordinate, lower/left is (0,0) and upper/right is (1,1)\n\
out vec2 VertexTexCoordVS;				// Vertex texture coordinate, lower/left is (0,0) and upper/right is (<TextureWidth>,<TextureHeight>) output\n\
out vec2 VertexTexCoordNormalizedVS;	// Vertex texture coordinate, lower/left is (0,0) and upper/right is (1,1) output\n\
\n\
// Uniforms\n\
uniform highp ivec2 TextureSize;	// Texture size in texel\n\
\n\
// Programs\n\
void main()\n\
{\n\
	// Pass through the vertex position\n\
	gl_Position = VertexPosition;\n\
\n\
	// Pass through the scaled vertex texture coordinate\n\
	VertexTexCoordVS = VertexTexCoord0*TextureSize;\n\
\n\
	// Pass through the vertex texture coordinate\n\
	VertexTexCoordNormalizedVS = VertexTexCoord0;\n\
}";


// OpenGL 3.0 ("#version 130") GLSL fragment shader source code, "#version" is added by "PLRenderer::ProgramGenerator"
static const PLGeneral::String sDeferredLighting_GLSL_FS = "\
// GLSL extensions\n\
#extension GL_ARB_texture_rectangle : enable\n\
\n\
// Attributes\n\
in vec2 VertexTexCoordVS;			// Vertex texture coordinate, lower/left is (0,0) and upper/right is (<TextureWidth>,<TextureHeight>) input from vertex shader\n\
in vec2 VertexTexCoordNormalizedVS;	// Vertex texture coordinate, lower/left is (0,0) and upper/right is (1,1) input from vertex shader\n\
\n\
// Uniforms\n\
#ifdef FS_DIRECTIONAL\n\
	uniform highp vec3 LightDirection;									// View space normalized light direction\n\
#else\n\
	uniform highp vec3  LightPosition;									// View space light position\n\
	uniform highp float	LightRadius;									// View space light radius\n\
	#ifdef FS_PROJECTIVE_POINT\n\
		uniform lowp  samplerCube ProjectivePointCubeMap;				// Cube map texture for projective point light\n\
		uniform highp mat3        ViewSpaceToCubeMapSpace;				// View space to cube map space transform matrix\n\
	#else\n\
		#ifdef FS_SPOT\n\
			uniform highp vec3 LightDirection;							// View space normalized light direction\n\
			#ifdef FS_PROJECTIVE_SPOT\n\
				uniform lowp  sampler2D ProjectiveSpotMap;				// Texture for projective spot light\n\
				uniform highp mat4      ViewSpaceToSpotMapSpace;		// View space to spot map space transform matrix\n\
			#endif\n\
			#ifdef FS_SPOT_CONE\n\
				#ifdef FS_SPOT_SMOOTHCONE\n\
					uniform highp vec2  SpotConeCos;					// Cosinus of the outer and inner cone angle in view space\n\
				#else\n\
					uniform highp float SpotConeCos;					// Cosinus of the cone angle in view space\n\
				#endif\n\
			#endif\n\
		#endif\n\
	#endif\n\
	#ifdef FS_SHADOWMAPPING\n\
		#ifdef FS_SPOT\n\
			uniform highp sampler2DShadow ShadowMap;					// 2D shadow map\n\
			uniform highp mat4			  ViewSpaceToShadowMapSpace;	// View space to shadow map space transform matrix\n\
		#else\n\
			uniform highp samplerCube ShadowMap;						// Cube shadow map\n\
			uniform highp mat3		  ViewSpaceToShadowCubeMapSpace;	// View space to shadow cube map space transform matrix\n\
			uniform highp float		  InvLightRadius;					// 1/LightRadius\n\
		#endif\n\
		#ifdef FS_SOFTSHADOWMAPPING\n\
			uniform highp float TexelSize;								// Shadow map texel size\n\
		#endif\n\
	#endif\n\
#endif\n\
uniform lowp  vec3			LightColor;									// Light color\n\
uniform highp vec2			InvFocalLen;								// 1/FocalLen (for example: 0.57771897 x 0.4142136)\n\
uniform highp sampler2DRect RenderTargetTexture0;						// Render target texture 0\n\
uniform highp sampler2DRect RenderTargetTexture1;						// Render target texture 1\n\
uniform highp sampler2DRect RenderTargetTexture2;						// Render target texture 2\n\
\n\
// Programs\n\
// Performs the Blinn-Phong lighting calculation\n\
highp vec3 BlinnPhong(highp vec3 lightVector, highp vec3 lightColor, highp vec3 viewVector, highp vec3 normalVector, highp vec3 diffuseColor, highp vec3 specularColor, highp float specularExponent)\n\
{\n\
	// [TODO] There seem to be invalid normal vectors here (NAN)\n\
	if (isnan(normalVector.x) || isnan(normalVector.y) || isnan(normalVector.z))\n\
		normalVector = vec3(0, 0, 1);\n\
	if (isnan(lightVector.x) || isnan(lightVector.y) || isnan(lightVector.z))\n\
		lightVector = vec3(0, 0, 1);\n\
	if (isnan(viewVector.x) || isnan(viewVector.y) || isnan(viewVector.z))\n\
		viewVector = vec3(0, 0, 1);\n\
\n\
	// Diffuse term\n\
	highp vec3 diffuseLighting = clamp(dot(lightVector, normalVector), 0.0f, 1.0f)*diffuseColor*lightColor;\n\
\n\
	// Specular term\n\
	#ifdef FS_NO_SPECULAR\n\
		#define specularLighting 0\n\
	#else\n\
		// Calculate the half vector between the light vector and the view vector. This is cheaper then calculating the actual reflective vector.\n\
		highp vec3 halfVector = normalize(lightVector + viewVector);\n\
\n\
		// Ensure that the specular exponent is never ever <=0, else NANs may be produced by pow!\n\
		#define FLT_MIN 1.175494351e-38F // Minimum positive value\n\
		if (specularExponent < FLT_MIN)\n\
			specularExponent = FLT_MIN;\n\
		#undef FLT_MIN\n\
\n\
		// Specular term\n\
		highp vec3 specularLighting = pow(clamp(dot(halfVector, normalVector), 0.0f, 1.0f), specularExponent)*specularColor*lightColor;\n\
	#endif\n\
\n\
	// Final color\n\
	return diffuseLighting + specularLighting;\n\
}\n\
\n\
#ifdef FS_SOFTSHADOWMAPPING\n\
	// Fake PCF (percentage-closer filtering)\n\
	highp vec4 texPCF(highp sampler2DShadow map, highp vec4 location, highp vec2 offset)\n\
	{\n\
		return shadow2DProj(map, vec4(location.xy + location.w*offset, location.z, location.w));\n\
	}\n\
\n\
	highp float texPCF(highp samplerCube map, highp vec3 location, highp vec3 offset)\n\
	{\n\
		// Get shadow map data\n\
		highp vec4 shadowMap = textureCube(map, location + offset);\n\
\n\
		// Unpack\n\
		return shadowMap.r/1 + shadowMap.g/256 + shadowMap.b/65536 + shadowMap.a/16777216;\n\
	}\n\
#endif\n\
\n\
#ifdef FS_SHADOWMAPPING\n\
	highp float texPCF(highp samplerCube map, highp vec3 location)\n\
	{\n\
		// Get shadow map data\n\
		highp vec4 shadowMap = textureCube(map, location);\n\
\n\
		// Unpack\n\
		return shadowMap.r/1 + shadowMap.g/256 + shadowMap.b/65536 + shadowMap.a/16777216;\n\
	}\n\
#endif\n\
\n\
highp vec3 decodeNormalVector(highp vec2 normal)\n\
{\n\
	highp vec2 fenc = normal*4 - 2;\n\
	highp float f = dot(fenc, fenc);\n\
	highp float g = sqrt(1 - f/4);\n\
	highp vec3 n;\n\
	n.xy = fenc*g;\n\
	n.z = 1 - f/2;\n\
	return n;\n\
}\n\
\n\
// Reconstructs view-space position using given parameters\n\
highp vec3 uv_to_eye(highp vec2 uv, highp float eye_z, highp vec2 invFocalLen)\n\
{\n\
	// Convert from texture space [0,1] range into [-1,1] clip space\n\
	uv = uv*2 - 1;\n\
\n\
	// Reconstructs view-space position\n\
	// [TODO] Why do I need to invert in here, but not within 'SRPDeferredHBAO_GLSL'?\n\
	return vec3(uv * invFocalLen * eye_z, -eye_z);\n\
}\n\
\n\
// Program entry point\n\
void main()\n\
{\n\
	// Reconstruct view-space position (current surface point)\n\
	highp vec4 sampleRT1 = texture2DRect(RenderTargetTexture1, VertexTexCoordVS);\n\
	highp vec3 position  = uv_to_eye(VertexTexCoordNormalizedVS, sampleRT1.b, InvFocalLen);\n\
\n\
	// Perform the lighting calculation in view space\n\
#ifdef FS_DIRECTIONAL\n\
	highp vec3 normal    = decodeNormalVector(sampleRT1.rg);\n\
	highp vec4 sampleRT0 = texture2DRect(RenderTargetTexture0, VertexTexCoordVS);\n\
	highp vec4 sampleRT2 = texture2DRect(RenderTargetTexture2, VertexTexCoordVS);\n\
	#ifdef FS_NO_ALBEDO\n\
		sampleRT0.rgb = vec3(1);	// Set to default\n\
	#endif\n\
	#ifdef FS_NO_SPECULARCOLOR\n\
		sampleRT2.rgb = vec3(1);	// Set to default\n\
	#endif\n\
	#ifdef FS_NO_SPECULAREXPONENT\n\
		sampleRT2.a = vec3(45);		// Set to default\n\
	#endif\n\
	gl_FragColor.rgb = BlinnPhong(LightDirection, LightColor, -normalize(position), normal, sampleRT0.rgb, sampleRT2.rgb, sampleRT2.a);\n\
\n\
	// Not shadowed by default\n\
	#define shadow 1\n\
#else\n\
	// Calculate the view space light vector pointing from the position to the light position\n\
	highp vec3 lightVector = LightPosition - position;\n\
\n\
	// Get the distance between the light and position\n\
	highp float distance = length(lightVector);\n\
\n\
	// If the position is outside of area of effect, discard position\n\
	if (distance > LightRadius) {\n\
		// Early escape: Not influcenced by the light\n\
		#ifdef FS_DISCARD\n\
			discard;\n\
		#else\n\
			gl_FragColor = vec4(0);\n\
			return;\n\
		#endif\n\
	}\n\
\n\
	// Perform shadow mapping\n\
	#ifdef FS_SHADOWMAPPING\n\
		#ifdef FS_SPOT\n\
			// Calculate the shadow vector\n\
			highp vec4 shadowVector = ViewSpaceToShadowMapSpace*vec4(position, 1);\n\
\n\
			// Shadow mapping\n\
			#ifdef FS_SOFTSHADOWMAPPING\n\
				// Fake PCF (percentage-closer filtering)\n\
				highp float shadow = (texPCF(ShadowMap, shadowVector, vec2(-TexelSize,  TexelSize)).x +\n\
									  texPCF(ShadowMap, shadowVector, vec2( TexelSize,  TexelSize)).x +\n\
									  texPCF(ShadowMap, shadowVector, vec2(-TexelSize, -TexelSize)).x +\n\
									  texPCF(ShadowMap, shadowVector, vec2( TexelSize, -TexelSize)).x) * 0.25f;\n\
			#else\n\
				highp float shadow = shadow2DProj(ShadowMap, shadowVector).x;\n\
			#endif\n\
		#else\n\
			// Shadow mapping\n\
			highp vec3 shadowVector = ViewSpaceToShadowCubeMapSpace*(-lightVector*InvLightRadius);\n\
			highp float shadowVecLength = length(shadowVector);\n\
			#ifdef FS_SOFTSHADOWMAPPING\n\
				// Shadowed?\n\
				highp float shadow = (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3(TexelSize, TexelSize, TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow += (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3(-TexelSize, -TexelSize, -TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow += (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3( TexelSize, -TexelSize, -TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow += (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3(-TexelSize,  TexelSize, -TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow += (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3( TexelSize, -TexelSize,  TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow += (shadowVecLength < texPCF(ShadowMap, shadowVector, vec3(-TexelSize,  TexelSize,  TexelSize))) ? 0.16666667f : 0.0f;\n\
				shadow = clamp(shadow, 0.0f, 1.0f);\n\
			#else\n\
				// Unpack\n\
				highp float depthValue = texPCF(ShadowMap, shadowVector);\n\
\n\
				// Shadowed?\n\
				highp float shadow = (shadowVecLength < depthValue) ? 1 : 0;\n\
			#endif\n\
		#endif\n\
\n\
		// Is the position completely shadowed?\n\
		if (shadow <= 0) {\n\
			// Early escape: Not influcenced by the light\n\
			#ifdef FS_DISCARD\n\
				discard;\n\
			#else\n\
				gl_FragColor = vec4(0);\n\
				return;\n\
			#endif\n\
		}\n\
	#else\n\
		// Not shadowed by default\n\
		#define shadow 1\n\
	#endif\n\
\n\
	// Get the light color (lightColor)\n\
	#ifdef FS_PROJECTIVE_POINT\n\
		// Calculate the cube map space vector ('-' -> The light is the source of the ray, not the surface!)\n\
		highp vec3 cubeMapVector = ViewSpaceToCubeMapSpace*(-lightVector);\n\
\n\
		// Get cube map texture for projective point light\n\
		lowp vec3 lightColor = textureCube(ProjectivePointCubeMap, cubeMapVector).rgb;\n\
		// Perform sRGB to linear space conversion (gamma correction)\n\
		#ifdef FS_GAMMACORRECTION\n\
			lightColor = pow(lightColor, vec3(2.2f, 2.2f, 2.2f));\n\
		#endif\n\
		// Apply light color\n\
		lightColor *= LightColor;\n\
	#else\n\
		// Start with the original light color\n\
		lowp vec3 lightColor = LightColor;\n\
\n\
		// Add spot light\n\
		#ifdef FS_SPOT\n\
			// Projective spot map\n\
			#ifdef FS_PROJECTIVE_SPOT\n\
				// Calculate the projective spot map texture coordinate\n\
				highp vec4 projectiveSpotMapUV = ViewSpaceToSpotMapSpace*(-vec4(position, 1));\n\
\n\
				// No back projection, please!\n\
				if (projectiveSpotMapUV.z < 0) {\n\
					// Early escape: Not influcenced by the light\n\
					#ifdef FS_DISCARD\n\
						discard;\n\
					#else\n\
						gl_FragColor = vec4(0);\n\
						return;\n\
					#endif\n\
				}\n\
\n\
				// Get the projective spot map texel data\n\
				lowp vec3 projectiveSpotMapTexel = texture2DProj(ProjectiveSpotMap, projectiveSpotMapUV).rgb;\n\
				// Perform sRGB to linear space conversion (gamma correction)\n\
				#ifdef FS_GAMMACORRECTION\n\
					projectiveSpotMapTexel = pow(projectiveSpotMapTexel, vec3(2.2f, 2.2f, 2.2f));\n\
				#endif\n\
				// Modulate the color of the light using the projective spot map texel data\n\
				lightColor *= projectiveSpotMapTexel;\n\
			#endif\n\
\n\
			// Spot light with cone\n\
			#ifdef FS_SPOT_CONE\n\
				// Calculate the angle between the current position and the spot light\n\
				highp float currentSpotConeCos = dot(normalize(position - LightPosition), LightDirection);\n\
\n\
				// Spot light with smooth cone\n\
				#ifdef FS_SPOT_SMOOTHCONE\n\
					// Get the current smooth spot cone attenuation\n\
					highp float currentSpotConeCosAttenuation = smoothstep(SpotConeCos.x, SpotConeCos.y, currentSpotConeCos);\n\
\n\
					// Is the position completly outside the spot cone?\n\
					if (currentSpotConeCosAttenuation <= 0) {\n\
						// Early escape: The position is outside the light cone and therefore not influcenced by the light\n\
						#ifdef FS_DISCARD\n\
							discard;\n\
						#else\n\
							gl_FragColor = vec4(0);\n\
							return;\n\
						#endif\n\
					}\n\
\n\
					// Modulate the color of the light using the current smooth spot cone attenuation\n\
					lightColor *= currentSpotConeCosAttenuation;\n\
				#else\n\
					// A position is inside or outside the spot cone, there is no 'in between'\n\
					if (SpotConeCos > currentSpotConeCos) {\n\
						// Early escape: The position is outside the light cone and therefore not influcenced by the light\n\
						#ifdef FS_DISCARD\n\
							discard;\n\
						#else\n\
							gl_FragColor = vec4(0);\n\
							return;\n\
						#endif\n\
					}\n\
				#endif\n\
			#endif\n\
		#endif\n\
	#endif\n\
\n\
	// Perform the lighting calculation in view space\n\
	highp vec3 normal    = decodeNormalVector(sampleRT1.rg);\n\
	highp vec4 sampleRT0 = texture2DRect(RenderTargetTexture0, VertexTexCoordVS);\n\
	highp vec4 sampleRT2 = texture2DRect(RenderTargetTexture2, VertexTexCoordVS);\n\
	#ifdef FS_NO_ALBEDO\n\
		sampleRT0.rgb = vec3(1);	// Set to default\n\
	#endif\n\
	#ifdef FS_NO_SPECULARCOLOR\n\
		sampleRT2.rgb = vec3(1);	// Set to default\n\
	#endif\n\
	#ifdef FS_NO_SPECULAREXPONENT\n\
		sampleRT2.a = vec3(45);		// Set to default\n\
	#endif\n\
	gl_FragColor.rgb = BlinnPhong(normalize(lightVector), lightColor, -normalize(position), normal, sampleRT0.rgb, sampleRT2.rgb, sampleRT2.a);\n\
\n\
	// Apply attenuation\n\
	gl_FragColor.rgb *= clamp(1 - distance/LightRadius, 0.0f, 1.0f);\n\
#endif\n\
\n\
	// Apply ambient occlusion or the calculated realtime shadow\n\
#ifdef FS_NO_AMBIENTOCCLUSION\n\
	// Just modulate the calculated lighting color with the calculated shadowing\n\
	gl_FragColor.rgb *= vec3(shadow);\n\
#else\n\
	// Modulate the calculated lighting color with the calculated shadowing or the ambient occlusion value, do not multiply both shadow values\n\
	gl_FragColor.rgb *= vec3(min(sampleRT0.a, shadow));\n\
#endif\n\
\n\
	// Still here? Write any alpha value so all color components were written.\n\
	gl_FragColor.a = 1;\n\
}";
