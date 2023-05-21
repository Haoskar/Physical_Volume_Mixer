// TestVolume_c++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//These two lines are needed for LoadIconMetric for some reason
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

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
#include <unknwnbase.h>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>


#include <shellapi.h>
#include <Windows.h>
#include <CommCtrl.h>

#include "libserialport.h"
#include "ProgramVolume.h"

#include "resource.h"

std::mutex mtx;
std::atomic<boolean> main_program_loop = false;
std::thread *main_thread = nullptr;

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }



const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);

HRESULT CreateAudioClient(IMMDevice*, IAudioClient**);
HRESULT CreateAudioSessionManager(IMMDevice*, IAudioSessionManager**);
HRESULT getDefaultAudioEndPointDevice(IMMDevice**);
void PrintEndpointNames();

int check(enum sp_return result);
int GUIcheck(HWND, enum sp_return);


//std::vector<ProgramVolume> volumeVector;
IMMDevice* pAudioEndPoint = NULL;
IAudioClient* pAudioClient = NULL;
IAudioSessionManager* pAudioSessionManager = NULL;
IAudioSessionManager2* pAudioSessionManager2 = NULL;
IAudioEndpointVolume* pAudioEndpointVolume = NULL;




class ProgVolumeList {
private:
	std::vector<size_t> newClients;

public:
	bool updated = false;
	std::vector<ProgramVolume*> audioClients;


	void add(ProgramVolume* p) {
		audioClients.push_back(p);
		newClients.push_back(audioClients.size() - 1);
		updated = true;
	}

	/*void add(IAudioSessionControl* pNewSession) {
		ProgramVolume pv(pNewSession);
		audioClients.push_back(pv);
		newClients.push_back(audioClients.size() - 1);
	}*/

	void registerNewClients() {
		for (size_t i : newClients) {
			audioClients.at(i)->registerClient();
		}

		newClients.clear();
	}

	void cleanupList() {
		std::vector<ProgramVolume*> toRemove;
		for (ProgramVolume* p : this->audioClients) {
			bool ret = p->release();
			if (ret) {
				updated = true;
				toRemove.push_back(p);
			}
		}

		std::erase_if(audioClients, [&toRemove](ProgramVolume* p) {
			for (auto removeP : toRemove)
			{
				if (p == removeP) return true;
			}
			return false; });
	}
};

ProgVolumeList audioClients;

class AudioNotifications : public IAudioSessionNotification
{

private:

	LONG             m_cRefAll;
	~AudioNotifications() {};

public:
	AudioNotifications() : m_cRefAll(1) {};

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
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
	ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&m_cRefAll);
	}

	ULONG STDMETHODCALLTYPE Release() {
		ULONG ulRef = InterlockedDecrement(&m_cRefAll);
		if (0 == ulRef)
		{
			std::cout << "\n\nI am delete godbye\n";
			delete this;
		}
		return ulRef;
	}

	HRESULT OnSessionCreated(IAudioSessionControl* pNewSession) {
		std::cout << "\n\n I AM IN THE OnSessionCreated\n";
		HRESULT hr = S_OK;

		ISimpleAudioVolume* pSimpleAudio = NULL;

		hr = pNewSession->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudio);
		PRINT_ON_ERROR(hr);

		wchar_t* dispName;
		pNewSession->GetDisplayName(&dispName);
		std::cout << "I am: " << pNewSession << "\n";
		std::wcout << "Name: " << dispName;
		float vol = -1;
		pSimpleAudio->GetMasterVolume(&vol);
		std::cout << "\nVolume: " << vol << "\n\n";


		SAFE_RELEASE(pSimpleAudio);

		mtx.lock();
		ProgramVolume* pv = new ProgramVolume(pNewSession);
		audioClients.add(pv);
		mtx.unlock();

		return S_OK;
	}
};



