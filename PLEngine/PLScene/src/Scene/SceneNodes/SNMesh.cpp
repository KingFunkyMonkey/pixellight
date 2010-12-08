/*********************************************************\
 *  File: SNMesh.cpp                                     *
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
#include <PLGeneral/Tools/Timing.h>
#include <PLGeneral/File/File.h>
#include <PLGeneral/File/Directory.h>
#include <PLGeneral/File/Url.h>
#include <PLGeneral/Log/Log.h>
#include <PLCore/Tools/LoadableType.h>
#include <PLCore/Tools/LoadableManager.h>
#include <PLCore/Application/ConsoleApplication.h>
#include <PLRenderer/RendererContext.h>
#include <PLRenderer/Renderer/FontManager.h>
#include <PLRenderer/Renderer/VertexBuffer.h>
#include <PLRenderer/Effect/EffectManager.h>
#include <PLRenderer/Material/MaterialManager.h>
#include <PLMesh/Skeleton.h>
#include <PLMesh/MeshManager.h>
#include <PLMesh/MeshLODLevel.h>
#include <PLMesh/MeshMorphTarget.h>
#include <PLMesh/SkeletonHandler.h>
#include "PLScene/Visibility/VisNode.h"
#include "PLScene/Scene/SNCamera.h"
#include "PLScene/Scene/SceneContext.h"
#include "PLScene/Scene/SceneNodes/Loader/SkinLoader.h"
#include "PLScene/Scene/SceneNodes/SNMesh.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
using namespace PLGeneral;
using namespace PLCore;
using namespace PLMath;
using namespace PLGraphics;
using namespace PLRenderer;
using namespace PLMesh;
namespace PLScene {


//[-------------------------------------------------------]
//[ RTTI interface                                        ]
//[-------------------------------------------------------]
pl_implement_class(SNMesh)


//[-------------------------------------------------------]
//[ Public definitions                                    ]
//[-------------------------------------------------------]
const String SNMesh::IconMesh = "Data/Textures/IconMesh.dds";


//[-------------------------------------------------------]
//[ Public RTTI get/set functions                         ]
//[-------------------------------------------------------]
String SNMesh::GetMesh() const
{
	return m_sMesh;
}

void SNMesh::SetMesh(const String &sValue)
{
	if (m_sMesh != sValue) {
		m_sMesh = sValue;

		// Is this node visible?
		if (((GetFlags() & LoadAtOnce) || IsVisible()))
			LoadMesh(m_sMesh);
	}
}

String SNMesh::GetSkin() const
{
	return m_sSkin;
}

void SNMesh::SetSkin(const String &sValue)
{
	if (m_sSkin != sValue) {
		m_sSkin = sValue;
		LoadSkin(m_sSkin);
	}
}

bool SNMesh::GetStaticMesh() const
{
	return m_bStaticMesh;
}

void SNMesh::SetStaticMesh(bool bValue)
{
	if (m_bStaticMesh != bValue) {
		m_bStaticMesh = bValue;

		// Is this node visible?
		if ((GetFlags() & LoadAtOnce) || IsVisible())
			LoadMesh(m_sMesh);
	}
}

bool SNMesh::GetBuildInfo() const
{
	return m_bBuildInfo;
}

void SNMesh::SetBuildInfo(bool bValue)
{
	if (m_bBuildInfo != bValue) {
		m_bBuildInfo = bValue;

		// Is this node visible?
		if ((GetFlags() & LoadAtOnce) || IsVisible())
			LoadMesh(m_sMesh);
	}
}

bool SNMesh::GetCalculateNormals() const
{
	return m_bCalculateNormals;
}

void SNMesh::SetCalculateNormals(bool bValue)
{
	if (m_bCalculateNormals != bValue) {
		m_bCalculateNormals = bValue;

		// Is this node visible?
		if ((GetFlags() & LoadAtOnce) || IsVisible())
			LoadMesh(m_sMesh);
	}
}

bool SNMesh::GetCalculateTSVs() const
{
	return m_bCalculateTSVs;
}

void SNMesh::SetCalculateTSVs(bool bValue)
{
	if (m_bCalculateTSVs != bValue) {
		m_bCalculateTSVs = bValue;

		// Is this node visible?
		if ((GetFlags() & LoadAtOnce) || IsVisible())
			LoadMesh(m_sMesh);
	}
}

bool SNMesh::GetGenerateStrips() const
{
	return m_bGenerateStrips;
}

void SNMesh::SetGenerateStrips(bool bValue)
{
	if (m_bGenerateStrips != bValue) {
		m_bGenerateStrips = bValue;

		// Is this node visible?
		if ((GetFlags() & LoadAtOnce) || IsVisible())
			LoadMesh(m_sMesh);
	}
}


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
/**
*  @brief
*    Default constructor
*/
SNMesh::SNMesh() :
	Mesh(this),
	Skin(this),
	StaticMesh(this),
	BuildInfo(this),
	CalculateNormals(this),
	CalculateTSVs(this),
	GenerateStrips(this),
	Icon(this),
	Flags(this),
	DebugFlags(this),
	m_bStaticMesh(true),
	m_bBuildInfo(false),
	m_bCalculateNormals(false),
	m_bCalculateTSVs(false),
	m_bGenerateStrips(false),
	m_pMeshHandler(NULL)
{
}

