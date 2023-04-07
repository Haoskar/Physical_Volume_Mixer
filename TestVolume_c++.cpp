// TestVolume_c++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <combaseapi.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <devicetopology.h>
#include <endpointvolume.h>
#include <AudioSessionTypes.h>
#include <functiondiscoverykeys_devpkey.h>
#include <Audiopolicy.h>
#include <objbase.h>
#include <strsafe.h>
#include <Psapi.h>
#include <unknwnbase.h>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>


#include "libserialport.h"
#include "ProgramVolume.h"

std::mutex mtx;


#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

#define PRINT_ON_ERROR(hres)  \
               if(FAILED(hres)) {printf("\n\nFAILURE: %X\n\n", hres);}

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);

HRESULT CreateAudioClient(IMMDevice*, IAudioClient**);
HRESULT CreateAudioSessionManager(IMMDevice*, IAudioSessionManager**);
HRESULT getDefaultAudioEndPointDevice(IMMDevice** );
void PrintEndpointNames();

int check(enum sp_return result);



std::vector<ProgramVolume> volumeVector;
IMMDevice* pAudioEndPoint = NULL;
IAudioClient* pAudioClient = NULL;
IAudioSessionManager* pAudioSessionManager = NULL;
IAudioSessionManager2* pAudioSessionManager2 = NULL;
IAudioEndpointVolume* pAudioEndpointVolume = NULL;

class AudioNotifications: public IAudioSessionNotification
{

private:

        LONG             m_cRefAll;
        ~AudioNotifications(){};

public:
    AudioNotifications(): m_cRefAll(1) {};

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv)  
    {    
        if (IID_IUnknown == riid)
        {
            AddRef();
            *ppv = (IUnknown*)this;
        }
        else if (__uuidof(IAudioSessionNotification) == riid)
        {
            AddRef();
            *ppv = (IAudioSessionNotification*)this;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef(){
        return InterlockedIncrement(&m_cRefAll);
    }
     
    ULONG STDMETHODCALLTYPE Release(){
        ULONG ulRef = InterlockedDecrement(&m_cRefAll);
        if (0 == ulRef)
        {
			std::cout << "\n\nI am delete godbye\n";
            delete this;
        }
        return ulRef;
    }

    HRESULT OnSessionCreated(IAudioSessionControl *pNewSession){
        std::cout << "\n\n I AM IN THE OnSessionCreated\n";
        HRESULT hr = S_OK;

        ISimpleAudioVolume* pSimpleAudio = NULL;

        hr = pNewSession->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudio);
        PRINT_ON_ERROR(hr);

        wchar_t *dispName;
        pNewSession->GetDisplayName(&dispName);
		std::cout << "I am: " << pNewSession << "\n";
        std::wcout << "Name: " << dispName;
        float vol = -1;
        pSimpleAudio->GetMasterVolume(&vol);
        std::cout << "\nVolume: " << vol << "\n\n";


        //SAFE_RELEASE(pNewSession);
        SAFE_RELEASE(pSimpleAudio);

        return S_OK;
    }
};

//-----------------------------------------------------------
// Client implementation of IAudioSessionEvents interface.
// WASAPI calls these methods to notify the application when
// a parameter or property of the audio session changes.
//-----------------------------------------------------------
class CAudioSessionEvents : public IAudioSessionEvents
{
private:
	LONG _cRef;

	~CAudioSessionEvents()
	{
	}

public:
	CAudioSessionEvents() :
		_cRef(1)
	{
	}

