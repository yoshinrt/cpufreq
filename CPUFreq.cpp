/*****************************************************************************

		CPUFreq --- read tsc and display CPU frequency	v1.02
		Copyright(C) 1998 by Deyu Deyu Software


1999/02/28	1MHz = 996KHz Ç≈åvéZÇµÇƒÅC400MHz ë‰ÇÃÉ}ÉVÉìÇ…ëŒâû
2002/11/25	è„ÇÃèCê≥îpé~(äæ)
			1G over ÇÃ CPU Ç…ëŒâû
			2åÖà»â∫ÇÃÇ∆Ç´ÇÕì™ÇÃêîéöÇãÛîíÇ…ÇµÇΩ
2002/11/27	XP Ç≈ÉAÉCÉRÉìÉIÅ[É_Å[Ç™ãtÇ…Ç»ÇÈÇÃÇèCê≥

*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "dds.h"

/*** macros *****************************************************************/

#define	clr( reg )	xor	reg, reg
#define rdtsc		{ __asm _emit 0x0F __asm _emit 0x31 }

/*** constant definitions ***************************************************/

#define INTERVAL_INIT	1000
#define INTERVAL_NML	DebugParam( 1000, 1000 )

#define	KHZ_PAR_MHZ		1000 /* KHz = 1MHz */
#define DEFAULT_COLOR	RGB( 0, 255, 255 )

enum {
	EVENT_INIT = 1,
	EVENT_NML
};

enum {
	ICONID_H,
	ICONID_L
};

enum {
	ICONNUM_MHZ	= 10,
	ICONNUM_SPC
};

/*** gloval var & protorype definitions *************************************/

char	szWinClassName[] = "CPUFreqClass";
HINSTANCE	hInst;

HDC		hdcBase,
		hdcDigits;

HBITMAP	hbmColor,
		hbmMask,
		hbmDigits;

COLORREF	crDigit = DEFAULT_COLOR;

NOTIFYICONDATA	nid[ 2 ];

BOOL	bXChgIconOrder;

/*** read tsc ***************************************************************/

#pragma warning( push )
#pragma warning( disable : 4035 )

UINT __cdecl CheckFreq( void ){
	static UINT		uPrevCount[2];
	
	static DWORD	dwPrevTime;
	DWORD			dwCurTime,
					dwDTime;
	
	dwDTime =(( dwCurTime = GetTickCount())- dwPrevTime );
	dwPrevTime = dwCurTime;
	
	__asm{
		rdtsc
		mov		ebx, eax						; save the u64CurTime
		mov		ecx, edx						;
		sub		eax, uPrevCount[0]				; edx:eax -= u64PrevCount
		sbb		edx, uPrevCount[ TYPE UINT ]	;
		mov		uPrevCount[0], ebx				; u64PrevCount = u64CurCount
		mov		uPrevCount[ TYPE UINT ], ecx	;
		cmp		dwDTime, edx					;
		jbe		ChkFreq1						; if dwDTime <= edx overflow
		div		dwDTime							; eax = u64DCount / uDTime
		jmp		ChkFreq2						; exit
	  ChkFreq1:
		clr		( eax )							; eax = 0;
	  ChkFreq2:
	}
}

#pragma warning( pop )

/*** create 2 digits icon bitmap ********************************************/