/**
*  @brief
*    Destructor
*/
SNMesh::~SNMesh()
{
	// Unload the used mesh
	UnloadMesh();
}

/**
*  @brief
*    Loads a mesh
*/
bool SNMesh::LoadMesh(const String &sFilename, const String &sParams, const String &sMethod)
{
	// Check the parameter
	if (sFilename.GetLength()) {
		// Create the mesh handler, if there's currently none
		if (m_pMeshHandler) {
			// Unload the old mesh
			UnloadMesh();
		}
		if (!m_pMeshHandler)
			m_pMeshHandler = new MeshHandler();

		// Mesh already loaded?
		PLMesh::Mesh *pMesh = GetSceneContext()->GetMeshManager().Get(sFilename);
		if (!pMesh) {
			// Create and load mesh
			pMesh = GetSceneContext()->GetMeshManager().LoadMesh(sFilename, sParams, sMethod, false, m_bStaticMesh);
			if (pMesh) {
				// Perform some mesh operations
				if (m_bCalculateNormals || m_bCalculateTSVs || m_bGenerateStrips) {
					MeshMorphTarget *pMorphTarget = pMesh->GetMorphTarget(0);
					VertexBuffer *pVertexBuffer = pMorphTarget ? pMorphTarget->GetVertexBuffer() : NULL;
					if (pVertexBuffer) {
						// Calculate the normal vectors
						if (m_bCalculateNormals && !pVertexBuffer->GetData(0, VertexBuffer::Normal))
							pMesh->CalculateNormals();

						// Calculate the tangent space vectors
						if (m_bCalculateTSVs && (!pVertexBuffer->GetData(0, VertexBuffer::Tangent) ||
							!pVertexBuffer->GetData(0, VertexBuffer::Binormal)))
							pMesh->CalculateTangentSpaceVectors();

						// Optimize geometries
						if (m_bGenerateStrips && pMesh->GetLODLevel(0))
							pMesh->GetLODLevel(0)->GenerateStrips();
					}
				}
			}
		}
		if (pMesh) {
			// Assign this mesh to the mesh handler
			m_pMeshHandler->SetMesh(pMesh);

			// Load a special skin?
			if (m_sSkin.GetLength())
				LoadSkin(m_sSkin);

			// Call mesh initialization function
			MeshInitFunction();

			// Done
			return true;
		}
	}

	// Error!
	return false;
}