	// IUnknown methods -- AddRef, Release, and QueryInterface

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		ULONG ulRef = InterlockedDecrement(&_cRef);
		if (0 == ulRef)
		{
			delete this;
		}
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID  riid,
		VOID** ppvInterface)
	{
		if (IID_IUnknown == riid)
		{
			AddRef();
			*ppvInterface = (IUnknown*)this;
		}
		else if (__uuidof(IAudioSessionEvents) == riid)
		{
			AddRef();
			*ppvInterface = (IAudioSessionEvents*)this;
		}
		else
		{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	// Notification methods for audio session events

	HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(
		LPCWSTR NewDisplayName,
		LPCGUID EventContext)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnIconPathChanged(
		LPCWSTR NewIconPath,
		LPCGUID EventContext)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(
		float NewVolume,
		BOOL NewMute,
		LPCGUID EventContext)
	{
		if (NewMute)
		{
			printf("MUTE\n");
		}
		else
		{
			printf("Volume = %d percent\n",
				(UINT32)(100 * NewVolume + 0.5));
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(
		DWORD ChannelCount,
		float NewChannelVolumeArray[],
		DWORD ChangedChannel,
		LPCGUID EventContext)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(
		LPCGUID NewGroupingParam,
		LPCGUID EventContext)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnStateChanged(
		AudioSessionState NewState)
	{
        std::string pszState = "?????";

		switch (NewState)
		{
		case AudioSessionStateActive:
			pszState = "active";
			break;
		case AudioSessionStateInactive:
			pszState = "inactive";
			break;
		}
		printf("New session state = %s\n", pszState.c_str());

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnSessionDisconnected(
		AudioSessionDisconnectReason DisconnectReason)
	{
        std::string pszReason = "?????";

		switch (DisconnectReason)
		{
		case DisconnectReasonDeviceRemoval:
			pszReason = "device removed";
			break;
		case DisconnectReasonServerShutdown:
			pszReason = "server shut down";
			break;
		case DisconnectReasonFormatChanged:
			pszReason = "format changed";
			break;
		case DisconnectReasonSessionLogoff:
			pszReason = "user logged off";
			break;
		case DisconnectReasonSessionDisconnected:
			pszReason = "session disconnected";
			break;
		case DisconnectReasonExclusiveModeOverride:
			pszReason = "exclusive-mode override";
			break;
		}
		printf("Audio session disconnected (reason: %s)\n",
			pszReason.c_str());

		return S_OK;
	}
};

int main()
{
	HRESULT hr = S_OK;
/*
	sp_port* ports;

	check(sp_get_port_by_name("COM4", &ports));
	check(sp_open(ports, SP_MODE_READ_WRITE));

	check(sp_set_baudrate(ports, 9600));
	check(sp_set_bits(ports, 8));
	check(sp_set_parity(ports, SP_PARITY_NONE));
	check(sp_set_stopbits(ports, 1));
	check(sp_set_flowcontrol(ports, SP_FLOWCONTROL_NONE));


	char* buf = (char*)malloc(20);
	unsigned int timeout = 0;

	sp_flush(ports, SP_BUF_BOTH);

    struct sp_port *rx_port = ports;
    struct sp_port *tx_port;
*/
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	PRINT_ON_ERROR(hr);

	ISimpleAudioVolume* pSimpleAudio = NULL;
	IAudioSessionEnumerator* pAudioSessionEnumerator = NULL;
	IAudioSessionControl* pAudioSessionControl = NULL;
	IAudioSessionControl2* pAudioSessionControl2 = NULL;

    
	hr = getDefaultAudioEndPointDevice(&pAudioEndPoint);
	PRINT_ON_ERROR(hr);
	//EXIT_ON_ERROR(hr);

	hr = CreateAudioClient(pAudioEndPoint, &pAudioClient);
	PRINT_ON_ERROR(hr);
	//EXIT_ON_ERROR(hr)

	hr = CreateAudioSessionManager(pAudioEndPoint, &pAudioSessionManager);
	PRINT_ON_ERROR(hr);
	//EXIT_ON_ERROR(hr)

		//Audio session enumerator
	pAudioEndPoint->Activate(
		__uuidof(IAudioSessionManager2),
		CLSCTX_ALL,
		NULL,
		(void**)&pAudioSessionManager2
	);

	pAudioEndPoint->Activate(
		__uuidof(IAudioEndpointVolume),
		CLSCTX_ALL,
		NULL,
		(void**)&pAudioEndpointVolume
	);

    std::cout << "Initialized all the things\n";

    
    AudioNotifications *notifying = new AudioNotifications();

	//IAudioSessionNotification *pAudioSessionNotification = NULL;
	//notifying->QueryInterface(__uuidof(IAudioSessionNotification), (void **) & pAudioSessionNotification);

    hr = pAudioSessionManager2->RegisterSessionNotification(notifying);
	PRINT_ON_ERROR(hr);

    hr = pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);
	PRINT_ON_ERROR(hr);

    int tempcount = 0;
    hr = pAudioSessionEnumerator->GetCount(&tempcount);
    PRINT_ON_ERROR(hr)

    std::cout << "called the functions\n";
    CAudioSessionEvents *audiosess = new CAudioSessionEvents();
    pAudioSessionControl->RegisterAudioSessionNotification(audiosess);

    for (int i = 0; i < tempcount; i++) {
        hr = pAudioSessionEnumerator->GetSession(i, &pAudioSessionControl);
        PRINT_ON_ERROR(hr);

        hr = pAudioSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudio);
        PRINT_ON_ERROR(hr);

        wchar_t *dispName;
        pAudioSessionControl->GetDisplayName(&dispName);
        std::wcout << "Name: " << dispName;
        float vol = -1;
        pSimpleAudio->GetMasterVolume(&vol);
        std::cout << "\n Volume: " << vol << "\n\n";

		
		SAFE_RELEASE(pSimpleAudio);
        SAFE_RELEASE(pAudioSessionControl);

    }

    std::cout << "Enumerating through the things\n";

	std::string masterbuf("");
	while (true)
	{

        /*
		int result = check(sp_blocking_read(rx_port, buf, 6, timeout));
		buf[result] = '\0';
		masterbuf.append(buf);
		int rotCW = -1;
		int index = -1;

		while (true) {
			int searched_index = masterbuf.find("\n");
			if (searched_index != -1) {
				std::string usbmessage = masterbuf.substr(0, searched_index);
				masterbuf = masterbuf.erase(0, searched_index+1);

				std::cout << usbmessage << "\n";

				int comsplit = usbmessage.find(",");
				if (comsplit != -1) {
					index = stoi(usbmessage.substr(0, comsplit));
					rotCW = stoi(usbmessage.substr(comsplit + 1, std::string::npos));
				}

				if (index < 0 || rotCW < 0) {
					std::cout << "SOMETHING IS WRONG INDEX: " << index << " rotCW: " << rotCW;
				}



				if (index == 0) {
					float vol = 0;
					pAudioEndpointVolume->GetMasterVolumeLevelScalar(&vol);
					if (rotCW)
						vol = vol + 0.01 < 100 ? vol + 0.01 : vol;
					else
						vol = vol - 0.01 < 100 ? vol - 0.01 : vol;
					pAudioEndpointVolume->SetMasterVolumeLevelScalar(vol, NULL);
				}
				else {
					index--;
					
					mtx.lock();
					if (index < volumeVector.size()) {
						float vol = 0;
						vol = volumeVector.at(index).getVolume();
						if (rotCW)
							vol = vol + 0.01 < 100 ? vol + 0.01 : vol;
						else
							vol = vol - 0.01 < 100 ? vol - 0.01 : vol;
						volumeVector.at(index).setVolume(vol);
					}
					mtx.unlock();
				}


			}
			else {
				break;
			}
		}
    */
	}


	//Clean up----------------------------------------------------------------------------
//	free(buf);

//	sp_free_port(ports);

    pAudioSessionControl->UnregisterAudioSessionNotification(audiosess);
	pAudioSessionManager2->UnregisterSessionNotification(notifying);
	SAFE_RELEASE(pAudioSessionEnumerator);
	SAFE_RELEASE(pAudioEndPoint);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pAudioSessionManager);
	SAFE_RELEASE(pAudioSessionManager2);
	SAFE_RELEASE(pAudioEndpointVolume);
	SAFE_RELEASE(notifying);
	return 0;
}//END------------------------------------------------------------------------------------




















//----------------------------------------------------------------------------------------
//--------------------------------HELPERS-------------------------------------------------
//----------------------------------------------------------------------------------------

HRESULT getDefaultAudioEndPointDevice(IMMDevice** ppAudioEndPoint) {
	if (!ppAudioEndPoint)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pEnumerator = NULL;


	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);

	pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, ppAudioEndPoint);


	return hr;
}

