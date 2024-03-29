/*
    video.c

    Contains video APIs

    Copyright (c) Microsoft Corporation 1992. All rights reserved

*/

#include <windows.h>
#include <mmsystem.h>

#include "win32.h"
#include "msviddrv.h"
#include "msvideo.h"
#include "msvideoi.h"
#ifdef WIN32
#include <mmddk.h>
#endif

#include <ver.h>

#ifndef NOTHUNKS
#include "thunks.h"
#endif //!NOTHUNKS

/*
 * Variables

*/

SZCODE  szNull[]        = TEXT("");
SZCODE  szVideo[]       = TEXT("msvideo");
#ifndef WIN32
SZCODE  szDrivers[]     = "Drivers";
SZCODE  szDrivers32[]   = "Drivers32";
#else
STATICDT SZCODE  szDrivers[]     = DRIVERS_SECTION;
#endif

STATICDT SZCODE  szSystemIni[]   = TEXT("system.ini");

UINT    wTotalVideoDevs;                  // total video devices
UINT    wVideoDevs32;              // 32-bit video devices
UINT    bVideo0Invalid;              // if true: ignore MSVideo
                      // (see videoGetNumDevs)
HINSTANCE ghInst;                         // our module handle


/*
 * @doc INTERNAL  VIDEO validation code for VIDEOHDRs
*/

#define IsVideoHeaderPrepared(hVideo, lpwh)      ((lpwh)->dwFlags &  VHDR_PREPARED)
#define MarkVideoHeaderPrepared(hVideo, lpwh)    ((lpwh)->dwFlags |= VHDR_PREPARED)
#define MarkVideoHeaderUnprepared(hVideo, lpwh)  ((lpwh)->dwFlags &=~VHDR_PREPARED)



/*
 * @doc EXTERNAL  VIDEO

 * @func DWORD | videoMessage | This function sends messages to a
 *   video device channel.

 * @parm HVIDEO | hVideo | Specifies the handle to the video device channel.

 * @parm UINT | wMsg | Specifies the message to send.

 * @parm DWORD | dwP1 | Specifies the first parameter for the message.

 * @parm DWORD | dwP2 | Specifies the second parameter for the message.

 * @rdesc Returns the message specific value returned from the driver.

 * @comm This function is used for configuration messages such as
 *      <m DVM_SRC_RECT> and <m DVM_DST_RECT>, and
 *      device specific messages.

 * @xref <f videoConfigure>

*/
DWORD WINAPI videoMessage(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

#ifndef NOTHUNKS
    if (Is32bitHandle(hVideo)) {
        return videoMessage32(Map32bitHandle(hVideo), msg, dwP1, dwP2);
    }
#endif
    return SendDriverMessage (hVideo, msg, dwP1, dwP2);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoGetNumDevs | This function returns the number of MSVIDEO
 *   devices installed.

 * @rdesc Returns the number of MSVIDEO devices listed in the
 *  [drivers] (or [drivers32] for NT) section of the SYSTEM.INI file.

 * @comm Because the indexes of the MSVIDEO devices in the SYSTEM.INI
 *       file can be non-contiguous, applications should not assume
 *       the indexes range between zero and the number of devices minus
 *       one.

 * @xref <f videoOpen>
*/
DWORD WINAPI videoGetNumDevs(void)
{
    TCHAR szKey[(sizeof(szVideo)/sizeof(TCHAR)) + 2];
    TCHAR szbuf[128];
    TCHAR szMSVideo32[128];
    int i;

#ifndef NOTHUNKS

    // find how many 32-bit devices there are. indices 0 to wVideoDevs32
    // will be on the 32-bit side, and wVideoDevs32 to wTotalVideoDevs on the
    // 16-bit side
    wVideoDevs32 = (UINT)videoGetNumDevs32();
    wTotalVideoDevs = wVideoDevs32;

    // now add in the 16-bit devices
#else
    wTotalVideoDevs = 0;
#endif

    // NT 3.5 shipped with a hack solution to the capGetDriverDescription
    // problem which was to copy the MSVideo line from [Drivers32] to
    // [Drivers]

    // we now have to allow for this hack by detecting the case where the two
    // entries are the same. We then set bVideo0Invalid, and do not include it
    // in wTotalVideoDevs.


    bVideo0Invalid = FALSE;

    lstrcpy(szKey, szVideo);
    szKey[(sizeof(szVideo)/sizeof(TCHAR)) - 1] = (TCHAR) 0;
    szKey[sizeof(szVideo)/sizeof(TCHAR)] = (TCHAR) 0;

    // first read the [drivers32] MSVideo to compare against
    GetPrivateProfileString(szDrivers32, szKey, szNull, szMSVideo32, sizeof(szMSVideo32)/sizeof(TCHAR), szSystemIni);

    // now read the [Drivers] entries - compare the first one against this

    for (i=0; i < MAXVIDEODRIVERS; i++) {
        if (GetPrivateProfileString(szDrivers,szKey,szNull, szbuf,sizeof(szbuf)/sizeof(TCHAR),szSystemIni)) {
        if ((i == 0) && (lstrcmp(szbuf, szMSVideo32) == 0)) {
        bVideo0Invalid = TRUE;
        } else {
        wTotalVideoDevs++;
        }
    }

        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR) TEXT('1'+ i); // advance driver ordinal
    }
    return (DWORD)wTotalVideoDevs;
}

