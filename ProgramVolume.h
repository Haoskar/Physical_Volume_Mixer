#pragma once

#include <Audioclient.h>
#include <strsafe.h>
#include <iostream>
#include <Audiopolicy.h>
#include <Psapi.h>


#define PRINT_ON_ERROR(hres)  \
               if(FAILED(hres)) {printf("\n\nFAILURE: %X\n\n", hres);}

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
		/*if (NewMute)
		{
			printf("MUTE\n");
		}
		else
		{
			printf("Volume = %d percent\n",
				(UINT32)(100 * NewVolume + 0.5));
		}*/

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
		/*std::string pszState = "?????";

		switch (NewState)
		{
		case AudioSessionStateActive:
			pszState = "active";
			break;
		case AudioSessionStateInactive:
			pszState = "inactive";
			break;
		case AudioSessionStateExpired:
			pszState = "expired";
			break;
		}
		printf("New session state = %s\n", pszState.c_str());*/

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnSessionDisconnected(
		AudioSessionDisconnectReason DisconnectReason)
	{
		/*std::string pszReason = "?????";

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
			pszReason.c_str());*/

		return S_OK;
	}
};

class ProgramVolume
{
private:
	CAudioSessionEvents* audiosess = NULL;

	~ProgramVolume() {
		std::cout << "\n\nI AM KILLING MYSELF GODBYE: ";
		std::wcout << display_name << "\n\n";
		if (audiosess != NULL)
			pAudioSessionControl->UnregisterAudioSessionNotification(audiosess);
		if (pAudioSessionControl != NULL)
			pAudioSessionControl->Release();
		if (pSimpleAudio != NULL)
			pSimpleAudio->Release();

	}

public:
	IAudioSessionControl* pAudioSessionControl = NULL;
	ISimpleAudioVolume* pSimpleAudio = NULL;

	std::wstring display_name = L"";

	ProgramVolume(IAudioSessionControl *pAudioSessionControl){
		HRESULT hr = S_OK;

		if (pAudioSessionControl == NULL)
			throw ERROR_BAD_ARGUMENTS;

		pAudioSessionControl->AddRef();
		this->pAudioSessionControl = pAudioSessionControl;

		hr = pAudioSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudio);
		PRINT_ON_ERROR(hr);

		
		//get the name of the program
		
		wchar_t *tempDName;
		pAudioSessionControl->GetDisplayName(&tempDName);
		display_name.assign(tempDName);
		std::cout << "My SessionControl name is: ";
		std::wcout << display_name << L"\n";
		CoTaskMemFree(tempDName);

		DWORD processId = 0;
		IAudioSessionControl2* pAudioSessionControl2 = NULL;
		
		if (display_name == L"") {
			std::cout << "\nDisplay name is empty\n";
			
			hr = pAudioSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pAudioSessionControl2);
			PRINT_ON_ERROR(hr);
			hr = pAudioSessionControl2->GetProcessId(&processId);
			PRINT_ON_ERROR(hr);

			const DWORD processPathLength = 1024;
			wchar_t processPath[processPathLength] = L"";
			if (GetProcessImageFileNameW(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId), processPath, processPathLength) != 0) {
				wchar_t* wcs;
				wcs = wcsrchr(processPath, L'.');
				if (wcs != NULL) {
					*wcs = L'\0';

					wcs = wcsrchr(processPath, L'\\') + 1;
					display_name.assign(wcs);

				}
			}
		}

		std::cout << "My final display name is: ";
		std::wcout << display_name << L"\n\n";
	}



	bool release() {
		if (pAudioSessionControl == NULL)
			return false;

		AudioSessionState state;
		pAudioSessionControl->GetState(&state);

		if (state == AudioSessionStateExpired) {
			std::cout << "\n\nI AM RELEASING MYSELF GODBYE: ";
			std::wcout << display_name << "\n\n";
			delete this;
			return true;
		}

		return false;
	}

	void mute() { pSimpleAudio->SetMute(TRUE, NULL); }
	void unmute() { pSimpleAudio->SetMute(FALSE, NULL); }


	int setVolume(float volume) {
		if (volume > 1 || volume < 0)
			return ERANGE;
		pSimpleAudio->SetMasterVolume(volume, NULL);
		return 0;
	}

	float getVolume() {
		float vol = 0;
		pSimpleAudio->GetMasterVolume(&vol);
		return vol;
	}

	int mute(const BOOL mute) {
		int r = 0;
		if (mute == TRUE || mute == FALSE) {
			pSimpleAudio->SetMute(mute, NULL);
		}
		else
			r = EDOM;
		return r;
	}

	void print() {
		std::wcout << "ProgName: " << display_name;
		std::cout << "\nVolume: " << getVolume() << "\n";
	}

	void registerClient() {
		audiosess = new CAudioSessionEvents();
		if (audiosess == NULL)
			return;
		if (pAudioSessionControl == NULL)
			return;
		
		pAudioSessionControl->RegisterAudioSessionNotification(audiosess);
	}
};
