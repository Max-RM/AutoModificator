#define NOMINMAX 1
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "Comctl32.lib")

static const wchar_t* kAppTitle = L"AutoModificator v020";
static const wchar_t* kStatusBarText = L"2025 \u00A9    TNT ENTERTAINMENT inc";
static const bool kDisableResize = true;
static const int kMinimumWidth = 380;

struct PatchEntry {
	uint64_t address;
	std::string defaultHex; // e.g. "0C"
	std::string targetHex;  // e.g. "FF"
};

struct SwitchEntry {
	std::wstring title;
	std::vector<PatchEntry> patches;
};

struct AppConfig {
	std::wstring path; // path to file to patch (relative to EXE directory allowed)
	std::vector<SwitchEntry> switches;
	bool enableAll = false; // show master toggle if true
	bool enableRam = false; // show RAM Patch Mode toggle if true
	bool ramWarningDisabled = false; // skip RAM warning dialog if true
};

// Utility: trim spaces
static inline void Trim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){return !isspace(ch);}));
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){return !isspace(ch);}).base(), s.end());
}

// Utility: split by delimiter with trimming
static std::vector<std::string> SplitAndTrim(const std::string& s, const std::string& delim) {
	std::vector<std::string> parts;
	std::string::size_type start = 0;
	while (true) {
		auto pos = s.find(delim, start);
		std::string token = s.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
		Trim(token);
		parts.push_back(token);
		if (pos == std::string::npos) break;
		start = pos + delim.size();
	}
	return parts;
}

// Extract the substring of an array starting at the '[' position with proper depth handling
static bool ExtractBracketArray(const std::string& text, size_t startBracketPos, std::string& outArrayContent) {
	if (startBracketPos == std::string::npos || startBracketPos >= text.size() || text[startBracketPos] != '[') return false;
	int depth = 0;
	for (size_t i = startBracketPos; i < text.size(); ++i) {
		char c = text[i];
		if (c == '[') {
			if (depth == 0) {
				// start after this
			}
			depth++;
		} else if (c == ']') {
			depth--;
			if (depth == 0) {
				// content is between startBracketPos+1 and i-1
				outArrayContent = text.substr(startBracketPos + 1, i - startBracketPos - 1);
				return true;
			}
		}
	}
	return false;
}