/*
 * @doc EXTERNAL VIDEO

 * @func DWORD | videoGetErrorText | This function retrieves a
 *   description of the error identified by the error number.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *    This might be NULL if the error is not device specific.

 * @parm UINT | wError | Specifies the error number.

 * @parm LPSTR | lpText | Specifies a far pointer to a buffer used to
 *       return the zero-terminated string corresponding to the error number.

 * @parm UINT | wSize | Specifies the length, in bytes, of the buffer
 *       referenced by <p lpText>.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_BADERRNUM | Specified error number is out of range.
 *   @flag DV_ERR_SIZEFIELD | The return buffer is not large enough
 *         to handle the error text.

 * @comm If the error description is longer than the buffer,
 *   the description is truncated. The returned error string is always
 *   zero-terminated. If <p wSize> is zero, nothing is copied and zero
 *   is returned.
*/
DWORD WINAPI videoGetErrorText(HVIDEO hVideo, UINT wError,
            LPSTR lpText, UINT wSize)
{
    VIDEO_GETERRORTEXT_PARMS vet;

    if (IsBadWritePtr (lpText, wSize))
        return DV_ERR_PARAM1;

    lpText[0] = 0;
    if (((wError >= DV_ERR_BASE) && (wError <= DV_ERR_LASTERROR))) {
        if (wSize > 1) {
            if (!LoadStringA(ghInst, wError, lpText, wSize))
                return DV_ERR_BADERRNUM;
            else
                return DV_ERR_OK;
        }
        else
            return DV_ERR_SIZEFIELD;
    }
    else if (wError >= DV_ERR_USER_MSG && hVideo) {
        vet.dwError = (DWORD) wError;
        vet.lpText = lpText;
        vet.dwLength = (DWORD) wSize;
        return videoMessage (hVideo, DVM_GETERRORTEXT, (DWORD) (LPVOID) &vet,
                        (DWORD) NULL);
    }
    else
        return DV_ERR_BADERRNUM;
}


/*
 * @doc EXTERNAL VIDEO

 * @func DWORD | videoGetChannelCaps | This function retrieves a
 *   description of the capabilities of a channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm LPCHANNEL_CAPS | lpChannelCaps | Specifies a far pointer to a
 *      <t CHANNEL_CAPS> structure.

 * @parm DWORD | dwSize | Specifies the size, in bytes, of the
 *       <t CHANNEL_CAPS> structure.

 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_UNSUPPORTED | Function is not supported.

 * @comm The <t CHANNEL_CAPS> structure returns the capability
 *   information. For example, capability information might
 *   include whether or not the channel can crop and scale images,
 *   or show overlay.
*/
DWORD WINAPI videoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps,
            DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpChannelCaps, sizeof (CHANNEL_CAPS)))
        return DV_ERR_PARAM1;

    // _fmemset (lpChannelCaps, 0, sizeof (CHANNEL_CAPS));

    lpChannelCaps->dwFlags = 0;
    lpChannelCaps->dwSrcRectXMod = 0;
    lpChannelCaps->dwSrcRectYMod = 0;
    lpChannelCaps->dwSrcRectWidthMod = 0;
    lpChannelCaps->dwSrcRectHeightMod = 0;
    lpChannelCaps->dwDstRectXMod = 0;
    lpChannelCaps->dwDstRectYMod = 0;
    lpChannelCaps->dwDstRectWidthMod = 0;
    lpChannelCaps->dwDstRectHeightMod = 0;

    return videoMessage(hVideo, DVM_GET_CHANNEL_CAPS, (DWORD) lpChannelCaps,
        (DWORD) dwSize);
}


/*
 * @doc EXTERNAL VIDEO

 * @func DWORD | videoUpdate | This function directs a channel to
 *   repaint the display.  It applies only to VIDEO_EXTERNALOUT channels.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm HWND | hWnd | Specifies the handle of the window to be used
 *      by the channel for image display.

 * @parm HDC | hDC | Specifies a handle to a device context.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_UNSUPPORTED | Specified message is unsupported.
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.

 * @comm This message is normally sent
 *   whenever the client window receives a <m WM_MOVE>, <m WM_SIZE>,
 *   or <m WM_PAINT> message.
*/
DWORD WINAPI videoUpdate (HVIDEO hVideo, HWND hWnd, HDC hDC)
{
    if ((!hVideo) || (!hWnd) || (!hDC) )
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_UPDATE, (DWORD) hWnd, (DWORD) hDC);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoOpen | This function opens a channel on the
 *  specified video device.

 * @parm LPHVIDEO | lphvideo | Specifies a far pointer to a buffer
 *   used to return an <t HVIDEO> handle. The video capture driver
 *   uses this location to return
 *   a handle that uniquely identifies the opened video device channel.
 *   Use the returned handle to identify the device channel when
 *   calling other video functions.

 * @parm DWORD | dwDeviceID | Identifies the video device to open.
 *      The value of <p dwDeviceID> varies from zero to one less
 *      than the number of video capture devices installed in the system.

 * @parm DWORD | dwFlags | Specifies flags for opening the device.
 *      The following flags are defined:

 *   @flag VIDEO_EXTERNALIN| Specifies the channel is opened
 *         for external input. Typically, external input channels
 *      capture images into a frame buffer.

 *   @flag VIDEO_EXTERNALOUT| Specifies the channel is opened
 *      for external output. Typically, external output channels
 *      display images stored in a frame buffer on an auxilary monitor
 *      or overlay.

 *   @flag VIDEO_IN| Specifies the channel is opened
 *      for video input. Video input channels transfer images
 *      from a frame buffer to system memory buffers.

 *   @flag VIDEO_OUT| Specifies the channel is opened
 *      for video output. Video output channels transfer images
 *      from system memory buffers to a frame buffer.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the specified device ID is out of range.
 *   @flag DV_ERR_ALLOCATED | Indicates the specified resource is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.

 * @comm
 *   At a minimum, all capture drivers support a VIDEO_EXTERNALIN
 *   and a VIDEO_IN channel.
 *   Use <f videoGetNumDevs> to determine the number of video
 *   devices present in the system.

 * @xref <f videoClose>
