#pragma once

#include <Audioclient.h>
#include <strsafe.h>
#include <iostream>

class ProgramVolume
{
private:



public:
	int printable = TRUE;
	ISimpleAudioVolume* pSimpleAudio;
	TCHAR progName[2048] = TEXT("NO NAME");
	DWORD progNameSize = 2048;
	//LPWSTR displayName = NULL;

	ProgramVolume(ISimpleAudioVolume* pSimpleAudio) {
		if (pSimpleAudio == NULL)
			throw ERROR_BAD_ARGUMENTS;
		this->pSimpleAudio = pSimpleAudio;
	}

	ProgramVolume(const ProgramVolume& p) {
		pSimpleAudio = p.pSimpleAudio;
		progNameSize = p.progNameSize;
		StringCbCopy(progName, progNameSize, p.progName);


	}

	~ProgramVolume() {
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
		std::wcout << "ProgName: " << progName;
		std::cout << "\nVolume: " << getVolume() << "\n";
	}

};
