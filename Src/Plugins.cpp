/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file Plugins.cpp
 *
 *  @brief Support for VBS Scriptlets, VB ActiveX DLL, VC++ COM DLL
 */ 
// RCS ID line follows -- this is updated by CVS
// $Id$

#include "StdAfx.h"
#include "FileTransform.h"
#include "Plugins.h"
#include "lwdisp.h"
#include "coretools.h"
#include "RegExp.h"
#include "FileFilterMgr.h"
#include "resource.h"
#include "Exceptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////////////////////////////////////////////
// scriptlet/activeX support for function names

// list the function IDs and names in a script or activeX dll
int GetFunctionsFromScript(LPDISPATCH piDispatch, BSTR *& namesArray, int *& IdArray, INVOKEKIND wantedKind)
{
	HRESULT hr;
	UINT iValidFunc = 0;
	if (piDispatch)
	{
		ITypeInfo *piTypeInfo;
		if SUCCEEDED(hr = piDispatch->GetTypeInfo(0, 0, &piTypeInfo))
		{
			TYPEATTR *pTypeAttr;
			if SUCCEEDED(hr = piTypeInfo->GetTypeAttr(&pTypeAttr))
			{
				// allocate arrays for the returned structures
				// the names array is NULL terminated
				namesArray = new BSTR [pTypeAttr->cFuncs+1];
				ZeroMemory(namesArray, sizeof(BSTR) * (pTypeAttr->cFuncs+1));
				IdArray = new int [pTypeAttr->cFuncs+1];

				UINT iMaxFunc = pTypeAttr->cFuncs - 1;
				for (UINT iFunc = 0 ; iFunc <= iMaxFunc ; ++iFunc)
				{
					UINT iFuncDesc = iMaxFunc - iFunc;
					FUNCDESC *pFuncDesc;
					if SUCCEEDED(hr = piTypeInfo->GetFuncDesc(iFuncDesc, &pFuncDesc))
					{
						// exclude properties
						// exclude IDispatch inherited methods
						if (pFuncDesc->invkind & wantedKind && !(pFuncDesc->wFuncFlags & 1))
						{
							BSTR bstrName;
							UINT cNames;
							if SUCCEEDED(hr = piTypeInfo->GetNames(pFuncDesc->memid,
								&bstrName, 1, &cNames))
							{
								IdArray[iValidFunc] = pFuncDesc->memid;
								namesArray[iValidFunc] = bstrName;
								iValidFunc ++;
							}
						}
						piTypeInfo->ReleaseFuncDesc(pFuncDesc);
					}
				}
				piTypeInfo->ReleaseTypeAttr(pTypeAttr);
			}
			piTypeInfo->Release();
		}
	}
	return iValidFunc;
}

int GetMethodsFromScript(LPDISPATCH piDispatch, BSTR *& namesArray, int *& IdArray)
{
	return GetFunctionsFromScript(piDispatch, namesArray, IdArray, INVOKE_FUNC);
}
int GetPropertyGetsFromScript(LPDISPATCH piDispatch, BSTR *& namesArray, int *& IdArray)
{
	return GetFunctionsFromScript(piDispatch, namesArray, IdArray, INVOKE_PROPERTYGET);
}


// search a function name in a scriptlet or activeX dll
BOOL SearchScriptForMethodName(LPDISPATCH piDispatch, WCHAR * functionName)
{
	BOOL bFound = FALSE;

	BSTR * namesArray=0;
	int * IdArray=0;
	int nFnc = GetMethodsFromScript(piDispatch, namesArray, IdArray);
	delete [] IdArray;

	int iFnc;
	for (iFnc = 0 ; iFnc < nFnc ; iFnc++)
	{
		if (wcscmp(namesArray[iFnc], functionName) == 0)
			bFound = TRUE;
		::SysFreeString(namesArray[iFnc]);
	}
	delete [] namesArray;
	return bFound;
}