*/
DWORD WINAPI videoOpen (LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags)
{
    TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];
    TCHAR szbuf[128];
    UINT w;
    VIDEO_OPEN_PARMS vop;       // Same as IC_OPEN struct!!!
    DWORD dwVersion = VIDEOAPIVERSION;

    if (IsBadWritePtr ((LPVOID) lphVideo, sizeof (HVIDEO)) )
        return DV_ERR_PARAM1;

    vop.dwSize = sizeof (VIDEO_OPEN_PARMS);
    vop.fccType = OPEN_TYPE_VCAP;       // "vcap"
    vop.fccComp = 0L;
    vop.dwVersion = VIDEOAPIVERSION;
    vop.dwFlags = dwFlags;      // In, Out, External In, External Out
    vop.dwError = DV_ERR_OK;

    w = (WORD)dwDeviceID;
    *lphVideo = NULL;

    if (!wTotalVideoDevs)   // trying to open without finding how many devs.
        videoGetNumDevs();

    if (!wTotalVideoDevs)              // No drivers installed
        return DV_ERR_BADINSTALL;

    if (w >= MAXVIDEODRIVERS)
        return DV_ERR_BADDEVICEID;

#ifndef NOTHUNKS

    // if it's one of the 32-bit indices then we must open it on the
    // 32-bit side or fail.


    if (w < wVideoDevs32) {
    return videoOpen32( lphVideo, dwDeviceID, dwFlags);
    } else {
    // make it a valid 16-bit index
    w -= wVideoDevs32;
    }
#endif // NOTHUNKS


    // ignore the fake MSVideo copied from [Drivers32]
    if (bVideo0Invalid) {
    w++;
    }


    lstrcpy(szKey, szVideo);
    szKey[(sizeof(szVideo)/sizeof(TCHAR)) - 1] = (TCHAR)0;
    if( w > 0 ) {
        szKey[(sizeof(szVideo)/sizeof(TCHAR))] = (TCHAR)0;
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR) TEXT('1' + (w-1) );  // driver ordinal
    }
    if (GetPrivateProfileString(szDrivers, szKey, szNull, szbuf, sizeof(szbuf)/sizeof(TCHAR), szSystemIni)) {

#ifdef THIS_IS_ANCIENT_CODE
        // Removed for VFW1.1
        // Only early Alpha 1.0 drivers required this...

        // Check driver version number by doing a configuration open...
        // Version 1 used LPARAM = dwFlags
        // Version 2 uses LPARAM = LPVIDEO_OPEN_PARMS

        if (hVideoTemp = OpenDriver(szKey, szDrivers, (LPARAM) NULL)) {
            HVIDEO hVideoTemp;

            // Version 1 drivers had the added bug of returning
            // the version from this message, instead of in
            // lParam1
            if (videoMessage (hVideoTemp, DVM_GETVIDEOAPIVER,
                        (LPARAM) (LPVOID) &dwVersion, 0L) == 1)
                dwVersion = 1;
            CloseDriver(hVideoTemp, 0L, 0L );
        }

        if (dwVersion == 1)
            *lphVideo = OpenDriver(szKey, szDrivers, dwFlags);
        else
#endif // THIS_IS_ANCIENT_CODE

        *lphVideo = OpenDriver(szKey, szDrivers, (LPARAM) (LPVOID) &vop);

    if( ! *lphVideo ) {
            if (vop.dwError)    // if driver returned an error code...
                return vop.dwError;
            else {
#ifdef WIN32
        if (GetFileAttributes(szbuf) == (DWORD) -1)
#else
                OFSTRUCT of;

                if (OpenFile (szbuf, &of, OF_EXIST) == HFILE_ERROR)
#endif
                    return (DV_ERR_BADINSTALL);
                else
                    return (DV_ERR_NOTDETECTED);
            }
    }
    } else {
        return( DV_ERR_BADINSTALL );
    }

    return DV_ERR_OK;

}


typedef struct tagVS_VERSION
{
    WORD wTotLen;
    WORD wValLen;
    TCHAR szSig[16];
    VS_FIXEDFILEINFO vffInfo;
} VS_VERSION;


// videoGetDriverDesc -

// private api to correctly thunk avicap's capGetDriverDescription.

// if the index is < wVideoDevs32, then thunk the call through to the
// 32-bit side, otherwise do it here by scanning [drivers].


DWORD WINAPI videoCapDriverDescAndVer (
        DWORD wDriverIndex,
        LPSTR lpszName, UINT cbName,
        LPSTR lpszVer, UINT cbVer)
{
    LPTSTR   lpVersion;
    UINT    wVersionLen;
    BOOL    bRetCode;
    TCHAR   szGetName[MAX_PATH];
    DWORD   dwVerInfoSize;
    DWORD   dwVerHnd;
    TCHAR   szBuf[MAX_PATH];
    BOOL    fGetName;
    BOOL    fGetVersion;

    static TCHAR szNull[]        = TEXT("");
    static TCHAR szVideo[]       = TEXT("msvideo");
    static TCHAR szSystemIni[]   = TEXT("system.ini");
    static TCHAR szDrivers[]     = TEXT("Drivers");
    static TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];

    static TCHAR szVer[]         = TEXT("ver.dll");
    static TCHAR szGetFileVerInfo[] = TEXT("GetFileVersionInfo");
    static TCHAR szGetFileVerInfoSize[] = TEXT("GetFileVersionInfoSize");
    static TCHAR szVerQueryValue[] = TEXT("VerQueryValue");

    typedef BOOL (WINAPI FAR *LPVERQUERYVALUE)(
        const void FAR* pBlock,
        LPCSTR lpSubBlock,
        void FAR* FAR* lplpBuffer,
        UINT FAR* lpuLen
        );

    typedef BOOL (WINAPI FAR *LPGETFILEVERSIONINFO)(
        LPCSTR lpstrFilename,
        DWORD dwHandle,
        DWORD dwLen,
        void FAR* lpData
        );

    typedef DWORD (WINAPI FAR *LPGETFILEVERSIONINFOSIZE)(
        LPCSTR lpstrFilename,
        DWORD FAR *lpdwHandle
        );

    LPGETFILEVERSIONINFOSIZE lpfnGetFileVersionInfoSize;
    LPGETFILEVERSIONINFO     lpfnGetFileVersionInfo;
    LPVERQUERYVALUE          lpfnVerQueryValue;

    HINSTANCE                hinstVer;


