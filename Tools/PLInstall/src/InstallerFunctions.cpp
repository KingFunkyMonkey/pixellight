/*********************************************************\
 *  File: InstallerFunctions.cpp                         *
 *
 *  Copyright (C) 2002-2013 The PixelLight Team (http://www.pixellight.org/)
 *
 *  This file is part of PixelLight.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 *  and associated documentation files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies or
 *  substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <PLCore/Runtime.h>
#include <PLCore/File/Url.h>
#include <PLCore/System/System.h>
#include "InstallerFunctions.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
using namespace PLCore;


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
/**
*  @brief
*    Constructor
*/
InstallerFunctions::InstallerFunctions()
{
}

/**
*  @brief
*    Destructor
*/
InstallerFunctions::~InstallerFunctions()
{
}

void InstallerFunctions::ConnectProgressEventHandler(EventHandler<int> *pProgressEventHandler)
{
	m_cEventProgressUpdate.Connect(*pProgressEventHandler);
}

bool InstallerFunctions::InstallRuntime()
{
	// Install the PixelLight runtime

	// I know it's a bit redundant but we should check the current installation
	if (!CheckRuntimeInstallation()) {
		m_cEventProgressUpdate(1);

		// Get the directory this executable is in
		// -> There's no need to add platform architecture information because this executable is directly within the proper runtime directory
		m_cEventProgressUpdate(1);
		const String sExecutableDirectory = System::GetInstance()->GetCurrentDir();
		m_cEventProgressUpdate(1);

		// Write the PixelLight-runtime directory into the registry and or path or whatever is needed on the specific platform
		String sMessage;
		if (Runtime::SetDirectory(sExecutableDirectory, &sMessage)) {
			m_sLastSuccessMessage = "PixelLight runtime installed at \"" +  Url(sExecutableDirectory).GetNativePath() + "\"\n\nYou may need to restart your system";	// Show the user a native path he's familiar with
			m_cEventProgressUpdate(1);

			// Success
			return true;
		} else {
			// Error!
			m_sLastErrorMessage = "Failed to write the PixelLight-runtime directory into the registry: \"" + sMessage + '\"';
			m_cEventProgressUpdate(1);

			// Error occurred!
			return false;
		}
	}

	// The PixelLight runtime is already installed
	m_cEventProgressUpdate(4);

	// Success
	return true;
}

int InstallerFunctions::GetInstallRuntimeProgressSteps() const
{
	return INSTALL_RUNTIME_PROGRESS_STEPS;
}

bool InstallerFunctions::CheckRuntimeInstallation()
{
	// Get the current PixelLight runtime directory (e.g. "file://C:/PixelLight/Runtime/x86" on Windows)
	// -> Do also ensure that the registry entry is pointing to the same directory the PLCore shared library is in
	const String sDirectory = Runtime::GetDirectory();
	m_cEventProgressUpdate(1);
	const String sPLCoreSharedLibraryDirectory = Runtime::GetPLCoreSharedLibraryDirectory();

	m_cEventProgressUpdate(1);
	const String sRegistryDirectory = Runtime::GetRegistryDirectory();
	// [TODO] Windows: One thing... "Runtime::GetPLCoreSharedLibraryDirectory()" is going to load "PLCore.dll" and the OS will also have a look
	// into the directory PLInstall is in, meaning this dll will be found even if the directory is not within the PATH environment variable.
	// -> So, this test is pointless because it will always tell us "Yes, I was able to find PLCore.dll" :/
	// -> We probably should access the PATH environment variable by using the registry
	m_cEventProgressUpdate(1);

	if (sDirectory.GetLength() && sPLCoreSharedLibraryDirectory == sRegistryDirectory) {
		// The PATH environment or the registry key is pointing to the current directory => PixelLight is already installed correctly
		m_cEventProgressUpdate(1);
		m_sLastSuccessMessage = "PixelLight runtime is already installed at \"" +  Url(sDirectory).GetNativePath() + '\"';	// Show the user a native path he's familiar with

		// Success
		return true;
	}

	// The PixelLight runtime could not be found => the runtime isn't installed correctly
	m_cEventProgressUpdate(1);
	m_sLastErrorMessage = "The PixelLight runtime directory could not be found";

	// Error!
	return false;
}

int InstallerFunctions::GetCheckRuntimeProgressSteps() const
{
	return CHECK_RUNTIME_PROGRESS_STEPS;
}

String InstallerFunctions::GetLastErrorDescription() const
{
	return m_sLastErrorMessage;
}

String InstallerFunctions::GetLastSuccessMessage() const
{
	return m_sLastSuccessMessage;
}
