/*********************************************************\
 *  File: SRPDeferredGlow.cpp                            *
 *
 *  Copyright (C) 2002-2010 The PixelLight Team (http://www.pixellight.org/)
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


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <PLRenderer/RendererContext.h>
#include <PLRenderer/Renderer/Program.h>
#include <PLRenderer/Renderer/VertexShader.h>
#include <PLRenderer/Renderer/ProgramUniform.h>
#include <PLRenderer/Renderer/ShaderLanguage.h>
#include <PLRenderer/Renderer/ProgramAttribute.h>
#include <PLRenderer/Renderer/FragmentShader.h>
#include <PLRenderer/Renderer/TextureBufferRectangle.h>
#include <PLRenderer/Effect/EffectManager.h>
#include "PLCompositing/FullscreenQuad.h"
#include "PLCompositing/Shaders/Deferred/SRPDeferredGBuffer.h"
#include "PLCompositing/Shaders/Deferred/SRPDeferredGlow.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
using namespace PLGeneral;
using namespace PLMath;
using namespace PLRenderer;
using namespace PLScene;
namespace PLCompositing {


//[-------------------------------------------------------]
//[ RTTI interface                                        ]
//[-------------------------------------------------------]
pl_implement_class(SRPDeferredGlow)


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
/**
*  @brief
*    Default constructor
*/
SRPDeferredGlow::SRPDeferredGlow() :
	EventHandlerDirty(&SRPDeferredGlow::OnDirty, this),
	ShaderLanguage(this),
	GlowFactor(this),
	GlowBlurPasses(this),
	GlowDownscale(this),
	Flags(this),
	m_bResultIndex(0),
	m_pVertexShader(NULL),
	m_pDownscaleFragmentShader(NULL),
	m_pDownscaleProgram(NULL),
	m_pDownscalePositionProgramAttribute(NULL),
	m_pDownscaleTextureSizeProgramUniform(NULL),
	m_pDownscaleTextureProgramUniform(NULL),
	m_pBlurFragmentShader(NULL),
	m_pBlurProgram(NULL),
	m_pBlurPositionProgramAttribute(NULL),
	m_pBlurTextureSizeProgramUniform(NULL),
	m_pBlurUVScaleProgramUniform(NULL),
	m_pBlurTextureProgramUniform(NULL),
	m_pResultFragmentShader(NULL),
	m_pResultProgram(NULL),
	m_pResultPositionProgramAttribute(NULL),
	m_pResultTextureSizeProgramUniform(NULL),
	m_pResultGlowFactorProgramUniform(NULL),
	m_pResultTextureProgramUniform(NULL),
	m_bResultDiscard(true)
{
	// Init data
	m_pRenderTarget[0] = m_pRenderTarget[1] = NULL;
}

/**
*  @brief
*    Destructor
*/
SRPDeferredGlow::~SRPDeferredGlow()
{
	// Destroy the render targets
	for (int i=0; i<2; i++) {
		if (m_pRenderTarget[i])
			delete m_pRenderTarget[i];
	}

	// Destroy the downscale stuff
	if (m_pDownscaleProgram)
		delete m_pDownscaleProgram;
	if (m_pDownscaleFragmentShader)
		delete m_pDownscaleFragmentShader;

	// Destroy the blur stuff
	if (m_pBlurProgram)
		delete m_pBlurProgram;
	if (m_pBlurFragmentShader)
		delete m_pBlurFragmentShader;

	// Destroy the result stuff
	if (m_pResultProgram)
		delete m_pResultProgram;
	if (m_pResultFragmentShader)
		delete m_pResultFragmentShader;

	// Destroy the vertex shader
	if (m_pVertexShader)
		delete m_pVertexShader;
}