/**
*  @brief
*    Loads a skin
*/
bool SNMesh::LoadSkin(const String &sFilename, const String &sParams, const String &sMethod)
{
	// Set skin filename
	m_sSkin = sFilename;

	// Is there a mesh handler?
	if (m_pMeshHandler) {
		// Get the used mesh
		PLMesh::Mesh *pMesh = m_pMeshHandler->GetMesh();
		if (!pMesh)
			return false; // Error!

		// Use the default materials of the used mesh?
		if (m_sSkin.GetLength()) {
			// Get file extension
			String sExtension = Url(sFilename).GetExtension();
			if (sExtension.GetLength()) {
				// Supported skin file given?
				if (LoadableManager::GetInstance()->IsFormatLoadSupported(sExtension, "Skin")) {
					PL_LOG(Debug, "Load skin: " + sFilename)

					// Get loadable type
					const LoadableType *pLoadableType = LoadableManager::GetInstance()->GetType("Skin");
					if (pLoadableType) {
						// Get loader
						Loader *pLoader = pLoadableType->GetLoader(sExtension);
						if (pLoader) {
							if (pLoader->IsInstanceOf("PLScene::SkinLoader")) {
								if (pLoader->CanLoad()) {
									// Open the file
									File cFile;
									if (pLoader->OpenFile(cFile, sFilename, false)) {
										// Get method name
										static const String sLoad = "Load";
										static const String sLoadParams = "LoadParams";
										String sMethodName = sMethod;
										if (!sMethodName.GetLength())
											sMethodName = sParams.GetLength() ? sLoadParams : sLoad;

										// Finally, load the skin!
										if (sParams.GetLength()) {
											pLoader->CallMethod(sMethodName, "Param0=\"" + Type<SNMesh&>::ConvertToString(*this) + "\" Param1=\"" + Type<File&>::ConvertToString(cFile) + "\" " + sParams);
											return true;
										} else {
											Params<bool, SNMesh&, File&> cParams(*this, cFile);
											pLoader->CallMethod(sMethodName, cParams);
											return cParams.Return;
										}
									} else {
										PL_LOG(Error, "Can't open the file '" + sFilename + "' to load in the loadable 'Skin'!")
									}
								} else {
									PL_LOG(Error, "Can't load the loadable 'Skin' from '" + sFilename + "' because loading of this file format is not supported!")
								}
							} else {
								PL_LOG(Error, "Can't load the loadable 'Skin' from '" + sFilename + "' because the file format is no valid 'Skin' format!")
							}
						} else {
							PL_LOG(Error, "Can't load the loadable 'Skin' from '" + sFilename + "' because the file format is not supported!")
						}
					} else {
						PL_LOG(Error, "Can't load '" + sFilename + "' because the loadable type 'Skin' is unknown!")
					}

				// Just replace all materials... 
				} else {
					Material *pMaterial = GetSceneContext()->GetRendererContext().GetMaterialManager().LoadResource(sFilename);
					if (pMaterial) {
						for (uint32 i=0; i<pMesh->GetNumOfMaterials(); i++)
							m_pMeshHandler->SetMaterial(i, pMaterial);

						// Done
						return true;
					}
				}
			} else {
				PL_LOG(Error, "Can't load the loadable 'Skin' from '" + sFilename + "' because there's no filename extension!")
			}
		} else {
			for (uint32 i=0; i<pMesh->GetNumOfMaterials(); i++)
				m_pMeshHandler->SetMaterial(i, pMesh->GetMaterial(i));

			// Done
			return true;
		}
	}

	// Error!
	return false;
}

/**
*  @brief
*    Saves the skin
*/
bool SNMesh::SaveSkin(const String &sFilename, const String &sParams, const String &sMethod)
{
	// Get the filename extension
	const String sExtension = Url(sFilename).GetExtension();
	if (!sExtension.GetLength())
		return false; // Error!

	// Supported skin file given?
	if (LoadableManager::GetInstance()->IsFormatSaveSupported(sExtension, "Skin")) {
		PL_LOG(Debug, "Save skin: " + sFilename)

		// Get loadable type
		LoadableType *pLoadableType = LoadableManager::GetInstance()->GetType("Skin");
		if (!pLoadableType) {
			PL_LOG(Error, "Can't save '" + sFilename + "' because the loadable type 'Skin' is unknown!")

			// Error!
			return false;
		}

		// Get loader
		Loader *pLoader = pLoadableType->GetLoader(sExtension);
		if (!pLoader) {
			PL_LOG(Error, "Can't save the loadable 'Skin' from '" + sFilename + "' because the file format is not supported!")

			// Error!
			return false;
		}
		if (!pLoader->IsInstanceOf("PLScene::SkinLoader")) {
			PL_LOG(Error, "Can't save the loadable 'Skin' from '" + sFilename + "' because the file format is no valid 'Skin' format!")

			// Error!
			return false;
		}

		// Try to open the file directly
		File cFile(sFilename);

		// Open the file itself
		if (cFile.Open(File::FileWrite | File::FileCreate)) {
			// Get method name
			static const String sSave = "Save";
			static const String sSaveParams = "SaveParams";
			String sMethodName = sMethod;
			if (!sMethodName.GetLength())
				sMethodName = sParams.GetLength() ? sSaveParams : sSave;

			// Finally, load the skin!
			if (sParams.GetLength()) {
				pLoader->CallMethod(sMethodName, "Param0=\"" + Type<SNMesh&>::ConvertToString(*this) + "\" Param1=\"" + Type<File&>::ConvertToString(cFile) + "\" " + sParams);
				return true;
			} else {
				Params<bool, SNMesh&, File&> cParams(*this, cFile);
				pLoader->CallMethod(sMethodName, cParams);
				return cParams.Return;
			}
		}
	}

	// Error!
	return false;
}

