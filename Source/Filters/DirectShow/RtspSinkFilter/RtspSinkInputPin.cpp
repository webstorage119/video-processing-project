#include "stdafx.h"
#include "RtspSinkInputPin.h"
#include "RtspSinkFilter.h"
#include <DirectShow/CustomMediaTypes.h>
#include <cassert>
// live555
#include <Base64.hh>
// Experimental for H264
#ifdef RTVC_USE_MS_H264_DECODER
#include <dvdmedia.h>
#include <wmcodecdsp.h>
#endif
#include <Media/MediaTypes.h>

RtspSinkInputPin::RtspSinkInputPin( HRESULT* phr, RtspSinkFilter* pFilter, unsigned uiId )
: CRenderedInputPin(NAME("CSIR VPP RTSP Sink Filter Input Pin"), pFilter, &m_stateLock, phr, L"Input"),
	m_pFilter(pFilter), // Parent filter
  m_uiId(uiId),
  m_eType(MT_NOT_SET),
  m_eSubtype(MST_NOT_SET)
{
	if (!SUCCEEDED(*phr))
	{
		return;
	}
}

RtspSinkInputPin::~RtspSinkInputPin(void)
{
}

HRESULT RtspSinkInputPin::Receive( IMediaSample *pSample )
{
  CheckPointer(pSample,E_POINTER);

  CAutoLock lock(&m_receiveLock);

  REFERENCE_TIME tStart, tStop;
  pSample->GetTime(&tStart, &tStop);

  DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
    (LPCTSTR) CDisp(tStart),
    (LPCTSTR) CDisp(tStop),
    pSample->GetActualDataLength()));

  // get Data
  PBYTE pbData;
  HRESULT hr = pSample->GetPointer(&pbData);
  if (FAILED(hr)) {
    return hr;
  }

  // send the sample using our RTP library
  return m_pFilter->sendMediaSample(m_uiId, pSample);
}

HRESULT RtspSinkInputPin::BreakConnect()
{
  HRESULT hr = CRenderedInputPin::BreakConnect();
  m_pFilter->OnDisconnect(m_uiId);
  m_eType = MT_NOT_SET;
  m_eSubtype = MST_NOT_SET;
  return hr;
}

HRESULT RtspSinkInputPin::CompleteConnect(IPin *pReceivePin)
{
  HRESULT hr = CRenderedInputPin::CompleteConnect(pReceivePin);
  if (SUCCEEDED(hr))
  {
    m_pFilter->OnConnect(m_uiId);
  }
  return hr;
}

HRESULT RtspSinkInputPin::CheckMediaType( const CMediaType *pmt )
{
  // only accept types the RTP library can handle:
  // Currently this includes H.264, AMR and AAC
  if (pmt->majortype == MEDIATYPE_Video)
  {
    if (pmt->subtype == MEDIASUBTYPE_VPP_H264)
    {
      extractVideoParameters(pmt);
      extractH264Parameters(pmt);
      m_eType = MT_VIDEO;
      m_eSubtype = MST_H264;
      return S_OK;
    }
    else if (pmt->subtype == MEDIASUBTYPE_MMF)
    {
      extractVideoParameters(pmt);
      // TODO: not sure if this is necessary?
      // extractH264Parameters(pmt);
      m_eType = MT_VIDEO;
      m_eSubtype = MST_MMF;
      return S_OK;
    }
  }
  else if (pmt->majortype == MEDIATYPE_Audio)
  {
    if (pmt->subtype == MEDIASUBTYPE_AMR)
    {
      extractAudioParameters(pmt);
      m_eType = MT_AUDIO;
      m_eSubtype = MST_AMR;
      return S_OK;
    }
    else if (pmt->subtype == MEDIASUBTYPE_AAC)
    {
      extractAudioParameters(pmt);
      m_eType = MT_AUDIO;
      m_eSubtype = MST_AAC;
      return S_OK;
    }
  }

  return VFW_E_TYPE_NOT_ACCEPTED;
}