// search a property name (with get interface) in a scriptlet or activeX dll
BOOL SearchScriptForDefinedProperties(LPDISPATCH piDispatch, WCHAR * functionName)
{
	BOOL bFound = FALSE;

	BSTR * namesArray=0;
	int * IdArray=0;
	int nFnc = GetPropertyGetsFromScript(piDispatch, namesArray, IdArray);
	delete [] IdArray;

	int iFnc;
	for (iFnc = 0 ; iFnc < nFnc ; iFnc++)
	{
		if (wcscmp(namesArray[iFnc], functionName) == 0)
			bFound = TRUE;
		::SysFreeString(namesArray[iFnc]);
	}
	delete [] namesArray;
	return bFound;
}




////////////////////////////////////////////////////////////////////////////////
// find scripts/activeX for an event : each event is assigned to a subdirectory 


/**
 * @brief Get a list of scriptlet file
 *
 * @return Returns an array of LPSTR
 */
static void GetScriptletsAt(LPCTSTR szSearchPath, LPCTSTR extension, CStringArray& scriptlets )
{
	WIN32_FIND_DATA ffi;
	CString strFileSpec;
	
	strFileSpec.Format(_T("%s*%s"), szSearchPath, extension);
	HANDLE hff = FindFirstFile(strFileSpec, &ffi);
	
	if (  hff != INVALID_HANDLE_VALUE )
	{
		do
		{
			if (!(ffi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{

				strFileSpec.Format(_T("%s%s"), szSearchPath, ffi.cFileName);
				scriptlets.Add(strFileSpec);  
			}
		}
		while (FindNextFile(hff, &ffi));
		FindClose(hff);
	}
}




void PluginInfo::LoadFilterString()
{
	filters = new RegList;

	CString sLine = filtersText;
	CString sPiece;

	while(1)
	{
		sPiece = sLine.Mid(sLine.ReverseFind(';')+1);
		sLine = sLine.Left(sLine.ReverseFind(';'));
		if (sPiece.IsEmpty())
			break;

		sPiece.TrimLeft();
		sPiece.MakeUpper();

		CRegExp * regexp = new CRegExp;
		if (regexp->RegComp(sPiece))
			filters->AddTail(regexp);
		else
			delete regexp;
	};
}


BOOL PluginInfo::TestAgainstRegList(LPCTSTR szTest)
{
	if (filters == NULL || szTest == NULL || szTest[0] == 0)
		return FALSE;

	CString sLine = szTest;
	CString sPiece;

	while(1)
	{
		sPiece = sLine.Mid(sLine.ReverseFind('|')+1);
		sLine = sLine.Left(sLine.ReverseFind('|'));
		if (sPiece.IsEmpty())
			break;

		sPiece.TrimLeft();
		sPiece.MakeUpper();

		if (::TestAgainstRegList(*filters, sPiece))
			return TRUE;
	};

	return FALSE;
}

/**
 * @brief Try to load a plugin
 *
 * @return true if plugin seems valid
 */
static bool LoadPlugin(PluginInfo & plugin, const CString & scriptletFilepath, LPCWSTR transformationEvent)
{
	// Search for the class "WinMergeScript"
	LPDISPATCH lpDispatch = CreateDispatchBySource(scriptletFilepath, L"WinMergeScript");
	if (lpDispatch == 0)
		return false;

	// Is this plugin for this transformationEvent ?
	HRESULT h;
	VARIANT ret;
	// invoke method get PluginEvent
	VariantInit(&ret);
	if (SearchScriptForDefinedProperties(lpDispatch, L"PluginEvent"))
		h = ::invokeW(lpDispatch, &ret, L"PluginEvent", opGet[0], NULL);
	if (FAILED(h) || ret.vt != VT_BSTR || (wcscmp(ret.bstrVal, transformationEvent) != 0))
	{
		lpDispatch->Release();
		return false;

	}
	VariantClear(&ret);

	// Check that the plugin offers the requested functions
	// set the mode for the events which uses it
	BOOL bUnicodeMode = SCRIPT_A | SCRIPT_W;
	BOOL bFound = TRUE;
	if (wcscmp(transformationEvent, L"BUFFER_PREDIFF") == 0)
	{
		bFound &= SearchScriptForMethodName(lpDispatch, L"PrediffBufferW");
			bUnicodeMode &= ~SCRIPT_A;
	}
	else if (wcscmp(transformationEvent, L"FILE_PREDIFF") == 0)
	{
		bFound &= SearchScriptForMethodName(lpDispatch, L"PrediffFile");
	}
	else if (wcscmp(transformationEvent, L"BUFFER_PACK_UNPACK") == 0)
	{
		bFound &= SearchScriptForMethodName(lpDispatch, L"UnpackBufferA");
		bFound &= SearchScriptForMethodName(lpDispatch, L"PackBufferA");
		bUnicodeMode &= ~SCRIPT_W;
	}
	else if (wcscmp(transformationEvent, L"FILE_PACK_UNPACK") == 0)
	{
		bFound &= SearchScriptForMethodName(lpDispatch, L"UnpackFile");
		bFound &= SearchScriptForMethodName(lpDispatch, L"PackFile");
		bUnicodeMode &= ~SCRIPT_W;
	}
	if (bFound == FALSE)
	{
		lpDispatch->Release();
		return false;
	}


	// get PluginDescription
	if (SearchScriptForDefinedProperties(lpDispatch, L"PluginDescription"))
		h = ::invokeW(lpDispatch, &ret, L"PluginDescription", opGet[0], NULL);
	if (!FAILED(h) && ret.vt == VT_BSTR)
		plugin.description = ret.bstrVal;
	else
		// no description, use filename
		plugin.description = scriptletFilepath.Mid(scriptletFilepath.ReverseFind('\\') + 1);
	VariantClear(&ret);

	// get PluginIsAutomatic
	if (SearchScriptForDefinedProperties(lpDispatch, L"PluginIsAutomatic"))
		h = ::invokeW(lpDispatch, &ret, L"PluginIsAutomatic", opGet[0], NULL);
	if (!FAILED(h) && ret.vt == VT_BOOL)
		plugin.bAutomatic = ret.boolVal;
	else
		plugin.bAutomatic = FALSE;
	VariantClear(&ret);

	// get PluginFileFilters
	if (SearchScriptForDefinedProperties(lpDispatch, L"PluginFileFilters"))
		h = ::invokeW(lpDispatch, &ret, L"PluginFileFilters", opGet[0], NULL);
	if (!FAILED(h) && ret.vt == VT_BSTR)
		plugin.filtersText= ret.bstrVal;
	else
	{
		plugin.bAutomatic = FALSE;
		plugin.filtersText = ".";
	}
	VariantClear(&ret);

	plugin.LoadFilterString();

	// keep the filename
	plugin.name	= scriptletFilepath.Mid(scriptletFilepath.ReverseFind('\\')+1);

	plugin.bUnicodeMode = bUnicodeMode;

	plugin.lpDispatch = lpDispatch;
	return true;
}

static void ReportPluginLoadFailure(const CString & scriptletFilepath, LPCWSTR transformationEvent)
{
	USES_CONVERSION;
	CString sEvent = OLE2CT(transformationEvent);
	CString msg;
	msg.Format(_T("Exception loading plugin for event: %s\r\n%s"), sEvent, scriptletFilepath);
	AfxMessageBox(msg);
}

/**
 * @brief Guard call to LoadPlugin with Windows SEH to trap GPFs
 *
 * @return true/false result from LoadPlugin
 */
static bool LoadPluginWrapper(PluginInfo & plugin, const CString & scriptletFilepath, LPCWSTR transformationEvent)
{
	__try {
		return LoadPlugin(plugin, scriptletFilepath, transformationEvent);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		ReportPluginLoadFailure(scriptletFilepath, transformationEvent);
	}
	return false;
}

/** 
 * @brief Get available scriptlets for an event
 *
 * @return Returns an array of valid LPDISPATCH
 */
static PluginArray * GetAvailableScripts( LPCWSTR transformationEvent, BOOL getScriptletsToo ) 
{
	CString path = GetModulePath() + _T("\\MergePlugins\\");

	CStringArray scriptlets;
	GetScriptletsAt(path, _T(".sct"), scriptlets );		// VBS/JVS scriptlet
	GetScriptletsAt(path, _T(".ocx"), scriptlets );		// VB COM object
	GetScriptletsAt(path, _T(".dll"), scriptlets );		// VC++ COM object

	PluginArray * pPlugins = new PluginArray;

	int i;
	for (i = 0 ; i < scriptlets.GetSize() ; i++)
	{
		// Note all the info about the plugin
		PluginInfo plugin;

		CString scriptletFilepath = scriptlets.GetAt(i);
		if (LoadPluginWrapper(plugin, scriptletFilepath, transformationEvent))
		{
			pPlugins->Add(plugin);
		}
	}

	return pPlugins;
}

static void FreeAllScripts(PluginArray *& pArray) 
{
	int i;
	for (i = 0 ; i < pArray->GetSize() ; i++)
	{
		pArray->GetAt(i).lpDispatch->Release();
		if (pArray->GetAt(i).filters)
			DeleteRegList(*(pArray->GetAt(i).filters));
		delete pArray->GetAt(i).filters;
	}

	pArray->RemoveAll();
	delete pArray;
	pArray = NULL;
}

////////////////////////////////////////////////////////////////////////////////////
// class CScriptsOfThread : cache the interfaces during a thread life

CScriptsOfThread * CScriptsOfThread::m_aAvailableThreads[NMAXTHREADS] = {0};

CScriptsOfThread::CScriptsOfThread()
{
	// initialize the thread data

	// count number of events
	int i;
	for (i = 0 ;  ; i ++)
		if (TransformationCategories[i] == NULL)
			break;
	nTransformationEvents = i;

	typedef PluginArray * LPPluginArray;
	m_aPluginsByEvent = new LPPluginArray [nTransformationEvents];
	ZeroMemory(m_aPluginsByEvent, nTransformationEvents*sizeof(LPPluginArray));
	m_nThreadId = GetCurrentThreadId();


	for (i = 0 ; i < NMAXTHREADS ; i++)
		if (m_aAvailableThreads[i] && m_aAvailableThreads[i]->m_nThreadId == GetCurrentThreadId())
			break;
	if (i < NMAXTHREADS)
	{
		// error : the thread is already in the list 
		ASSERT(0);
		return;
	}
		
	// add the thread in the array
	for (i = 0 ; i < NMAXTHREADS ; i++)
		if (m_aAvailableThreads[i] == 0)
			break;
	if (i == NMAXTHREADS)
	{
		// no free place, don't register
		ASSERT(0);
	}
	else
	{
		// register in the array
		m_aAvailableThreads[i] = this;
		// CoInitialize the thread, keep the returned value for the destructor 
		hrInitialize = CoInitialize(NULL);
		ASSERT(hrInitialize == S_OK || hrInitialize == S_FALSE);
	}
}


BOOL CScriptsOfThread::bInMainThread()
{
	return (this == m_aAvailableThreads[0]);
}

CScriptsOfThread::~CScriptsOfThread()
{
	// unregister from the list
	int i;
	for (i = 0 ; i < NMAXTHREADS ; i++)
		if (m_aAvailableThreads[i] && m_aAvailableThreads[i]->m_nThreadId == GetCurrentThreadId())
			break;
	if (i == NMAXTHREADS)
	{
		// not in the list ?
		ASSERT(0);
	}
	else
	{
		m_aAvailableThreads[i] = NULL;
		if (hrInitialize == S_OK || hrInitialize == S_FALSE)
			CoUninitialize();
	}

	FreeAllScripts();
	delete [] m_aPluginsByEvent;
}

PluginArray * CScriptsOfThread::GetAvailableScripts(LPCWSTR transformationEvent)
{
	int i;
	for (i = 0 ; i < nTransformationEvents ; i ++)
		if (wcscmp(transformationEvent, TransformationCategories[i]) == 0)
		{
			if (m_aPluginsByEvent[i] == NULL)
				m_aPluginsByEvent[i] = ::GetAvailableScripts(transformationEvent, bInMainThread());
			return m_aPluginsByEvent[i];
		}
	// return a pointer to an empty list
	static PluginArray noPlugin;
	return &noPlugin;
}

// the mode must be supported by all the scripts
BOOL CScriptsOfThread::GetUnicodeModeOfScripts(LPCWSTR transformationEvent)
{
	int i;
	for (i = 0 ; i < nTransformationEvents ; i ++)
		if (wcscmp(transformationEvent, TransformationCategories[i]) == 0)
		{
			if (m_aPluginsByEvent[i] == NULL)
				m_aPluginsByEvent[i] = ::GetAvailableScripts(transformationEvent, bInMainThread());

			BOOL bUnicodeMode = 0xFFFFFFFF;
			for (int j = 0 ; j <  m_aPluginsByEvent[i]->GetSize() ; j++)
				bUnicodeMode &= m_aPluginsByEvent[i]->GetAt(j).bUnicodeMode;
			return bUnicodeMode;
		}
	return 0;
}
// the mode must be supported by all the scripts if their filter match the filteredText
BOOL CScriptsOfThread::GetUnicodeModeOfScripts(LPCWSTR transformationEvent, LPCTSTR filteredText)
{
	int i;
	for (i = 0 ; i < nTransformationEvents ; i ++)
		if (wcscmp(transformationEvent, TransformationCategories[i]) == 0)
		{
			if (m_aPluginsByEvent[i] == NULL)
				m_aPluginsByEvent[i] = ::GetAvailableScripts(transformationEvent, bInMainThread());

			BOOL bUnicodeMode = 0xFFFFFFFF;
			for (int j = 0 ; j <  m_aPluginsByEvent[i]->GetSize() ; j++)
			{
				PluginInfo & plugin = m_aPluginsByEvent[i]->ElementAt(j);
				if (plugin.bAutomatic == FALSE)
					continue;
				if (plugin.TestAgainstRegList(filteredText) == FALSE)
					continue;
				bUnicodeMode &= m_aPluginsByEvent[i]->GetAt(j).bUnicodeMode;
			}
			return bUnicodeMode;
		}
	return 0;
}



void CScriptsOfThread::FreeAllScripts()
{
	// release all the scripts of the thread
	int i;
	for (i = 0 ; i < nTransformationEvents ; i++)
		if (m_aPluginsByEvent[i])
			::FreeAllScripts(m_aPluginsByEvent[i]);
}

void CScriptsOfThread::FreeScriptsForEvent(LPCWSTR transformationEvent)
{
	int i;
	for (i = 0 ; i < nTransformationEvents ; i ++)
		if (wcscmp(transformationEvent, TransformationCategories[i]) == 0)
		{
			if (m_aPluginsByEvent[i])
			::FreeAllScripts(m_aPluginsByEvent[i]);
			return;
		}
}


PluginInfo * CScriptsOfThread::GetPluginByName(LPCWSTR transformationEvent, LPCTSTR name)
{
	int i;
	for (i = 0 ; i < nTransformationEvents ; i ++)
		if (wcscmp(transformationEvent, TransformationCategories[i]) == 0)
		{
			if (m_aPluginsByEvent[i] == NULL)
				m_aPluginsByEvent[i] = ::GetAvailableScripts(transformationEvent, bInMainThread());

			for (int j = 0 ; j <  m_aPluginsByEvent[i]->GetSize() ; j++)
				if (_tcscmp(m_aPluginsByEvent[i]->GetAt(j).name, name) == 0)
					return &(m_aPluginsByEvent[i]->ElementAt(j));
		}
	return NULL;
}

PluginInfo *  CScriptsOfThread::GetPluginInfo(LPDISPATCH piScript)
{
	int i, j;
	for (i = 0 ; i < nTransformationEvents ; i ++) 
	{
		if (m_aPluginsByEvent[i] == NULL)
			continue;
		PluginArray * pArray = m_aPluginsByEvent[i];
		for (j = 0 ; j < pArray->GetSize() ; j++)
			if ((*pArray)[j].lpDispatch == piScript)
				return & (*pArray)[j];
	}

	return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// reallocation, take care of flag bWriteable

static void reallocBuffer(LPSTR & pszBuf, UINT & nOldSize, UINT nSize, BOOL bWriteable)
{
	if (!bWriteable)
		// alloc a new buffer
		pszBuf = (LPSTR) malloc(nSize);
	else if (nSize > nOldSize) 
	{
		// free the previous buffer, alloc a new one (so we don't copy the old values)
		free(pszBuf);
		pszBuf = (LPSTR) malloc(nSize);
	}
	else
		// just truncate the buffer
		pszBuf = (LPSTR) realloc(pszBuf, nSize);
	nOldSize = nSize;
}
static void reallocBuffer(LPWSTR & pszBuf, UINT & nOldSize, UINT nSize, BOOL bWriteable)
{
	if (!bWriteable)
		// alloc a new buffer
		pszBuf = (LPWSTR) malloc(nSize*sizeof(WCHAR));
	else if (nSize > nOldSize) 
	{
		// free the previous buffer, alloc a new one (so we don't copy the old values)
		free(pszBuf);
		pszBuf = (LPWSTR) malloc(nSize*sizeof(WCHAR));
	}
	else
		// just truncate the buffer
		pszBuf = (LPWSTR) realloc(pszBuf, nSize*sizeof(WCHAR));
	nOldSize = nSize;
}


////////////////////////////////////////////////////////////////////////////////
// wrap invokes with error handlers

/**
 * @brief Display a message box with the plugin name and the error message
 *
 * @note Use MessageBox instead of AfxMessageBox so we can set the caption.
 * VB/VBS plugins has an internal error handler, and a message box with caption,
 * and we try to reproduce it for other plugins.
 */
static void ShowPluginErrorMessage(LPDISPATCH piScript, LPTSTR description)
{
	PluginInfo * pInfo = CScriptsOfThread::GetScriptsOfThreads()->GetPluginInfo(piScript);
	ASSERT(pInfo != NULL);
	ASSERT (description != NULL);	
	MessageBox(AfxGetMainWnd()->GetSafeHwnd(), description, pInfo->name, MB_ICONSTOP);
}

/**
 * @brief safe invoke helper (by ordinal)
 *
 * @note Free all variants passed to it (except ByRef ones) 
 */
static HRESULT safeInvokeA(LPDISPATCH pi, VARIANT *ret, DISPID id, LPCCH op, ...)
{
	HRESULT h;
	SE_Handler seh;
	TCHAR errorText[500];
	BOOL bExceptionCatched = FALSE;
	
	try 
	{
		h = invokeA(pi, ret, id, op, (VARIANT*)(&op+1));
	}
	catch(CException * e) 
	{
		// structured exception are catched here thanks to class SE_Exception
		if (!(e->GetErrorMessage(errorText, 500, NULL)))
			// don't localize this as we do not localize the known exceptions
			_tcscpy(errorText, _T("Unknown CException"));
		e->Delete();
		bExceptionCatched = TRUE;
	}
	catch(...) 
	{
		// don't localize this as we do not localize the known exceptions
		_tcscpy(errorText, _T("Unknown C++ exception"));
		bExceptionCatched = TRUE;
	}

	if (bExceptionCatched)
	{
		ShowPluginErrorMessage(pi, errorText);
		// set h to FAILED
		h = -1;
	}

	return h;
}
/**
 * @brief safe invoke helper (by function name)
 *
 * @note Free all variants passed to it (except ByRef ones) 
 */
static HRESULT safeInvokeW(LPDISPATCH pi, VARIANT *ret, BSTR silent, LPCCH op, ...)
{
	HRESULT h;
	SE_Handler seh;
	TCHAR errorText[500];
	BOOL bExceptionCatched = FALSE;
	
	try 
	{
		h = invokeW(pi, ret, silent, op, (VARIANT*)(&op+1));
	}
	catch(CException * e) 
	{
		// structured exception are catched here thanks to class SE_Exception
		if (!(e->GetErrorMessage(errorText, 500, NULL)))
			// don't localize this as we do not localize the known exceptions
			_tcscpy(errorText, _T("Unknown CException"));
		e->Delete();
		bExceptionCatched = TRUE;
	}
	catch(...) 
	{
		// don't localize this as we do not localize the known exceptions
		_tcscpy(errorText, _T("Unknown C++ exception"));
		bExceptionCatched = TRUE;
	}

	if (bExceptionCatched)
	{
		ShowPluginErrorMessage(pi, errorText);
		// set h to FAILED
		h = -1;
	}

	return h;
}

////////////////////////////////////////////////////////////////////////////////
// invoke for plugins

/*
 * ----- about VariantClear -----
 * VariantClear is done in safeInvokeW/safeInvokeA except for :
 * - the returned value
 * - BYREF arguments
 * note : BYREF arguments don't need VariantClear if the refered value
 * is deleted in the function destructor. Example :
 * {
 *   int Value;
 *   VARIANT vValue;
 *   vValue.plVal = &vValue;
 *   ...
 */

BOOL InvokePrediffBuffer(BSTR & bstrBuf, int & nChanged, LPDISPATCH piScript)
{
	UINT nBufSize = SysStringLen(bstrBuf);

	// prepare the arguments
	// argument text buffer by reference
	VARIANT vpbstrBuf;
	vpbstrBuf.vt = VT_BYREF | VT_BSTR;
	vpbstrBuf.pbstrVal = &bstrBuf;
	// argument buffer size by reference
	VARIANT vpiSize;
	vpiSize.vt = VT_BYREF | VT_I4;
	vpiSize.plVal = (long*) &nBufSize;
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// for VC, if the invoked function changes the buffer address, 
	// it must free the old buffer with SysFreeString
	// VB does it automatically
	// VARIANT_BOOL DiffingPreprocessW(BSTR * buffer, UINT * nSize, VARIANT_BOOL * bChanged)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"PrediffBufferW", opFxn[3], 
                            vpboolChanged, vpiSize, vpbstrBuf);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
	{
		// remove trailing charracters in the rare case that bstrBuf was not resized 
		if (SysStringLen(bstrBuf) != nBufSize)
			bSuccess = !FAILED(SysReAllocStringLen(&bstrBuf, bstrBuf, nBufSize));
		if (bSuccess)
			nChanged ++;
	}

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}

BOOL InvokeUnpackBuffer(COleSafeArray & array, int & nChanged, LPDISPATCH piScript, int & subcode)
{
	UINT nArraySize = array.GetOneDimSize();

	// prepare the arguments
	// argument file buffer
	VARIANT vparrayBuf;
	vparrayBuf.vt = VT_BYREF | VT_ARRAY | VT_UI1;
	vparrayBuf.pparray = &(LPVARIANT(array)->parray);
	// argument buffer size by reference
	VARIANT vpiSize;
	vpiSize.vt = VT_BYREF | VT_I4;
	vpiSize.plVal = (long*) &nArraySize;
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument subcode by reference
	VARIANT viSubcode;
	viSubcode.vt = VT_BYREF | VT_I4;
	viSubcode.plVal = (long*) &subcode;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// VARIANT_BOOL UnpackBufferA(SAFEARRAY * array, UINT * nSize, VARIANT_BOOL * bChanged, UINT * subcode)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"UnpackBufferA", opFxn[4], 
                            viSubcode, vpboolChanged, vpiSize, vparrayBuf);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
	{
		// remove trailing charracters if the array was not resized
		if (array.GetOneDimSize() != nArraySize)
			array.ResizeOneDim(nArraySize);
		nChanged ++;
	}

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}

BOOL InvokePackBuffer(COleSafeArray & array, int & nChanged, LPDISPATCH piScript, int subcode)
{
	UINT nArraySize = array.GetOneDimSize();

	// prepare the arguments
	// argument file buffer
	VARIANT vparrayBuf;
	vparrayBuf.vt = VT_BYREF | VT_ARRAY | VT_UI1;
	vparrayBuf.pparray = &(LPVARIANT(array)->parray);
	// argument buffer size by reference
	VARIANT vpiSize;
	vpiSize.vt = VT_BYREF | VT_I4;
	vpiSize.plVal = (long*) &nArraySize;
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument subcode
	VARIANT viSubcode;
	viSubcode.vt = VT_I4;
	viSubcode.lVal = subcode;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// VARIANT_BOOL PackBufferA(SAFEARRAY * array, UINT * nSize, VARIANT_BOOL * bChanged, UINT subcode)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"PackBufferA", opFxn[4], 
                            viSubcode, vpboolChanged, vpiSize, vparrayBuf);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
	{
		// remove trailing charracters if the array was not resized
		if (array.GetOneDimSize() != nArraySize)
			array.ResizeOneDim(nArraySize);
		nChanged ++;
	}

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}