/**
*  @brief
*    Unloads the current mesh
*/
bool SNMesh::UnloadMesh()
{
	if (m_pMeshHandler && m_pMeshHandler->GetMesh()) {
		// Call mesh de-initialization function
		MeshDeInitFunction();

		// Clear handler
		m_pMeshHandler->SetMesh();
		delete m_pMeshHandler;
		m_pMeshHandler = NULL;
	}

	// Done
	return true;
}

/*
*  @brief
*    Returns the world position of a skeleton joint
*/
bool SNMesh::GetSkeletonJointWorldPosition(const String &sJointName, Vector3 &vPosition)
{
	// Get the mesh handler
	MeshHandler *pMeshHandler = GetMeshHandler();
	if (pMeshHandler) {
		// Is there a skeleton handler?
		SkeletonHandler *pSkeletonHandler = pMeshHandler->GetSkeletonHandler();
		if (pSkeletonHandler) {
			// Has this skeleton handler a skeleton resource?
			Skeleton *pSkeleton = pSkeletonHandler->GetResource();
			if (pSkeleton) {
				// Is there a skeleton joint with the given name?
				Joint *pJoint = pSkeleton->Get(sJointName);
				if (pJoint) {
					// Get skeleton handler
					Array<JointHandler> &lstJointHandlers = pSkeletonHandler->GetJointHandlers();
					uint32 nID = pJoint->GetID();
					JointHandler &cJointHandler = lstJointHandlers[nID];
					if (&cJointHandler != &Array<JointHandler>::Null) {
						// ... AND finally we can calculate the world position of the given joint
						vPosition = GetTransform().GetMatrix()*cJointHandler.GetTranslationAbsolute();

						// Done
						return true;
					}
				}
			}
		}
	}

	// Error!
	return false;
}


//[-------------------------------------------------------]
//[ Protected virtual functions                           ]
//[-------------------------------------------------------]
/**
*  @brief
*    This function is called after the mesh handler loads a mesh
*/
void SNMesh::MeshInitFunction()
{
	// Setup scene node bounding box
	MeshHandler *pMeshHandler = GetMeshHandler();
	if (pMeshHandler && pMeshHandler->GetResource()) {
		// Get and set the mesh bounding box automatically?
		if (!(GetFlags() & NoAutomaticBoundingBox)) {
			AABoundingBox cAABB;
			pMeshHandler->GetResource()->GetBoundingBox(cAABB.vMin, cAABB.vMax);
			if (cAABB.vMin.IsNull() && cAABB.vMax.IsNull()) {
				// We need to calculate the bounding box of the mesh :(
				pMeshHandler->GetResource()->CalculateBoundingBox(cAABB.vMin, cAABB.vMax);

				// Set a valid bounding box of the mesh so we do not need to calculate it again...
				pMeshHandler->GetResource()->SetBoundingBox(cAABB.vMin, cAABB.vMax);
			}
			SetAABoundingBox(cAABB);
		}
	}
}

/**
*  @brief
*    This function is called after the mesh handler unloads a mesh
*/
void SNMesh::MeshDeInitFunction()
{
	// The default implementation is empty
}


//[-------------------------------------------------------]
//[ Public virtual SceneNode functions                    ]
//[-------------------------------------------------------]
MeshHandler *SNMesh::GetMeshHandler()
{
	// Do we need to try to load in the mesh right now?
	if (!m_pMeshHandler)
		LoadMesh(m_sMesh);

	// Return the mesh handler... if there's one
	return m_pMeshHandler;
}