void CreateDigitIcon(
	NOTIFYICONDATA	&nid,
	UINT	uNum1,
	UINT	uNum2,
	char	*szTips,
	BOOL	bChangeIcon ){
	
	ICONINFO	ii;
	HICON		hIconOld = nid.hIcon;
	
	/* init IconInfo */
	ii.fIcon	= TRUE;
	ii.hbmMask	= hbmMask;
	ii.hbmColor	= hbmColor;
	
	if( bChangeIcon || !hIconOld ){
		/* make mask bitmap */
		SelectObject( hdcBase, hbmMask );
		BitBlt( hdcBase, 0, 0, 8, 16, hdcDigits, 8 * uNum1, 0, SRCCOPY );
		BitBlt( hdcBase, 8, 0, 8, 16, hdcDigits, 8 * uNum2, 0, SRCCOPY );
		
		/* make color bitmap */
		SelectObject( hdcBase, hbmColor );
		BitBlt( hdcBase, 0, 0, 8, 16, hdcDigits, 8 * uNum1, 0, SRCCOPY );
		BitBlt( hdcBase, 8, 0, 8, 16, hdcDigits, 8 * uNum2, 0, SRCCOPY );
		
		/* make icon */
		nid.hIcon = CreateIconIndirect( &ii );
	}
	
	strcpy( nid.szTip, szTips );
	Shell_NotifyIcon( hIconOld ? NIM_MODIFY : NIM_ADD, &nid );
	
	if( bChangeIcon && hIconOld ) DestroyIcon( hIconOld );
}

/*** set frequency into task tray *******************************************/

void SetFreqDigit( UINT uNum, BOOL bChangeIcon ){
	
	char	szBuf[ 32 ];
	static UINT
			uICONChangeID0,
			uICONChangeID1;
	
	UINT	uDigit0,
			uDigit1,
			uDigit2,
			uDigit3,
			uICONChangeIDNext0,
			uICONChangeIDNext1;
	
	sprintf( szBuf, "%d.%03dMHz", uNum / 1000, uNum % 1000 );
	
	UINT uFreq =( uNum + 500 )/ KHZ_PAR_MHZ;
	
	if( uFreq >= 1000 ){
		uDigit0 = uFreq / 1000;
		uDigit1 = uFreq /  100 % 10;
		uDigit2 = uFreq /   10 % 10;
		uDigit3 = uFreq        % 10;
		
	}else{	
		uDigit0 = uFreq / 100;
		uDigit1 = uFreq /  10 % 10;
		uDigit2 = uFreq /   1 % 10;
		uDigit3 = ICONNUM_MHZ;
	}
	
	if( uFreq < 100 ) uDigit0 = ICONNUM_SPC;
	if( uFreq <  10 ) uDigit1 = ICONNUM_SPC;
	
	uICONChangeIDNext0 = ( uDigit0 << 8 ) | uDigit1;
	uICONChangeIDNext1 = ( uDigit2 << 8 ) | uDigit3;
	
	if( bXChgIconOrder ){
		CreateDigitIcon(
			nid[ 1 ],
			uDigit2,
			uDigit3,
			szBuf,
			uICONChangeID1 != uICONChangeIDNext1 || bChangeIcon
		);
		
		CreateDigitIcon(
			nid[ 0 ],
			uDigit0,
			uDigit1,
			szBuf,
			uICONChangeID0 != uICONChangeIDNext0 || bChangeIcon
		);
	}else{
		CreateDigitIcon(
			nid[ 0 ],
			uDigit0,
			uDigit1,
			szBuf,
			uICONChangeID0 != uICONChangeIDNext0 || bChangeIcon
		);
		
		CreateDigitIcon(
			nid[ 1 ],
			uDigit2,
			uDigit3,
			szBuf,
			uICONChangeID1 != uICONChangeIDNext1 || bChangeIcon
		);
	}
	
	uICONChangeID0 = uICONChangeIDNext0;
	uICONChangeID1 = uICONChangeIDNext1;
}

/*** registory management ***************************************************/

COLORREF RegColorSetting( COLORREF color ){
	
	HKEY	hKey;
	static char	szKeyName[]   = REGISTORY_KEY_DDS "CPUFreq",
				szValueName[] = "crDigit";
	
	COLORREF	crData    = color;
	DWORD		dwBufSize = sizeof( crData );
	
	if( crDigit == color ){				/*** get setting ********************/
		if( RegOpenKeyEx(
			HKEY_CURRENT_USER, szKeyName, 0, KEY_READ, &hKey
		)== ERROR_SUCCESS ){
			
			RegQueryValueEx( hKey, szValueName, NULL, NULL,
				( LPBYTE )&crData, &dwBufSize );
			RegCloseKey( hKey );
		}
	}else{								/*** set setting ********************/
		if( RegCreateKeyEx(
			HKEY_CURRENT_USER, szKeyName, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL
		)== ERROR_SUCCESS ){
			
			RegSetValueEx( hKey, szValueName, 0, REG_DWORD, ( CONST BYTE *)&crData, sizeof( crData ));
			RegCloseKey( hKey );
		}
	}
	return( crData );
}

