/**
 *  @file version.cpp
 *
 *  @brief Implementation of CVersionInfo class
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include "coretypes.h"
#include "UnicodeString.h"
#include "version.h"

/** 
 * @brief Structure used to store language and codepage.
 */
struct LANGUAGEANDCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
};

/** 
 * @brief Constructor for asking only numeric version info.
 * This constructor creates instance that only reads version numbers,
 * not several strings there are. This saves some time.
 * @param [in] bVersionOnly If TRUE only version numbers are read.
 */
CVersionInfo::CVersionInfo(BOOL bVersionOnly)
: m_wLanguage(0)
, m_bVersionOnly(bVersionOnly)
, m_bDllVersion(FALSE)
{
	GetVersionInfo();
}

/** 
 * @brief Constructor for asking version infor when language is known.
 * This constructor creates instance which gets version-info matching
 * given language-ID. There can be several different language-codepage
 * combinations, but we use first version info block that matches the
 * given language. So even if codepage isn't correct, we get correct
 * version information.
 * @param [in] wLanguage Language-ID for which the version info is wanted.
 */
CVersionInfo::CVersionInfo(WORD wLanguage)
: m_wLanguage(wLanguage)
, m_bVersionOnly(FALSE)
, m_bDllVersion(FALSE)
{
	GetVersionInfo();
}

/** 
 * @brief Constructor.
 * @param [in] szFileToVersion Filename to read version from.
 * @param [in] bDllVersion If TRUE queries DLL version.
 */
CVersionInfo::CVersionInfo(LPCTSTR szFileToVersion, 
						   BOOL bDllVersion)
: m_wLanguage(0)
, m_bVersionOnly(FALSE)
, m_bDllVersion(bDllVersion)
{
	if (szFileToVersion != NULL)
		m_strFileName = szFileToVersion;
	GetVersionInfo();
}

/** 
 * @brief Constructor.
 * @param [in] szFileToVersion Filename to read version from.
 * @param [in] szLanguage Language for version.
 * @param [in] szCodePage Codepage for version.
 */
CVersionInfo::CVersionInfo(LPCTSTR szFileToVersion /* = NULL*/, 
						   LPCTSTR szLanguage /* = NULL*/,
						   LPCTSTR szCodepage /* = NULL*/)
: m_wLanguage(0)
, m_bVersionOnly(FALSE)
, m_bDllVersion(FALSE)
{
	if (szFileToVersion != NULL)
		m_strFileName = szFileToVersion;
	if (szLanguage != NULL)
		m_strLanguage = szLanguage;
	if (szCodepage != NULL)
		m_strCodepage = szCodepage;
	GetVersionInfo();
}

/** 
 * @brief Constructor for asking version number from known module.
 * @param [in] hModule Handle to module for version info.
 */
CVersionInfo::CVersionInfo(HINSTANCE hModule)
: m_wLanguage(0)
, m_bVersionOnly(FALSE)
, m_bDllVersion(FALSE)
{
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(hModule, szFileName, MAX_PATH);
	m_strFileName = szFileName;
	GetVersionInfo();
}

/** 
 * @brief Return file version string.
 * @return File version as string.
 */
String CVersionInfo::GetFileVersion() const
{
	return m_strFileVersion;
}

/** 
 * @brief Return private build number.
 * @return Private build number as string.
 */
String CVersionInfo::GetPrivateBuild() const
{
	return m_strPrivateBuild;
}

/** 
 * @brief Return special build number.
 * @return Special build number as string.
 */
String CVersionInfo::GetSpecialBuild() const
{
	return m_strSpecialBuild;
}

/** 
 * @brief Return company name.
 * @return Company name.
 */
String CVersionInfo::GetCompanyName() const
{
	return m_strCompanyName;
}

/** 
 * @brief Return file description string.
 * @return File description string.
 */
String CVersionInfo::GetFileDescription() const
{
	return m_strFileDescription;
}

/** 
 * @brief Return internal name.
 * @return Internal name.
 */
String CVersionInfo::GetInternalName() const
{
	return m_strInternalName;
}

/** 
 * @brief Return copyright info.
 * @return Copyright info.
 */
String CVersionInfo::GetLegalCopyright() const
{
	return m_strLegalCopyright;
}

/** 
 * @brief Return original filename.
 * @return Original filename.
 */
String CVersionInfo::GetOriginalFilename() const
{
	return m_strOriginalFilename;
}

/** 
 * @brief Return product's version number.
 * @return Product's version number as string.
 */
String CVersionInfo::GetProductVersion() const
{
	return m_strProductVersion;
}