inline void RtspSinkInputPin::extractVideoParameters(const CMediaType *pMediaType)
{
  assert(pMediaType->formattype == FORMAT_VideoInfo);
  if (pMediaType->subtype == MEDIASUBTYPE_VPP_H264)
    m_videoDescriptor.Codec = lme::H264;

  if (pMediaType->subtype == MEDIASUBTYPE_MMF)
  {
    m_videoDescriptor.Codec = lme::MMF;
    // The following code reads the information stored in the MMF format buffer
    // extract frame bit limits and initial channel
    // Extract width and height of video -> this is assuming that all multiplexed videos have the same dimension!!!!!!!		
    // Add the number of streams parameter to the subsession
    BYTE* pFormat = pMediaType->pbFormat;
    // Get size of entire format header: this is our custom header for media type MM1
    int nSize = pMediaType->cbFormat;
    unsigned uiAdditionalHeaderSize(0);
    std::string sSps, sPps;
    // Channel start
    unsigned uiChannelStart = 0, uiChannelCount = 0;
    std::vector<int> vFramebitLimits;

    // The last byte stores the additional header size
    memcpy(&uiAdditionalHeaderSize, pFormat + nSize - sizeof(int), sizeof(int));
    // skip the standard DS info
    BYTE* pPos = &pFormat[nSize - uiAdditionalHeaderSize];

    char buffer[256];
    // SPS
    unsigned uiSpsLen = 0;
    memcpy(&uiSpsLen, pPos, sizeof(unsigned));
    pPos += sizeof(unsigned);
    memcpy(buffer, pPos + 4, uiSpsLen - 4); // omit start code
    sSps = std::string(buffer, uiSpsLen - 4);
    pPos += uiSpsLen;

    // PPS
    unsigned uiPpsLen = 0;
    memcpy(&uiPpsLen, pPos, sizeof(unsigned));
    pPos += sizeof(unsigned);
    memcpy(buffer, pPos, uiPpsLen - 4); // omit start code
    sPps = std::string(buffer, uiPpsLen - 4);
    pPos += uiPpsLen;

    // Channel start
    memcpy(&uiChannelStart, pPos, sizeof(unsigned));
    pPos += sizeof(unsigned);

    // Channel count
    memcpy(&uiChannelCount, pPos, sizeof(unsigned));
    pPos += sizeof(unsigned);

    for (size_t i = 0; i < uiChannelCount; ++i)
    {
      int val = 0;
      memcpy(&val, pPos, sizeof(int));
      m_videoDescriptor.FrameBitLimits.push_back(val);
      pPos += sizeof(int);
    }

    m_videoDescriptor.InitialChannel = uiChannelStart;
    char* szSps = base64Encode(sSps.c_str(), sSps.length());
    char* szPps = base64Encode(sPps.c_str(), sPps.length());
    m_videoDescriptor.Sps = std::string(szSps);
    m_videoDescriptor.Pps = std::string(szPps);
    delete[] szSps;
    delete[] szPps;
  }

  VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
  int nWidth = pVih->bmiHeader.biWidth;
  int nHeight = pVih->bmiHeader.biHeight;
  m_videoDescriptor.Width = nWidth;
  m_videoDescriptor.Height = nHeight;
}