// Minimal JSON parser for specific schema {"path":"...","switches":[{"title":"...","patches":["..."]}, ...]}
// Assumes valid JSON without comments.
static bool ParseJsonConfig(const std::wstring& filePath, AppConfig& out) {
	std::ifstream f;
	f.open(filePath.c_str(), std::ios::binary);
	if (!f) return false;
	std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	// Extract path
	auto findStringValue = [&](const char* key) -> std::string {
		std::string k = std::string("\"") + key + "\"";
		auto p = content.find(k);
		if (p == std::string::npos) return {};
		p = content.find(':', p);
		if (p == std::string::npos) return {};
		p = content.find('"', p);
		if (p == std::string::npos) return {};
		auto q = content.find('"', p + 1);
		if (q == std::string::npos) return {};
		return content.substr(p + 1, q - (p + 1));
	};

	// Extract boolean (true/false) loosely
	auto findBooleanValue = [&](const char* key, bool& valueOut) -> bool {
		std::string k = std::string("\"") + key + "\"";
		auto p = content.find(k);
		if (p == std::string::npos) return false;
		p = content.find(':', p);
		if (p == std::string::npos) return false;
		// skip whitespace
		p++;
		while (p < content.size() && isspace(static_cast<unsigned char>(content[p]))) p++;
		if (content.compare(p, 4, "true") == 0) { valueOut = true; return true; }
		if (content.compare(p, 5, "false") == 0) { valueOut = false; return true; }
		return false;
	};

	std::string pathUtf8 = findStringValue("path");
	if (pathUtf8.empty()) return false;
	// UTF-8 to UTF-16 naive (assumes ASCII in config paths)
	out.path = std::wstring(pathUtf8.begin(), pathUtf8.end());

	// Optional master toggle
	bool master = false;
	if (findBooleanValue("enableAll", master)) {
		out.enableAll = master;
	}

	// Optional RAM toggle and warning disabled flag
	bool ramToggle = false;
	if (findBooleanValue("enableRam", ramToggle)) {
		out.enableRam = ramToggle;
	}
	bool warnDisabled = false;
	if (findBooleanValue("ramWarningDisabled", warnDisabled)) {
		out.ramWarningDisabled = warnDisabled;
	}

	// Find switches array with proper depth handling
	std::string switchesKey = "\"switches\"";
	auto sp = content.find(switchesKey);
	if (sp == std::string::npos) { out.switches.clear(); return true; }
	sp = content.find('[', sp);
	if (sp == std::string::npos) { out.switches.clear(); return true; }
	std::string switchesArray;
	if (!ExtractBracketArray(content, sp, switchesArray)) { out.switches.clear(); return true; }

	// Split objects by '}' boundaries while tolerating commas
	std::vector<std::string> objects;
	{
		int depth = 0;
		std::string current;
		for (size_t i = 0; i < switchesArray.size(); ++i) {
			char c = switchesArray[i];
			if (c == '{') {
				if (depth == 0) current.clear();
				depth++;
				current.push_back(c);
			} else if (c == '}') {
				current.push_back(c);
				depth--;
				if (depth == 0) {
					objects.push_back(current);
				}
			} else if (depth > 0) {
				current.push_back(c);
			}
		}
	}

	out.switches.clear();
	for (auto& obj : objects) {
		SwitchEntry sw;
		// title
		auto titlePos = obj.find("\"title\"");
		if (titlePos != std::string::npos) {
			auto p = obj.find('"', obj.find(':', titlePos));
			auto q = obj.find('"', p + 1);
			if (p != std::string::npos && q != std::string::npos) {
				std::string t = obj.substr(p + 1, q - (p + 1));
				sw.title = std::wstring(t.begin(), t.end());
			}
		}
		// patches array
		auto patchesPos = obj.find("\"patches\"");
		if (patchesPos != std::string::npos) {
			auto ap = obj.find('[', patchesPos);
			std::string arr;
			if (ap != std::string::npos && ExtractBracketArray(obj, ap, arr)) {
				// extract string items
				std::vector<std::string> items;
				std::string cur;
				bool inStr = false;
				for (size_t i = 0; i < arr.size(); ++i) {
					char c = arr[i];
					if (c == '"') {
						if (inStr) {
							items.push_back(cur);
							cur.clear();
							inStr = false;
						} else {
							inStr = true;
						}
					} else if (inStr) {
						cur.push_back(c);
					}
				}
				for (auto& s : items) {
					auto parts = SplitAndTrim(s, ">>");
					if (parts.size() == 3) {
						PatchEntry pe{};
						// address like 0x31D5EBC
						std::string addr = parts[0];
						Trim(addr);
						if (addr.rfind("0x", 0) == 0 || addr.rfind("0X", 0) == 0) addr = addr.substr(2);
						pe.address = std::strtoull(addr.c_str(), nullptr, 16);
						std::string defv = parts[1]; Trim(defv);
						std::string tgv = parts[2]; Trim(tgv);
						// Uppercase hex
						std::transform(defv.begin(), defv.end(), defv.begin(), ::toupper);
						std::transform(tgv.begin(), tgv.end(), tgv.begin(), ::toupper);
						pe.defaultHex = defv;
						pe.targetHex = tgv;
						sw.patches.push_back(pe);
					}
				}
			}
		}
		out.switches.push_back(sw);
	}
	return true;
}

static std::wstring GetExecutableDir() {
	wchar_t buf[MAX_PATH];
	GetModuleFileNameW(nullptr, buf, MAX_PATH);
	std::wstring path(buf);
	size_t pos = path.find_last_of(L"/\\");
	return (pos == std::wstring::npos) ? L"." : path.substr(0, pos);
}

static std::wstring NormalizeRelativePath(const std::wstring& baseDir, const std::wstring& rel) {
	if (rel.size() >= 2 && (rel[1] == L':' || (rel[0] == L'/' || rel[0] == L'\\'))) {
		return rel; // absolute
	}
	std::wstring result = baseDir + L"\\" + rel;
	// handle "..\\" segments roughly
	for (;;) {
		size_t pos = result.find(L"\\..\\");
		if (pos == std::wstring::npos) break;
		size_t prev = result.rfind(L'\\', pos - 1);
		if (prev == std::wstring::npos) break;
		result.erase(prev, (pos + 3) - prev);
	}
	return result;
}

// UI state
struct CheckboxState {
	HWND hWnd;
	bool disabled;
	std::vector<PatchEntry> patches;
};