/** 
 * @brief Format version string from numbers.
 * Version number consists of four WORD (16-bit) numbers. This function
 * formats those numbers to string, where numbers are separated by
 * dots. If the last number is zero it is not printed.
 * @param [in] First two (most significant) numbers for version number.
 * @param [in] Last two numbers for version number.
 * @return Formatted version string.
 */
static String MakeVersionString(DWORD hi, DWORD lo)
{
	TCHAR ver[50];
	if (LOWORD(lo) == 0)
	{
		_sntprintf(ver, countof(ver) - 1, _T("%d.%d.%d"), HIWORD(hi),
				LOWORD(hi), HIWORD(lo));
	}
	else
	{
		_sntprintf(ver, countof(ver) - 1, _T("%d.%d.%d.%d"), HIWORD(hi),
				LOWORD(hi), HIWORD(lo), LOWORD(lo));
	}
	String sver(ver);
	return sver;
}

/** 
 * @brief Return numeric product's version number.
 * This function returns version number given as a number in version info.
 * @return Product's version number as string.
 */
String CVersionInfo::GetFixedProductVersion()
{
	if (!m_bVersionFound)
		return _T("");
	return MakeVersionString(m_FixedFileInfo.dwProductVersionMS
		, m_FixedFileInfo.dwProductVersionLS);
}

/** 
 * @brief Return numeric file's version number.
 * This function returns version number given as a number in version info.
 * @return File's version number as string.
 */
String CVersionInfo::GetFixedFileVersion()
{
	if (!m_bVersionFound)
		return _T("");
	return MakeVersionString(m_FixedFileInfo.dwFileVersionMS
		, m_FixedFileInfo.dwFileVersionLS);
}

/** 
 * @brief Return numeric file's version number.
 * This function returns version number given as two DWORDs.
 * @param [out] versionMS High DWORD for version number.
 * @param [out] versionLS Low DWORD for version number.
 * @return TRUE if version info was found, FALSE otherwise.
 */
BOOL CVersionInfo::GetFixedFileVersion(DWORD &versionMS, DWORD &versionLS)
{
	if (m_bVersionFound)
	{
		versionMS = m_FixedFileInfo.dwFileVersionMS;
		versionLS = m_FixedFileInfo.dwFileVersionLS;
		return TRUE;
	}
	return FALSE;
}

/** 
 * @brief Return comment string.
 * @return Comment string.
 */
String CVersionInfo::GetComments() const
{
	return m_strComments;
}

/** 
 * @brief Read version info from file.
 * This function reads version information from file's version resource
 * to member variables.
 */
void CVersionInfo::GetVersionInfo()
{
	m_pVffInfo = NULL;
	ZeroMemory(&m_FixedFileInfo, sizeof(m_FixedFileInfo));
	ZeroMemory(&m_dvi, sizeof(m_dvi));

	DWORD dwVerHnd = 0;			// An 'ignored' parameter, always '0'
	TCHAR szFileName[MAX_PATH];

	if (m_strFileName.empty())
		GetModuleFileName(NULL, szFileName, MAX_PATH);
	else
		_tcsncpy(szFileName, m_strFileName.c_str(), MAX_PATH - 1);
	
	DWORD dwVerInfoSize = GetFileVersionInfoSize(szFileName, &dwVerHnd);
	if (dwVerInfoSize)
	{
		m_bVersionFound = TRUE;
		m_pVffInfo = new BYTE[dwVerInfoSize];
		if (GetFileVersionInfo(szFileName, dwVerHnd, dwVerInfoSize, m_pVffInfo))
		{
			GetFixedVersionInfo();
			if (m_bVersionOnly == FALSE)
				QueryStrings();
		}
		delete [] m_pVffInfo;
		m_pVffInfo = NULL;
	}

	if (m_bDllVersion)
	{
		if (HINSTANCE hinstDll = LoadLibrary(szFileName))
		{
			DLLGETVERSIONPROC DllGetVersion = (DLLGETVERSIONPROC) 
					GetProcAddress(hinstDll, "DllGetVersion");
			if (DllGetVersion)
			{
				m_dvi.cbSize = sizeof(m_dvi);
				if FAILED(DllGetVersion(&m_dvi))
				{
					m_dvi.cbSize = 0;
				}
			}
			FreeLibrary(hinstDll);
		}
	}
}

/** 
 * @brief Read strings from version info data.
 */