int MEGAmain(HWND hwnd, struct sp_port *port)
{
	HRESULT hr = S_OK;

	GUIcheck(hwnd, sp_open(port, SP_MODE_READ_WRITE));

	GUIcheck(hwnd, sp_set_baudrate(port, 9600));
	GUIcheck(hwnd, sp_set_bits(port, 8));
	GUIcheck(hwnd, sp_set_parity(port, SP_PARITY_NONE));
	GUIcheck(hwnd, sp_set_stopbits(port, 1));
	GUIcheck(hwnd, sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));


	char* buf = (char*)malloc(20);
	unsigned int timeout = 0;

	sp_flush(port, SP_BUF_BOTH);

	struct sp_port* rx_port = port;
	struct sp_port* tx_port = port;

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


	AudioNotifications* notifying = new AudioNotifications();


	hr = pAudioSessionManager2->RegisterSessionNotification(notifying);
	PRINT_ON_ERROR(hr);

	hr = pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);
	PRINT_ON_ERROR(hr);

	int tempcount = 0;
	hr = pAudioSessionEnumerator->GetCount(&tempcount);
	PRINT_ON_ERROR(hr)

		std::cout << "called the functions\n";
	mtx.lock();
	for (int i = 0; i < tempcount; i++) {
		hr = pAudioSessionEnumerator->GetSession(i, &pAudioSessionControl);
		PRINT_ON_ERROR(hr);

		ProgramVolume* pv = new ProgramVolume(pAudioSessionControl);

		audioClients.add(pv);

	}
	mtx.unlock();

	std::cout << "Enumerating through the things\n";

	std::string masterbuf("");
	while (main_program_loop)
	{

		//Read from USB
		int result = check(sp_nonblocking_read(rx_port, buf, 16));
		buf[result] = '\0';
		masterbuf.append(buf);
		int rotCW = -1;
		int index = -1;

		size_t searched_index = masterbuf.find("\n");
		if (searched_index != -1) {
			std::string usbmessage = masterbuf.substr(0, searched_index);
			masterbuf = masterbuf.erase(0, searched_index + 1);

			std::cout << usbmessage << "\n";

			size_t comsplit = usbmessage.find(",");
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
					vol = vol + 0.01f < 100 ? vol + 0.01f : vol;
				else
					vol = vol - 0.01f < 100 ? vol - 0.01f : vol;
				pAudioEndpointVolume->SetMasterVolumeLevelScalar(vol, NULL);
			}
			else {
				index--;

				mtx.lock();
				if (index < audioClients.audioClients.size()) {
					float vol = 0;
					vol = audioClients.audioClients.at(index)->getVolume();
					if (rotCW)
						vol = vol + 0.01f < 100 ? vol + 0.01f : vol;
					else
						vol = vol - 0.01f < 100 ? vol - 0.01f : vol;
					audioClients.audioClients.at(index)->setVolume(vol);
				}
				mtx.unlock();
			}


		}

		////Write To USB
		//mtx.lock();
		//if (audioClients.updated) {
		//	char cprogname[50] = "";
		//	size_t ksjdflaksdjflkdjsf = 20;
		//	std::cout << "\n\nSending DATA \n\n";
		//	char start[] = "s\n";
		//	std::cout << "Start: " << check(sp_blocking_write(tx_port, start, std::strlen(start), 0)) << "\n\n";

		//	for (size_t j = 0; j < audioClients.audioClients.size(); ++j) {
		//		std::string data_send;
		//		if (wcstombs_s(&ksjdflaksdjflkdjsf, cprogname, (size_t)50, audioClients.audioClients.at(j)->display_name.c_str(), 50) == EINVAL) {
		//			std::cout << "ERROR CAN'T COPY\n";
		//		}
		//		//data_send.append("+,");
		//		data_send.append(cprogname);
		//		data_send.append("\n");
		//		std::cout << "Size: " << data_send.size() << " " << data_send.size() << " Data send: \"" << data_send.c_str() << "\"\n";
		//		std::cout << "Bytes: " << check(sp_blocking_write(tx_port, data_send.c_str(), std::strlen(data_send.c_str()), 0)) << "\n";
		//	}

		//	char end[] = "e\n";
		//	std::cout << "End: " << check(sp_blocking_write(tx_port, end, std::strlen(end), 0)) << "\n\n";

		//	audioClients.updated = false;
		//}

		//Register new clients
		mtx.lock();
		audioClients.registerNewClients();
		audioClients.cleanupList();

		mtx.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	//Clean up----------------------------------------------------------------------------
	free(buf);

	sp_close(port);
	sp_free_port(port);


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