BOOL InvokeUnpackFile(LPCTSTR fileSource, LPCTSTR fileDest, int & nChanged, LPDISPATCH piScript, int & subCode)
{
	USES_CONVERSION;
	// argument text  
	VARIANT vbstrSrc;
	vbstrSrc.vt = VT_BSTR;
	vbstrSrc.bstrVal = T2BSTR(fileSource);
	// argument transformed text 
	VARIANT vbstrDst;
	vbstrDst.vt = VT_BSTR;
	vbstrDst.bstrVal = T2BSTR(fileDest);
	// argument subcode by reference
	VARIANT vpiSubcode;
	vpiSubcode.vt = VT_BYREF | VT_I4;
	vpiSubcode.plVal = (long*) &subCode;
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// VARIANT_BOOL UnpackFile(BSTR fileSrc, BSTR fileDst, VARIANT_BOOL * bChanged, INT * bSubcode)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"UnpackFile", opFxn[4], 
                            vpiSubcode, vpboolChanged, vbstrDst, vbstrSrc);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
		nChanged ++;

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}

BOOL InvokePackFile(LPCTSTR fileSource, LPCTSTR fileDest, int & nChanged, LPDISPATCH piScript, int subCode)
{
	USES_CONVERSION;
	// argument text  
	VARIANT vbstrSrc;
	vbstrSrc.vt = VT_BSTR;
	vbstrSrc.bstrVal = T2BSTR(fileSource);
	// argument transformed text 
	VARIANT vbstrDst;
	vbstrDst.vt = VT_BSTR;
	vbstrDst.bstrVal = T2BSTR(fileDest);
	// argument subcode
	VARIANT viSubcode;
	viSubcode.vt = VT_I4;
	viSubcode.lVal = subCode;
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// VARIANT_BOOL PackFile(BSTR fileSrc, BSTR fileDst, VARIANT_BOOL * bChanged, INT bSubcode)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"PackFile", opFxn[4], 
                            viSubcode, vpboolChanged, vbstrDst, vbstrSrc);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
		nChanged ++;

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}

