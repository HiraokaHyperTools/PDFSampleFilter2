// UsePdfium.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include <atlbase.h>
#include <atlstr.h>
#include <fpdfview.h>
#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fcntl.h>
#include <io.h>

struct DRect {
	// left
	double l;
	// top (PDF coord, up is plus)
	double t;
	// right
	double r;
	// bottom (PDF coord, up is plus)
	double b;

	bool SeemsToContinue(const DRect& prev) const
	{
		double h = t - b;
		double sharedHeight = std::fmin(t, prev.t) - std::fmax(b, prev.b);
		return true
			&& (prev.r <= l && l < prev.r + h / 2)
			&& h * 0.5 <= sharedHeight
			;
	}
};

int Apply(LPCWSTR pdfFile)
{
	std::wcout << L"--- " << pdfFile << std::endl;

	CW2A test_doc(pdfFile, CP_UTF8);
	FPDF_DOCUMENT doc = FPDF_LoadDocument(test_doc, NULL);
	if (!doc) {
		ULONG errorCode = FPDF_GetLastError();
		std::wcout << L"& FPDF_LoadDocument failed with code: " << errorCode << std::endl;
		return 1;
	}

	const char* props[] = {
		"Title",
		"Author",
		"Subject",
		"Keywords",
		"Creator",
		"Producer",
		"CreationDate",
		"ModDate",
		nullptr,
	};

	WCHAR content[2048];
	for (int x = 0; props[x]; x++) {
		ULONG cb = FPDF_GetMetaText(doc, props[x], content, sizeof(WCHAR) * 2048);
		if (cb != 0) {
			std::wcout << props[x] << L": " << content << std::endl;
		}
		else {
			std::wcout << props[x] << L" not found." << std::endl;
		}
	}

	int numPages = FPDF_GetPageCount(doc);
	for (int y = 0; y < numPages; y++) {
		FPDF_PAGE page = FPDF_LoadPage(doc, y);
		if (page != NULL) {
			std::wcout << L"Page " << y << std::endl;
			FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
			if (textPage != NULL) {
				int numRects = FPDFText_CountRects(textPage, 0, -1);
				DRect prevRect;
				for (int x = 0; x < numRects; x++) {
					DRect rect;
					if (FPDFText_GetRect(textPage, x, &rect.l, &rect.t, &rect.r, &rect.b)) {
						bool continuous = x != 0 && rect.SeemsToContinue(prevRect);
						int numText = FPDFText_GetBoundedText(textPage, rect.l, rect.t, rect.r, rect.b, (PUSHORT)content, 2048);
						if (1 <= numText) {
							std::wcout << L" Rect " << std::setw(3) << x
								<< L" " << std::setw(8) << rect.l
								<< L" " << std::setw(8) << rect.t
								<< L" " << std::setw(8) << rect.r
								<< L" " << std::setw(8) << rect.b
								<< L" "
								<< (continuous ? L"|" : L"+")
								<< L" `" << content << L"`"
								<< std::endl;
						}
						prevRect = rect;
					}
				}
				FPDFText_ClosePage(textPage);
			}
			FPDF_ClosePage(page);
		}
	}

	std::wcout << L"EOD" << std::endl;

	FPDF_CloseDocument(doc);
	return 0;
}

int Walk(LPCWSTR path)
{
	DWORD attr = GetFileAttributesW(path);
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return 1;
	}
	else if (attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		std::wcout << L"--- " << path << std::endl;

		_wfinddata_t fd;
		intptr_t handle = _wfindfirst(CAtlStringW(path) + L"\\*", &fd);
		if (handle != -1)
		{
			do
			{
				if (wcscmp(fd.name, L".") != 0 && wcscmp(fd.name, L"..") != 0)
				{
					CAtlStringW fullPath = path;
					fullPath += L"\\";
					fullPath += fd.name;
					if (fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
					{
						Walk(fullPath);
					}
					else
					{
						// Check if the file is a PDF
						if (fullPath.Right(4).CompareNoCase(L".pdf") == 0)
						{
							Apply(fullPath);
						}
					}
				}
			} while (_wfindnext(handle, &fd) == 0);

			_findclose(handle);
		}
		return 0;
	}
	else
	{
		return Apply(path);
	}
}

int wmain(int argc, wchar_t** argv)
{
	if (argc < 2) {
		fputws(L"UsePdfium [input.pdf | dir]", stderr);
		return 1;
	}

	// [visual c++ - Why are certain Unicode characters causing std::wcout to fail in a console app? - Stack Overflow](https://stackoverflow.com/questions/19193429/why-are-certain-unicode-characters-causing-stdwcout-to-fail-in-a-console-app)
	if (_setmode(_fileno(stdout), _O_U16TEXT) == -1) {
		return 1;
	}

	// [PDFium - Getting Started with PDFium](https://pdfium.googlesource.com/pdfium/+/HEAD/docs/getting-started.md)

	FPDF_LIBRARY_CONFIG config;
	config.version = 2;
	config.m_pUserFontPaths = NULL;
	config.m_pIsolate = NULL;
	config.m_v8EmbedderSlot = 0;

	FPDF_InitLibraryWithConfig(&config);

	int exitCode = Walk(argv[1]);

	FPDF_DestroyLibrary();
	return exitCode;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