/*** Get os verion **********************************************************/

DWORD IsWindowsXP( void ){
	OSVERSIONINFO	VerInfo;
	
	VerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	GetVersionEx( &VerInfo );
	
	return( VerInfo.dwMajorVersion * 100 + VerInfo.dwMinorVersion >= 501 );
}

/*** window procedure *******************************************************/

LRESULT CALLBACK WindowProc(
	HWND	hWnd,
	UINT	Message,
	WPARAM	wParam,
	LPARAM	lParam ){
	
	HDC		hdcClient;
	
	static UINT		uFreq;
	static HMENU	hMenu,
					hMenuPopup;
	
	switch( Message ){
	  case WM_CREATE:	/*** initialize variables ***************************/
		
		/* make compatible memory DC */
		hdcClient	= GetDC( hWnd );
		hdcBase		= CreateCompatibleDC( hdcClient );
		hdcDigits	= CreateCompatibleDC( hdcClient );
		
		/* load bitmaps */
		hbmDigits	= LoadBitmap( hInst, MAKEINTRESOURCE( IDB_DIGITS ));
		hbmColor	= CreateCompatibleBitmap( hdcClient, 16, 16 );
		hbmMask		= CreateBitmap( 16, 16, 1, 1, NULL );
		
		ReleaseDC( hWnd, hdcClient );
		SelectObject( hdcDigits, hbmDigits );
		SetTextColor( hdcBase, crDigit );
		SetBkColor	( hdcBase, 0 );
		
		/* init shell notify icon */
		nid[ 0 ].cbSize	= sizeof( NOTIFYICONDATA );
		nid[ 0 ].hWnd	= hWnd;
		nid[ 0 ].uID	= ICONID_H;
		nid[ 0 ].uFlags	= NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid[ 0 ].uCallbackMessage	= WM_APP;
		nid[ 0 ].hIcon	= 0;
		
		nid[ 1 ].cbSize	= sizeof( NOTIFYICONDATA );
		nid[ 1 ].hWnd	= hWnd;
		nid[ 1 ].uID	= ICONID_L;
		nid[ 1 ].uFlags	= NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid[ 1 ].uCallbackMessage	= WM_APP;
		nid[ 1 ].hIcon	= 0;
		
		/* initialize menu */
		hMenu = LoadMenu( hInst, MAKEINTRESOURCE( IDR_MENU ));
		hMenuPopup = GetSubMenu( hMenu, 0 );
		
	  Case WM_TIMER:	/*** timer interval *********************************/
		
		if( wParam == EVENT_INIT ){
			KillTimer( hWnd, EVENT_INIT );
			SetTimer( hWnd, EVENT_NML, INTERVAL_NML, NULL );
		}
		
		if( uFreq = CheckFreq()) SetFreqDigit( uFreq, FALSE );
		
	  Case WM_APP:	/*** tasktray icon clicked ******************************/
		
		switch( lParam ){
		  case WM_LBUTTONDOWN:
		  case WM_RBUTTONDOWN:
			
			POINT	ptCur;
			
			GetCursorPos( &ptCur );
			SetForegroundWindow( hWnd );
			TrackPopupMenuEx( hMenuPopup, 0, ptCur.x, ptCur.y, hWnd, NULL );
			PostMessage( hWnd, WM_APP, 0, 0 );
			
		  Case WM_LBUTTONDBLCLK:
			PostMessage( hWnd, WM_COMMAND, ID_MENU_SETTING, 0 );
		}
		
	  Case WM_COMMAND:	/*** menu is selected *******************************/
		
		switch( LOWORD( wParam )){
		  case ID_MENU_CLOSE:
			PostMessage( hWnd, WM_CLOSE, 0, 0 );
			
		  Case ID_MENU_SETTING:
			
			CHOOSECOLOR	cc;
			COLORREF	cr[ 16 ]={ 0 };
			
			cc.lStructSize	= sizeof( CHOOSECOLOR );
			cc.hwndOwner	= NULL;
			cc.rgbResult	= crDigit;
			cc.lpCustColors	= cr;
			cc.Flags		= CC_RGBINIT | CC_SOLIDCOLOR;
			
			if( ChooseColor( &cc )){
				SetTextColor( hdcBase, cc.rgbResult );
				RegColorSetting( cc.rgbResult );
				SetFreqDigit( uFreq, TRUE );
			}
		}
		
	  Case WM_DESTROY:	/*** quit program ***********************************/
		
		/* delete tasktray icon */
		Shell_NotifyIcon( NIM_DELETE, &nid[ 0 ]);
		Shell_NotifyIcon( NIM_DELETE, &nid[ 1 ]);
		DestroyIcon( nid[ 0 ].hIcon );
		DestroyIcon( nid[ 1 ].hIcon );
		
		/* delete DCs */
		DeleteDC( hdcBase );
		DeleteDC( hdcDigits );
		
		/* delete bitmaps */
		DeleteObject( hbmMask );
		DeleteObject( hbmColor );
		DeleteObject( hbmDigits );
		
		/* destroy menu */
		DestroyMenu( hMenuPopup );
		DestroyMenu( hMenu );
		
		PostQuitMessage( 0 );
		
	  Default:
		return( DefWindowProc( hWnd, Message, wParam, lParam ));
	}
	return( 0 );
}