void CVersionInfo::QueryStrings()
{
	if (m_wLanguage != 0)
	{
		WORD codepage;
		GetCodepageForLanguage(m_wLanguage, codepage);
		TCHAR temp[20];
		_sntprintf(temp, countof(temp) - 1, _T("%04x"), m_wLanguage);
		m_strLanguage = temp;
		_sntprintf(temp, countof(temp) - 1, _T("%04x"), codepage);
		m_strCodepage = temp;
	}
	else if (m_strLanguage.empty()
		|| m_strCodepage.empty())
	{
		LANGUAGEANDCODEPAGE *lpTranslate;
		DWORD langLen;
		if (VerQueryValue((LPVOID)m_pVffInfo,
				_T("\\VarFileInfo\\Translation"),
				(LPVOID *)&lpTranslate, (UINT *)&langLen))
		{
			TCHAR temp[20];
			_sntprintf(temp, countof(temp) - 1, _T("%4.4X"),
					lpTranslate[0].wLanguage);
			m_strLanguage = temp;
			_sntprintf(temp, countof(temp) - 1, _T("%4.4X"),
					lpTranslate[0].wCodePage);
			m_strCodepage = temp;
		}
	}
	QueryValue(_T("CompanyName"), m_strCompanyName);
	QueryValue(_T("FileDescription"), m_strFileDescription);
	QueryValue(_T("FileVersion"), m_strFileVersion); 
	QueryValue(_T("InternalName"), m_strInternalName); 
	QueryValue(_T("LegalCopyright"), m_strLegalCopyright); 
	QueryValue(_T("OriginalFilename"), m_strOriginalFilename); 
	QueryValue(_T("ProductName"), m_strProductName); 
	QueryValue(_T("ProductVersion"), m_strProductVersion); 
	QueryValue(_T("Comments"), m_strComments);
	QueryValue(_T("SpecialBuild"), m_strSpecialBuild);
	QueryValue(_T("PrivateBuild"), m_strPrivateBuild);
}

/** 
 * @brief Read value from version info data.
 * @param [in] szId Name of value/string to read.
 * @param [out] Value read.
 */
void CVersionInfo::QueryValue(LPCTSTR szId, String& s)
{
	assert(m_pVffInfo != NULL);
	LPTSTR   lpVersion;			// String pointer to 'version' text
	UINT    uVersionLen;
	BOOL    bRetCode;

	TCHAR szSelector[256];
	_sntprintf(szSelector, countof(szSelector) - 1,
			_T("\\StringFileInfo\\%s%s\\%s"),
			m_strLanguage.c_str(), m_strCodepage.c_str(), szId);
	bRetCode = VerQueryValue((LPVOID)m_pVffInfo,
		szSelector,
		(LPVOID *)&lpVersion,
		&uVersionLen);
	
	if (bRetCode)
	{
		s = lpVersion;
		if (!s.empty())
			s = string_trim_ws(s);
	}
	else
		s.clear();
}

/** 
 * @brief Read numeric version information.
 */
void CVersionInfo::GetFixedVersionInfo()
{
	VS_FIXEDFILEINFO * pffi;
	UINT len = sizeof(*pffi);
	BOOL bRetCode = VerQueryValue(
		(LPVOID)m_pVffInfo, _T("\\"), (LPVOID *)&pffi, &len);
	memcpy(&m_FixedFileInfo, pffi, sizeof(m_FixedFileInfo));
	m_dvi.dwMajorVersion = HIWORD(m_FixedFileInfo.dwFileVersionMS);
	m_dvi.dwMinorVersion = LOWORD(m_FixedFileInfo.dwFileVersionMS);
	m_dvi.dwBuildNumber = HIWORD(m_FixedFileInfo.dwFileVersionLS);
}

/** 
 * @brief Get codepage for given language.
 * This function finds codepage value for given language from version info.
 * That is, we have certain combinations of language-codepage in version info.
 * This function tells which codepage exists with given language, so we can
 * find existing version info data.
 * @param [in] wLanguage Language ID for which we need matching codepage.
 * @param [out] wCodePage Found codepage.
 * @return TRUE if language was found from version info block.
 */
BOOL CVersionInfo::GetCodepageForLanguage(WORD wLanguage, WORD & wCodePage)
{
	LANGUAGEANDCODEPAGE *lpTranslate;
	UINT cbTranslate;
	// Read the list of languages and code pages.

	VerQueryValue((LPVOID)m_pVffInfo, 
				_T("\\VarFileInfo\\Translation"),
				(LPVOID*)&lpTranslate,
				&cbTranslate);

	// Read the file description for each language and code page.

	const int nLangCount = cbTranslate / sizeof(LANGUAGEANDCODEPAGE);
	int i = 0;
	BOOL bFound = FALSE;
	while (bFound == FALSE && i < nLangCount)
	{
		if (lpTranslate[i].wLanguage == wLanguage)
		{
			wCodePage = lpTranslate[i].wCodePage;
			bFound = TRUE;
		}
		else
			++i;
	}
	return bFound;
}
