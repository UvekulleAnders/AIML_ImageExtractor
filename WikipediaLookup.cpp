// WikipediaLookup.cpp : implementation of the Wikipedia name lookup
//

#include "pch.h"
#include "WikipediaLookup.h"
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

namespace
{
	std::string ToUtf8(const CString& text)
	{
		const int length = ::WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), nullptr, 0, nullptr, nullptr);
		std::string result(length, '\0');
		if (length > 0)
			::WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), &result[0], length, nullptr, nullptr);
		return result;
	}

	CString FromUtf8(const std::string& text)
	{
		const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		CString result;
		if (length > 0)
		{
			::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
				result.GetBuffer(length), length);
			result.ReleaseBuffer(length);
		}
		return result;
	}

	std::string UrlEncode(const CString& text)
	{
		static const char hex[] = "0123456789ABCDEF";
		std::string encoded;
		for (unsigned char c : ToUtf8(text))
		{
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
				encoded += static_cast<char>(c);
			else
			{
				encoded += '%';
				encoded += hex[c >> 4];
				encoded += hex[c & 15];
			}
		}
		return encoded;
	}

	// Downloads https://host/path and returns the response body.
	bool HttpGet(const wchar_t* host, const std::string& path, std::string& body, CString& error)
	{
		body.clear();
		HINTERNET session = ::WinHttpOpen(L"AIML_ImageExtractor/1.0 (species name lookup)",
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (session == nullptr)
		{
			error = _T("Could not start the internet connection.");
			return false;
		}
		::WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);

		bool ok = false;
		HINTERNET connection = ::WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
		HINTERNET request = nullptr;
		if (connection != nullptr)
		{
			const CString widePath(path.c_str());
			request = ::WinHttpOpenRequest(connection, L"GET", widePath, nullptr, WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		}
		if (request != nullptr &&
			::WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
			::WinHttpReceiveResponse(request, nullptr))
		{
			DWORD status = 0;
			DWORD statusSize = sizeof(status);
			::WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
			if (status == 200)
			{
				ok = true;
				DWORD available = 0;
				while (::WinHttpQueryDataAvailable(request, &available) && available > 0)
				{
					std::string chunk(available, '\0');
					DWORD read = 0;
					if (!::WinHttpReadData(request, &chunk[0], available, &read) || read == 0)
						break;
					body.append(chunk, 0, read);
				}
			}
			else
				error.Format(_T("%s answered with HTTP status %lu."), host, status);
		}
		else
			error.Format(_T("Could not reach %s."), host);

		if (request != nullptr)
			::WinHttpCloseHandle(request);
		if (connection != nullptr)
			::WinHttpCloseHandle(connection);
		::WinHttpCloseHandle(session);
		return ok;
	}

	// Reads the JSON string value that follows `"key":"` starting at the search position.
	bool JsonString(const std::string& json, const std::string& key, size_t from, std::string& value)
	{
		const std::string needle = "\"" + key + "\":\"";
		size_t pos = json.find(needle, from);
		if (pos == std::string::npos)
			return false;
		pos += needle.size();

		value.clear();
		while (pos < json.size() && json[pos] != '"')
		{
			if (json[pos] == '\\' && pos + 1 < json.size())
			{
				const char next = json[++pos];
				if (next == 'u' && pos + 4 < json.size())
				{
					const unsigned long code = strtoul(json.substr(pos + 1, 4).c_str(), nullptr, 16);
					wchar_t wide = static_cast<wchar_t>(code);
					value += ToUtf8(CString(wide));
					pos += 4;
				}
				else if (next == 'n')
					value += '\n';
				else
					value += next;
			}
			else
				value += json[pos];
			++pos;
		}
		return true;
	}
}

bool WikipediaLookup::FindLatinName(const CString& swedishName, CString& latinName, CString& error)
{
	latinName.Empty();
	const CString name = CString(swedishName).Trim();
	if (name.IsEmpty())
	{
		error = _T("Enter a Swedish name first.");
		return false;
	}

	// Swedish Wikipedia: article title -> Wikidata item.
	std::string body;
	if (!HttpGet(L"sv.wikipedia.org",
		"/w/api.php?action=query&format=json&redirects=1&prop=pageprops&ppprop=wikibase_item&titles=" + UrlEncode(name),
		body, error))
		return false;

	std::string item;
	if (!JsonString(body, "wikibase_item", 0, item))
	{
		error.Format(_T("No Swedish Wikipedia article found for \"%s\"."), static_cast<LPCTSTR>(name));
		return false;
	}

	// Wikidata property P225 is the taxon (scientific) name.
	if (!HttpGet(L"www.wikidata.org",
		"/w/api.php?action=wbgetentities&format=json&props=claims&ids=" + item, body, error))
		return false;

	const size_t claim = body.find("\"P225\"");
	std::string latin;
	if (claim == std::string::npos || !JsonString(body, "value", claim, latin) || latin.empty())
	{
		error.Format(_T("The article for \"%s\" is not a species with a scientific name."), static_cast<LPCTSTR>(name));
		return false;
	}
	latinName = FromUtf8(latin);
	return true;
}

bool WikipediaLookup::FindEnglishName(const CString& latinName, CString& englishName, CString& error)
{
	englishName.Empty();
	const CString name = CString(latinName).Trim();
	if (name.IsEmpty())
	{
		error = _T("Enter or look up a Latin name first.");
		return false;
	}

	std::string body;
	if (!HttpGet(L"en.wikipedia.org",
		"/w/api.php?action=query&format=json&redirects=1&titles=" + UrlEncode(name), body, error))
		return false;

	// A missing page has "missing" in its entry. Otherwise the page title,
	// after following the redirect from the Latin name, is the English name.
	std::string title;
	if (body.find("\"missing\"") != std::string::npos || !JsonString(body, "title", body.find("\"pages\""), title))
	{
		error.Format(_T("No English Wikipedia article found for \"%s\"."), static_cast<LPCTSTR>(name));
		return false;
	}
	englishName = FromUtf8(title);
	return true;
}