#ifndef NOTHUNKS

    // must have called videoGetNumDevs first before thunking
    if (!wTotalVideoDevs) {
    videoGetNumDevs();
    }


    if (wDriverIndex < wVideoDevs32) {
    return videoGetDriverDesc32(wDriverIndex, lpszName, (short) cbName,
            lpszVer, (short) cbVer);
    } else {
    wDriverIndex -= wVideoDevs32;
    if (bVideo0Invalid) {
        wDriverIndex++;
    }
    }
#endif

    //this is a 16-bit driver - search [Drivers] for it

    fGetName = lpszName != NULL && cbName != 0;
    fGetVersion = lpszVer != NULL && cbVer != 0;

    if (fGetName)
        lpszName[0] = TEXT('\0');
    if (fGetVersion)
        lpszVer [0] = TEXT('\0');

    lstrcpy(szKey, szVideo);
    szKey[sizeof(szVideo)/sizeof(TCHAR) - 1] = TEXT('\0');
    if( wDriverIndex > 0 ) {
        szKey[sizeof(szVideo)/sizeof(TCHAR)] = TEXT('\0');
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR)(TEXT('1') + (wDriverIndex-1) );  // driver ordinal
    }

    if (GetPrivateProfileString(szDrivers, szKey, szNull, szBuf, sizeof(szBuf)/sizeof(TCHAR), szSystemIni) < 2)
        return FALSE;

    // Copy in the driver name initially, just in case the driver
    // has omitted a description field.
    if (fGetName)
        lstrcpyn(lpszName, szBuf, cbName);


    hinstVer = LoadLibrary(szVer);
    if ( hinstVer == NULL) {
        return FALSE;
    }

    *(FARPROC *)&lpfnGetFileVersionInfoSize =
        GetProcAddress(hinstVer, szGetFileVerInfoSize);

    *(FARPROC *)&lpfnGetFileVersionInfo =
        GetProcAddress(hinstVer, szGetFileVerInfo);

    *(FARPROC *)&lpfnVerQueryValue =
        GetProcAddress(hinstVer, szVerQueryValue );

#if 0
    {
        char szBuffer[256];

        wsprintf( szBuffer, "hinstVer = %#X\r\n", hinstVer );
        OutputDebugString(szBuffer);

        wsprintf( szBuffer, "lpfnGetFileVersionInfoSize = %#X\r\n", lpfnGetFileVersionInfoSize );
        OutputDebugString(szBuffer);

        wsprintf( szBuffer, "lpfnGetFileVersionInfo = %#X\r\n", lpfnGetFileVersionInfo );
        OutputDebugString(szBuffer);

        wsprintf( szBuffer, "lpfnVerQueryValue = %#X\r\n", lpfnVerQueryValue );
        OutputDebugString(szBuffer);

    }
#endif

    // You must find the size first before getting any file info
    dwVerInfoSize =
        (*lpfnGetFileVersionInfoSize)(szBuf, &dwVerHnd);

    if (dwVerInfoSize) {
        LPTSTR   lpstrVffInfo;             // Pointer to block to hold info
        HANDLE  hMem;                     // handle to mem alloc'ed

        // Get a block big enough to hold version info
        hMem          = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
        lpstrVffInfo  = GlobalLock(hMem);

        // Get the File Version first
        if( (*lpfnGetFileVersionInfo)(szBuf, 0L, dwVerInfoSize, lpstrVffInfo)) {
             VS_VERSION FAR *pVerInfo = (VS_VERSION FAR *) lpstrVffInfo;

             // fill in the file version
             wsprintf(szBuf,
                      TEXT("Version:  %d.%d.%d.%d"),
                      HIWORD(pVerInfo->vffInfo.dwFileVersionMS),
                      LOWORD(pVerInfo->vffInfo.dwFileVersionMS),
                      HIWORD(pVerInfo->vffInfo.dwFileVersionLS),
                      LOWORD(pVerInfo->vffInfo.dwFileVersionLS));
             if (fGetVersion)
                lstrcpyn (lpszVer, szBuf, cbVer);
        }

        // Now try to get the FileDescription
        // First try this for the "Translation" entry, and then
        // try the American english translation.
        // Keep track of the string length for easy updating.
        // 040904E4 represents the language ID and the four
        // least significant digits represent the codepage for
        // which the data is formatted.  The language ID is
        // composed of two parts: the low ten bits represent
        // the major language and the high six bits represent
        // the sub language.

        lstrcpy(szGetName, TEXT("\\StringFileInfo\\040904E4\\FileDescription"));

        wVersionLen   = 0;
        lpVersion     = NULL;

        // Look for the corresponding string.
        bRetCode      =  (*lpfnVerQueryValue)((LPVOID)lpstrVffInfo,
                        (LPTSTR)szGetName,
                        (void FAR* FAR*)&lpVersion,
                        (UINT FAR *) &wVersionLen);

        if (fGetName && bRetCode && wVersionLen && lpVersion)
           lstrcpyn (lpszName, lpVersion, cbName);

        // Let go of the memory
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }

    FreeLibrary(hinstVer);

    return TRUE;


}



/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoClose | This function closes the specified video
 *   device channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *  If this function is successful, the handle is invalid
 *   after this call.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NONSPECIFIC | The driver failed to close the channel.

 * @comm If buffers have been sent with <f videoStreamAddBuffer> and
 *   they haven't been returned to the application,
 *   the close operation fails. You can use <f videoStreamReset> to mark all
 *   pending buffers as done.

 * @xref <f videoOpen> <f videoStreamInit> <f videoStreamFini> <f videoStreamReset>
