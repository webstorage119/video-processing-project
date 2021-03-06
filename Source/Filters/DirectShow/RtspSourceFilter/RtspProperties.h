/** @file

MODULE				: RtspProperties

FILE NAME			: RtspProperties.h

DESCRIPTION			: Properties dialog for RTSP Source Filter
					  
LICENSE: Software License Agreement (BSD License)

Copyright (c) 2010, CSIR
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the CSIR nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
#pragma once

#include <DirectShow/FilterPropertiesBase.h>

#include "resource.h"

#define BUFFER_SIZE 256

/**
 * \ingroup DirectShowFilters
 * Properties dialog for Rtsp PCM Source Filter
 */
class RtspProperties : public FilterPropertiesBase
{
public:

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
	{
		RtspProperties *pNewObject = new RtspProperties(pUnk);
		if (pNewObject == NULL) 
		{
			*pHr = E_OUTOFMEMORY;
		}
		return pNewObject;
	}

	RtspProperties::RtspProperties(IUnknown *pUnk) : 
	FilterPropertiesBase(NAME("Rtsp Properties"), pUnk, IDD_RTSP_DIALOG, IDS_RTSP_CAPTION)
	{;}

	HRESULT ReadSettings()
	{
		int nLength = 0;
		char szBuffer[BUFFER_SIZE];
		HRESULT hr = m_pSettingsInterface->GetParameter(STREAM_USING_TCP, sizeof(szBuffer), szBuffer, &nLength);
		if (SUCCEEDED(hr))
		{
			WPARAM wParam;
			if (szBuffer[0] == '0')
				wParam = 0;	
			else
				wParam = 1;
			long lResult = SendMessage(				// returns LRESULT in lResult
				GetDlgItem(m_Dlg, IDC_CHECK_TCP),	// handle to destination control
				(UINT) BM_SETCHECK,					// message ID
				(WPARAM) wParam,							// = 0; not used, must be zero
				(LPARAM) 0							// = (LPARAM) MAKELONG ((short) nUpper, (short) nLower)
				);
		}
		return hr;
	}

	HRESULT OnApplyChanges(void)
	{
    HRESULT hr;
		int iCheck = SendMessage( GetDlgItem(m_Dlg, IDC_CHECK_TCP),	(UINT) BM_GETCHECK,	0, 0);
		if (iCheck == 0)
		{
			hr = m_pSettingsInterface->SetParameter(STREAM_USING_TCP, "false");
		}
		else
		{
			hr = m_pSettingsInterface->SetParameter(STREAM_USING_TCP, "true");
		}
		return hr;
	} 
};