void SNMesh::DrawDebug(Renderer &cRenderer, const VisNode *pVisNode)
{
	// Call base implementation
	SceneNode::DrawDebug(cRenderer, pVisNode);

	// Is there a mesh handler?
	if (pVisNode && m_pMeshHandler) {
		// Draw anything?
		uint32 nDebugFlags = GetDebugFlags();
		if ((nDebugFlags & DebugShowWireframe) ||
			(nDebugFlags & DebugShowVertices) ||
			(nDebugFlags & DebugShowTangents) ||
			(nDebugFlags & DebugShowBinormals) ||
			(nDebugFlags & DebugShowNormals) ||
			(nDebugFlags & DebugShowVertexNumbers) ||
			(nDebugFlags & DebugShowJoints) ||
			(nDebugFlags & DebugShowJointNames) ||
			(nDebugFlags & DebugShowSkeleton) ||
			(nDebugFlags & DebugShowAnchorPoints)) {
			// Set render states
			cRenderer.GetRendererContext().GetEffectManager().Use();
			cRenderer.SetRenderState(RenderState::ZEnable,      false);
			cRenderer.SetRenderState(RenderState::ZWriteEnable, false);

			// Draw wireframe?
			if (nDebugFlags & DebugShowWireframe) {
				uint32 nFixedFillModeT = cRenderer.GetRenderState(RenderState::FixedFillMode);
				cRenderer.SetRenderState(RenderState::FixedFillMode, Fill::Line);
				m_pMeshHandler->Draw(false, false);
				cRenderer.SetRenderState(RenderState::FixedFillMode, nFixedFillModeT);
			}

			// Draw vertices?
			if (nDebugFlags & DebugShowVertices)
				m_pMeshHandler->DrawVertices(Color4::White);

			// Get normal scale value
			ConsoleApplication *pApplication = ConsoleApplication::GetApplication();
			const float fNormalScale = (pApplication ? pApplication->GetConfig().GetVar("PLScene::EngineDebugConfig", "NormalScale").GetFloat() : 1.0f);

			// Draw tangents? (red = x/u)
			if (nDebugFlags & DebugShowTangents)
				m_pMeshHandler->DrawTangents(Color4::Red, pVisNode->GetWorldViewProjectionMatrix(), fNormalScale, 1.0f);

			// Draw binormals? (green = y/v)
			if (nDebugFlags & DebugShowBinormals)
				m_pMeshHandler->DrawBinormals(Color4::Green, pVisNode->GetWorldViewProjectionMatrix(), fNormalScale, 1.0f);

			// Draw normals? (blue = z/w)
			if (nDebugFlags & DebugShowNormals)
				m_pMeshHandler->DrawNormals(Color4::Blue, pVisNode->GetWorldViewProjectionMatrix(), fNormalScale, 1.0f);

			// Draw vertex numbers?
			if (nDebugFlags & DebugShowVertexNumbers) {
				Font *pFont = (Font*)cRenderer.GetFontManager().GetDefaultFontTexture();
				if (pFont)
					m_pMeshHandler->DrawVertexNumbers(*pFont, Color4::White, pVisNode->GetWorldViewProjectionMatrix());
			}

			// Draw skeleton?
			if ((nDebugFlags & DebugShowJoints) || (nDebugFlags & DebugShowJointNames) || (nDebugFlags & DebugShowSkeleton)) {
				SkeletonHandler *pSkeletonHandler = m_pMeshHandler->GetSkeletonHandler();
				if (pSkeletonHandler) {
					Skeleton *pSkeleton = pSkeletonHandler->GetResource();
					if (pSkeleton) {
						// Draw joints?
						if (nDebugFlags & DebugShowJoints)
							pSkeletonHandler->DrawJoints(cRenderer, Color4::Green, pVisNode->GetWorldViewProjectionMatrix(), 4.0f);

						// Draw joint names?
						if (nDebugFlags & DebugShowJointNames) {
							Font *pFont = (Font*)cRenderer.GetFontManager().GetDefaultFontTexture();
							if (pFont)
								pSkeletonHandler->DrawJointNames(*pFont, Color4::Green, pVisNode->GetWorldViewProjectionMatrix());
						}

						// Draw skeleton?
						if (nDebugFlags & DebugShowSkeleton)
							pSkeletonHandler->DrawSkeleton(cRenderer, Color4::Blue, pVisNode->GetWorldViewProjectionMatrix(), 1.0f);
					}
				}
			}

			// Draw anchor points?
			if (nDebugFlags & DebugShowAnchorPoints) {
				Font *pFont = (Font*)cRenderer.GetFontManager().GetDefaultFontTexture();
				if (pFont)
					m_pMeshHandler->DrawAnchorPoints(*pFont, Color4::Green, pVisNode->GetWorldViewProjectionMatrix());
			}
		}
	}
}


//[-------------------------------------------------------]
//[ Protected virtual SceneNode functions                 ]
//[-------------------------------------------------------]
void SNMesh::DeInitFunction()
{
	// Unload the used mesh... we do this in here while virtual functions still work correctly...
	UnloadMesh();
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // PLScene