*/
DWORD WINAPI videoClose (HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

#ifndef NOTHUNKS
    if (Is32bitHandle(hVideo)) {
        return videoClose32(Map32bitHandle(hVideo));
    }
#endif // NOTHUNKS
    return (CloseDriver(hVideo, 0L, 0L ) ? DV_ERR_OK : DV_ERR_NONSPECIFIC);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoConfigure | This function sets or retrieves
 *      the options for a configurable driver.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm UINT | msg  | Specifies the option to set or retrieve. The
 *       following options are defined:

 *   @flag DVM_PALETTE | Indicates a palette is being sent to the driver
 *         or retrieved from the driver.

 *   @flag DVM_PALETTERGB555 | Indicates an RGB555 palette is being
 *         sent to the driver.

 *   @flag DVM_FORMAT | Indicates format information is being sent to
 *         the driver or retrieved from the driver.

 * @parm DWORD | dwFlags | Specifies flags for configuring or
 *   interrogating the device driver. The following flags are defined:

 *   @flag VIDEO_CONFIGURE_SET | Indicates values are being sent to the driver.

 *   @flag VIDEO_CONFIGURE_GET | Indicates values are being obtained from the driver.

 *   @flag VIDEO_CONFIGURE_QUERY | Determines if the
 *      driver supports the option specified by <p msg>. This flag
 *      should be combined with either the VIDEO_CONFIGURE_SET or
 *      VIDEO_CONFIGURE_GET flag. If this flag is
 *      set, the <p lpData1>, <p dwSize1>, <p lpData2>, and <p dwSize2>
 *      parameters are ignored.

 *   @flag VIDEO_CONFIGURE_QUERYSIZE | Returns the size, in bytes,
 *      of the configuration option in <p lpdwReturn>. This flag is only valid if
 *      the VIDEO_CONFIGURE_GET flag is also set.

 *   @flag VIDEO_CONFIGURE_CURRENT | Requests the current value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_NOMINAL | Requests the nominal value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MIN | Requests the minimum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MAX | Get the maximum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.

 * @parm LPDWORD | lpdwReturn  | Points to a DWORD used for returning information
 *      from the driver.  If
 *      the VIDEO_CONFIGURE_QUERYSIZE flag is set, <p lpdwReturn> is
 *      filled with the size of the configuration option.

 * @parm LPVOID | lpData1  |Specifies a pointer to message specific data.

 * @parm DWORD | dwSize1  | Specifies the size, in bytes, of the <p lpData1>
 *       buffer.

 * @parm LPVOID | lpData2  | Specifies a pointer to message specific data.

 * @parm DWORD | dwSize2  | Specifies the size, in bytes, of the <p lpData2>
 *       buffer.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.

 * @xref <f videoOpen> <f videoMessage>

*/
DWORD WINAPI videoConfigure (HVIDEO hVideo, UINT msg, DWORD dwFlags,
        LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1,
                LPVOID lpData2, DWORD dwSize2)
{
    VIDEOCONFIGPARMS    vcp;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (lpData1)
        if (IsBadHugeReadPtr (lpData1, dwSize1))
            return DV_ERR_CONFIG1;

    if (lpData2)
        if (IsBadHugeReadPtr (lpData2, dwSize2))
            return DV_ERR_CONFIG2;

    if (dwFlags & VIDEO_CONFIGURE_QUERYSIZE) {
        if (!lpdwReturn)
            return DV_ERR_NONSPECIFIC;
        if (IsBadWritePtr (lpdwReturn, sizeof (DWORD)) )
            return DV_ERR_NONSPECIFIC;
    }

    vcp.lpdwReturn = lpdwReturn;
    vcp.lpData1 = lpData1;
    vcp.dwSize1 = dwSize1;
    vcp.lpData2 = lpData2;
    vcp.dwSize2 = dwSize2;

    return videoMessage(hVideo, msg, dwFlags,
        (DWORD)(LPVIDEOCONFIGPARMS)&vcp );
}



/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoConfigureStorage | This function saves or loads
 *         all configurable options for a channel.  Options
 *      can be saved and recalled for each application or each application
 *      instance.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm LPSTR | lpstrIdent  | Identifies the application or instance.
 *      Use an arbitrary string which uniquely identifies your application
 *      or instance.

 * @parm DWORD | dwFlags | Specifies any flags for the function. The following
 *   flags are defined:
 *   @flag VIDEO_CONFIGURE_GET | Requests that the values be loaded.
 *   @flag VIDEO_CONFIGURE_SET | Requests that the values be saved.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.

 * @comm The method used by a driver to save configuration options is
 *      device dependent.

 * @xref <f videoOpen>
*/
DWORD WINAPI videoConfigureStorage (HVIDEO hVideo,
            LPSTR lpstrIdent, DWORD dwFlags)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_CONFIGURESTORAGE,
        (DWORD)lpstrIdent, dwFlags);
}




/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoDialog | This function displays a channel-specific
 *     dialog box used to set configuration parameters.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm HWND | hWndParent | Specifies the parent window handle.

 * @parm DWORD | dwFlags | Specifies flags for the dialog box. The
 *   following flag is defined:
 *   @flag VIDEO_DLG_QUERY | If this flag is set, the driver immediately
 *         returns zero if it supplies a dialog box for the channel,
 *           or DV_ERR_NOTSUPPORTED if it does not.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.

 * @comm Typically, each dialog box displayed by this
 *      function lets the user select options appropriate for the channel.
 *      For example, a VIDEO_IN channel dialog box lets the user select
 *      the image dimensions and bit depth.

 * @xref <f videoOpen> <f videoConfigureStorage>
*/
DWORD WINAPI videoDialog (HVIDEO hVideo, HWND hWndParent, DWORD dwFlags)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if ((!hWndParent) || (!IsWindow (hWndParent)) )
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_DIALOG, (DWORD)hWndParent, dwFlags);
}





/*
 * @doc INTERNAL  VIDEO

 * @api DWORD | videoPrepareHeader | This function prepares the
 *    header and data
 *    by performing a <f GlobalPageLock>.

 * @rdesc Returns zero if the function was successful. Otherwise, it
 *   specifies an error number.
*/
DWORD WINAPI videoPrepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{
    if (!HugePageLock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR)))
        return DV_ERR_NOMEM;

    if (!HugePageLock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength)) {
        HugePageUnlock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR));
        return DV_ERR_NOMEM;
    }

    lpVideoHdr->dwFlags |= VHDR_PREPARED;

    return DV_ERR_OK;
}

