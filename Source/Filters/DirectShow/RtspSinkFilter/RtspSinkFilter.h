#pragma once
#include <map>
#include <memory>
#include <DirectShow/CSettingsInterface.h>
#include <DirectShow/CStatusInterface.h>
#include <Filters/DirectShow/FilterParameters.h>
#include <Media/IRateController.h>
#include <Media/SingleChannelManager.h>
#include <Media/RtspService.h>
#include "RtspSinkGuids.h"
#include "RtspSinkProperties.h"

// Forward
class RtspSinkInputPin;
class INetworkCodecControlInterface;
class StepBasedRateController;

/**
 * @brief This RtspSinkFilter class supports at most two input pins, one for video and one for audio.
 */
class RtspSinkFilter : public CBaseFilter,				   /* Source Filter */
                       public ISpecifyPropertyPages, /* property pages */
                       public CSettingsInterface,	   /* Rtvc Settings Interface */
                       public CStatusInterface	     /* Rtvc Status Interface */
{   
	/// Give the pin friend access
	friend class RtspSinkInputPin;

public:
  ///this needs to be declared for the extra interface (adds the COM AddRef, etc methods)
  DECLARE_IUNKNOWN;

  /// DLL factory method
	static CUnknown *WINAPI CreateInstance(IUnknown* pUnk, HRESULT* phr);

	/// override this to publicize our interfaces
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	
	/// From CSettingsInterface
	virtual void initParameters()
	{
    // reuse connection of RTSP register
    addParameter(RTSP_PORT, &m_uiRtspPort, 554);
    addParameter(FRAME_BIT_LIMITS, &m_sFrameBitLimits, "");
    addParameter(STARTING_INDEX, &m_uiStartingIndex, 0);
  }

	/// CBase Filter methods - Overridden since we don't just have one input and output pin as the standard transform filter does
	virtual int GetPinCount();
	/// Override this method: the pins get created in this method of the transform filter
	virtual CBasePin * GetPin(int n);
	/// Method needed to connect pins based on their names
	virtual STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);
  /**
   * @brief ISpecifyPropertyPages
   */
  STDMETHODIMP GetPages(CAUUID *pPages)
  {
    if (pPages == NULL) return E_POINTER;
    pPages->cElems = 1;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
      return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_RtspSinkProperties;
    return S_OK;
  }

  /// Connection management
  /// Overridden to create more input pins on connection
  virtual void OnConnect(int nIndex);
  /// Overridden to remove unused input pins on disconnect
  virtual void OnDisconnect(int nIndex);

  STDMETHODIMP Run(REFERENCE_TIME tStart);
  STDMETHODIMP Pause();
  STDMETHODIMP Stop();
	
  HRESULT sendMediaSample(unsigned uiId, IMediaSample* pSample);

  /// starts live555 session
  void startLive555EventLoop();
  /**
   * @brief Method that is called when an RTSP client session is played
   */
  void onRtspClientSessionPlay(unsigned uiClientSessionId);

private:
  /// Ends the live555 session
  void stopLive555EventLoop();

	///Private Constructor
	RtspSinkFilter(IUnknown* pUnk, HRESULT* phr);
	virtual ~RtspSinkFilter(void);

  void createInputPin(HRESULT* pHr);

	/// vector of output pins
	std::vector<RtspSinkInputPin*> m_vInputPins;

	CCritSec m_stateLock;

  // map for incoming media samples mapping pin index to type
  typedef std::map<unsigned, RTSP_MediaSubtype> SubtypeMap_t;
  SubtypeMap_t m_mMediaSubtype;

  /// LiveMediaExt channel manager
  lme::SingleChannelManager m_channelManager;
  /// DS codec control
  INetworkCodecControlInterface* m_pDsNetworkControlInterface;
  /// Rate control factory
  lme::IRateAdaptationFactory* m_pFactory;
  /// Rate control
  StepBasedRateController* m_pRateController;
  /// frame bit limits
  std::string m_sFrameBitLimits;
  /// Starting index
  unsigned m_uiStartingIndex;
  /// RTSP service
  lme::RtspService m_rtspService;
  /// error code for init
  boost::system::error_code m_ecInit;
  /// Live555 Thread Handle
  HANDLE m_hLiveMediaThreadHandle;
  /// live555 thread id
  DWORD m_dwThreadID;
  /// Handle to let the filter know that the RTSP thread has finished
  /// The Stop method waits for a signal that the liveMedia event loop has ended before it proceeds
  HANDLE m_hLiveMediaStopEvent;
  /// Handle to wait for successful initialisation of live555 
  HANDLE m_hLiveMediaInitEvent;
  /// TO start streaming on an IDR
  bool m_bHaveSeenSpsPpsIdr;
  /// RTSP port
  unsigned m_uiRtspPort;
};