inline void RtspSinkInputPin::extractAudioParameters(const CMediaType *pMediaType)
{
  if (pMediaType->subtype == MEDIASUBTYPE_AMR)
    m_audioDescriptor.Codec = lme::AMR;
  else if (pMediaType->subtype == MEDIASUBTYPE_AAC)
    m_audioDescriptor.Codec = lme::AAC;
  else
    assert(false);

  WAVEFORMATEX *pWF = (WAVEFORMATEX *)pMediaType->pbFormat;
  // Get bits per sample
  m_audioDescriptor.BitsPerSample = pWF->wBitsPerSample;
  // Get samples per second
  m_audioDescriptor.SamplingFrequency = pWF->nSamplesPerSec;
  // Get channels
  m_audioDescriptor.Channels = pWF->nChannels;
  
  if (pMediaType->subtype == MEDIASUBTYPE_AAC)
  {
    // extract AAC specific info
    DWORD cbDecoderSpecific = pWF->cbSize;
    ASSERT(cbDecoderSpecific == 2);
    unsigned char audioSpecificConfig[2];
    memcpy(audioSpecificConfig, pWF + 1, cbDecoderSpecific);

    // Adopted from liveMedia\ADTSAudioFileSource.cpp
    const int buf_size = 5;
    char fConfigStr[buf_size];
#if 0
    sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
#else
    sprintf_s(fConfigStr, buf_size, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
#endif
    std::string sAudioConfigStr(fConfigStr, buf_size);
    m_audioDescriptor.ConfigString = sAudioConfigStr;
  }
}

void RtspSinkInputPin::extractH264Parameters(const CMediaType *pMediaType)
{
  // Extract width and height of video -> this is assuming that all multiplexed videos have the same dimension!!!!!!!		
  // Add the number of streams parameter to the subsession
  BYTE* pFormat = pMediaType->pbFormat;
  // Get size of entire format header: this is our custom header for media type H264
  int nSize = pMediaType->cbFormat;
  // The last byte stores the additional header size
  unsigned uiAdditionalSize(0);
  memcpy(&uiAdditionalSize, pFormat + nSize - sizeof(int), sizeof(int));

  if (uiAdditionalSize > 0)
  {
    unsigned char startCode[4] = { 0, 0, 0, 1 };
    char* pBuffer = new char[uiAdditionalSize];
    memcpy(pBuffer, pFormat + (nSize - uiAdditionalSize), uiAdditionalSize);
    // check for first start code
    if (memcmp(startCode, pBuffer, 4) == 0)
    {
      int index = 0;
      // adjust to compensate for start code
      for (size_t i = 4; i < uiAdditionalSize - 8; ++i) // skip 1st start code and since start code is 4 bytes long don't cmp past the end
      {
        // search for second start code
        if (memcmp(startCode, &pBuffer[i], 4) == 0)
        {
          index = i;
          break;
        }
      }

      std::string sps, pps;
      // if we found the second start code
      if (index != 0)
      {
        sps = std::string(pBuffer + 4, index - 4);
        pps = std::string(pBuffer + index + 4, uiAdditionalSize - index - 4 - sizeof(int));

        char* szSps = base64Encode(sps.c_str(), sps.length());
        char* szPps = base64Encode(pps.c_str(), pps.length());
        m_videoDescriptor.Sps = std::string(szSps);
        m_videoDescriptor.Pps = std::string(szPps);
        delete[] szSps;
        delete[] szPps;
      }
    }
    delete[] pBuffer;
  }
}

HRESULT RtspSinkInputPin::GetMediaType( int iPosition, CMediaType *pMediaType )
{
  if (iPosition < 0)
  {
    return E_INVALIDARG;
  }
  else if (iPosition == 0)
  {
    if (true)
    {
      // Get the input pin's media type and return this as the output media type - we want to retain
      // all the information about the image
      //HRESULT hr = ConnectionMediaType(pMediaType);
      //if (FAILED(hr))
      //{
      //  return hr;
      //}

      pMediaType->majortype = MEDIATYPE_Video;
      //pMediaType->formattype = FORMAT_VideoInfo;
      pMediaType->subtype = MEDIASUBTYPE_VPP_H264;

      //ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
      //VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
      //BITMAPINFOHEADER* pBi = &(pVih->bmiHeader);
      //// We only support outputing 24bit RGB (This solves issues were RGB32 is encoded and decoded
      //if (pBi->biBitCount != 24)
      //  pBi->biBitCount = 24;
      //// in the case of YUV I420 input to the H264 encoder, we need to change this back to RGB
      //pBi->biCompression = BI_RGB;

      // Store SPS and PPS in media format header
      //int nCurrentFormatBlockSize = pMediaType->cbFormat;

      //if (m_uiSeqParamSetLen + m_uiPicParamSetLen > 0)
      //{
      //  // old size + one int to store size of SPS/PPS + SPS/PPS/prepended by start codes
      //  int iAdditionalLength = sizeof(int) + m_uiSeqParamSetLen + m_uiPicParamSetLen;
      //  int nNewSize = nCurrentFormatBlockSize + iAdditionalLength;
      //  pMediaType->ReallocFormatBuffer(nNewSize);

      //  BYTE* pFormat = pMediaType->Format();
      //  BYTE* pStartPos = &(pFormat[nCurrentFormatBlockSize]);
      //  // copy SPS
      //  memcpy(pStartPos, m_pSeqParamSet, m_uiSeqParamSetLen);
      //  pStartPos += m_uiSeqParamSetLen;
      //  // copy PPS
      //  memcpy(pStartPos, m_pPicParamSet, m_uiPicParamSetLen);
      //  pStartPos += m_uiPicParamSetLen;
      //  // Copy additional header size
      //  memcpy(pStartPos, &iAdditionalLength, sizeof(int));
      //}
      //else
      //{
      //  // not configured: just copy in size of 0
      //  pMediaType->ReallocFormatBuffer(nCurrentFormatBlockSize + sizeof(int));
      //  BYTE* pFormat = pMediaType->Format();
      //  memset(pFormat + nCurrentFormatBlockSize, 0, sizeof(int));
      //}
    }
    // TODO: support ms media types
    /*
    else
    {
      pMediaType->InitMediaType();
      pMediaType->SetType(&MEDIATYPE_Video);
      pMediaType->SetSubtype(&MEDIASUBTYPE_H264);
      //MEDIASUBTYPE_h264
      //MEDIASUBTYPE_X264
      //MEDIASUBTYPE_x264
      //MEDIASUBTYPE_AVC1
      //FORMAT_MPEG2Video

      pMediaType->SetFormatType(&FORMAT_VideoInfo2);
      VIDEOINFOHEADER2* pvi2 = (VIDEOINFOHEADER2*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
      ZeroMemory(pvi2, sizeof(VIDEOINFOHEADER2));
      pvi2->bmiHeader.biBitCount = 24;
      pvi2->bmiHeader.biSize = 40;
      pvi2->bmiHeader.biPlanes = 1;
      pvi2->bmiHeader.biWidth = m_nInWidth;
      pvi2->bmiHeader.biHeight = m_nInHeight;
      pvi2->bmiHeader.biSize = m_nInWidth * m_nInHeight * 3;
      pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
      pvi2->bmiHeader.biCompression = DWORD('1cva');
      //pvi2->AvgTimePerFrame = m_tFrame;
      //pvi2->AvgTimePerFrame = 1000000;
      const REFERENCE_TIME FPS_25 = UNITS / 25;
      pvi2->AvgTimePerFrame = FPS_25;
      //SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      pvi2->rcTarget = pvi2->rcSource;

      pvi2->dwPictAspectRatioX = m_nInWidth;
      pvi2->dwPictAspectRatioY = m_nInHeight;
    }*/
    return S_OK;
  }
  /*
  else if (iPosition == 1)
  {
    if (m_pInput->IsConnected() == FALSE)
      return E_FAIL;

    // there is only one type we can produce
    pMediaType->majortype = MEDIATYPE_Audio;
    pMediaType->subtype = MEDIASUBTYPE_PCM;
    pMediaType->formattype = FORMAT_WaveFormatEx;
    pMediaType->SetTemporalCompression(FALSE);

    WAVEFORMATEX *wfx = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
    memset(wfx, 0, sizeof(*wfx));
    wfx->wFormatTag = WAVE_FORMAT_PCM;
    wfx->cbSize = 0;
    wfx->nChannels = 1;
    wfx->wBitsPerSample = 16;
    wfx->nSamplesPerSec = 8000;
    wfx->nBlockAlign = wfx->nChannels*(wfx->wBitsPerSample >> 3);
    wfx->nAvgBytesPerSec = wfx->nChannels * (wfx->wBitsPerSample >> 3) * wfx->nSamplesPerSec;

    return NOERROR;

  }
  else if (iPosition == 2)
  {
    if (m_pInput->IsConnected() == FALSE) return E_FAIL;
    if (!m_pEncoder) return E_FAIL;

    pMediaType->majortype = MEDIATYPE_Audio;
    pMediaType->subtype = MEDIASUBTYPE_AAC;
    pMediaType->formattype = FORMAT_WaveFormatEx;

    WAVEFORMATEX *wfx = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(*wfx) + extradata_size);
    memset(wfx, 0, sizeof(*wfx));
    wfx->cbSize = extradata_size;
    wfx->nChannels = info.channels;
    wfx->nSamplesPerSec = info.samplerate;
    wfx->wFormatTag = 0xff;					// AAC

    // decoder specific info
    BYTE		*ex = ((BYTE*)wfx) + sizeof(*wfx);
    memcpy(ex, extradata, extradata_size);

    //  // there is only one type we can produce
    //pMediaType->majortype = MEDIATYPE_Audio;
    //pMediaType->subtype = MEDIASUBTYPE_AMR;
    //pMediaType->formattype = FORMAT_WaveFormatEx;
    //pMediaType->lSampleSize = 0;

    //WAVEFORMATEX *wfx = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
    //memset(wfx, 0, sizeof(*wfx));
    //wfx->wBitsPerSample = 16;
    //wfx->nChannels = 1;
    //wfx->nSamplesPerSec = 8000;
    //wfx->nBlockAlign = 1;
    //wfx->nAvgBytesPerSec = 0;
    //wfx->wFormatTag = 0;

    return NOERROR;

  }*/

  // is this ok or do we need to return S_OK?
  return VFW_S_NO_MORE_ITEMS;
}