static AppConfig g_config;
static std::wstring g_targetFile;
static std::vector<CheckboxState> g_checkboxes;
static HWND g_mainWnd = nullptr;
static HWND g_status = nullptr;
static HWND g_masterToggle = nullptr;
static const UINT kMasterToggleId = 9000;
static HWND g_ramToggle = nullptr;
static const UINT kRamToggleId = 9001;

// RAM patching state
static bool g_ramModeEnabled = false;
static HANDLE g_targetProcess = nullptr;
static uintptr_t g_targetModuleBase = 0; // base address of main module in target process

struct PeSectionInfo {
	uint32_t virtualAddress; // RVA
	uint32_t sizeOfRawData;
	uint32_t pointerToRawData; // file offset
};
static std::vector<PeSectionInfo> g_peSections; // built from on-disk file

static bool BuildPeSectionsFromFile(const std::wstring& filePath) {
	std::ifstream f(filePath.c_str(), std::ios::binary);
	if (!f) return false;
	// Read DOS header
	IMAGE_DOS_HEADER dos{};
	if (!f.read(reinterpret_cast<char*>(&dos), sizeof(dos))) return false;
	if (dos.e_magic != IMAGE_DOS_SIGNATURE) return false;
	// Read NT headers signature
	if (!f.seekg(dos.e_lfanew, std::ios::beg)) return false;
	DWORD sig = 0;
	if (!f.read(reinterpret_cast<char*>(&sig), sizeof(sig))) return false;
	if (sig != IMAGE_NT_SIGNATURE) return false;
	// Read file header
	IMAGE_FILE_HEADER fh{};
	if (!f.read(reinterpret_cast<char*>(&fh), sizeof(fh))) return false;
	// Skip optional header to sections but we need size to skip fixed structure variable sized
	WORD magic = 0;
	std::streampos optStart = f.tellg();
	if (!f.read(reinterpret_cast<char*>(&magic), sizeof(magic))) return false;
	// Go back to start of optional header
	f.seekg(optStart, std::ios::beg);
	ULONG optSize = fh.SizeOfOptionalHeader;
	if (!f.seekg(optSize, std::ios::cur)) return false;
	// Now at section headers
	g_peSections.clear();
	for (WORD i = 0; i < fh.NumberOfSections; ++i) {
		IMAGE_SECTION_HEADER sh{};
		if (!f.read(reinterpret_cast<char*>(&sh), sizeof(sh))) return false;
		PeSectionInfo si{};
		si.virtualAddress = sh.VirtualAddress;
		si.sizeOfRawData = sh.SizeOfRawData;
		si.pointerToRawData = sh.PointerToRawData;
		g_peSections.push_back(si);
	}
	return !g_peSections.empty();
}

static uint64_t FileOffsetToRva(uint64_t fileOffset) {
	for (const auto& s : g_peSections) {
		uint64_t start = s.pointerToRawData;
		uint64_t end = start + s.sizeOfRawData;
		if (fileOffset >= start && fileOffset < end) {
			uint64_t delta = fileOffset - start;
			return (uint64_t)s.virtualAddress + delta;
		}
	}
	return fileOffset; // fallback: assume already RVA
}

static bool EnableDebugPrivilege() {
	HANDLE hToken = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) return false;
	TOKEN_PRIVILEGES tp{};
	LUID luid{};
	if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) { CloseHandle(hToken); return false; }
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
	CloseHandle(hToken);
	return ok && GetLastError() == ERROR_SUCCESS;
}

static DWORD FindProcessIdByExeName(const std::wstring& exeNameLower) {
	DWORD result = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return 0;
	PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
	if (Process32FirstW(snapshot, &pe)) {
		do {
			std::wstring name = pe.szExeFile;
			std::transform(name.begin(), name.end(), name.begin(), ::towlower);
			if (name == exeNameLower) { result = pe.th32ProcessID; break; }
		} while (Process32NextW(snapshot, &pe));
	}
	CloseHandle(snapshot);
	return result;
}

static uintptr_t GetModuleBaseAddress(DWORD pid, const std::wstring& exeNameLower) {
	uintptr_t base = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snapshot == INVALID_HANDLE_VALUE) return 0;
	MODULEENTRY32W me{}; me.dwSize = sizeof(me);
	if (Module32FirstW(snapshot, &me)) {
		do {
			std::wstring name = me.szModule;
			std::transform(name.begin(), name.end(), name.begin(), ::towlower);
			if (name == exeNameLower) { base = (uintptr_t)me.modBaseAddr; break; }
		} while (Module32NextW(snapshot, &me));
	}
	CloseHandle(snapshot);
	return base;
}

