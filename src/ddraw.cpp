#include <chrono>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

#include <Windows.h>

#include "utils.h"

using namespace std::literals;

static constexpr auto DD_OK = ::DWORD{0x00000000};

auto g_direct_draw_funcs = std::unordered_map<std::string, ::PROC>{};
auto g_surface_funcs = std::unordered_map<std::string, ::PROC>{};
static std::uint8_t *g_surface_memory{};
static auto g_new_surface = std::vector<std::uint8_t>(432 * 2560);
static void *g_locked_surface{};

#define DUMMYUNIONNAMEN(n)

enum DDSD_Flags
{
    DDSD_CAPS = 0x1,
    DDSD_HEIGHT = 0x2,
    DDSD_WIDTH = 0x4,
    DDSD_PITCH = 0x8,
    DDSD_BACKBUFFERCOUNT = 0x20,
    DDSD_ZBUFFERBITDEPTH = 0x40,
    DDSD_ALPHABITDEPTH = 0x80,
    DDSD_LPSURFACE = 0x800,
    DDSD_PIXELFORMAT = 0x1000,
    DDSD_CKDESTOVERLAY = 0x2000,
    DDSD_CKDESTBLT = 0x4000,
    DDSD_CKSRCOVERLAY = 0x8000,
    DDSD_CKSRCBLT = 0x10000,
    DDSD_MIPMAPCOUNT = 0x20000,
    DDSD_REFRESHRATE = 0x40000,
    DDSD_LINEARSIZE = 0x80000,
    DDSD_TEXTURESTAGE = 0x100000,
    DDSD_FVF = 0x200000,
    DDSD_SRCVBHANDLE = 0x400000,
    DDSD_DEPTH = 0x800000,
    DDSD_ALL = 0xFFF9EE1,
};

enum DDPF_Flags
{
    DDPF_ALPHAPIXELS = 0x00000001,
    DDPF_ALPHA = 0x00000002,
    DDPF_FOURCC = 0x00000004,
    DDPF_PALETTEINDEXED4 = 0x00000008,
    DDPF_PALETTEINDEXEDTO8 = 0x00000010,
    DDPF_PALETTEINDEXED8 = 0x00000020,
    DDPF_RGB = 0x00000040,
    DDPF_COMPRESSED = 0x00000080,
    DDPF_RGBTOYUV = 0x00000100,
    DDPF_YUV = 0x00000200,
    DDPF_ZBUFFER = 0x00000400,
    DDPF_PALETTEINDEXED1 = 0x00000800,
    DDPF_PALETTEINDEXED2 = 0x00001000,
    DDPF_ZPIXELS = 0x00002000,
    DDPF_STENCILBUFFER = 0x00004000,
    DDPF_ALPHAPREMULT = 0x00008000,
    DDPF_LUMINANCE = 0x00020000,
    DDPF_BUMPLUMINANCE = 0x00040000,
    DDPF_BUMPDUDV = 0x00080000
};

