#include "DirectGrab.h"
#include <windows.h>
#include <dshow.h>
#include <iostream>
#include "SampleGrabber.h"
#include <chrono>

#pragma comment(lib,"Strmiids.lib")

#define LOG_ON true
#define CheckHR(res, msg)	if (!_checkHR(res, msg)){invokeError(res, msg);}

boolean wasInitalized = false;

template <class T> void SafeRelease(T **ppT){
	if (*ppT)	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

boolean _checkHR(HRESULT hr, WCHAR* msg) {
	if (LOG_ON && msg != nullptr) {
		if (FAILED(hr)) {
			std::wcout << "[DirectGrab Error] When: " << msg << " with HRESULT: " << hr << std::endl;
			DebugBreak();
		}
		else {
			std::wcout << "[DirectGrab] " << msg << " successful" << std::endl;
		}
	}
	return SUCCEEDED(hr);
}

boolean _checkHR(HRESULT hr) {
	return _checkHR(hr, nullptr);
}

STDMETHODIMP CameraListener::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CameraListener::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
	return S_OK;
}

ULONG STDMETHODCALLTYPE CameraListener::AddRef(void) {
	return S_OK;
}

ULONG STDMETHODCALLTYPE CameraListener::Release(void) {
	return S_OK;
}

DGCamera::DGCamera(IBaseFilter* cameraFilter, WCHAR* niceName) {
	this->cameraFilter = cameraFilter;
	this->niceName = niceName;
	//Create Graph
	CheckHR(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (void **)&graph), L"Creating Graph");
	//Add Null Renderer
	CheckHR(CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&nullRendererFilter)), L"Creating NullRenderer");
	CheckHR(graph->AddFilter(nullRendererFilter, L"Null Filter"), L"Adding NullRenderer");
	//Add Sample Grabber Filter
	CheckHR(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, 
		IID_PPV_ARGS(&sampleGrabberFilter)), L"Creating SampleGrabber");
	CheckHR(graph->AddFilter(sampleGrabberFilter, L"Sample Grabber"), L"Adding SampleGrabber");
	//Get Sample Grabber Interface and set Media Type
	CheckHR(sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, 
		(void**)&sampleGrabber), L"Getting Grabber from Filter");
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(mt));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = GUID_NULL;
	CheckHR(sampleGrabber->SetMediaType(&mt),L"Setting Grabber's Media Type");
	//Add Camera Filter as Source
	CheckHR(graph->AddFilter(cameraFilter, L"Capture Source"), L"Adding Camera");
	//Get Output pin
	IEnumPins*      pins = 0;
	CheckHR(cameraFilter->EnumPins(&pins), L"Enum Pins");
	CheckHR(pins->Next(1, &outputPin, 0), L"Getting output Pin");
	SafeRelease(&pins);
	//Render Graph
	CheckHR(graph->Render(outputPin), L"Rendering Graph");
	//Getting MediaControl to run Graph
	CheckHR(graph->QueryInterface(IID_IMediaControl, (void **)&control), 
		L"Getting media Control");
	CheckHR(control->Run(), L"Running the graph");
}

DGCamera::~DGCamera() {
	control->Stop();
	delete[] niceName;
	event = nullptr;
	listener = nullptr;
	SafeRelease(&event);
	SafeRelease(&control);
	SafeRelease(&outputPin);
	SafeRelease(&sampleGrabber);
	SafeRelease(&nullRendererFilter);
	SafeRelease(&sampleGrabberFilter);
	SafeRelease(&cameraFilter);
	SafeRelease(&graph);
}

WCHAR* DGCamera::getName() {
	return niceName;
}

void DGCamera::setCameraListener(CameraListener* listener){
	this->listener = listener;
	CheckHR(sampleGrabber->SetCallback(listener, 0), L"Set Callback");
}



void DGCamera::setFormat(int capIndex, int fps){
	//Stop the Graph to change Format
	CheckHR(control->Stop(), L"Stopping Graph");
	IAMStreamConfig* cfg;
	CheckHR(outputPin->QueryInterface(IID_IAMStreamConfig, (void **)&cfg), 
		L"Getting StreamConfig"); 

	VIDEO_STREAM_CONFIG_CAPS cap;
	AM_MEDIA_TYPE*  fmt;
	CheckHR(cfg->GetStreamCaps(capIndex, &fmt, (BYTE*)&cap), L"Getting StreamCaps");

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)fmt->pbFormat;
	vih->AvgTimePerFrame = 10000000 / fps;
	CheckHR(cfg->SetFormat(fmt), L"Setting Format");
	SafeRelease(&cfg);
	//Start the Graph again
	CheckHR(control->Run(), L"Reruning Graph");
}

void DGCamera::invokeError(HRESULT hr, WCHAR * msg){
	if (listener != nullptr) {
		listener->onError(hr, msg);
	}
}

DIRECTGRAB_API std::vector<DGCamera*>* getAvailableCameras(const WCHAR* nameFilter){
	#define CheckHRP(res) if (!_checkHR(res)) {return nullptr;}

	if (!wasInitalized) {
		CheckHRP(CoInitialize(0));
		wasInitalized = true;
	}
	std::vector<DGCamera*>* result = new std::vector<DGCamera*>();
	
	ICreateDevEnum* devs = 0;
	CheckHRP(CoCreateInstance(CLSID_SystemDeviceEnum, 0, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&devs));

	IEnumMoniker*   cams = 0;
	CheckHRP(devs->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &cams, 0));

	IMoniker *pMoniker = NULL;
	
	while (cams->Next(1, &pMoniker, 0) == S_OK)
	{
		IPropertyBag *pPropBag;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag)))
		{
			VARIANT varName;
			VariantInit(&varName);
			CheckHRP(pPropBag->Read(L"FriendlyName", &varName, 0));
			std::wcout << L"Found Camera with name: " << varName.bstrVal << std::endl << std::flush;

			if (nameFilter != nullptr && wcscmp(nameFilter, varName.bstrVal) != 0) {
				std::wcout << L"it does not match name filter" << std::endl << std::flush;
			} else {
				VARIANT devName;
				VariantInit(&devName);
				CheckHRP(pPropBag->Read(L"DevicePath", &devName, 0));
				std::wcout << L"and DevicePath: " << devName.bstrVal << std::endl;

				IBaseFilter* filter = 0;
				CheckHRP(pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&filter));
				WCHAR* niceName = new WCHAR[wcslen(varName.bstrVal) + 1];
				wcscpy(niceName, varName.bstrVal);
				result->push_back(new DGCamera(filter, niceName));
				VariantClear(&devName);
			}		
			VariantClear(&varName);
			SafeRelease(&pPropBag);
		}
		SafeRelease(&pMoniker);
	}
	SafeRelease(&cams);
	SafeRelease(&devs);
	return result;
}

DIRECTGRAB_API std::vector<DGCamera*>* getAvailableCameras() {
	return getAvailableCameras(nullptr);
}
