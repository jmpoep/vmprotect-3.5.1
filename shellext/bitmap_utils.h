#ifndef BITMAP_UTILS_H
#define BITMAP_UTILS_H

#include <windows.h>
#include <map>
#include <Uxtheme.h>
#include <GdiPlus.h>

#pragma comment(lib, "UxTheme.lib")

typedef HRESULT (WINAPI *FN_GetBufferedPaintBits) (HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer, int *pcxRow);
typedef HPAINTBUFFER (WINAPI *FN_BeginBufferedPaint) (HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (WINAPI *FN_EndBufferedPaint) (HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);

class BitmapUtils
{
public:
	BitmapUtils();
	~BitmapUtils();
	HBITMAP IconToBitmapPARGB32(HINSTANCE hInst, UINT uIcon);
	HBITMAP IconToBitmapPARGB32(HICON hIcon) const;
private:
	HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp) const;
    HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon) const;
    bool HasAlpha(Gdiplus::ARGB *pargb, SIZE& sizImage, int cxRow) const;
    HRESULT ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow) const;
	std::map<UINT, HBITMAP> bitmaps_;

	bool IsVistaOrLater() const;
	HMODULE hUxTheme_;
	FN_GetBufferedPaintBits pfnGetBufferedPaintBits_;
	FN_BeginBufferedPaint pfnBeginBufferedPaint_;
	FN_EndBufferedPaint pfnEndBufferedPaint_;
};

#endif