#pragma pack(push, 1)
typedef struct _DDCOLORKEY
{
    ::DWORD dwColorSpaceLowValue;
    ::DWORD dwColorSpaceHighValue;
} DDCOLORKEY;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DDSCAPS
{
    ::DWORD dwCaps;
} DDSCAPS;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DDPIXELFORMAT
{
    DWORD dwSize;
    DDPF_Flags dwFlags;
    DWORD dwFourCC;
    union
    {
        DWORD dwRGBBitCount;
        DWORD dwYUVBitCount;
        DWORD dwZBufferBitDepth;
        DWORD dwAlphaBitDepth;
        DWORD dwLuminanceBitCount;
        DWORD dwBumpBitCount;
        DWORD dwPrivateFormatBitCount;

    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD dwRBitMask;
        DWORD dwYBitMask;
        DWORD dwStencilBitDepth;
        DWORD dwLuminanceBitMask;
        DWORD dwBumpDuBitMask;
        DWORD dwOperations;
    } DUMMYUNIONNAMEN(2);
    union
    {
        DWORD dwGBitMask;
        DWORD dwUBitMask;
        DWORD dwZBitMask;
        DWORD dwBumpDvBitMask;
        struct
        {
            WORD wFlipMSTypes;
            WORD wBltMSTypes;
        } MultiSampleCaps;

    } DUMMYUNIONNAMEN(3);
    union
    {
        DWORD dwBBitMask;
        DWORD dwVBitMask;
        DWORD dwStencilBitMask;
        DWORD dwBumpLuminanceBitMask;
    } DUMMYUNIONNAMEN(4);
    union
    {
        DWORD dwRGBAlphaBitMask;
        DWORD dwYUVAlphaBitMask;
        DWORD dwLuminanceAlphaBitMask;
        DWORD dwRGBZBitMask;
        DWORD dwYUVZBitMask;
    } DUMMYUNIONNAMEN(5);
} DDPIXELFORMAT;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DDSURFACEDESC2
{
    DWORD dwSize;
    DDSD_Flags dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    union
    {
        LONG lPitch;
        DWORD dwLinearSize;
    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD dwBackBufferCount;
        DWORD dwDepth;
    } DUMMYUNIONNAMEN(5);
    union
    {
        DWORD dwMipMapCount;

        DWORD dwRefreshRate;
        DWORD dwSrcVBHandle;
    } DUMMYUNIONNAMEN(2);
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    LPVOID lpSurface;
    union
    {
        DDCOLORKEY ddckCKDestOverlay;
        DWORD dwEmptyFaceColor;
    } DUMMYUNIONNAMEN(3);
    DDCOLORKEY ddckCKDestBlt;
    DDCOLORKEY ddckCKSrcOverlay;
    DDCOLORKEY ddckCKSrcBlt;
    union
    {
        DDPIXELFORMAT ddpfPixelFormat;
        DWORD dwFVF;
    } DUMMYUNIONNAMEN(4);
    DDSCAPS ddsCaps;
    DWORD dwTextureStage;
    char pad[0xc];
} *LPDDSURFACEDESC2, DDSURFACEDESC2;
#pragma pack(pop)

auto to_string(const ::DDPIXELFORMAT *pixel_format) -> std::string
{
    if (pixel_format == nullptr)
    {
        return "nullptr";
    }
    return std::format(
        "DDPIXELFORMAT: [dwSize: {}, dwFlags: {}, dwFourCC: {}, dwRGBBitCount: {}, dwRBitMask: {:b}, "
        "dwGBitMask: {:b}, dwBBitMask: {:b}, dwRGBAlphaBitMask: {:b}]",
        pixel_format->dwSize,
        static_cast<::DWORD>(pixel_format->dwFlags),
        pixel_format->dwFourCC,
        pixel_format->dwRGBBitCount,
        pixel_format->dwRBitMask,
        pixel_format->dwGBitMask,
        pixel_format->dwBBitMask,
        pixel_format->dwRGBAlphaBitMask);
}

auto to_string(const ::DDSURFACEDESC2 *surface_desc) -> std::string
{
    auto strm = std::stringstream{};

    strm << std::format("DDSURFACEDESC2:\n\tSize: {}\n", surface_desc->dwSize);

    strm << std::format("\tDDSD_HEIGHT: {}\n", surface_desc->dwHeight);
    strm << std::format("\tDDSD_WIDTH: {}\n", surface_desc->dwWidth);
    strm << std::format("\tDDSD_PITCH: {}\n", surface_desc->lPitch);
    strm << std::format("\tDDSD_LPSURFACE: {}\n", surface_desc->lpSurface);
    strm << std::format("\tDDSD_PIXELFORMAT: {}\n", to_string(&surface_desc->ddpfPixelFormat));

    return strm.str();
}

auto to_string(const ::RECT *rect) -> std::string
{
    if (rect == nullptr)
    {
        return "nullptr";
    }
    return std::format(
        "RECT: [left: {}, top: {}, right: {}, bottom: {}]", rect->left, rect->top, rect->right, rect->bottom);
}

