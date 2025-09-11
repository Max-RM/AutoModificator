#define NOMINMAX 1
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "Comctl32.lib")

static const wchar_t* kAppTitle = L"AutoModificator v019";
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

	std::string pathUtf8 = findStringValue("path");
	if (pathUtf8.empty()) return false;
	// UTF-8 to UTF-16 naive (assumes ASCII in config paths)
	out.path = std::wstring(pathUtf8.begin(), pathUtf8.end());

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
	std::fstream f;
	f.open(g_targetFile.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if (!f) return false;
	f.seekp(static_cast<std::streamoff>(address), std::ios::beg);
	char b = static_cast<char>(value);
	f.write(&b, 1);
	f.flush();
	return (bool)f;
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
	int clientHeight = padding + (totalRows > 0 ? (totalRows * 24 + (totalRows - 1) * rowGap) : 0) + padding + bottomBarHeight + padding;

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
}

static void ApplySwitch(size_t index, bool enabled) {
	if (index >= g_checkboxes.size()) return;
	for (auto& p : g_checkboxes[index].patches) {
		BYTE v = 0;
		if (!ParseHexByte(enabled ? p.targetHex : p.defaultHex, v)) continue;
		WriteByteAt(p.address, v);
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
			if (id >= 1000) {
				size_t idx = id - 1000;
				LRESULT state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
				ApplySwitch(idx, state == BST_CHECKED);
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