//[-------------------------------------------------------]
//[ Private functions                                     ]
//[-------------------------------------------------------]
/**
*  @brief
*    Calculates the glow
*/
void SRPDeferredGlow::CalculateGlow(const String &sShaderLanguage, VertexBuffer &cVertexBuffer, TextureBufferRectangle &cOriginalTexture, float fBrightThreshold, uint32 nBlurPasses, float fDownscale)
{
	// Get the internal texture format to use
	const TextureBuffer::EPixelFormat nInternalFormat = cOriginalTexture.GetFormat();

	// Get the renderer instance
	Renderer &cRenderer = cOriginalTexture.GetRenderer();

	// Backup current render target
	Surface *pSurfaceBackup = cRenderer.GetRenderTarget();

	{ // Create/update the two render targets
		// Get the size of the original HDR texture
		const Vector2i vRTSize = cOriginalTexture.GetSize()/fDownscale;
		if (vRTSize.x != 0 && vRTSize.y != 0) {
			for (int i=0; i<2; i++) {
				// Render target size change?
				if (m_pRenderTarget[i] && (m_pRenderTarget[i]->GetSize() != vRTSize || m_pRenderTarget[i]->GetFormat() != nInternalFormat)) {
					// Destroy the downscale render target
					if (m_pRenderTarget[i]) {
						delete m_pRenderTarget[i];
						m_pRenderTarget[i] = NULL;
					}
				}

				// Create the downscale render target right now?
				if (!m_pRenderTarget[i])
					m_pRenderTarget[i] = cRenderer.CreateSurfaceTextureBufferRectangle(vRTSize, nInternalFormat, SurfaceTextureBuffer::NoMultisampleAntialiasing);
			}
		}
	}

	// Create the shaders and programs right now?
	if (!m_pVertexShader || m_pVertexShader->GetShaderLanguage() != sShaderLanguage) {
		// If there's an previous instance of the program, destroy it first
		if (m_pDownscaleProgram) {
			delete m_pDownscaleProgram;
			m_pDownscaleProgram = NULL;
		}
		if (m_pDownscaleFragmentShader) {
			delete m_pDownscaleFragmentShader;
			m_pDownscaleFragmentShader = NULL;
		}
		if (m_pBlurProgram) {
			delete m_pBlurProgram;
			m_pBlurProgram = NULL;
		}
		if (m_pBlurFragmentShader) {
			delete m_pBlurFragmentShader;
			m_pBlurFragmentShader = NULL;
		}
		if (m_pVertexShader) {
			delete m_pVertexShader;
			m_pVertexShader = NULL;
		}
		m_pDownscalePositionProgramAttribute	= NULL;
		m_pDownscaleTextureSizeProgramUniform	= NULL;
		m_pDownscaleTextureProgramUniform		= NULL;
		m_pBlurPositionProgramAttribute			= NULL;
		m_pBlurTextureSizeProgramUniform		= NULL;
		m_pBlurUVScaleProgramUniform			= NULL;
		m_pBlurTextureProgramUniform			= NULL;

		// Shader source code
		String sVertexShaderSourceCode;
		String sFragmentShaderSourceCode_Downscale;
		String sFragmentShaderSourceCode_Blur;
		if (sShaderLanguage == "GLSL") {
			#include "SRPDeferredGlow_GLSL.h"
			sVertexShaderSourceCode				= sDeferredGlow_GLSL_VS;
			sFragmentShaderSourceCode_Downscale	= sDeferredGlow_GLSL_FS_Downscale;
			sFragmentShaderSourceCode_Blur		= sDeferredGlow_GLSL_FS_Blur;
		} else if (sShaderLanguage == "Cg") {
			#include "SRPDeferredGlow_Cg.h"
			sVertexShaderSourceCode				= sDeferredGlow_Cg_VS;
			sFragmentShaderSourceCode_Downscale = sDeferredGlow_Cg_FS_Downscale;
			sFragmentShaderSourceCode_Blur		= sDeferredGlow_Cg_FS_Blur;
		}

		// Get the shader language instance
		PLRenderer::ShaderLanguage *pShaderLanguage = cRenderer.GetShaderLanguage(sShaderLanguage);
		if (pShaderLanguage) {
			// Create a vertex shader instance
			m_pVertexShader = pShaderLanguage->CreateVertexShader();
			if (m_pVertexShader) {
				// Set the vertex shader source code
				m_pVertexShader->SetSourceCode(sVertexShaderSourceCode);
			}

			// Create a fragment shader instance
			m_pDownscaleFragmentShader = pShaderLanguage->CreateFragmentShader();
			if (m_pDownscaleFragmentShader) {
				// Set the fragment shader source code
				m_pDownscaleFragmentShader->SetSourceCode(sFragmentShaderSourceCode_Downscale);
			}
			m_pBlurFragmentShader = pShaderLanguage->CreateFragmentShader();
			if (m_pBlurFragmentShader) {
				// Set the fragment shader source code
				m_pBlurFragmentShader->SetSourceCode(sFragmentShaderSourceCode_Blur);
			}

			// Create a program instance
			m_pDownscaleProgram = pShaderLanguage->CreateProgram();
			if (m_pDownscaleProgram) {
				// Assign the created vertex and fragment shaders to the program
				m_pDownscaleProgram->SetVertexShader(m_pVertexShader);
				m_pDownscaleProgram->SetFragmentShader(m_pDownscaleFragmentShader);

				// Add our nark which will inform us as soon as the program gets dirty
				m_pDownscaleProgram->EventDirty.Connect(&EventHandlerDirty);

				// Get attributes and uniforms
				OnDirty(m_pDownscaleProgram);
			}
			m_pBlurProgram = pShaderLanguage->CreateProgram();
			if (m_pBlurProgram) {
				// Assign the created vertex and fragment shaders to the program
				m_pBlurProgram->SetVertexShader(m_pVertexShader);
				m_pBlurProgram->SetFragmentShader(m_pBlurFragmentShader);

				// Add our nark which will inform us as soon as the program gets dirty
				m_pBlurProgram->EventDirty.Connect(&EventHandlerDirty);

				// Get attributes and uniforms
				OnDirty(m_pBlurProgram);
			}
		}
	}

	// First step: Downscale -> Make the downscale GPU program to the currently used one
	if (cRenderer.SetProgram(m_pDownscaleProgram)) {
		// Make the render target 0 to the current render target
		cRenderer.SetRenderTarget(m_pRenderTarget[0]);
		m_bResultIndex = 0;

		// Set program vertex attributes, this creates a connection between "Vertex Buffer Attribute" and "Vertex Shader Attribute"
		if (m_pDownscalePositionProgramAttribute)
			m_pDownscalePositionProgramAttribute->Set(&cVertexBuffer, PLRenderer::VertexBuffer::Position);

		// Set the "TextureSize" fragment shader parameter
		if (m_pDownscaleTextureSizeProgramUniform)
			m_pDownscaleTextureSizeProgramUniform->Set(cOriginalTexture.GetSize());

		// Set the "Texture" fragment shader parameter
		if (m_pDownscaleTextureProgramUniform) {
			const int nTextureUnit = m_pDownscaleTextureProgramUniform->Set(&cOriginalTexture);
			if (nTextureUnit >= 0) {
				cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressU, TextureAddressing::Wrap);
				cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressV, TextureAddressing::Wrap);
				cRenderer.SetSamplerState(nTextureUnit, Sampler::MagFilter, TextureFiltering::Linear);
				cRenderer.SetSamplerState(nTextureUnit, Sampler::MinFilter, TextureFiltering::Linear);
				cRenderer.SetSamplerState(nTextureUnit, Sampler::MipFilter, TextureFiltering::None);
			}
		}

		// Draw the fullscreen quad
		cRenderer.DrawPrimitives(Primitive::TriangleStrip, 0, 4);
	}

	// Second step: Gaussian convolution filter to glow - Make the bloom GPU program to the currently used one
	if (cRenderer.SetProgram(m_pBlurProgram)) {
		// Set program vertex attributes, this creates a connection between "Vertex Buffer Attribute" and "Vertex Shader Attribute"
		if (m_pBlurPositionProgramAttribute)
			m_pBlurPositionProgramAttribute->Set(&cVertexBuffer, PLRenderer::VertexBuffer::Position);

		// Set the "TextureSize" fragment shader parameter - both render targets have the same size
		if (m_pBlurTextureSizeProgramUniform)
			m_pBlurTextureSizeProgramUniform->Set(m_pRenderTarget[0]->GetSize());

		// Horizontal and vertical blur
		for (uint32 i=0; i<nBlurPasses; i++) {
			// Make the render target 1 to the current render target
			cRenderer.SetRenderTarget(m_pRenderTarget[!m_bResultIndex]);

			// Set the "UVScale" fragment shader parameter
			if (m_pBlurUVScaleProgramUniform) {
				if (i%2 != 0)
					m_pBlurUVScaleProgramUniform->Set(0.0f, 1.0f);
				else
					m_pBlurUVScaleProgramUniform->Set(1.0f, 0.0f);
			}

			// Set the "Texture" fragment shader parameter
			if (m_pBlurTextureProgramUniform) {
				const int nTextureUnit = m_pBlurTextureProgramUniform->Set(m_pRenderTarget[m_bResultIndex != 0]->GetTextureBuffer());
				if (nTextureUnit >= 0) {
					cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressU, TextureAddressing::Wrap);
					cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressV, TextureAddressing::Wrap);
					cRenderer.SetSamplerState(nTextureUnit, Sampler::MagFilter, TextureFiltering::Linear);
					cRenderer.SetSamplerState(nTextureUnit, Sampler::MinFilter, TextureFiltering::Linear);
					cRenderer.SetSamplerState(nTextureUnit, Sampler::MipFilter, TextureFiltering::None);
				}
			}

			// Draw the fullscreen quad
			cRenderer.DrawPrimitives(Primitive::TriangleStrip, 0, 4);

			// The result is now within the other render target
			m_bResultIndex = !m_bResultIndex;
		}
	}

	// Restore the previous render target
	cRenderer.SetRenderTarget(pSurfaceBackup);
}