extern "C"
{

HRESULT WINAPI
Lock_hook(void *that, ::LPRECT lpDestRect, ::DDSURFACEDESC2 *lpDDSurface, ::DWORD dwFlags, void *lpDDBltFx)
{
    log("Lock_hook called {} [{}] {} with flags: {:b}",
        that,
        to_string(lpDestRect),
        static_cast<void *>(lpDDSurface),
        dwFlags);

    g_locked_surface = that;

    ensure(g_surface_funcs.contains("Lock"), "Lock_hook called, but Lock function is not registered");
    const auto orig_func = g_surface_funcs["Lock"];

    const auto result =
        reinterpret_cast<decltype(&Lock_hook)>(orig_func)(that, lpDestRect, lpDDSurface, dwFlags, lpDDBltFx);

    log("Surface description: {}", to_string(lpDDSurface));

    g_surface_memory = reinterpret_cast<std::uint8_t *>(lpDDSurface->lpSurface);
    lpDDSurface->lpSurface = (void *)g_new_surface.data();
    ensure(g_surface_memory != nullptr, "Lock_hook returned a null surface pointer");

    // lpDDSurface->lPitch = 0x500;
    // lpDDSurface->ddpfPixelFormat.dwSize = 0x20;
    // lpDDSurface->ddpfPixelFormat.dwFlags = DDPF_RGB;
    // lpDDSurface->ddpfPixelFormat.dwFourCC = 0x00000000l;
    // lpDDSurface->ddpfPixelFormat.dwRGBBitCount = 16;
    // lpDDSurface->ddpfPixelFormat.dwRBitMask = 63488;
    // lpDDSurface->ddpfPixelFormat.dwGBitMask = 2016;
    // lpDDSurface->ddpfPixelFormat.dwBBitMask = 31;
    // lpDDSurface->ddpfPixelFormat.dwRGBAlphaBitMask = 0;

    log("Surface description: {}", to_string(lpDDSurface));

    if (result != DD_OK)
    {
        log("Lock_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI Unlock_hook(void *that, ::LPRECT lpDestRect)
{
    log("Unlock_hook called {} [{}]", that, to_string(lpDestRect));

    PROC **com = reinterpret_cast<PROC **>(that);
    auto funcs = *com;
    auto f = reinterpret_cast<HRESULT(WINAPI *)(void *, LPDDSURFACEDESC2)>(funcs[22]);

    auto desc = ::DDSURFACEDESC2{};
    desc.dwSize = sizeof(::DDSURFACEDESC2);
    f(that, &desc);

    log("Surface description: {}", to_string(&desc));

    const auto *old_surface_ptr = reinterpret_cast<const std::uint16_t *>(g_new_surface.data());
    auto *new_surface_ptr = reinterpret_cast<std::uint32_t *>(g_surface_memory);

    // for (auto i = 0u; i < g_new_surface.size(); ++i)
    // {
    //     new_surface_ptr[i] = old_surface_ptr[i];
    // }

    std::memset(new_surface_ptr, 0xff, g_new_surface.size());
    auto j = 0;

    for (auto y = 0; y < 430 * 2; y += 2)
    {
        for (auto x = 0; x < 640; ++x)
        {
            const auto i = y * 640 + x;
            const auto pixel_16 = old_surface_ptr[i];

            std::uint8_t r = (pixel_16 & 0xF800) >> 11;
            std::uint8_t g = (pixel_16 & 0x07E0) >> 5;
            std::uint8_t b = (pixel_16 & 0x001F);

            r = (r << 3) | (r >> 2);
            g = (g << 2) | (g >> 4);
            b = (b << 3) | (b >> 2);

            new_surface_ptr[j++] = (r << 16) | (g << 8) | b;
        }
    }

    // for (auto i = 0; i < 430 * 640; i += 2)
    // {
    //     const auto pixel_16 = old_surface_ptr[i];
    //
    //     std::uint8_t r = (pixel_16 & 0xF800) >> 11;
    //     std::uint8_t g = (pixel_16 & 0x07E0) >> 5;
    //     std::uint8_t b = (pixel_16 & 0x001F);
    //
    //     r = (r << 3) | (r >> 2);
    //     g = (g << 2) | (g >> 4);
    //     b = (b << 3) | (b >> 2);
    //
    //     new_surface_ptr[i] = (r << 16) | (g << 8) | b;
    //     //  new_surface_ptr[j] = pixel_16;
    //     //  j += 2;
    //     //
    //     //      if (i != 0 && i % 640 == 0)
    //     //      {
    //     //          j += 1;
    //     //      }
    // }
    //
    // std::memcpy(g_surface_memory, g_new_surface.data(), g_new_surface.size());

    ensure(g_surface_funcs.contains("Unlock"), "Unlock_hook called, but Unlock function is not registered");
    const auto orig_func = g_surface_funcs["Unlock"];

    const auto result = reinterpret_cast<decltype(&Unlock_hook)>(orig_func)(that, lpDestRect);

    if (result != DD_OK)
    {
        log("Unlock_hook failed with error code: {}", result);
    }

    log("Unlock_hook completed successfully");

    return result;
}

HRESULT WINAPI
Blt_hook(void *that, ::LPRECT lpDestRect, void *lpSrcSurface, ::LPRECT lpSrcRect, ::DWORD dwFlags, void *lpDDBltFx)
{
    log("Blt_hook called {} [{}] -> {} [{}] with flags: {:b}",
        lpSrcSurface,
        to_string(lpSrcRect),
        that,
        to_string(lpDestRect),
        dwFlags);

    ensure(g_surface_funcs.contains("Blt"), "Blt_hook called, but Blt function is not registered");
    const auto orig_func = g_surface_funcs["Blt"];

    const auto result =
        reinterpret_cast<decltype(&Blt_hook)>(orig_func)(that, lpDestRect, lpSrcSurface, lpSrcRect, dwFlags, lpDDBltFx);

    if (result != DD_OK)
    {
        log("Blt_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI BltBatch_hook(void *that, void *lpDDBltBatch, ::DWORD dwCount, ::DWORD dwFlags)
{
    log("BltBatch_hook called");

    ensure(g_surface_funcs.contains("BltBatch"), "BltBatch_hook called, but BltBatch function is not registered");
    const auto orig_func = g_surface_funcs["BltBatch"];

    const auto result = reinterpret_cast<decltype(&BltBatch_hook)>(orig_func)(that, lpDDBltBatch, dwCount, dwFlags);

    if (result != DD_OK)
    {
        log("BltBatch_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI BltFast_hook(void *that, ::DWORD dwX, ::DWORD dwY, void *lpSrcSurface, void *lpSrcRect, ::DWORD dwFlags)
{
    log("BltFast_hook called");

    ensure(g_surface_funcs.contains("BltFast"), "BltFast_hook called, but BltFast function is not registered");
    const auto orig_func = g_surface_funcs["BltFast"];

    const auto result =
        reinterpret_cast<decltype(&BltFast_hook)>(orig_func)(that, dwX, dwY, lpSrcSurface, lpSrcRect, dwFlags);

    if (result != DD_OK)
    {
        log("BltFast_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI GetSurfaceDesc_hook(void *that, ::DDSURFACEDESC2 *lpDDSurfaceDesc)
{
    log("GetSurfaceDesc_hook called");

    ensure(
        g_surface_funcs.contains("GetSurfaceDesc"),
        "GetSurfaceDesc_hook called, but GetSurfaceDesc function is not registered");
    const auto orig_func = g_surface_funcs["GetSurfaceDesc"];

    const auto result = reinterpret_cast<decltype(&GetSurfaceDesc_hook)>(orig_func)(that, lpDDSurfaceDesc);

    log("Surface description: {}", to_string(lpDDSurfaceDesc));

    if (result != DD_OK)
    {
        log("GetSurfaceDesc_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI SetSurfaceDesc_hook(void *that, ::DDSURFACEDESC2 *lpDDSurfaceDesc, ::DWORD dwFlags)
{
    log("SetSurfaceDesc_hook called with flags: {:b}", dwFlags);

    ensure(
        g_surface_funcs.contains("SetSurfaceDesc"),
        "SetSurfaceDesc_hook called, but SetSurfaceDesc function is not registered");
    const auto orig_func = g_surface_funcs["SetSurfaceDesc"];

    log("Surface description (before): {}", to_string(lpDDSurfaceDesc));

    const auto result = reinterpret_cast<decltype(&SetSurfaceDesc_hook)>(orig_func)(that, lpDDSurfaceDesc, dwFlags);

    log("Surface description (after): {}", to_string(lpDDSurfaceDesc));

    if (result != DD_OK)
    {
        log("SetSurfaceDesc_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT SetColorKey_hook(void *that, ::DWORD dwFlags, ::DDCOLORKEY *lpDDColorKey)
{
    log("SetColorKey_hook called with flags: {:b}", dwFlags);

    ensure(
        g_surface_funcs.contains("SetColorKey"), "SetColorKey_hook called, but SetColorKey function is not registered");
    const auto orig_func = g_surface_funcs["SetColorKey"];

    const auto result = reinterpret_cast<decltype(&SetColorKey_hook)>(orig_func)(that, dwFlags, lpDDColorKey);

    if (result != DD_OK)
    {
        log("SetColorKey_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI GetPixelFormat_hook(void *that, ::DDPIXELFORMAT *lpDDPixelFormat)
{
    log("GetPixelFormat_hook called");

    ensure(
        g_surface_funcs.contains("GetPixelFormat"),
        "GetPixelFormat_hook called, but GetPixelFormat function is not registered");
    const auto orig_func = g_surface_funcs["GetPixelFormat"];

    log("Pixel format: {}", to_string(lpDDPixelFormat));
    const auto result = reinterpret_cast<decltype(&GetPixelFormat_hook)>(orig_func)(that, lpDDPixelFormat);
    log("Pixel format: {}", to_string(lpDDPixelFormat));

    // lpDDPixelFormat->dwSize = 0x20;
    // lpDDPixelFormat->dwFlags = DDPF_RGB;
    // lpDDPixelFormat->dwFourCC = 0x00000000l;
    // lpDDPixelFormat->dwRGBBitCount = 16;
    // lpDDPixelFormat->dwRBitMask = 63488;
    // lpDDPixelFormat->dwGBitMask = 2016;
    // lpDDPixelFormat->dwBBitMask = 31;
    // lpDDPixelFormat->dwRGBAlphaBitMask = 0;

    if (result != DD_OK)
    {
        log("GetPixelFormat_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI CreateSurface_hook(void *that, void *lpDDSurfaceDesc, void **lplpDDSurface, IUnknown *pUnkOuter)
{
    log("CreateSurface_hook called");

    ensure(
        g_direct_draw_funcs.contains("CreateSurface"),
        "CreateSurface_hook called, but CreateSurface function is not registered");
    const auto orig_func = g_direct_draw_funcs["CreateSurface"];

    const auto result =
        reinterpret_cast<decltype(&CreateSurface_hook)>(orig_func)(that, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

    log("Surface description: {}", to_string(reinterpret_cast<::DDSURFACEDESC2 *>(lpDDSurfaceDesc)));

    auto *com_obj = reinterpret_cast<::PROC **>(*lplpDDSurface);
    ensure(com_obj != nullptr, "CreateSurface_hook returned a null surface pointer");
    auto *com_vtable = *com_obj;

    if (!g_surface_funcs.contains("Blt"))
    {
        g_surface_funcs["Blt"] = std::exchange(com_vtable[5], reinterpret_cast<::PROC>(Blt_hook));
    }
    if (!g_surface_funcs.contains("BltBatch"))
    {
        g_surface_funcs["BltBatch"] = std::exchange(com_vtable[6], reinterpret_cast<::PROC>(BltBatch_hook));
    }
    if (!g_surface_funcs.contains("BltFast"))
    {
        g_surface_funcs["BltFast"] = std::exchange(com_vtable[7], reinterpret_cast<::PROC>(BltFast_hook));
    }
    if (!g_surface_funcs.contains("Lock"))
    {
        g_surface_funcs["Lock"] = std::exchange(com_vtable[25], reinterpret_cast<::PROC>(Lock_hook));
    }
    if (!g_surface_funcs.contains("Unlock"))
    {
        g_surface_funcs["Unlock"] = std::exchange(com_vtable[32], reinterpret_cast<::PROC>(Unlock_hook));
    }
    if (!g_surface_funcs.contains("GetSurfaceDesc"))
    {
        g_surface_funcs["GetSurfaceDesc"] =
            std::exchange(com_vtable[22], reinterpret_cast<::PROC>(GetSurfaceDesc_hook));
    }
    if (!g_surface_funcs.contains("SetSurfaceDesc"))
    {
        g_surface_funcs["SetSurfaceDesc"] =
            std::exchange(com_vtable[23], reinterpret_cast<::PROC>(SetSurfaceDesc_hook));
    }
    if (!g_surface_funcs.contains("GetPixelFormat"))
    {
        g_surface_funcs["GetPixelFormat"] =
            std::exchange(com_vtable[21], reinterpret_cast<::PROC>(GetPixelFormat_hook));
    }
    if (!g_surface_funcs.contains("SetColorKey"))
    {
        g_surface_funcs["SetColorKey"] = std::exchange(com_vtable[29], reinterpret_cast<::PROC>(SetColorKey_hook));
    }

    if (result != DD_OK)
    {
        log("CreateSurface_hook failed with error code: {}", result);
    }

    log("\tSurface: {}", *lplpDDSurface);

    return result;
}

HRESULT WINAPI SetDisplayMode_hook(
    void *that,
    ::DWORD dwWidth,
    ::DWORD dwHeight,
    ::DWORD dwBPP,
    ::DWORD dwRefreshRate,
    ::DWORD dwFlags)
{
    log("SetDisplayMode_hook called with width: {}, height: {}, bpp: {}, refresh rate: {}, flags: {}",
        dwWidth,
        dwHeight,
        dwBPP,
        dwRefreshRate,
        dwFlags);

    ensure(
        g_direct_draw_funcs.contains("SetDisplayMode"),
        "SetDisplayMode_hook called, but SetDisplayMode function is not registered");
    const auto orig_func = g_direct_draw_funcs["SetDisplayMode"];

    const auto result = reinterpret_cast<decltype(&SetDisplayMode_hook)>(
        orig_func)(that, dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);

    if (result != DD_OK)
    {
        log("SetDisplayMode_hook failed with error code: {}", result);
    }

    return result;
}

HRESULT WINAPI DirectDrawSurface_QueryInterface_hook(void *that, const GUID *riid, void **ppvObject)
{
    log("DirectDrawSurface_QueryInterface_hook called");

    ensure(
        g_direct_draw_funcs.contains("QueryInterface"),
        "QueryInterface_hook called, but QueryInterface function is not "
        "registered");
    const auto orig_func = g_direct_draw_funcs["QueryInterface"];

    const auto result =
        reinterpret_cast<decltype(&DirectDrawSurface_QueryInterface_hook)>(orig_func)(that, riid, ppvObject);

    auto *com_obj = reinterpret_cast<::PROC **>(*ppvObject);
    auto *com_vtable = *com_obj;
    g_direct_draw_funcs["CreateSurface"] = std::exchange(com_vtable[6], reinterpret_cast<::PROC>(CreateSurface_hook));
    g_direct_draw_funcs["SetDisplayMode"] =
        std::exchange(com_vtable[21], reinterpret_cast<::PROC>(SetDisplayMode_hook));

    if (result != DD_OK)
    {
        log("DirectDrawSurface_QueryInterface_hook failed with error code: {}", result);
    }

    return result;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, void FAR **lplpDD, IUnknown FAR *pUnkOuter)
{
    log("DirectDrawCreate called");

    const auto ddraw_dll = ::LoadLibraryA("C:\\Windows\\system32\\ddraw.dll");
    ensure(ddraw_dll != NULL, "failed to load ddraw.dll");

    const auto direct_draw_create =
        reinterpret_cast<decltype(&DirectDrawCreate)>(::GetProcAddress(ddraw_dll, "DirectDrawCreate"));
    ensure(direct_draw_create != NULL, "failed to get address of DirectDrawCreate");

    auto result = direct_draw_create(lpGUID, lplpDD, pUnkOuter);

    auto *com_obj = reinterpret_cast<::PROC **>(*lplpDD);
    auto *com_vtable = *com_obj;
    g_direct_draw_funcs["QueryInterface"] =
        std::exchange(com_vtable[0], reinterpret_cast<::PROC>(DirectDrawSurface_QueryInterface_hook));

    if (result != DD_OK)
    {
        log("DirectDrawCreate failed with error code: {}", result);
    }

    return result;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateA(void *lpCallback, LPVOID lpContext)
{
    log("DirectDrawEnumerateA called");

    const auto ddraw_dll = ::LoadLibrary("C:\\windows\\system32\\ddraw.dll");
    ensure(ddraw_dll != NULL, "failed to load ddraw.dll");

    const auto direct_draw_enumerate =
        reinterpret_cast<decltype(&DirectDrawEnumerateA)>(::GetProcAddress(ddraw_dll, "DirectDrawEnumerateA"));
    ensure(direct_draw_enumerate != NULL, "failed to get address of DirectDrawEnumerateA");

    auto result = direct_draw_enumerate(lpCallback, lpContext);

    if (result != DD_OK)
    {
        log("DirectDrawEnumerateA failed with error code: {}", result);
    }

    return result;
}
}
