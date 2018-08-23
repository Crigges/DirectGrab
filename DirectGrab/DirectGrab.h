#ifdef DIRECTGRAB_EXPORTS
#define DIRECTGRAB_API __declspec(dllexport)
#else
#define DIRECTGRAB_API __declspec(dllimport)
#endif

#include <vector>
#include "SampleGrabber.h" 


struct DIRECTGRAB_API CameraListener : ISampleGrabberCB {

	//Implements these:
	//Called when device is disconnected
	virtual void onDisconnect() = 0;
	//If any error occours generaly a bad sign and should not happen
	virtual void onError(HRESULT hr, WCHAR* msg) = 0;
	//Called when a frame is received
	STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample) = 0;

	//Ignore these	
	STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
};

class DIRECTGRAB_API DGCamera {
public:
	DGCamera(IBaseFilter*, WCHAR* niceName);
	~DGCamera();
	WCHAR* getName();
	void setCameraListener(CameraListener* listener);
	void setFormat(int capIndex, int fps);
private:
	void invokeError(HRESULT hr, WCHAR* msg);
	IBaseFilter* cameraFilter;
	WCHAR* niceName;
	IGraphBuilder* graph;
	IBaseFilter *nullRendererFilter;
	IBaseFilter *sampleGrabberFilter;
	ISampleGrabber *sampleGrabber;
	IPin* outputPin;
	IMediaControl* control;
	IMediaEvent* event;
	CameraListener* listener = nullptr;
};

DIRECTGRAB_API std::vector<DGCamera*>* getAvailableCameras();

DIRECTGRAB_API std::vector<DGCamera*>* getAvailableCameras(const WCHAR* nameFilter);