static bool AttachToTargetProcess() {
	// Determine exe name from g_targetFile
	size_t pos = g_targetFile.find_last_of(L"/\\");
	std::wstring exeName = (pos == std::wstring::npos) ? g_targetFile : g_targetFile.substr(pos + 1);
	std::wstring exeLower = exeName; std::transform(exeLower.begin(), exeLower.end(), exeLower.begin(), ::towlower);
	EnableDebugPrivilege();
	DWORD pid = FindProcessIdByExeName(exeLower);
	if (pid == 0) return false;
	HANDLE h = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (!h) return false;
	uintptr_t base = GetModuleBaseAddress(pid, exeLower);
	if (base == 0) { CloseHandle(h); return false; }
	g_targetProcess = h;
	g_targetModuleBase = base;
	return true;
}

static void DetachFromTargetProcess() {
	if (g_targetProcess) { CloseHandle(g_targetProcess); g_targetProcess = nullptr; }
	g_targetModuleBase = 0;
}

static INT_PTR ShowRamWarningDialogAndGetResult(bool& dontShowAgain) {
	// Prefer TaskDialogIndirect for checkbox if available
	if (HMODULE hComctl = LoadLibraryW(L"comctl32.dll")) {
		auto TaskDialogIndirectPtr = (decltype(&TaskDialogIndirect))GetProcAddress(hComctl, "TaskDialogIndirect");
		if (TaskDialogIndirectPtr) {
			TASKDIALOGCONFIG cfg{}; cfg.cbSize = sizeof(cfg);
			cfg.hwndParent = g_mainWnd;
			cfg.pszWindowTitle = kAppTitle;
			cfg.pszMainInstruction = L"RAM Patch Mode";
			cfg.pszContent = L"Make sure target app is running and AutoModificator is started as Administrator. Errors may occur.";
			cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
			cfg.pszVerificationText = L"Don't show this window again";
			BOOL verification = FALSE;
			int button = 0;
			TaskDialogIndirectPtr(&cfg, &button, nullptr, &verification);
			dontShowAgain = verification ? true : false;
			FreeLibrary(hComctl);
			return (button == IDOK) ? IDOK : IDCANCEL;
		}
		FreeLibrary(hComctl);
	}
	int btn = MessageBoxW(g_mainWnd, L"RAM Patch Mode Warning. Make sure target app is running and you have admin rights. Errors may occur.", kAppTitle, MB_ICONWARNING | MB_OKCANCEL);
	dontShowAgain = false;
	return btn;
}

static bool ReadByteAt(uint64_t address, BYTE& out) {
	std::ifstream f;
	f.open(g_targetFile.c_str(), std::ios::binary);
	if (!f) return false;
	f.seekg(static_cast<std::streamoff>(address), std::ios::beg);
	char b = 0;
	if (!f.read(&b, 1)) return false;
	out = static_cast<BYTE>(b);
	return true;
}

static bool WriteByteAt(uint64_t address, BYTE value) {
	if (!g_ramModeEnabled) {
		std::fstream f;
		f.open(g_targetFile.c_str(), std::ios::in | std::ios::out | std::ios::binary);
		if (!f) return false;
		f.seekp(static_cast<std::streamoff>(address), std::ios::beg);
		char b = static_cast<char>(value);
		f.write(&b, 1);
		f.flush();
		return (bool)f;
	} else {
		if (!g_targetProcess || g_targetModuleBase == 0) return false;
		SIZE_T written = 0;
		uintptr_t targetAddr = g_targetModuleBase + (uintptr_t)address; // address expected as RVA in RAM mode
		BYTE v = value;
		BOOL ok = WriteProcessMemory(g_targetProcess, (LPVOID)targetAddr, &v, 1, &written);
		return ok && written == 1;
	}
}

static bool ParseHexByte(const std::string& s, BYTE& out) {
	if (s.size() < 2) return false;
	unsigned int v = 0;
	std::stringstream ss;
	ss << std::hex << s;
	ss >> v;
	if (ss.fail()) return false;
	out = static_cast<BYTE>(v & 0xFFu);
	return true;
}

