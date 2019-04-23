#include <vector>
#include "skse64/GameReferences.h"

Actor* GetPlayer() {
  return reinterpret_cast<Actor*>(*g_thePlayer);
}

//Converts Windows1251 text into UTF8.
const char* Cp1251ToUtf8(const char* str) {
  const auto result_u = MultiByteToWideChar(1251, 0, str, -1, nullptr, 0);

  if (!result_u) { return nullptr; }

  const auto ures = new wchar_t[result_u];

  if (!MultiByteToWideChar(1251, 0, str, -1, ures, result_u)) {
    delete[] ures;
    return nullptr;
  }

  const auto result_c = WideCharToMultiByte(65001, 0, ures, -1, nullptr, 0,
                                            nullptr, nullptr);
  if (!result_c) {
    delete[] ures;
    return nullptr;
  }

  const auto cres = new char[result_c];

  if (!WideCharToMultiByte(65001, 0, ures, -1, cres, result_c, nullptr,
                           nullptr)) {
    delete[] cres;
    return nullptr;
  }

  delete[] ures;
  return cres;
}

bool is_valid_utf8(const char * string) {
	if (!string) { return true; }
	auto bytes = reinterpret_cast<const unsigned char *>(string);
	unsigned int cp;
	int num;
	while (*bytes != 0x00) {
		if ((*bytes & 0x80) == 0x00) {
			// U+0000 to U+007F 
			cp = (*bytes & 0x7F);
			num = 1;
		}
		else if ((*bytes & 0xE0) == 0xC0) {
			// U+0080 to U+07FF 
			cp = (*bytes & 0x1F);
			num = 2;
		}
		else if ((*bytes & 0xF0) == 0xE0) {
			// U+0800 to U+FFFF 
			cp = (*bytes & 0x0F);
			num = 3;
		}
		else if ((*bytes & 0xF8) == 0xF0) {
			// U+10000 to U+10FFFF 
			cp = (*bytes & 0x07);
			num = 4;
		}
		else { return false; }
		bytes += 1;
		for (auto i = 1; i < num; ++i) {
			if ((*bytes & 0xC0) != 0x80) { return false; }
			cp = (cp << 6) | (*bytes & 0x3F);
			bytes += 1;
		}
		if ((cp > 0x10FFFF) ||
			((cp <= 0x007F) && (num != 1)) ||
			((cp >= 0xD800) && (cp <= 0xDFFF)) ||
			((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
			((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
			((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4))) {
			return false;
		}
	}
	return true;
}

//Formats a string, allowing you to insert various types into it.
const char* Format(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::vector<char> v(1024);

  while (true) {
    va_list args2;
    va_copy(args2, args);
    const auto res = vsnprintf(v.data(), v.size(), fmt, args2);
    if ((res >= 0) && (res < static_cast<int>(v.size()))) {
      va_end(args);
      va_end(args2);
      return std::string(v.data()).c_str();
    }
    size_t size;

    if (res < 0)
      size = v.size() * 2;
    else
      size = static_cast<size_t>(res) + 1;

    v.clear();
    v.resize(size);
    va_end(args2);
  }
}
