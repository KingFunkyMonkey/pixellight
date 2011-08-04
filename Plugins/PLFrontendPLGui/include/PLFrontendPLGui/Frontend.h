/*********************************************************\
 *  File: Frontend.h                                     *
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


#ifndef __PLFRONTENDPLGUI_FRONTEND_H__
#define __PLFRONTENDPLGUI_FRONTEND_H__
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <PLCore/Base/Event/EventHandler.h>
#include <PLCore/Frontend/FrontendImpl.h>
#include <PLCore/Frontend/FrontendPixelLight.h>
#include "PLFrontendPLGui/PLFrontendPLGui.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace PLGui {
	class Widget;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace PLFrontendPLGui {


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    PLGui frontend implementation class
*
*  @remarks
*    An frontend class for a typical GUI application. Runs a
*    main loop that does the necessary message procressing and
*    provides methods to manage a main application window.
*/
class Frontend : public PLCore::FrontendImpl {


	//[-------------------------------------------------------]
	//[ RTTI interface                                        ]
	//[-------------------------------------------------------]
	pl_class(PLFRONTENDPLGUI_RTTI_EXPORT, Frontend, "PLFrontendPLGui", PLCore::FrontendImpl, "PLGui frontend implementation class")
		// Constructors
		pl_constructor_0(DefaultConstructor,	"Default constructor",	"")
	pl_class_end


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		PLFRONTENDPLGUI_API Frontend();

		/**
		*  @brief
		*    Destructor
		*/
		PLFRONTENDPLGUI_API virtual ~Frontend();

		/**
		*  @brief
		*    Get main window
		*
		*  @return
		*    Main window, can be a null pointer
		*/
		PLFRONTENDPLGUI_API PLGui::Widget *GetMainWindow() const;

		/**
		*  @brief
		*    Set main window
		*
		*  @param[in] pMainWindow
		*    Pointer to the main window of the application (a null pointer is also valid)
		*/
		PLFRONTENDPLGUI_API void SetMainWindow(PLGui::Widget *pMainWindow);


	//[-------------------------------------------------------]
	//[ Public virtual PLCore::FrontendImpl functions         ]
	//[-------------------------------------------------------]
	public:
		virtual PLCore::handle GetNativeWindowHandle() const override;
		virtual void Ping() override;


	//[-------------------------------------------------------]
	//[ Protected virtual PLCore::FrontendImpl functions      ]
	//[-------------------------------------------------------]
	protected:
		virtual int Run(const PLCore::String &sApplicationClass, const PLCore::String &sExecutableFilename, const PLCore::Array<PLCore::String> &lstArguments) override;
		virtual void Redraw() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Frontend functions                  ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Called when application should open it's main window
		*
		*  @remarks
		*    Use this function to create a main window and set as pointer to it using SetMainWindow().
		*    The default implementation does the following tasks:
		*    - If a GUI filename is provided, create a window by loading that file
		*    - Otherwise, create an empty top-level window
		*
		*  @note
		*    - "OnCreateMainWindow()": The created main window automatically activates itself and therefore is catching the focus
		*    - "OnCreateMainWindow()": Adds a "PLGui::ModClose" modifier with "ExitApplication=true" to the created main window,
		*      therefore the application will shut down as soon as clicking on 'x' within the window title
		*/
		PLFRONTENDPLGUI_API virtual void OnCreateMainWindow();

		/**
		*  @brief
		*    Called when the display mode was changed
		*
		*  @note
		*    - The default implementation is empty
		*/
		PLFRONTENDPLGUI_API virtual void OnDisplayMode();

		/**
		*  @brief
		*    Called when the fullscreen mode was changed
		*
		*  @note
		*    - The default implementation is empty
		*/
		PLFRONTENDPLGUI_API virtual void OnFullscreenMode();


	//[-------------------------------------------------------]
	//[ Protected event handlers                              ]
	//[-------------------------------------------------------]
	protected:
		PLCore::EventHandler<>	EventHandlerDestroy;
		PLCore::EventHandler<>	EventHandlerDisplayMode;
		PLCore::EventHandler<>	EventHandlerFullscreenMode;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		PLCore::FrontendPixelLight	m_cFrontend;	/**< The frontend instance */
		PLGui::Widget			   *m_pMainWindow;	/**< Main window of the application (can be a null pointer) */


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Called when main window was destroyed
		*/
		void OnDestroy();


};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // PLFrontendPLGui


#endif // __PLFRONTENDPLGUI_FRONTEND_H__
