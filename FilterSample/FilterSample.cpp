// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) 2025 HIRAOKA HYPERS TOOLS, Inc.

#include <new>
#include <windows.h>
#include <strsafe.h>
#include <shlwapi.h>
#include "FilterBase.h"

// BEGIN: include
#include <atlbase.h>
#include <atlstr.h>

#include <cmath>

#include <fpdfview.h>
#include <fpdf_doc.h>
#include <fpdf_text.h>
// END: include

void DllAddRef();
void DllRelease();

// Filter for ".filtersample" files

class CFilterSample : public CFilterBase
{
public:
	CFilterSample(REFCLSID clsid) : m_cRef(1), m_iEmitState(EMITSTATE_TITLE), m_doc(NULL), m_pageIndex(0), m_numPages(0), m_fileAccess(), m_clsid(clsid)
	{
		DllAddRef();
	}

	~CFilterSample()
	{
		// BEGIN: dtor
		if (m_doc)
		{
			FPDF_CloseDocument(m_doc);
			m_doc = NULL;
		}
		// END: dtor
		DllRelease();
	}

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] =
		{
			QITABENTMULTI(CFilterSample, IPersist, IPersistFile),
			QITABENT(CFilterSample, IPersistFile),
			QITABENT(CFilterSample, IPersistStream),
			QITABENT(CFilterSample, IInitializeWithStream),
			QITABENT(CFilterSample, IFilter),
			{0, 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&m_cRef);
		if (cRef == 0)
		{
			delete this;
		}
		return cRef;
	}

	// IPersist
	IFACEMETHODIMP GetClassID(
		/* [out] */ __RPC__out CLSID* pClassID)
	{
		if (pClassID == NULL)
		{
			return E_POINTER;
		}
		else
		{
			*pClassID = m_clsid;
			return S_OK;
		}
	}

	virtual HRESULT OnInit();
	virtual HRESULT GetNextChunkValue(CChunkValue& chunkValue);

private:
	// BEGIN: IFilter implementation specific funcs

	static int GetBlock(
		void* param,
		unsigned long position,
		unsigned char* pBuf,
		unsigned long size
	);

	// END: IFilter implementation specific funcs

	long m_cRef;

	// BEGIN: IFilter implementation specific vars

	FPDF_FILEACCESS m_fileAccess;
	FPDF_DOCUMENT m_doc;
	int m_numPages;
	int m_pageIndex;
	CLSID m_clsid;

	// some props we want to emit don't come from the doc.  We use this as our state
	enum EMITSTATE {
		EMITSTATE_TITLE,
		EMITSTATE_AUTHOR,
		EMITSTATE_SUBJECT,
		EMITSTATE_KEYWORDS,
		EMITSTATE_PAGES,
	};
	DWORD m_iEmitState;

	// END: IFilter implementation specific vars
};

HRESULT CFilterSample_CreateInstance(REFCLSID clsid, REFIID riid, void** ppv)
{
	HRESULT hr = E_OUTOFMEMORY;
	CFilterSample* pFilter = new (std::nothrow) CFilterSample(clsid);
	if (pFilter)
	{
		hr = pFilter->QueryInterface(riid, ppv);
		pFilter->Release();
	}
	return hr;
}

// This is called after the stream (m_pStream) has been setup and is ready for use
// This implementation of this filter passes that stream to the xml reader
HRESULT CFilterSample::OnInit()
{
	// BEGIN: OnInit
	HRESULT hr;
	STATSTG statStg = { 0 };
	if (m_doc == NULL)
	{
		if (SUCCEEDED(hr = m_pStream->Stat(&statStg, STATFLAG_NONAME)))
		{
			if (statStg.cbSize.QuadPart < 0xFFFFFFFFU)
			{
				m_fileAccess.m_FileLen = (ULONG)statStg.cbSize.QuadPart;
				m_fileAccess.m_GetBlock = GetBlock;
				m_fileAccess.m_Param = this;
				m_doc = FPDF_LoadCustomDocument(&m_fileAccess, NULL);
				if (m_doc != NULL)
				{
					m_numPages = FPDF_GetPageCount(m_doc);

					return S_OK;
				}
				else
				{
					hr = E_FAIL;
				}
			}
			else
			{
				hr = E_FAIL; // file too big
			}
		}
		else
		{
			hr = E_FAIL; // can't get size
		}
	}
	else
	{
		hr = E_UNEXPECTED; // already initialized
	}
	return hr;
	// END: OnInit
}

