/*********************************************************\
 *  File: HDRLightAdaptation.h                           *
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


#ifndef __PLCOMPOSITING_HDR_LIGHTADAPTION_H__
#define __PLCOMPOSITING_HDR_LIGHTADAPTION_H__
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "PLCompositing/PLCompositing.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace PLRenderer {
	class Program;
	class VertexShader;
	class TextureBuffer;
	class ProgramUniform;
	class FragmentShader;
	class TextureBuffer2D;
	class ProgramAttribute;
	class SurfaceTextureBuffer;
	class TextureBufferRectangle;
}
namespace PLScene {
	class FullscreenQuad;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace PLCompositing {


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    HDR light adaptation component
*
*  @note
*    - The Pattanaik technique described in http://www.coretechniques.info/PostFX.zip is used
*/
class HDRLightAdaptation {


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] cRenderer
		*    Renderer to use
		*/
		PLCOM_API HDRLightAdaptation(PLRenderer::Renderer &cRenderer);

		/**
		*  @brief
		*    Destructur
		*/
		PLCOM_API ~HDRLightAdaptation();

		/**
		*  @brief
		*    Calculates the light adaptation
		*
		*  @param[in] sShaderLanguage
		*    Shader language to use (for example "GLSL" or "Cg"), if empty string, the default shader language of the renderer will be used, don't change the shader language on each call (performance!)
		*  @param[in] cAverageLuminanceTextureBuffer2D
		*    Current average luminance texture buffer
		*  @param[in] fTauTimeDifference
		*    Tau multiplied with the current time difference
		*
		*  @note
		*    - Use GetTextureBuffer() to receive the result of the calculation
		*/
		PLCOM_API void CalculateLightAdaptation(const PLGeneral::String &sShaderLanguage, PLRenderer::TextureBuffer &cAverageLuminanceTextureBuffer2D, float fTauTimeDifference);

		/**
		*  @brief
		*    Returns the 1x1 2D texture buffer storing the light adaptation
		*
		*  @return
		*    The 1x1 2D texture buffer storing the light adaptation, NULL on error
		*
		*  @note
		*    - The texture buffer is only valid after CalculateLightAdaptation() was called
		*/
		PLCOM_API PLRenderer::TextureBuffer *GetTextureBuffer() const;


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Called when a program became dirty
		*
		*  @param[in] pProgram
		*    Program which became dirty
		*/
		void OnDirty(PLRenderer::Program *pProgram);


	//[-------------------------------------------------------]
	//[ Private event handlers                                ]
	//[-------------------------------------------------------]
	private:
		PLCore::EventHandler<PLRenderer::Program*> EventHandlerDirty;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		PLRenderer::Renderer			 *m_pRenderer;							/**< Renderer to use, always valid! */
		PLScene::FullscreenQuad			 *m_pFullscreenQuad;					/**< Fullscreen quad instance, can be NULL */
		PLRenderer::VertexShader		 *m_pVertexShader;						/**< Vertex shader, can be NULL */
		PLRenderer::FragmentShader		 *m_pFragmentShader;					/**< Fragment shader, can be NULL */
		PLRenderer::Program				 *m_pProgram;							/**< GPU program, can be NULL */
		PLRenderer::ProgramAttribute	 *m_pPositionProgramAttribute;			/**< Position program attribute */
		PLRenderer::ProgramUniform		 *m_pFactorProgramUniform;				/**< Factor program uniform */
		PLRenderer::ProgramUniform		 *m_pPreviousTextureProgramUniform;		/**< Previous texture program uniform */
		PLRenderer::ProgramUniform		 *m_pCurrentTextureProgramUniform;		/**< Current texture program uniform */
		PLRenderer::SurfaceTextureBuffer *m_pLightAdaptationTextureBuffer2D[2];	/**< 1x1 2D texture buffer storing the previous and current light adaptation */
		bool							  m_bPreviousIndex;						/**< Index of the previous 2D light adaptation texture buffer */


};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // PLCompositing


#endif // __PLCOMPOSITING_HDR_LIGHTADAPTION_H__