BOOL InvokePrediffFile(LPCTSTR fileSource, LPCTSTR fileDest, int & nChanged, LPDISPATCH piScript)
{
	USES_CONVERSION;
	// argument text  
	VARIANT vbstrSrc;
	vbstrSrc.vt = VT_BSTR;
	vbstrSrc.bstrVal = T2BSTR(fileSource);
	// argument transformed text 
	VARIANT vbstrDst;
	vbstrDst.vt = VT_BSTR;
	vbstrDst.bstrVal = T2BSTR(fileDest);
	// argument flag changed (VT_BOOL is short)
	VARIANT_BOOL changed = 0;
	VARIANT vpboolChanged;
	vpboolChanged.vt = VT_BYREF | VT_BOOL;
	vpboolChanged.pboolVal = &changed;
	// argument return value (VT_BOOL is short)
	VARIANT vboolHandled;
	vboolHandled.vt = VT_BOOL;
	vboolHandled.boolVal = FALSE;

	// invoke method by name, reverse order for arguments
	// VARIANT_BOOL PrediffFile(BSTR fileSrc, BSTR fileDst, VARIANT_BOOL * bChanged)
	HRESULT h = ::safeInvokeW(piScript,	&vboolHandled, L"PrediffFile", opFxn[3], 
                            vpboolChanged, vbstrDst, vbstrSrc);
	int bSuccess = ! FAILED(h) && vboolHandled.boolVal;
	if (bSuccess && changed)
		nChanged ++;

	// clear the returned variant
	VariantClear(&vboolHandled);

	return 	(bSuccess);
}


BOOL InvokeTransformText(CString & text, int & changed, LPDISPATCH piScript, int fncId)
{
	USES_CONVERSION;

	// argument text  
	VARIANT pvPszBuf;
	pvPszBuf.vt = VT_BSTR;
	pvPszBuf.bstrVal = T2BSTR(text);
	// argument transformed text 
	VARIANT vTransformed;
	vTransformed.vt = VT_BSTR;
	vTransformed.bstrVal = NULL;

	// invoke method by ordinal
	// BSTR customFunction(BSTR text)
	HRESULT h = ::safeInvokeA(piScript, &vTransformed, fncId, opFxn[1], pvPszBuf);

	if (! FAILED(h))
	{
		// when UNICODE is not defined, the CString = operator performs the conversion LPWSTR to LPSTR
		text = vTransformed.bstrVal;
		changed = TRUE;
	}
	else
		changed = FALSE;

	// clear the returned variant
	VariantClear(&vTransformed);

	return (! FAILED(h));
}