/**
*  @brief
*    Returns the rectangle texture buffer storing the glow
*/
TextureBuffer *SRPDeferredGlow::GetTextureBuffer() const
{
	return m_pRenderTarget[m_bResultIndex] ? m_pRenderTarget[m_bResultIndex]->GetTextureBuffer() : NULL;
}

/**
*  @brief
*    Called when a program became dirty
*/
void SRPDeferredGlow::OnDirty(Program *pProgram)
{
	// Get attributes and uniforms
	if (pProgram == m_pDownscaleProgram) {
		m_pDownscalePositionProgramAttribute  = m_pDownscaleProgram->GetAttribute("VertexPosition");
		m_pDownscaleTextureSizeProgramUniform = m_pDownscaleProgram->GetUniform("TextureSize");
		m_pDownscaleTextureProgramUniform	  = m_pDownscaleProgram->GetUniform("Texture");
	} else if (pProgram == m_pBlurProgram) {
		m_pBlurPositionProgramAttribute  = m_pBlurProgram->GetAttribute("VertexPosition");
		m_pBlurTextureSizeProgramUniform = m_pBlurProgram->GetUniform("TextureSize");
		m_pBlurUVScaleProgramUniform	 = m_pBlurProgram->GetUniform("UVScale");
		m_pBlurTextureProgramUniform	 = m_pBlurProgram->GetUniform("Texture");
	} else if (pProgram == m_pResultProgram) {
		m_pResultPositionProgramAttribute  = m_pResultProgram->GetAttribute("VertexPosition");
		m_pResultTextureSizeProgramUniform = m_pResultProgram->GetUniform("TextureSize");
		m_pResultGlowFactorProgramUniform  = m_pResultProgram->GetUniform("GlowFactor");
		m_pResultTextureProgramUniform     = m_pResultProgram->GetUniform("Texture");
	}
}