void PrintEndpointNames()
{
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDeviceCollection* pCollection = NULL;
	IMMDevice* pEndpoint = NULL;
	IPropertyStore* pProps = NULL;
	LPWSTR pwszID = NULL;
	DWORD state = -1;

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

		hr = pEnumerator->EnumAudioEndpoints(
			eRender, DEVICE_STATE_ACTIVE,
			&pCollection);
	EXIT_ON_ERROR(hr)

		UINT  count;
	hr = pCollection->GetCount(&count);
	EXIT_ON_ERROR(hr)

		if (count == 0)
		{
			printf("No endpoints found.\n");
		}

	// Each loop prints the name of an endpoint device.
	for (ULONG i = 0; i < count; i++)
	{
		// Get pointer to endpoint number i.
		hr = pCollection->Item(i, &pEndpoint);
		EXIT_ON_ERROR(hr)

			// Get the endpoint ID string.
			hr = pEndpoint->GetId(&pwszID);
		EXIT_ON_ERROR(hr)

			hr = pEndpoint->GetState(&state);
		EXIT_ON_ERROR(hr);

		hr = pEndpoint->OpenPropertyStore(
			STGM_READ, &pProps);
		EXIT_ON_ERROR(hr)

			PROPVARIANT varName;
		// Initialize container for property value.
		PropVariantInit(&varName);

		// Get the endpoint's friendly-name property.
		hr = pProps->GetValue(
			PKEY_Device_FriendlyName, &varName);
		EXIT_ON_ERROR(hr)

			// Print endpoint friendly name and endpoint ID.
			printf("Endpoint %d: \"%S\" (%S)\t%d\n",
				i, varName.pwszVal, pwszID, state);

		CoTaskMemFree(pwszID);
		pwszID = NULL;
		PropVariantClear(&varName);
		SAFE_RELEASE(pProps)
			SAFE_RELEASE(pEndpoint)
	}
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pCollection)
		return;