// When GetNextChunkValue() is called we fill in the ChunkValue by calling SetXXXValue() with the property and value (and other parameters that you want)
// example:  chunkValue.SetTextValue(PKEY_ItemName, L"example text");
// return FILTER_E_END_OF_CHUNKS when there are no more chunks
HRESULT CFilterSample::GetNextChunkValue(CChunkValue& chunkValue)
{
	// BEGIN: GetNextChunkValue
	chunkValue.Clear();

	if (m_doc == NULL)
	{
		return E_FAIL;
	}

	class Util1 {
	public:
		static HRESULT TryToReadMetaText(
			FPDF_DOCUMENT doc,
			FPDF_BYTESTRING tag,
			const PROPERTYKEY& key,
			CChunkValue& chunkValue
		)
		{
			WCHAR content[1024] = { 0 };
			ULONG cb = FPDF_GetMetaText(doc, tag, content, sizeof(WCHAR) * 1024);
			if (cb != 0 && lstrlenW(content) != 0)
			{
				return chunkValue.SetTextValue(
					key,
					content,
					CHUNK_VALUE,
					0UL,
					0UL,
					0UL,
					CHUNK_EOS
				);
			}
			else
			{
				return S_FALSE;
			}
		}
	};

	struct DblRect {
		// left
		double l;
		// top (PDF coord, up is plus)
		double t;
		// right
		double r;
		// bottom (PDF coord, up is plus)
		double b;

		bool SeemsToContinue(const DblRect& prev) const
		{
			double h = t - b;
			double sharedHeight = std::fmin(t, prev.t) - std::fmax(b, prev.b);
			return true
				&& (prev.r <= l && l < prev.r + h / 2)
				&& h * 0.5 <= sharedHeight
				;
		}
	};

	// Not all data from the XML document has been read but additional props can be added
	// For this we use the m_iEmitState to iterate through them, each call will go to the next one
	switch (m_iEmitState)
	{
	case EMITSTATE_TITLE:
		++m_iEmitState;
		return Util1::TryToReadMetaText(m_doc, "Title", PKEY_Title, chunkValue);

	case EMITSTATE_AUTHOR:
		++m_iEmitState;
		return Util1::TryToReadMetaText(m_doc, "Author", PKEY_Author, chunkValue);

	case EMITSTATE_SUBJECT:
		++m_iEmitState;
		return Util1::TryToReadMetaText(m_doc, "Subject", PKEY_Subject, chunkValue);

	case EMITSTATE_KEYWORDS:
		++m_iEmitState;
		return Util1::TryToReadMetaText(m_doc, "Keywords", PKEY_Keywords, chunkValue);

	case EMITSTATE_PAGES:
		if (m_numPages <= m_pageIndex)
		{
			++m_iEmitState;
			return S_FALSE;
		}

		CStringW text;

		FPDF_PAGE page = FPDF_LoadPage(m_doc, m_pageIndex);
		if (page != NULL)
		{
			WCHAR boundedText[2048];

			FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
			if (textPage != NULL)
			{
				int numRects = FPDFText_CountRects(textPage, 0, -1);
				DblRect prevRect;
				for (int x = 0; x < numRects; x++)
				{
					DblRect rect;
					if (FPDFText_GetRect(textPage, x, &rect.l, &rect.t, &rect.r, &rect.b))
					{
						int numText = FPDFText_GetBoundedText(textPage, rect.l, rect.t, rect.r, rect.b, (PUSHORT)boundedText, 2048);
						if (1 <= numText)
						{
							text.Append(boundedText, numText);

							bool continuous = x != 0 && rect.SeemsToContinue(prevRect);

							text.Append(continuous ? L"" : L"");
						}
						prevRect = rect;
					}
				}
				FPDFText_ClosePage(textPage);
			}
			FPDF_ClosePage(page);
		}

		m_pageIndex += 1;

		return chunkValue.SetTextValue(
			PKEY_Search_Contents,
			text,
			CHUNK_TEXT,
			0UL,
			0UL,
			0UL,
			CHUNK_EOS
		);
	}

	// if we get to here we are done with this document
	return FILTER_E_END_OF_CHUNKS;

	// END: GetNextChunkValue
}

int CFilterSample::GetBlock(
	void* param,
	unsigned long position,
	unsigned char* pBuf,
	unsigned long size
)
{
	HRESULT hr;
	LPSTREAM pStm = reinterpret_cast<CFilterSample*>(param)->m_pStream;
	if (SUCCEEDED(hr = pStm->Seek(LARGE_INTEGER{ position }, STREAM_SEEK_SET, NULL)))
	{
		while (true)
		{
			if (size == 0)
			{
				return 1; // success
			}

			ULONG cbRead = 0;
			if (FAILED(hr = pStm->Read(pBuf, size, &cbRead)))
			{
				break;
			}

			if (cbRead == 0)
			{
				break;
			}

			pBuf += cbRead;
			size -= cbRead;
		}
	}

	return 0; // fail
}