HINSTANCE g_hInst = NULL;
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
BOOL wm_entermenuloop_first = FALSE;


//CHANGE THESE PLEASE
int numPortsInMenu = 0;
HMENU theShortcutMenu = NULL;
std::vector<std::string> comPorts;
std::string selectedItem;


LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr = S_OK;
	//Menu Command Stuffs
	int wmId;


	switch (uMsg)
	{
	case WM_CREATE: // creating main window  
	{
		NOTIFYICONDATAA ndata;
		ndata.cbSize = sizeof(NOTIFYICONDATAA);
		ndata.hWnd = hwnd;
		ndata.uID = 1;
		ndata.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
		StringCchCopy(ndata.szTip, ARRAYSIZE(ndata.szTip), "Physical Volume Control");
		hr = LoadIconMetric(g_hInst, MAKEINTRESOURCEW(IDI_ICON2), LIM_LARGE, &ndata.hIcon);
		ndata.uCallbackMessage = WMAPP_NOTIFYCALLBACK;

		Shell_NotifyIconA(NIM_ADD, &ndata);

		ndata.uVersion = NOTIFYICON_VERSION_4;
		return Shell_NotifyIcon(NIM_SETVERSION, &ndata);
	}

	case WM_COMMAND:
	{

		wmId = LOWORD(wParam);
		const int wParamHigh = HIWORD(wParam);

		switch (wmId)
		{
		case ID_A_EXIT:
			DestroyWindow(hwnd);
			break;
		default:
		{
			if (wmId > ID_WOOOO_ASD && wmId <= (ID_WOOOO_ASD + numPortsInMenu)) {
				const int menuItemSelected = wmId - ID_WOOOO_ASD - 1;
				selectedItem = comPorts.at(menuItemSelected);

				struct sp_port* port;
				GUIcheck(hwnd, sp_get_port_by_name(selectedItem.c_str(), &port));

				main_program_loop = false;
				if (main_thread != nullptr) {
					main_thread->join();
					delete main_thread;
				}
				

				main_program_loop = true;
				main_thread = new std::thread(MEGAmain, hwnd, port);
			}
			else {
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		}
		}
	}
	break;
	case WM_INITMENUPOPUP:
	{
		if (wm_entermenuloop_first == TRUE) {
			MessageBeep(0xFFFFFFFF);
			wm_entermenuloop_first = FALSE;
			break;
		}

		HMENU hMenu = HMENU(wParam);
		const int pos = LOWORD(lParam);

		struct sp_port** port_list;
		sp_list_ports(&port_list);

		const int menuItemCount = GetMenuItemCount(hMenu);
		
		comPorts.clear();

		numPortsInMenu = 0;
		int menuItemIndex = 0;
		for (; port_list[menuItemIndex] != NULL; menuItemIndex++, numPortsInMenu++)
		{

			MENUITEMINFOA mItem;
			mItem.cbSize = sizeof(MENUITEMINFOA);
			mItem.fMask = MIIM_TYPE | MIIM_ID | MIIM_CHECKMARKS | MIIM_STATE;
			mItem.hbmpChecked = NULL;
			mItem.hbmpUnchecked = NULL;
			mItem.fType = MFT_STRING;
			mItem.wID = ID_WOOOO_ASD + 1 + menuItemIndex;
			mItem.fState = MFS_ENABLED;

			std::string p_name = "UNKNOWN NAME";

			struct sp_port* port = port_list[menuItemIndex];
			
			char* port_name = sp_get_port_name(port);
			if (port_name != NULL)
			{
				p_name = port_name;
			}

			comPorts.push_back(p_name);


			if (selectedItem.compare(p_name) == 0)
				mItem.fState |= MFS_CHECKED;

			char* port_desc = sp_get_port_description(port);
			if (port_desc != NULL) {
				p_name += " | ";
				p_name += port_desc;
			}
			
			mItem.dwTypeData = LPTSTR(p_name.c_str());
			mItem.cch = std::strlen(p_name.c_str());

			//Modify the item that already exists or if there does not exist an item add one
			if (menuItemIndex < menuItemCount)
				SetMenuItemInfoA(hMenu, menuItemIndex, TRUE, &mItem);
			else
				InsertMenuItemA(hMenu, menuItemIndex, TRUE, &mItem);
		}

		//If there are any items left after adding all the COM ports delete them
		for (; menuItemIndex < menuItemCount; menuItemIndex++) {
			DeleteMenu(hMenu, menuItemIndex, MF_BYPOSITION);
		}

		sp_free_port_list(port_list);
		
	}
	break;

	case WM_ENTERMENULOOP:
	{
		wm_entermenuloop_first = TRUE;
		break;
	}
	break;

	case WMAPP_NOTIFYCALLBACK:
		switch (LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
		{
			POINT const pt = { LOWORD(wParam), HIWORD(wParam) };

			HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCEA(IDR_MENU1));
			if (hMenu)
			{
				
				HMENU hSubMenu = GetSubMenu(hMenu, 0);
				theShortcutMenu = hSubMenu;
				if (hSubMenu)
				{

					SetForegroundWindow(hwnd);


					UINT uFlags = TPM_RIGHTBUTTON;
					if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
					{
						uFlags |= TPM_RIGHTALIGN;
					}
					else
					{
						uFlags |= TPM_LEFTALIGN;
					}

					TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
				}
				DestroyMenu(hMenu);
			}
		}
		}
		break;

	case WM_DESTROY:
	{
		NOTIFYICONDATAA ndata;
		ndata.cbSize = sizeof(NOTIFYICONDATAA);
		ndata.hWnd = hwnd;
		ndata.uID = 1;

		main_program_loop = false;
		if (main_thread != nullptr) {
			main_thread->join();
			delete main_thread;
		}

		Shell_NotifyIconA(NIM_DELETE, &ndata);
		PostQuitMessage(WM_QUIT);
	}
	break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

HINSTANCE hinst;
HWND hwndMain;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	MSG msg;
	BOOL bRet;
	WNDCLASS wc;
	UNREFERENCED_PARAMETER(cmdline);

	// Register the window class for the main window. 

	if (!hInstPrev)
	{
		wc.style = 0;
		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = LoadIcon((HINSTANCE)NULL,
			IDI_APPLICATION);
		wc.hCursor = LoadCursor((HINSTANCE)NULL,
			IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = "MainMenu";
		wc.lpszClassName = "MainWndClass";

		if (!RegisterClass(&wc))
			return FALSE;
	}

	g_hInst = hInst;  // save instance handle 

	// Create the main window. 

	hwndMain = CreateWindow("MainWndClass", "Sample",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, (HWND)NULL,
		(HMENU)NULL, hinst, (LPVOID)NULL);

	// If the main window cannot be created, terminate 
	// the application. 

	if (!hwndMain)
		return FALSE;

	// Show the window and paint its contents. 

	//ShowWindow(hwndMain, 5);
	UpdateWindow(hwndMain);

	// Start the message loop. 

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Return the exit code to the system. 

	return msg.wParam;
}















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

/* Helper function for error handling. */
int GUIcheck(HWND hwnd, enum sp_return result)
{
	/* For this example we'll just exit on any error by calling abort(). */
	char* error_message;

	switch (result) {
	case SP_ERR_ARG:
		MessageBox(hwnd, "Error: Invalid argument.", "ERROR", MB_OK | MB_ICONERROR);
		abort();
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		printf("Error: Failed: %s\n", error_message);
		MessageBox(hwnd, (std::string("Error: Failed: ") + std::string(error_message)).c_str(), "ERROR", MB_OK | MB_ICONERROR);
		sp_free_error_message(error_message);
		break;
	case SP_ERR_SUPP:
		MessageBox(hwnd, "Error: Not supported.", "ERROR", MB_OK | MB_ICONERROR);
		abort();
	case SP_ERR_MEM:
		MessageBox(hwnd, "Error: Couldn't allocate memory.", "ERROR", MB_OK | MB_ICONERROR);
		abort();
	}
	return result;
}