Exit:
	printf("Error!\n");
	std::cout << hr;
	CoTaskMemFree(pwszID);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pCollection)
		SAFE_RELEASE(pEndpoint)
		SAFE_RELEASE(pProps)
}

#define REFTIMES_PER_SEC  10000000

HRESULT CreateAudioClient(IMMDevice* pDevice, IAudioClient** ppAudioClient)
{
	if (!pDevice)
	{
		return E_INVALIDARG;
	}

	if (!ppAudioClient)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	WAVEFORMATEX* pwfx = NULL;

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;

	UINT32 nFrames = 0;

	IAudioClient* pAudioClient = NULL;

	// Get the audio client.
	hr = pDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		NULL,
		(void**)&pAudioClient);

	// Get the device format.
	hr = pAudioClient->GetMixFormat(&pwfx);

	// Open the stream and associate it with an audio session.
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		hnsRequestedDuration,
		hnsRequestedDuration,
		pwfx,
		NULL);

	// If the requested buffer size is not aligned...
	if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
	{
		// Get the next aligned frame.
		hr = pAudioClient->GetBufferSize(&nFrames);

		hnsRequestedDuration = (REFERENCE_TIME)
			((10000.0 * 1000 / pwfx->nSamplesPerSec * nFrames) + 0.5);

		// Release the previous allocations.
		SAFE_RELEASE(pAudioClient);
		CoTaskMemFree(pwfx);

		// Create a new audio client.
		hr = pDevice->Activate(
			__uuidof(IAudioClient),
			CLSCTX_ALL,
			NULL,
			(void**)&pAudioClient);

		// Get the device format.
		hr = pAudioClient->GetMixFormat(&pwfx);

		// Open the stream and associate it with an audio session.
		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			hnsRequestedDuration,
			hnsRequestedDuration,
			pwfx,
			NULL);
	}
	else
	{
		hr;
	}

	// Return to the caller.
	*(ppAudioClient) = pAudioClient;
	(*ppAudioClient)->AddRef();


	// Clean up.
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pAudioClient);
	return hr;
}

HRESULT CreateAudioSessionManager(IMMDevice* pDevice, IAudioSessionManager** ppAudioSessionManager)
{
	if (!pDevice)
	{
		return E_INVALIDARG;
	}

	if (!ppAudioSessionManager)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	IAudioSessionManager* pAudioSessionManager = NULL;

	// Get the audio client.
	hr = pDevice->Activate(
		__uuidof(IAudioSessionManager),
		CLSCTX_ALL,
		NULL,
		(void**)&pAudioSessionManager);


	// Return to the caller.
	*(ppAudioSessionManager) = pAudioSessionManager;
	(*ppAudioSessionManager)->AddRef();


	// Clean up.
	SAFE_RELEASE(pAudioSessionManager);
	return hr;
}



/* Helper function for error handling. */
int check(enum sp_return result)
{
	/* For this example we'll just exit on any error by calling abort(). */
	char* error_message;

	switch (result) {
	case SP_ERR_ARG:
		printf("Error: Invalid argument.\n");
		abort();
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		printf("Error: Failed: %s\n", error_message);
		sp_free_error_message(error_message);
		abort();
	case SP_ERR_SUPP:
		printf("Error: Not supported.\n");
		abort();
	case SP_ERR_MEM:
		printf("Error: Couldn't allocate memory.\n");
		abort();
	case SP_OK:
	default:
		return result;
	}
}