/*
 * @doc INTERNAL  VIDEO

 * @api DWORD | videoUnprepareHeader | This function unprepares the header and
 *   data if the driver returns DV_ERR_NOTSUPPORTED.

 * @rdesc Currently always returns DV_ERR_OK.
*/
DWORD WINAPI videoUnprepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{

    HugePageUnlock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength);
    HugePageUnlock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR));

    lpVideoHdr->dwFlags &= ~VHDR_PREPARED;

    return DV_ERR_OK;
}


/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamAllocHdrAndBuffer | This function is used to allow
 *      drivers to optionally allocate video buffers.  Normally, the client
 *      application is responsible for allocating buffer memory, but devices
 *      which have on-board memory may optionally allocate headers and buffers
 *      using this function. Generally, this will avoid an additional data copy,
 *      resulting in faster capture rates.

 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.

 * @parm LPVIDEOHDR FAR * | plpvideoHdr | Specifies a pointer to the address of a
 *   <t VIDEOHDR> structure.  The driver saves the buffer address in this
 *   location, or NULL if it cannot allocate a buffer.

 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure
 *      and associated video buffer in bytes.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the driver does not have on-board memory.

 * @comm If the driver
 *   allocates buffers via this method, the <f videoStreamPrepareHeader> and
 *   <f videoStreamUnprepareHeader> functions must not be used.

 *   The buffer allocated must be accessible for DMA by the host.

 * @xref <f videoStreamFreeHdrAndBuffer>
*/
DWORD WINAPI videoStreamAllocHdrAndBuffer(HVIDEO hVideo,
        LPVIDEOHDR FAR * plpvideoHdr, DWORD dwSize)
{

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return(DV_ERR_NOTSUPPORTED);
#if 0
    if (IsBadWritePtr (plpvideoHdr, sizeof (VIDEOHDR *)) )
        return DV_ERR_PARAM1;

    *plpvideoHdr = NULL;        // Init to NULL ptr

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_ALLOCHDRANDBUFFER,
            (DWORD)plpvideoHdr, (DWORD)dwSize);

    if (*plpvideoHdr == NULL ||
                IsBadHugeWritePtr (*plpvideoHdr, dwSize)) {
        DebugErr(DBF_WARNING,"videoStreamAllocHdrAndBuffer: Allocation failed.");
        *plpvideoHdr = NULL;
        return wRet;
    }

    if (IsVideoHeaderPrepared(HVIDEO, *plpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamAllocHdrAndBuffer: header is already prepared.");
        return DV_ERR_OK;
    }

    (*plpvideoHdr)->dwFlags = 0;

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderPrepared(hVideo, *plpvideoHdr);

    return wRet;
#endif
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamFreeHdrAndBuffer | This function is used to free
 *      buffers allocated by the driver using the <f videoStreamAllocHdrAndBuffer>
 *      function.

 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.

 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a pointer to the
 *   <t VIDEOHDR> structure and associated buffer to be freed.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the driver does not have on-board memory.

 * @comm If the driver
 *   allocates buffers via this method, the <f videoStreamPrepareHeader> and
 *   <f videoStreamUnprepareHeader> functions must not be used.

 * @xref <f videoStreamAllocHdrAndBuffer>
*/

DWORD WINAPI videoStreamFreeHdrAndBuffer(HVIDEO hVideo,
        LPVIDEOHDR lpvideoHdr)
{

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return(DV_ERR_NOTSUPPORTED);
#if 0
    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamFreeHdrAndBuffer: buffer still in queue.");
        return DV_ERR_STILLPLAYING;
    }

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamFreeHdrAndBuffer: header is not prepared.");
    }

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_FREEHDRANDBUFFER,
            (DWORD)lpvideoHdr, (DWORD)0);

    if (wRet != DV_ERR_OK)
    {
        DebugErr(DBF_WARNING,"videoStreamFreeHdrAndBuffer: Error freeing buffer.");
    }

    return wRet;
#endif
}




/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamPrepareHeader | This function prepares a buffer
 *   for video streaming.

 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.

 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a pointer to a
 *   <t VIDEOHDR> structure identifying the buffer to be prepared.

 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.

 * @comm Use this function after <f videoStreamInit> or
 *   after <f videoStreamReset> to prepare the data buffers
 *   for streaming data.

 *   The <t VIDEOHDR> data structure and the data block pointed to by its
 *   <e VIDEOHDR.lpData> member must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared will have no effect
 *   and the function will return zero. Typically, this function is used
 *   to ensure that the buffer will be available for use at interrupt time.

 * @xref <f videoStreamUnprepareHeader>
*/
DWORD WINAPI videoStreamPrepareHeader(HVIDEO hVideo,
        LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (IsVideoHeaderPrepared(HVIDEO, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamPrepareHeader: header is already prepared.");
        return DV_ERR_OK;
    }

    lpvideoHdr->dwFlags = 0;

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_PREPAREHEADER,
            (DWORD)lpvideoHdr, (DWORD)dwSize);

#ifndef NOTHUNKS

    // 32-bit side can't do the locking but needs it locked

    if (wRet == DV_ERR_OK && Is32bitHandle(hVideo))
        wRet = videoPrepareHeader(lpvideoHdr, dwSize);
#endif // NOTHUNKS

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = videoPrepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderPrepared(hVideo, lpvideoHdr);

    return wRet;
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamUnprepareHeader | This function clears the
 *  preparation performed by <f videoStreamPrepareHeader>.

 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.

 * @parm LPVIDEOHDR | lpvideoHdr |  Specifies a pointer to a <t VIDEOHDR>
 *   structure identifying the data buffer to be unprepared.

 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates the structure identified by <p lpvideoHdr>
 *   is still in the queue.

 * @comm This function is the complementary function to <f videoStreamPrepareHeader>.
 *   You must call this function before freeing the data buffer with <f GlobalFree>.
 *   After passing a buffer to the device driver with <f videoStreamAddBuffer>, you
 *   must wait until the driver is finished with the buffer before calling
 *   <f videoStreamUnprepareHeader>. Unpreparing a buffer that has not been
 *   prepared or has been already unprepared has no effect,
 *   and the function returns zero.

 * @xref <f videoStreamPrepareHeader>
*/
DWORD WINAPI videoStreamUnprepareHeader(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamUnprepareHeader: buffer still in queue.");
        return DV_ERR_STILLPLAYING;
    }

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamUnprepareHeader: header is not prepared.");
        return DV_ERR_OK;
    }

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_UNPREPAREHEADER,
            (DWORD)lpvideoHdr, (DWORD)dwSize);