/*** main procedure *********************************************************/

int WINAPI WinMain(
	HINSTANCE	hInst,
	HINSTANCE	hPrevInst,
	LPSTR		lpCmdLine,
	int			iCmdShow ){
	
	HWND		hWnd;
	MSG			Msg;
	WNDCLASS	wcl;
	
	::hInst = hInst;
	
	/* close if already executed */
	if( hWnd = FindWindow( szWinClassName, NULL )){
		PostMessage( hWnd, WM_CLOSE, 0, 0 );
		return( 0 );
	}
	
	/* get color setting */
	crDigit = RegColorSetting( crDigit );
	
	/* Need to change icon order? */
	bXChgIconOrder = IsWindowsXP();
	
	/* define a window class */
	wcl.hInstance		= hInst;
	wcl.lpszClassName	= szWinClassName;
	wcl.lpfnWndProc		= WindowProc;
	wcl.style			= 0;
	
	wcl.hIcon			= LoadIcon( NULL, IDI_APPLICATION );
	wcl.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wcl.lpszMenuName	= NULL;
	
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	
	wcl.hbrBackground	=( HBRUSH )GetStockObject( WHITE_BRUSH );
	
	/* register the window class */
	if( !RegisterClass( &wcl )) return( 1 );
	
	hWnd = CreateWindow(
		szWinClassName,
		NULL,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,		// x
		CW_USEDEFAULT,		// y
		0,					// w
		0,					// h
		HWND_DESKTOP,
		NULL,
		hInst,
		NULL
	);
	
	/* display the window */
	DebugCmd( ShowWindow( hWnd, iCmdShow ); )
	DebugCmd( UpdateWindow( hWnd ); )
	
	/* initialize */
	CheckFreq();
	
	/* setup a timer */
	SetTimer( hWnd, EVENT_INIT, INTERVAL_INIT, NULL );
	
	/* create the message loop */
	while( GetMessage( &Msg, NULL, 0, 0 )){
		TranslateMessage( &Msg );
		DispatchMessage( &Msg );
	}
	return( Msg.wParam );
}