/** @file

MODULE				: CropFilter

FILE NAME			: CropFilter.cpp

DESCRIPTION			: 
					  
LICENSE: Software License Agreement (BSD License)

Copyright (c) 2008, CSIR
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

#include "CropFilter.h"

#include <DirectShow/CommonDefs.h>

#include <Image/PicCropperRGB24Impl.h>
#include <Image/PicCropperRGB32Impl.h>

CCropFilter::CCropFilter()
: CCustomBaseFilter(NAME("CSIR RTVC Crop Filter"), 0, CLSID_CropFilter),
	m_pCropper(NULL),
	m_nBytesPerPixel(BYTES_PER_PIXEL_RGB24),
	m_nStride(0),
	m_nPadding(0)
{
	//Call the initialise input method to load all acceptable input types for this filter
	InitialiseInputTypes();
	// Init parameters
	initParameters();
}

CCropFilter::~CCropFilter()
{
	if (m_pCropper)
	{
		delete m_pCropper;
		m_pCropper = NULL;
	}
}

CUnknown * WINAPI CCropFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT *pHr )
{
	CCropFilter *pFilter = new CCropFilter();
	if (pFilter== NULL) 
	{
		*pHr = E_OUTOFMEMORY;
	}
	return pFilter;
}


void CCropFilter::InitialiseInputTypes()
{
	AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_RGB24, &FORMAT_VideoInfo);
	AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_RGB32, &FORMAT_VideoInfo);
}

HRESULT CCropFilter::SetMediaType( PIN_DIRECTION direction, const CMediaType *pmt )
{
	HRESULT hr = CCustomBaseFilter::SetMediaType(direction, pmt);
	if (direction == PINDIR_INPUT)
	{
		//Set defaults and make sure that the target dimensions are valid
		if ((m_nOutWidth == 0)||(m_nOutWidth > m_nInWidth))
		{
			m_nOutWidth = m_nInWidth;
		}
		if ((m_nOutHeight == 0)||(m_nOutHeight > m_nInHeight))
		{
			m_nOutHeight = m_nInHeight;
		}
		//Determine whether we are connected to a RGB24 or 32 source
		if (pmt->majortype == MEDIATYPE_Video)
		{
			//The converter might already exist if the filter has been connected previously
			if (m_pCropper)
			{
				delete m_pCropper;
				m_pCropper = NULL;
			}
			if (pmt->subtype == MEDIASUBTYPE_RGB24)
			{
				m_pCropper = new PicCropperRGB24Impl();
				m_nBytesPerPixel = BYTES_PER_PIXEL_RGB24;
			}
			else if (pmt->subtype == MEDIASUBTYPE_RGB32)
			{
				m_pCropper = new PicCropperRGB32Impl();
				m_nBytesPerPixel = BYTES_PER_PIXEL_RGB32;
			}
		}
		RecalculateFilterParameters();
	}
	return hr;
}

HRESULT CCropFilter::GetMediaType( int iPosition, CMediaType *pMediaType )
{
	if (iPosition < 0)
	{
		return E_INVALIDARG;
	}
	else if (iPosition == 0)
	{
		// Get the input pin's media type and return this as the output media type - we want to retain
		// all the information about the image
		HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
		if (FAILED(hr))
		{
			return hr;
		}
		
		// Get the bitmap info header and adapt the cropped 
		//make sure that it's a video info header
		ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
		VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
		//Now we need to calculate the size of the output image
		BITMAPINFOHEADER* pBi = &(pVih->bmiHeader);
		
		pBi->biHeight = m_nOutHeight;
		ASSERT(pBi->biHeight > 0);

		pBi->biWidth = m_nOutWidth;
		ASSERT(pBi->biWidth > 0);
		pBi->biSizeImage = pBi->biWidth * pBi->biHeight * m_nBytesPerPixel;

		pVih->rcSource.top = 0;
		pVih->rcSource.left = 0;
		pVih->rcSource.right = pBi->biWidth;
		pVih->rcSource.bottom = pBi->biHeight;

		pVih->rcTarget.top = 0;
		pVih->rcTarget.left = 0;
		pVih->rcTarget.right = pBi->biWidth;
		pVih->rcTarget.bottom = pBi->biHeight;

		pMediaType->lSampleSize = pBi->biSizeImage;

		return S_OK;
	}
	return VFW_S_NO_MORE_ITEMS;
}

HRESULT CCropFilter::DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp )
{
	// Adding padding to take stride into account
	pProp->cbBuffer = (m_nOutWidth + m_nPadding) * m_nOutHeight * m_nBytesPerPixel;

	if (pProp->cbAlign == 0)
	{
		pProp->cbAlign = 1;
	}
	if (pProp->cBuffers == 0)
	{
		pProp->cBuffers = 1;
	}
	
	// Set allocator properties.
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}
	// Even when it succeeds, check the actual result.
	if (pProp->cbBuffer > Actual.cbBuffer) 
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CCropFilter::CheckTransform( const CMediaType *mtIn, const CMediaType *mtOut )
{
	//Make sure the input and output types are related
	if (mtOut->majortype != MEDIATYPE_Video)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	// RG: 27/08/2008 Bug FIX: Resulted in incorrect image in VMR9
	/*if (!
		((mtIn->subtype == MEDIASUBTYPE_RGB24)&&(mtOut->subtype == MEDIASUBTYPE_RGB24))
		||
		((mtIn->subtype == MEDIASUBTYPE_RGB32)&&(mtOut->subtype == MEDIASUBTYPE_RGB32))
		)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}*/
	if (mtIn->subtype == MEDIASUBTYPE_RGB24)
		if (mtOut->subtype != MEDIASUBTYPE_RGB24)
			return VFW_E_TYPE_NOT_ACCEPTED;

	if (mtIn->subtype == MEDIASUBTYPE_RGB32)
		if (mtOut->subtype != MEDIASUBTYPE_RGB32)
			return VFW_E_TYPE_NOT_ACCEPTED;

	if (mtOut->formattype != FORMAT_VideoInfo)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	return S_OK;
}