#ifndef NOTHUNKS

    // 32-bit side can't do the unlocking but needs it unlocked

    if (wRet == DV_ERR_OK && Is32bitHandle(hVideo))
        wRet = videoUnprepareHeader(lpvideoHdr, dwSize);
#endif // NOTHUNKS

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = videoUnprepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderUnprepared(hVideo, lpvideoHdr);

    return wRet;
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamAddBuffer | This function sends a buffer to a
 *   video-capture device. After the buffer is filled by the device,
 *   the device sends it back to the application.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a far pointer to a <t VIDEOHDR>
 *   structure that identifies the buffer.

 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_UNPREPARED | Indicates the <p lpvideoHdr> structure hasn't been prepared.
 *   @flag DV_ERR_STILLPLAYING | Indicates a buffer is still in the queue.
 *   @flag DV_ERR_PARAM1 | The <p lpvideoHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.

 * @comm The data buffer must be prepared with <f videoStreamPrepareHeader>
 *   before it is passed to <f videoStreamAddBuffer>. The <t VIDEOHDR> data
 *   structure and the data buffer referenced by its <e VIDEOHDR.lpData>
 *   member must be allocated with <f GlobalAlloc> using the GMEM_MOVEABLE
 *   and GMEM_SHARE flags, and locked with <f GlobalLock>. Set the
 *   <e VIDEOHDR.dwBufferLength> member to the size of the header.

 * @xref <f videoStreamPrepareHeader>
*/
DWORD WINAPI videoStreamAddBuffer(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer not prepared.");
        return DV_ERR_UNPREPARED;
    }

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer already in queue.");
        return DV_ERR_STILLPLAYING;
    }

    return (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_ADDBUFFER, (DWORD)lpvideoHdr, (DWORD)dwSize);
}



/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamStop | This function stops streaming on a video channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.

 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 * @comm If there are any buffers in the queue, the current buffer will be
 *   marked as done (the <e VIDEOHDR.dwBytesRecorded> member in
 *   the <t VIDEOHDR> header will contain the actual length of data), but any
 *   empty buffers in the queue will remain there. Calling this
 *   function when the channel is not started has no effect, and the
 *   function returns zero.

 * @xref <f videoStreamStart> <f videoStreamReset>
*/
DWORD WINAPI videoStreamStop(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage((HVIDEO)hVideo, DVM_STREAM_STOP, 0L, 0L);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamReset | This function stops streaming
 *         on the specified video device channel and resets the current position
 *      to zero.  All pending buffers are marked as done and
 *      are returned to the application.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:

 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.

 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.

 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>

DWORD WINAPI videoStreamReset(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage((HVIDEO)hVideo, DVM_STREAM_RESET, 0L, 0L);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamGetPosition | This function retrieves the current
 *   position of the specified video device channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm LPMMTIME | lpInfo | Specifies a far pointer to an <t MMTIME>
 *   structure.

 * @parm DWORD | dwSize | Specifies the size of the <t MMTIME> structure in bytes.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:

 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.

 * @comm Before using <f videoStreamGetPosition>, set the
 *   <e MMTIME.wType> member of the <t MMTIME> structure to indicate
 *   the time format desired. After
 *   <f videoStreamGetPosition> returns, check the <e MMTIME.wType>
 *   member to  determine if the your time format is supported. If
 *   not, <e MMTIME.wType> specifies an alternate format.
 *   Video capture drivers typically provide the millisecond time
 *   format.

 *   The position is set to zero when streaming is started with
 *   <f videoStreamStart>.
*/
DWORD WINAPI videoStreamGetPosition(HVIDEO hVideo, LPMMTIME lpInfo, DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpInfo, sizeof (MMTIME)) )
        return DV_ERR_PARAM1;

    return videoMessage(hVideo, DVM_STREAM_GETPOSITION,
            (DWORD)lpInfo, (DWORD)dwSize);
}



/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamInit | This function initializes a video
 *     device channel for streaming.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm DWORD | dwMicroSecPerFrame | Specifies the number of microseconds
 *     between frames.

 * @parm DWORD | dwCallback | Specifies the address of a callback
 *   function or a handle to a window called during video
 *   streaming. The callback function or window processes
 *  messages related to the progress of streaming.

 * @parm DWORD | dwCallbackInstance | Specifies user
 *  instance data passed to the callback function. This parameter is not
 *  used with window callbacks.

 * @parm DWORD | dwFlags | Specifies flags for opening the device channel.
 *   The following flags are defined:
 *   @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      a window handle.
 *   @flag CALLBACK_FUNCTION | If this flag is specified, <p dwCallback> is
 *      a callback procedure address.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the device ID specified in
 *         <p hVideo> is not valid.
 *   @flag DV_ERR_ALLOCATED | Indicates the resource specified is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.

 * @comm If a window or function is chosen to receive callback information, the following
 *   messages are sent to it to indicate the
 *   progress of video input:

 *   <m MM_DRVM_OPEN> is sent at the time of <f videoStreamInit>

 *   <m MM_DRVM_CLOSE> is sent at the time of <f videoStreamFini>

 *   <m MM_DRVM_DATA> is sent when a buffer of image data is available

 *   <m MM_DRVM_ERROR> is sent when an error occurs

 *   Callback functions must reside in a DLL.
 *   You do not have to use <f MakeProcInstance> to get
 *   a procedure-instance address for the callback function.

 * @cb void CALLBACK | videoFunc | <f videoFunc> is a placeholder for an
 *   application-supplied function name. The actual name must be exported by
 *   including it in an EXPORTS statement in the DLL's module-definition file.
 *   This is used only when a callback function is specified in
 *   <f videoStreamInit>.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel
 *   associated with the callback.

 * @parm DWORD | wMsg | Specifies the <m MM_DRVM_> messages. Messages indicate
 *       errors and when image data is available. For information on
 *       these messages, see <f videoStreamInit>.

 * @parm DWORD | dwInstance | Specifies the user instance
 *   data specified with <f videoStreamInit>.

 * @parm DWORD | dwParam1 | Specifies a parameter for the message.

 * @parm DWORD | dwParam2 | Specifies a parameter for the message.

 * @comm Because the callback is accessed at interrupt time, it must reside
 *   in a DLL and its code segment must be specified as FIXED in the
 *   module-definition file for the DLL. Any data the callback accesses
 *   must be in a FIXED data segment as well. The callback may not make any
 *   system calls except for <f PostMessage>, <f timeGetSystemTime>,
 *   <f timeGetTime>, <f timeSetEvent>, <f timeKillEvent>,
 *   <f midiOutShortMsg>, <f midiOutLongMsg>, and <f OutputDebugStr>.

 * @xref <f videoOpen> <f videoStreamFini> <f videoClose>