//[-------------------------------------------------------]
//[ Private virtual PLScene::SceneRendererPass functions  ]
//[-------------------------------------------------------]
void SRPDeferredGlow::Draw(Renderer &cRenderer, const SQCull &cCullQuery)
{
	// Get the instance of the "PLCompositing::SRPDeferredGBuffer" scene renderer pass and check whether or not color target 3 has real information in it
	SRPDeferredGBuffer *pSRPDeferredGBuffer = GetGBuffer();
	if (pSRPDeferredGBuffer && pSRPDeferredGBuffer->IsColorTarget3Used() && pSRPDeferredGBuffer->IsColorTarget3AlphaUsed()) {
		// Get the fullscreen quad instance
		FullscreenQuad *pFullscreenQuad = pSRPDeferredGBuffer->GetFullscreenQuad();
		if (pFullscreenQuad) {
			// Get the vertex buffer of the fullscreen quad
			VertexBuffer *pVertexBuffer = pFullscreenQuad->GetVertexBuffer();
			if (pVertexBuffer) {
				// Get the texture buffer to use
				TextureBufferRectangle *pTextureBuffer = pSRPDeferredGBuffer->GetRenderTargetTextureBuffer(3);
				if (pTextureBuffer) {
					// Get the shader language to use
					String sShaderLanguage = ShaderLanguage;
					if (!sShaderLanguage.GetLength())
						sShaderLanguage = cRenderer.GetDefaultShaderLanguage();

					// Reset all render states to default
					cRenderer.GetRendererContext().GetEffectManager().Use();
					cRenderer.SetColorMask();

					// Setup renderer
					const uint32 nFixedFillModeBackup = cRenderer.GetRenderState(RenderState::FixedFillMode);
					cRenderer.SetRenderState(RenderState::ScissorTestEnable, false);
					cRenderer.SetRenderState(RenderState::FixedFillMode,	 Fill::Solid);
					cRenderer.SetRenderState(RenderState::CullMode,			 Cull::None);
					cRenderer.SetRenderState(RenderState::ZEnable,			 false);
					cRenderer.SetRenderState(RenderState::ZWriteEnable,		 false);

					// Calculate the glow texture buffer
					if (GlowDownscale < 1.0f)
						GlowDownscale = 1.0f;
					CalculateGlow(sShaderLanguage, *pVertexBuffer, *pTextureBuffer, 0.0f, GlowBlurPasses, GlowDownscale);

					// Get glow texture buffer
					pTextureBuffer = (TextureBufferRectangle*)GetTextureBuffer();
					if (pTextureBuffer) {
						// Create the shaders and programs right now?
						if (!m_pResultFragmentShader || m_pResultFragmentShader->GetShaderLanguage() != sShaderLanguage || m_bResultDiscard != !(GetFlags() & NoDiscard)) {
							// If there's an previous instance of the program, destroy it first
							if (m_pResultProgram) {
								delete m_pResultProgram;
								m_pResultProgram = NULL;
							}
							if (m_pResultFragmentShader) {
								delete m_pResultFragmentShader;
								m_pResultFragmentShader = NULL;
							}
							m_pResultPositionProgramAttribute	= NULL;
							m_pResultTextureSizeProgramUniform	= NULL;
							m_pResultGlowFactorProgramUniform	= NULL;
							m_pResultTextureProgramUniform		= NULL;
							m_bResultDiscard					= true;

							// Get the shader language instance
							PLRenderer::ShaderLanguage *pShaderLanguage = cRenderer.GetShaderLanguage(sShaderLanguage);
							if (pShaderLanguage) {
								// Shader source code
								String sVertexShaderSourceCode;
								String sFragmentShaderSourceCode;
								if (sShaderLanguage == "GLSL") {
									#include "SRPDeferredGlow_GLSL.h"
									sVertexShaderSourceCode	= sDeferredGlow_GLSL_VS;
									if (GetFlags() & NoDiscard)
										sFragmentShaderSourceCode = sDeferredGlow_GLSL_FS_Result;
									else
										sFragmentShaderSourceCode = "#define FS_DISCARD\\n" + sDeferredGlow_GLSL_FS_Result;
								} else if (sShaderLanguage == "Cg") {
									#include "SRPDeferredGlow_Cg.h"
									sVertexShaderSourceCode = sDeferredGlow_Cg_VS;
									if (GetFlags() & NoDiscard)
										sFragmentShaderSourceCode = sDeferredGlow_Cg_FS_Result;
									else
										sFragmentShaderSourceCode = "#define FS_DISCARD\\n" + sDeferredGlow_Cg_FS_Result;
								}

								// Create a fragment shader instance
								m_pResultFragmentShader = pShaderLanguage->CreateFragmentShader();
								if (m_pResultFragmentShader) {
									// Set the fragment shader source code
									m_pResultFragmentShader->SetSourceCode(sFragmentShaderSourceCode);
								}

								// Create a program instance
								m_pResultProgram = pShaderLanguage->CreateProgram();
								if (m_pResultProgram) {
									// Assign the created vertex and fragment shaders to the program
									m_pResultProgram->SetVertexShader(m_pVertexShader);
									m_pResultProgram->SetFragmentShader(m_pResultFragmentShader);

									// Add our nark which will inform us as soon as the program gets dirty
									m_pResultProgram->EventDirty.Connect(&EventHandlerDirty);

									// Get attributes and uniforms
									OnDirty(m_pResultProgram);

									// Backup whether or not discard is used
									m_bResultDiscard = !(GetFlags() & NoDiscard);
								}
							}
						}

						// Make the result GPU program to the currently used one
						if (cRenderer.SetProgram(m_pResultProgram)) {
							// Set program vertex attributes, this creates a connection between "Vertex Buffer Attribute" and "Vertex Shader Attribute"
							if (m_pResultPositionProgramAttribute)
								m_pResultPositionProgramAttribute->Set(pVertexBuffer, PLRenderer::VertexBuffer::Position);

							// Set the "TextureSize" fragment shader parameter
							if (m_pResultTextureSizeProgramUniform)
								m_pResultTextureSizeProgramUniform->Set(pTextureBuffer->GetSize());

							// Set the "GlowFactor" fragment shader parameter
							if (m_pResultGlowFactorProgramUniform)
								m_pResultGlowFactorProgramUniform->Set(GlowFactor);

							// Set the "Texture" fragment shader parameter
							if (m_pResultTextureProgramUniform) {
								const int nTextureUnit = m_pResultTextureProgramUniform->Set(pTextureBuffer);
								if (nTextureUnit >= 0) {
									cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressU, TextureAddressing::Clamp);
									cRenderer.SetSamplerState(nTextureUnit, Sampler::AddressV, TextureAddressing::Clamp);
									cRenderer.SetSamplerState(nTextureUnit, Sampler::MagFilter, TextureFiltering::Linear);
									cRenderer.SetSamplerState(nTextureUnit, Sampler::MinFilter, TextureFiltering::Linear);
									cRenderer.SetSamplerState(nTextureUnit, Sampler::MipFilter, TextureFiltering::None);
								}
							}

							// Set blend mode
							if (!(GetFlags() & NoBlending)) {
								cRenderer.SetRenderState(RenderState::BlendEnable,	true);
								cRenderer.SetRenderState(RenderState::SrcBlendFunc,	BlendFunc::One);
								cRenderer.SetRenderState(RenderState::DstBlendFunc,	BlendFunc::One);
							}

							// Draw the fullscreen quad
							pFullscreenQuad->Draw();
						}
					}

					// Restore fixed fill mode render state
					cRenderer.SetRenderState(RenderState::FixedFillMode, nFixedFillModeBackup);
				}
			}
		}
	}
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // PLCompositing