STDMETHODIMP CCropFilter::SetParameter( const char* type, const char* value )
{
	// For now, one cannot set any parameters once the output has been connected -> this will affect the buffer size
	if (m_pOutput)
	{
		if (m_pOutput->IsConnected())
		{
			return E_FAIL;
		}
	}

	if (SUCCEEDED(CSettingsInterface::SetParameter(type, value)))
	{
		RecalculateFilterParameters();
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

DWORD CCropFilter::ApplyTransform( BYTE* pBufferIn, BYTE* pBufferOut )
{
	int nTotalSize = 0;
	//make sure we were able to initialise our converter
	if (m_pCropper)
	{
		// Create temp buffer for crop
		// TODO: Add stride parameter for crop class to avoid this extra mem alloc
		BYTE* pBuffer = new BYTE[m_nOutWidth * m_nOutHeight * m_nBytesPerPixel];

		//Call cropping conversion code
		m_pCropper->SetInDimensions(m_nInWidth, m_nInHeight);
		m_pCropper->SetOutDimensions(m_nOutWidth, m_nOutHeight);
		m_pCropper->SetCrop(m_nLeftCrop, m_nRightCrop, m_nTopCrop, m_nBottomCrop);
		m_pCropper->Crop((void*)pBufferIn, (void*)pBuffer);
		
		// Copy the cropped image with stride padding
		BYTE* pFrom = pBuffer;
		BYTE* pTo = pBufferOut;

		int nBytesPerLine = m_nOutWidth * m_nBytesPerPixel;
		for (size_t i = 0; i < m_nOutHeight; i++)
		{
			memcpy(pTo, pFrom, nBytesPerLine);
			pFrom += nBytesPerLine;
			pTo += nBytesPerLine;
			for (size_t j = 0; j < m_nPadding; j++)
			{
				*pTo = 0;
				pTo++;
			}
		}
		nTotalSize = (m_nOutWidth + m_nPadding) * m_nOutHeight * m_nBytesPerPixel;
		delete[] pBuffer;
	}
	else
	{
		DbgLog((LOG_TRACE, 0, TEXT("TestCropper: Cropper is not initialised - unable to transform")));
	}
	return nTotalSize;
}

void CCropFilter::RecalculateFilterParameters()
{
	//Set defaults and make sure that the target dimensions are valid
	m_nOutWidth = m_nInWidth - m_nLeftCrop - m_nRightCrop;
	m_nOutHeight = m_nInHeight - m_nTopCrop - m_nBottomCrop;
	m_nOutPixels = m_nOutWidth * m_nOutHeight;
	m_nStride =  (m_nOutWidth * (m_nBitCount / 8) + 3) & ~3;
	m_nPadding = m_nStride - (m_nBytesPerPixel * m_nOutWidth);

}

HRESULT CCropFilter::SetCropIfValid( int nTotalDimensionImage, int nNewCrop, int& nOldCrop, int nOppositeCrop  )
{
	if (nNewCrop >= 0)
	{
		if (nTotalDimensionImage - nOppositeCrop - nNewCrop > 0)
		{
			nOldCrop = nNewCrop;
			RecalculateFilterParameters();
			return S_OK;
		}
		else
		{
			// Fail: Total crop exceeds image dimension
			return E_FAIL;
		}
	}
	else
	{
		// negative value
		return E_FAIL;
	}
}