*/
DWORD WINAPI videoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD dwCallback,
              DWORD dwCallbackInst, DWORD dwFlags)
{
    VIDEO_STREAM_INIT_PARMS vsip;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) ) {
        if (IsBadCodePtr ((FARPROC) dwCallback) )
            return DV_ERR_PARAM2;
        if (!dwCallbackInst)
            return DV_ERR_PARAM2;
    }

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) ) {
        if (!IsWindow((HWND) LOWORD (dwCallback)) )
            return DV_ERR_PARAM2;
    }

    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = dwCallback;
    vsip.dwCallbackInst = dwCallbackInst;
    vsip.dwFlags = dwFlags;
    vsip.hVideo = (DWORD)hVideo;

    return videoMessage(hVideo, DVM_STREAM_INIT,
                (DWORD) (LPVIDEO_STREAM_INIT_PARMS) &vsip,
                (DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamFini | This function terminates streaming
 *     from the specified device channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates there are still buffers in the queue.

 * @comm If there are buffers that have been sent with
 *   <f videoStreamAddBuffer> that haven't been returned to the application,
 *   this operation will fail. Use <f videoStreamReset> to return all
 *   pending buffers.

 *   Each call to <f videoStreamInit> must be matched with a call to
 *   <f videoStreamFini>.

 *   For VIDEO_EXTERNALIN channels, this function is used to
 *   halt capturing of data to the frame buffer.

 *   For VIDEO_EXTERNALOUT channels supporting overlay,
 *   this function is used to disable the overlay.

 * @xref <f videoStreamInit>
*/
DWORD WINAPI videoStreamFini(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_STREAM_FINI, 0L, 0L);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamStart | This function starts streaming on the
 *   specified video device channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.

 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.

 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>

DWORD WINAPI videoStreamStart(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_STREAM_START, 0L, 0L);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoStreamGetError | This function returns the error
 *   most recently encountered.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.

 * @parm LPDWORD | lpdwErrorID | Specifies a far pointer to the <t DWORD>
 *      used to return the error ID.

 * @parm LPDWORD | lpdwErrorValue | Specifies a far pointer to the <t DWORD>
 *      used to return the number of frames skipped.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.

 * @comm While streaming video data, a capture
 *      driver can fill buffers faster than the client application can
 *      save the buffers to disk.  In this case, the
 *      DV_ERR_NO_BUFFERS error is returned in <p lpdwErrorID>
 *      and <p lpdwErrorValue> contains a count of the number of
 *      frames missed.  After
 *      receiving this message and returning the error status, a driver
 *      should reset its internal error flag to DV_ERR_OK and
 *      the count of missed frames to zero.

 *      Applications should send this message frequently during capture
 *      since some drivers which do not have access to interrupts use
 *      this message to trigger buffer processing.

 * @xref <f videoOpen>

DWORD WINAPI videoStreamGetError(HVIDEO hVideo, LPDWORD lpdwError,
        LPDWORD lpdwFramesSkipped)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpdwError, sizeof (DWORD)) )
        return DV_ERR_PARAM1;

    if (IsBadWritePtr (lpdwFramesSkipped, sizeof (DWORD)) )
        return DV_ERR_PARAM2;

    return videoMessage(hVideo, DVM_STREAM_GETERROR, (DWORD) lpdwError,
        (DWORD) lpdwFramesSkipped);
}

/*
 * @doc EXTERNAL  VIDEO

 * @api DWORD | videoFrame | This function transfers a single frame
 *   to or from a video device channel.

 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *      The channel must be of type VIDEO_IN or VIDEO_OUT.

 * @parm LPVIDEOHDR | lpVHdr | Specifies a far pointer to an <t VIDEOHDR>
 *      structure.

 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_PARAM1 | The <p lpVDHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.

 * @comm Use this function with a VIDEO_IN channel to transfer a single
 *      image from the frame buffer.
 *      Use this function with a VIDEO_OUT channel to transfer a single
 *      image to the frame buffer.

 * @xref <f videoOpen>

DWORD WINAPI videoFrame (HVIDEO hVideo, LPVIDEOHDR lpVHdr)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (!lpVHdr)
        return DV_ERR_PARAM1;

    if (IsBadWritePtr (lpVHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    return videoMessage(hVideo, DVM_FRAME, (DWORD) lpVHdr,
                        sizeof(VIDEOHDR));
}

/*
* @doc INTERNAL VIDEO

* @api void | videoCleanup | clean up video stuff
*   called in MSVIDEOs WEP()

*/
void FAR PASCAL videoCleanup(HTASK hTask)
{
#ifndef NOTHUNKS
    LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2);
    // Special internal message to give the 32 bit side a chance to clean
    // up -- which it may or may not need
    videoMessage32(0, DVM_START-1, (DWORD)hTask, 0);
    ICSendMessage32(0, DRV_USER-1, (DWORD)hTask, 0);
    return;
#endif
}