static void ShowErrorBox(const wchar_t* text) {
	MessageBoxW(g_mainWnd, text, kAppTitle, MB_ICONERROR | MB_OK);
}

static void LayoutAndCreateControls(HWND hwnd) {
	const int padding = 10;
	const int colGap = 10;
	const int rowGap = 6;
	const int bottomBarHeight = 24;

	HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
	HDC hdc = GetDC(hwnd);
	SelectObject(hdc, hFont);
	SIZE textSize{0,0};
	int longest = 0;
	std::vector<std::wstring> titles;
	for (auto& s : g_config.switches) {
		titles.push_back(s.title);
		GetTextExtentPoint32W(hdc, s.title.c_str(), (int)s.title.size(), &textSize);
		longest = std::max(longest, (int)textSize.cx);
	}
	ReleaseDC(hwnd, hdc);

	int colWidth = longest + 40; // checkbox box + padding
	int totalCols = 2;
	int totalRows = (int)((titles.size() + 1) / 2);
	int clientWidth = std::max(kMinimumWidth, padding + totalCols * colWidth + (totalCols - 1) * colGap + padding);
	int rowsHeight = (totalRows > 0 ? (totalRows * 24 + (totalRows - 1) * rowGap) : 0);
	int extraBottomToggles = 0;
	if (g_config.enableAll) extraBottomToggles += (rowGap + 22);
	if (g_config.enableRam) extraBottomToggles += (rowGap + 22);
	int clientHeight = padding + rowsHeight + padding + extraBottomToggles + bottomBarHeight + padding;

	// Resize window to fit desired client area
	RECT rc = {0, 0, clientWidth, clientHeight};
	AdjustWindowRectEx(&rc, (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtr(hwnd, GWL_EXSTYLE));
	int winW = rc.right - rc.left;
	int winH = rc.bottom - rc.top;
	SetWindowPos(hwnd, nullptr, 0, 0, winW, winH, SWP_NOMOVE | SWP_NOZORDER);

	// Create checkboxes
	g_checkboxes.clear();
	g_checkboxes.reserve(g_config.switches.size());
	for (size_t i = 0; i < g_config.switches.size(); ++i) {
		int row = (int)(i / 2);
		int col = (int)(i % 2);
		int x = padding + col * (colWidth + colGap);
		int y = padding + row * (24 + rowGap);
		HWND h = CreateWindowExW(0, L"BUTTON", g_config.switches[i].title.c_str(),
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, x, y, colWidth, 22, hwnd, (HMENU)(UINT_PTR)(1000 + i), GetModuleHandleW(nullptr), nullptr);
		CheckboxState state{};
		state.hWnd = h;
		state.disabled = false;
		state.patches = g_config.switches[i].patches;
		g_checkboxes.push_back(state);
	}

	// Status bar (common control, auto-docked to bottom)
	if (!g_status) {
		g_status = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
			WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
			0, 0, 0, 0,
			hwnd, (HMENU)(UINT_PTR)1, GetModuleHandleW(nullptr), nullptr);
		// set text
		SendMessageW(g_status, SB_SIMPLE, FALSE, 0);
		SendMessageW(g_status, SB_SETTEXTW, 0, (LPARAM)kStatusBarText);
	} else {
		SendMessageW(g_status, WM_SIZE, 0, 0);
	}

	// Initialize checkbox states and disable invalids if needed
	for (size_t i = 0; i < g_checkboxes.size(); ++i) {
		bool allPatchesMatchTarget = true;
		bool allPatchesMatchDefault = true;
		for (auto& p : g_checkboxes[i].patches) {
			BYTE b = 0, dv = 0, tv = 0;
			bool okRead = ReadByteAt(p.address, b);
			bool okDv = ParseHexByte(p.defaultHex, dv);
			bool okTv = ParseHexByte(p.targetHex, tv);
			if (!okRead || !okDv || !okTv) { allPatchesMatchDefault = allPatchesMatchTarget = false; break; }
			allPatchesMatchTarget = allPatchesMatchTarget && (b == tv);
			allPatchesMatchDefault = allPatchesMatchDefault && (b == dv);
		}
		if (!allPatchesMatchTarget && !allPatchesMatchDefault) {
			EnableWindow(g_checkboxes[i].hWnd, FALSE);
			g_checkboxes[i].disabled = true;
			ShowErrorBox(L"Invalid values found");
		} else {
			SendMessageW(g_checkboxes[i].hWnd, BM_SETCHECK, allPatchesMatchTarget ? BST_CHECKED : BST_UNCHECKED, 0);
		}
	}

	// Create/position bottom toggles at bottom after individual states are initialized
	if (g_config.enableAll || g_config.enableRam) {
		RECT rcClient{}; GetClientRect(hwnd, &rcClient);
		int statusH = 0;
		if (g_status) {
			RECT rcS{}; GetWindowRect(g_status, &rcS);
			POINT p1{rcS.left, rcS.top}; POINT p2{rcS.right, rcS.bottom};
			ScreenToClient(hwnd, &p1); ScreenToClient(hwnd, &p2);
			statusH = p2.y - p1.y;
		}
		int x = padding;
		int y = rcClient.bottom - statusH - padding - 22;
		// Master toggle (left)
		if (g_config.enableAll) {
			if (!g_masterToggle) {
				g_masterToggle = CreateWindowExW(0, L"BUTTON", L"Enable / Disable All",
					WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
					x, y, 200, 22, hwnd, (HMENU)(UINT_PTR)kMasterToggleId, GetModuleHandleW(nullptr), nullptr);
			} else {
				SetWindowPos(g_masterToggle, nullptr, x, y, 200, 22, SWP_NOZORDER | SWP_NOACTIVATE);
			}
			// Set master check based on all individual states
			bool allChecked = true;
			bool anyEnabled = false;
			for (auto& cb : g_checkboxes) {
				if (!cb.disabled) {
					anyEnabled = true;
					LRESULT st = SendMessageW(cb.hWnd, BM_GETCHECK, 0, 0);
					allChecked = allChecked && (st == BST_CHECKED);
				}
			}
			SendMessageW(g_masterToggle, BM_SETCHECK, (anyEnabled && allChecked) ? BST_CHECKED : BST_UNCHECKED, 0);
		}
		// RAM mode toggle (right of master or alone at left)
		if (g_config.enableRam) {
			int xRam = x + (g_config.enableAll ? 210 : 0);
			if (!g_ramToggle) {
				g_ramToggle = CreateWindowExW(0, L"BUTTON", L"RAM Patch Mode",
					WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
					xRam, y, 180, 22, hwnd, (HMENU)(UINT_PTR)kRamToggleId, GetModuleHandleW(nullptr), nullptr);
			} else {
				SetWindowPos(g_ramToggle, nullptr, xRam, y, 180, 22, SWP_NOZORDER | SWP_NOACTIVATE);
			}
			SendMessageW(g_ramToggle, BM_SETCHECK, g_ramModeEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
		}
	}
}

static void ApplySwitch(size_t index, bool enabled) {
	if (index >= g_checkboxes.size()) return;
    for (auto& p : g_checkboxes[index].patches) {
        BYTE v = 0;
        if (!ParseHexByte(enabled ? p.targetHex : p.defaultHex, v)) continue;
        uint64_t addr = p.address;
        if (g_ramModeEnabled) {
            // Convert file offset (ROM) to RVA for RAM patching
            addr = FileOffsetToRva(addr);
        }
        WriteByteAt(addr, v);
    }
}

static void SetAllSwitches(bool enabled) {
	for (size_t i = 0; i < g_checkboxes.size(); ++i) {
		if (g_checkboxes[i].disabled) continue;
		SendMessageW(g_checkboxes[i].hWnd, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
		ApplySwitch(i, enabled);
	}
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		InitCommonControls();
		LayoutAndCreateControls(hwnd);
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			UINT id = LOWORD(wParam);
			if (id == kMasterToggleId && g_config.enableAll) {
				LRESULT state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
				SetAllSwitches(state == BST_CHECKED);
			} else if (id == kRamToggleId && g_config.enableRam) {
				LRESULT state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
				bool wantEnable = (state == BST_CHECKED);
				if (wantEnable && !g_config.ramWarningDisabled) {
					bool dontShow = false;
					INT_PTR res = ShowRamWarningDialogAndGetResult(dontShow);
					if (dontShow) { g_config.ramWarningDisabled = true; }
					if (res != IDOK) {
						SendMessageW(g_ramToggle, BM_SETCHECK, BST_UNCHECKED, 0);
						return 0;
					}
				}
				if (wantEnable) {
					if (!BuildPeSectionsFromFile(g_targetFile) || !AttachToTargetProcess()) {
						ShowErrorBox(L"Failed to enable RAM mode. Ensure target app is running and admin rights.");
						SendMessageW(g_ramToggle, BM_SETCHECK, BST_UNCHECKED, 0);
						return 0;
					}
				} else {
					DetachFromTargetProcess();
				}
				g_ramModeEnabled = wantEnable;
			} else if (id >= 1000) {
				size_t idx = id - 1000;
				LRESULT state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
				ApplySwitch(idx, state == BST_CHECKED);
				// update master toggle state if present
				if (g_masterToggle && g_config.enableAll) {
					bool allChecked = true;
					bool anyEnabled = false;
					for (auto& cb : g_checkboxes) {
						if (!cb.disabled) {
							anyEnabled = true;
							LRESULT st = SendMessageW(cb.hWnd, BM_GETCHECK, 0, 0);
							allChecked = allChecked && (st == BST_CHECKED);
						}
					}
					SendMessageW(g_masterToggle, BM_SETCHECK, (anyEnabled && allChecked) ? BST_CHECKED : BST_UNCHECKED, 0);
				}
			}
		}
		return 0;
	case WM_GETMINMAXINFO:
		if (kDisableResize) {
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			RECT rc = {0,0,kMinimumWidth,200};
			AdjustWindowRect(&rc, (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE), FALSE);
			mmi->ptMinTrackSize.x = rc.right - rc.left;
			mmi->ptMinTrackSize.y = rc.bottom - rc.top;
		}
		return 0;
	case WM_SIZE:
		if (g_status) {
			SendMessageW(g_status, WM_SIZE, wParam, lParam);
		}
	// Re-anchor master toggle to bottom on resize
	if (g_masterToggle && g_config.enableAll) {
		RECT rcClient{}; GetClientRect(hwnd, &rcClient);
		int statusH = 0;
		if (g_status) {
			RECT rcS{}; GetWindowRect(g_status, &rcS);
			POINT p1{rcS.left, rcS.top}; POINT p2{rcS.right, rcS.bottom};
			ScreenToClient(hwnd, &p1); ScreenToClient(hwnd, &p2);
			statusH = p2.y - p1.y;
		}
		int padding = 10;
		int x = padding;
		int y = rcClient.bottom - statusH - padding - 22;
		SetWindowPos(g_masterToggle, nullptr, x, y, 200, 22, SWP_NOZORDER | SWP_NOACTIVATE);
	}
	// Re-anchor RAM toggle
	if (g_ramToggle && g_config.enableRam) {
		RECT rcClient{}; GetClientRect(hwnd, &rcClient);
		int statusH = 0;
		if (g_status) {
			RECT rcS{}; GetWindowRect(g_status, &rcS);
			POINT p1{rcS.left, rcS.top}; POINT p2{rcS.right, rcS.bottom};
			ScreenToClient(hwnd, &p1); ScreenToClient(hwnd, &p2);
			statusH = p2.y - p1.y;
		}
		int padding = 10;
		int x = padding + (g_config.enableAll ? 210 : 0);
		int y = rcClient.bottom - statusH - padding - 22;
		SetWindowPos(g_ramToggle, nullptr, x, y, 180, 22, SWP_NOZORDER | SWP_NOACTIVATE);
	}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmd) {
	// Load config.json from executable directory
	std::wstring exeDir = GetExecutableDir();
	std::wstring configPath = exeDir + L"\\config.json";
	if (!ParseJsonConfig(configPath, g_config)) {
		MessageBoxW(nullptr, L"Failed to read config.json", kAppTitle, MB_ICONERROR | MB_OK);
		return 1;
	}
	g_targetFile = NormalizeRelativePath(exeDir, g_config.path);

	WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
	wc.hIconSm = wc.hIcon;
	wc.lpszClassName = L"AutoModificatorWindow";
	RegisterClassExW(&wc);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (!kDisableResize) style |= WS_THICKFRAME | WS_MAXIMIZEBOX;

	g_mainWnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, kAppTitle, style,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 360, nullptr, nullptr, hInst, nullptr);
	if (!g_mainWnd) return 2;

	ShowWindow(g_mainWnd, nCmd);
	UpdateWindow(g_mainWnd);

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}

//Developed by MaxRM at TNT ENTERTAINMENT inc on September 11,2025
