#include "bitmap_utils.h"

/**
 * BitmapUtils
 */

BitmapUtils::BitmapUtils()
{
	hUxTheme_ = LoadLibrary(L"UXTHEME.DLL");

	if (hUxTheme_)
	{
		pfnGetBufferedPaintBits_ = (FN_GetBufferedPaintBits)::GetProcAddress(hUxTheme_, "GetBufferedPaintBits");
		pfnBeginBufferedPaint_ = (FN_BeginBufferedPaint)::GetProcAddress(hUxTheme_, "BeginBufferedPaint");
		pfnEndBufferedPaint_ = (FN_EndBufferedPaint)::GetProcAddress(hUxTheme_, "EndBufferedPaint");
	} else
	{
		pfnGetBufferedPaintBits_ = NULL;
		pfnBeginBufferedPaint_ = NULL;
		pfnEndBufferedPaint_ = NULL;
	}
}

BitmapUtils::~BitmapUtils()
{
    std::map<UINT, HBITMAP>::iterator it;
    for (it = bitmaps_.begin(); it != bitmaps_.end(); ++it){
        DeleteObject(it->second);
    }
    bitmaps_.clear();

	if (hUxTheme_)
		FreeLibrary(hUxTheme_);
}

bool BitmapUtils::IsVistaOrLater() const
{
	static OSVERSIONINFO vi = {};
	if(0 == vi.dwOSVersionInfoSize)
	{
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx(&vi);
	}
	return vi.dwMajorVersion >= 6;
}

HBITMAP BitmapUtils::IconToBitmapPARGB32(HINSTANCE hInst, UINT uIcon)
{
    std::map<UINT, HBITMAP>::iterator it = bitmaps_.lower_bound(uIcon);
    if (it != bitmaps_.end() && it->first == uIcon)
        return it->second;

	HBITMAP hBmp = HBMMENU_CALLBACK;
	if (IsVistaOrLater()) {
		HICON hIcon = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
		hBmp = IconToBitmapPARGB32(hIcon);
		DestroyIcon(hIcon);
	}

    if(hBmp)
        bitmaps_.insert(std::make_pair(uIcon, hBmp));

    return hBmp;
}

HBITMAP BitmapUtils::IconToBitmapPARGB32(HICON hIcon) const
{
    if (!hIcon || !IsVistaOrLater())
        return NULL;

    SIZE sizIcon;
    sizIcon.cx = GetSystemMetrics(SM_CXSMICON);
    sizIcon.cy = GetSystemMetrics(SM_CYSMICON);

    RECT rcIcon;
    SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);
    HBITMAP hBmp = NULL;

    HDC hdcDest = CreateCompatibleDC(NULL);
    if (hdcDest)
    {
        if (SUCCEEDED(Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp)))
        {
            HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hBmp);
            if (hbmpOld)
            {
                BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                BP_PAINTPARAMS paintParams = {};
                paintParams.cbSize = sizeof(paintParams);
                paintParams.dwFlags = BPPF_ERASE;
                paintParams.pBlendFunction = &bfAlpha;

                HDC hdcBuffer;
                HPAINTBUFFER hPaintBuffer = pfnBeginBufferedPaint_(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
                if (hPaintBuffer)
                {
                    if (DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL))
                    {
                        // If icon did not have an alpha channel we need to convert buffer to PARGB
                        ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
                    }

                    // This will write the buffer contents to the destination bitmap
                    pfnEndBufferedPaint_(hPaintBuffer, TRUE);
                }

                SelectObject(hdcDest, hbmpOld);
            }
        }

        DeleteDC(hdcDest);
    }

    return hBmp;
}

HRESULT BitmapUtils::Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp) const
{
    if (psize == 0)
        return E_INVALIDARG;

    if (phBmp == 0)
        return E_POINTER;

    *phBmp = NULL;

    BITMAPINFO bmi;
    SecureZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmi.bmiHeader.biWidth = psize->cx;
    bmi.bmiHeader.biHeight = psize->cy;
    bmi.bmiHeader.biBitCount = 32;

    HDC hdcUsed = hdc ? hdc : GetDC(NULL);
    if (hdcUsed)
    {
        *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
        if (hdc != hdcUsed)
        {
            ReleaseDC(NULL, hdcUsed);
        }
    }
    return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT BitmapUtils::ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon) const
{
    RGBQUAD *prgbQuad;
    int cxRow;
    HRESULT hr = pfnGetBufferedPaintBits_(hPaintBuffer, &prgbQuad, &cxRow);
    if (SUCCEEDED(hr))
    {
        Gdiplus::ARGB *pargb = reinterpret_cast<Gdiplus::ARGB *>(prgbQuad);
        if (!HasAlpha(pargb, sizIcon, cxRow))
        {
            ICONINFO info;
            if (GetIconInfo(hicon, &info))
            {
                if (info.hbmMask)
                {
                    hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
					DeleteObject(info.hbmMask);
                }

                DeleteObject(info.hbmColor);
            }
        }
    }

    return hr;
}

bool BitmapUtils::HasAlpha(__in Gdiplus::ARGB *pargb, SIZE& sizImage, int cxRow) const
{
    ULONG cxDelta = cxRow - sizImage.cx;
    for (ULONG y = sizImage.cy; y; --y)
    {
        for (ULONG x = sizImage.cx; x; --x)
        {
            if (*pargb++ & 0xFF000000)
            {
                return true;
            }
        }

        pargb += cxDelta;
    }

    return false;
}

HRESULT BitmapUtils::ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow) const
{
    BITMAPINFO bmi;
    SecureZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmi.bmiHeader.biWidth = sizImage.cx;
    bmi.bmiHeader.biHeight = sizImage.cy;
    bmi.bmiHeader.biBitCount = 32;

    HANDLE hHeap = GetProcessHeap();
    void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
    if (pvBits == 0)
        return E_OUTOFMEMORY;

    HRESULT hr = E_UNEXPECTED;
    if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
    {
        ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
        Gdiplus::ARGB *pargbMask = static_cast<Gdiplus::ARGB *>(pvBits);

        for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
        {
            for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
            {
                if (*pargbMask++)
                {
                    // transparent pixel
                    *pargb++ = 0;
                }
                else
                {
                    // opaque pixel
                    *pargb++ |= 0xFF000000;
                }
            }
            pargb += cxDelta;
        }

        hr = S_OK;
    }
    HeapFree(hHeap, 0, pvBits);

    return hr;
}