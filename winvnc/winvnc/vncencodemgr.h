//  Copyright (C) 2020 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2015 D. R. Commander. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

// vncEncodeMgr

// This class is used internally by vncClient to offload the handling of
// bitmap data encoding and translation.  Trying to avoid bloating the
// already rather bloaty vncClient class!

class vncEncodeMgr;

#if !defined(_WINVNC_VNCENCODEMGR)
#define _WINVNC_VNCENCODEMGR
#pragma once

// Includes

#include "vncencoder.h"
#include "vncencoderre.h"
#include "vncencodecorre.h"
#include "vncencodehext.h"
#include "vncencodezrle.h"
#ifdef _XZ
#include "vncEncodeXZ.h"
#endif
#include "vncEncodeZlib.h"
#include "vncEncodeZlibHex.h"
#include "vncEncodeTight.h"
#include "vncEncodeUltra.h"
#include "vncEncodeUltra2.h"
#include "vncbuffer.h"

// Mapping of coarse-grained to fine-grained quality levels, inherited from
// TigerVNC.  These map roughly to the compression ratios indicated, but only
// for a fairly arbitrary test case.  Further study on this is warranted.
// 9 = JPEG quality 100, no subsampling (ratio ~= 10:1)
//     [this should be lossless, except for round-off error]
// 8 = JPEG quality 92,  no subsampling (ratio ~= 20:1)
//     [this should be perceptually lossless, based on current research]
// 7 = JPEG quality 86,  no subsampling (ratio ~= 25:1)
// 6 = JPEG quality 79,  no subsampling (ratio ~= 30:1)
// 5 = JPEG quality 77,  4:2:2 subsampling (ratio ~= 40:1)
// 4 = JPEG quality 62,  4:2:2 subsampling (ratio ~= 50:1)
// 3 = JPEG quality 42,  4:2:2 subsampling (ratio ~= 60:1)
// 2 = JPEG quality 41,  4:2:0 subsampling (ratio ~= 70:1)
// 1 = JPEG quality 29,  4:2:0 subsampling (ratio ~= 80:1)
// 0 = JPEG quality 15,  4:2:0 subsampling (ratio ~= 100:1)

static const int coarsequal2finequal[10] =
{
	15, 29, 41, 42, 62, 77, 79, 86, 92, 100
};

static const subsamp_type coarsequal2subsamp[10] =
{
	SUBSAMP_4X, SUBSAMP_4X, SUBSAMP_4X, SUBSAMP_2X, SUBSAMP_2X, SUBSAMP_2X,
	SUBSAMP_1X, SUBSAMP_1X, SUBSAMP_1X, SUBSAMP_1X
};

//
// -=- Define the Encoding Manager interface
// 

class vncEncodeMgr
{
public:
	// Create/Destroy methods
	inline vncEncodeMgr();
	inline ~vncEncodeMgr();

	inline void SetBuffer(vncBuffer *buffer);

	// BUFFER INFO
	inline rfb::Rect GetSize();
	inline BYTE *GetClientBuffer();
	inline UINT GetClientBuffSize() {return m_clientbuffsize;}; // sf@2002
	inline BOOL GetPalette(RGBQUAD *quadbuff, UINT ncolours);

	inline omni_mutex& GetUpdateLock() {return m_buffer->m_desktop->GetUpdateLock();};

	// ENCODING & TRANSLATION
	inline UINT GetNumCodedRects(const rfb::Rect &rect);
	inline BOOL SetEncoding(CARD32 encoding,BOOL reinitialize);
	//inline UINT EncodeRect(const rfb::Rect &rect);
	inline UINT EncodeRect(const rfb::Rect &rect,VSocket *outconn);
	inline UINT EncodeBulkRects(const rfb::RectVector &allRects, int nScale, VSocket *outconn);


	// Tight - CONFIGURING ENCODER
	inline void SetCompressLevel(int level);
	inline void SetQualityLevel(int level);
	inline void SetFineQualityLevel(int level);
	inline void SetSubsampling(subsamp_type subsamp);
	inline void EnableLastRect(BOOL enable);
	inline BOOL IsLastRectEnabled() { return m_use_lastrect; }

	// CURSOR HANDLING
	inline void EnableXCursor(BOOL enable);
	inline void EnableRichCursor(BOOL enable);
	inline BOOL SetServerFormat();
	inline BOOL SetClientFormat(rfbPixelFormat &format);
	inline rfbPixelFormat GetClientFormat() {return m_clientformat;};
	inline void SetBufferOffset(int x,int y);
	// CURSOR HANDLING
	inline BOOL IsCursorUpdatePending();
	inline BOOL WasCursorUpdatePending();
	inline BOOL SendCursorShape(VSocket *outConn);
	inline BOOL SendEmptyCursorShape(VSocket *outConn);
	inline BOOL IsXCursorSupported();
	// CLIENT OPTIONS
	inline void AvailableQueueEnabled(BOOL enable){m_use_queue = enable;};
	inline void AvailableZRLE(BOOL enable){m_use_zrle = enable;};
#ifdef _XZ
	inline void AvailableXZ(BOOL enable){m_use_xz = enable;};
#endif
	inline void AvailableTight(BOOL enable){m_use_tight = enable;};
	inline void EnableQueuing(BOOL enable){m_fEnableQueuing = enable;};
	inline BOOL IsMouseWheelTight();
	// CACHE HANDLING
	inline void EnableCache(BOOL enabled);
	inline BOOL IsCacheEnabled();

	// sf@2005 - Grey palette
	inline void EnableGreyPalette(BOOL enable);

	// QUEUE ZLIBXOR
	inline void LastRect(VSocket *outConn);

	inline bool IsSlowEncoding() {return (m_encoding == rfbEncodingZYWRLE || m_encoding == rfbEncodingZRLE || m_encoding == rfbEncodingTight ||m_encoding == rfbEncodingZlib 
#ifdef _XZ
		|| m_encoding == rfbEncodingXZYW || m_encoding == rfbEncodingXZ
#endif
		);};
#ifdef _XZ
	inline bool IsBulkRectEncoding() {return (m_encoding == rfbEncodingXZ || m_encoding == rfbEncodingXZYW);};
#endif
	inline bool IsUltraEncoding() {return (m_encoding == rfbEncodingUltra || m_encoding == rfbEncodingUltra2);};
	inline bool IsUltra2Encoding() {return (m_encoding == rfbEncodingUltra2);};
	inline bool IsEncoderSet() { return ((m_encoder != NULL) && (m_encoding != rfbEncodingRaw)); };


protected:

	// Routine used internally to ensure the client buffer is OK
	inline BOOL CheckBuffer();

	// Pixel buffers and access to display buffer
	BYTE		*m_clientbuff;
	UINT		m_clientbuffsize;

	// Pixel formats, translation and encoding
	rfbPixelFormat	m_clientformat = {};
	BOOL			m_clientfmtset;
	rfbTranslateFnType	m_transfunc;
	vncEncoder*		m_encoder;
	vncEncoder*		zrleEncoder;
	bool			xz_encoder_in_use;
	vncEncoder*		m_hold_xz_encoder;
	bool			zlib_encoder_in_use;
	vncEncoder*		m_hold_zlib_encoder;
	bool			ultra_encoder_in_use;
	vncEncoder*		m_hold_ultra_encoder;
	vncEncoder*		m_hold_ultra2_encoder;
	bool			zlibhex_encoder_in_use;
	vncEncoder*		m_hold_zlibhex_encoder;
	bool			tight_encoder_in_use;
	vncEncoder*		m_hold_tight_encoder;

	// Tight 
	int				m_compresslevel;
	int				m_qualitylevel;
	int				m_finequalitylevel;
	subsamp_type	m_subsampling;
	BOOL			m_use_lastrect;

	// Tight - CURSOR HANDLING
	BOOL			m_use_xcursor;
	BOOL			m_use_richcursor;

	// Client detection
	// if tight->tight zrle->zrle both=ultra
	BOOL			m_use_queue;
	BOOL			m_use_zrle;
	BOOL			m_use_xz;
	BOOL			m_use_tight;
	BOOL			m_fEnableQueuing;

	// cache handling
	BOOL	m_cache_enabled;
	int		monitor_Offsetx;
	int		monitor_Offsety;

public:
	vncBuffer	*m_buffer;
	rfbServerInitMsg	m_scrinfo;
	CARD32		m_encoding;
	bool			ultra2_encoder_in_use;
};

//
// -=- Constructor/destructor
//

inline vncEncodeMgr::vncEncodeMgr()
{
	m_encoding = rfbEncodingRaw;
	m_transfunc = NULL;

	zrleEncoder=NULL;
	m_encoder = NULL;
	m_buffer=NULL;

	m_clientbuff = NULL;
	m_clientbuffsize = 0;
/*	m_clientbackbuff = NULL;
	m_clientbackbuffsize = 0;*/

	m_clientfmtset = FALSE;
	zlib_encoder_in_use = false;
	m_hold_zlib_encoder = NULL;
    ultra_encoder_in_use = false;
	ultra2_encoder_in_use = false;
	m_hold_ultra_encoder = NULL;
	m_hold_ultra2_encoder = NULL;
	zlibhex_encoder_in_use = false;
	m_hold_zlibhex_encoder = NULL;
	tight_encoder_in_use = false;
	m_hold_tight_encoder = NULL;
	xz_encoder_in_use = false;
	m_hold_xz_encoder = NULL;

	// Tight 
	m_compresslevel = 6;
	m_qualitylevel = -1;
	m_finequalitylevel = -1;
	m_subsampling = SUBSAMP_2X;
	m_use_lastrect = FALSE;
	// Tight CURSOR HANDLING
	m_use_xcursor = FALSE;
	m_use_richcursor = FALSE;
//	m_hcursor = NULL;

	monitor_Offsetx = 0;
	monitor_Offsety = 0;

}

inline vncEncodeMgr::~vncEncodeMgr()
{
	if (zrleEncoder && zrleEncoder != m_encoder)
		delete zrleEncoder;

	if (m_hold_zlib_encoder != NULL && m_hold_zlib_encoder != m_encoder) {
		delete m_hold_zlib_encoder;
		m_hold_zlib_encoder = NULL;
	}

	if (m_hold_ultra_encoder != NULL && m_hold_ultra_encoder != m_encoder) {
		delete m_hold_ultra_encoder;
		m_hold_ultra_encoder = NULL;
	}

	if (m_hold_ultra2_encoder != NULL && m_hold_ultra2_encoder != m_encoder) {
		delete m_hold_ultra2_encoder;
		m_hold_ultra2_encoder = NULL;
	}

	if (m_hold_zlibhex_encoder != NULL && m_hold_zlibhex_encoder != m_encoder) {
		delete m_hold_zlibhex_encoder;
		m_hold_zlibhex_encoder = NULL;
	}

	if (m_hold_tight_encoder != NULL && m_hold_tight_encoder != m_encoder)
	{
		// m_hold_tight_encoder->LogStats();
		delete m_hold_tight_encoder;
		m_hold_tight_encoder = NULL;
	}

	if (m_hold_xz_encoder != NULL && m_hold_xz_encoder != m_encoder)
	{
		delete m_hold_xz_encoder;
		m_hold_xz_encoder = NULL;
	}

	if (m_encoder != NULL)
	{
		delete m_encoder;
		m_encoder = NULL;
		m_hold_zlib_encoder = NULL;
		m_hold_ultra_encoder = NULL;
		m_hold_ultra2_encoder = NULL;
		m_hold_zlibhex_encoder = NULL;
		m_hold_tight_encoder = NULL;
		m_hold_xz_encoder = NULL;
	}
	if (m_clientbuff != NULL)
	{
		delete [] m_clientbuff;
		m_clientbuff = NULL;
	}
/*	if (m_clientbackbuff != NULL)
	{
		delete m_clientbackbuff;
		m_clientbackbuff = NULL;
	}*/
}

inline void
vncEncodeMgr::SetBuffer(vncBuffer *buffer)
{
	m_buffer=buffer;
	CheckBuffer();
}

//
// -=- Encoding of pixel data for transmission
//

inline BYTE *
vncEncodeMgr::GetClientBuffer()
{
	return m_clientbuff;
}

inline BOOL
vncEncodeMgr::GetPalette(RGBQUAD *quadlist, UINT ncolours)
{
	// Try to get the RGBQUAD data from the encoder
	// This will only work if the remote client is palette-based,
	// in which case the encoder will be storing RGBQUAD data
	if (m_encoder == NULL)
	{
		vnclog.Print(LL_INTWARN, VNCLOG("GetPalette called but no encoder set\n"));
		return FALSE;
	}

	// Now get the palette data
	return m_encoder->GetRemotePalette(quadlist, ncolours);
}

inline BOOL
vncEncodeMgr::CheckBuffer()
{
	// Get the screen format, in case it has changed
	m_buffer->m_desktop->FillDisplayInfo(&m_scrinfo);

	// If the client has not specified a pixel format then set one for it
	if (!m_clientfmtset) {
	    m_clientfmtset = TRUE;
	    m_clientformat = m_scrinfo.format;
	}

	// If the client has not selected an encoding then set one for it
	if ((m_encoder == NULL) && (!SetEncoding(rfbEncodingRaw,FALSE)))
		return FALSE;

	// Check the client buffer is sufficient
	const UINT clientbuffsize =
	    m_encoder->RequiredBuffSize(m_scrinfo.framebufferWidth,
									m_scrinfo.framebufferHeight);
	if (m_clientbuffsize != clientbuffsize)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("request client buffer[%u]\n"), clientbuffsize);
	    if (m_clientbuff != NULL)
	    {
			delete [] m_clientbuff;
			m_clientbuff = NULL;
	    }
	    m_clientbuffsize = 0;

	    m_clientbuff = new BYTE [clientbuffsize];

		memset(m_clientbuff, 0, clientbuffsize);
	    m_clientbuffsize = clientbuffsize;
	}

/*	// Check the client backing buffer matches the server back buffer
	const UINT backbuffsize = m_buffer->m_backbuffsize;
	if (m_clientbackbuffsize != backbuffsize)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("request client back buffer[%u]\n"), backbuffsize);
		if (m_clientbackbuff) {
			delete [] m_clientbackbuff;
			m_clientbackbuff = 0;
		}
		m_clientbackbuffsize = 0;

		m_clientbackbuff = new BYTE[backbuffsize];
		if (!m_clientbackbuff) {
			vnclog.Print(LL_INTERR, VNCLOG("unable to allocate client back buffer[%u]\n"), backbuffsize);
			return FALSE;
		}
		memset(m_clientbackbuff, 0, backbuffsize);
		m_clientbackbuffsize = backbuffsize;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("remote buffer=%u\n"), m_clientbuffsize);*/

	return TRUE;
}

// Set the encoding to use
inline BOOL
vncEncodeMgr::SetEncoding(CARD32 encoding,BOOL reinitialize)
{
	if (m_scrinfo.format.bitsPerPixel!=32 && encoding==rfbEncodingUltra2)
	{
		//This is not supported, jpeg require 32bit buffers
		//to avoif a server crash we switch to zrle encoding
		//hextile is better as u2 replacement
		encoding = rfbEncodingHextile;
	}

	if (encoding == m_encoding && m_encoder != NULL)
		return TRUE;

	if (reinitialize)
	{
		encoding=m_encoding;
		if (!m_buffer->CheckBuffer())
            return FALSE;
	}
	else m_encoding=encoding;

	// Delete the old encoder
	if (m_encoder != NULL)
	{
		if ( zlib_encoder_in_use )
		{
			m_hold_zlib_encoder = m_encoder;
		}
		else if ( ultra_encoder_in_use )
		{
			m_hold_ultra_encoder = m_encoder;
		}
		else if ( ultra2_encoder_in_use )
		{
			m_hold_ultra2_encoder = m_encoder;
		}
		else if ( zlibhex_encoder_in_use )
		{
			m_hold_zlibhex_encoder = m_encoder;
		}
		else if ( tight_encoder_in_use )
		{
			m_hold_tight_encoder = m_encoder;
		}
		else if ( xz_encoder_in_use )
		{
			m_hold_xz_encoder = m_encoder;
		}
		else
		{
		if (m_encoder != zrleEncoder)
			delete m_encoder;
		}
		m_encoder = NULL;
		
	}
	zlib_encoder_in_use = false;
	ultra_encoder_in_use = false;
	ultra2_encoder_in_use = false;
	zlibhex_encoder_in_use = false;
	tight_encoder_in_use = false;
	xz_encoder_in_use = false;
	// Returns FALSE if the desired encoding cannot be used
	switch(encoding)
	{

	case rfbEncodingRaw:

		vnclog.Print(LL_INTINFO, VNCLOG("raw encoder requested\n"));

		// Create a RAW encoder
		m_encoder = new vncEncoder;
		break;

	case rfbEncodingRRE:

		vnclog.Print(LL_INTINFO, VNCLOG("RRE encoder requested\n"));

		// Create a RRE encoder
		m_encoder = new vncEncodeRRE;
		break;

	case rfbEncodingCoRRE:

		vnclog.Print(LL_INTINFO, VNCLOG("CoRRE encoder requested\n"));

		// Create a CoRRE encoder
		m_encoder = new vncEncodeCoRRE;
		break;

	case rfbEncodingHextile:

		vnclog.Print(LL_INTINFO, VNCLOG("Hextile encoder requested\n"));

		// Create a CoRRE encoder
		m_encoder = new vncEncodeHexT;
		break;

	case rfbEncodingUltra:

		vnclog.Print(LL_INTINFO, VNCLOG("Ultra encoder requested\n"));

		// Create a Zlib encoder, if needed.
		// If a Zlib encoder was used previously, then reuse it here
		// to maintain zlib dictionary synchronization with the viewer.
		if ( m_hold_ultra_encoder == NULL )
		{
			m_encoder = new vncEncodeUltra;
		}
		else
		{
			m_encoder = m_hold_ultra_encoder;
		}
		if (m_encoder == NULL)
			return FALSE;
		ultra_encoder_in_use = true;//

		// sf@2003 - UltraEncoder Queing does not work with DSM - Disable it in this case until
		// some work is done to improve Queuing/DSM compatibility
		((vncEncodeUltra*)(m_encoder))->EnableQueuing(m_fEnableQueuing);
		EnableXCursor(false);
		EnableRichCursor(false);
		EnableCache(false);
		break;

	case rfbEncodingUltra2:

		vnclog.Print(LL_INTINFO, VNCLOG("Ultra encoder requested\n"));
		if (m_scrinfo.format.bitsPerPixel!=32 || m_clientformat.bitsPerPixel != 32)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("jpeg encoder is only supported on 32bit color display\n"));
			return false;
		}
		// Create a Zlib encoder, if needed.
		// If a Zlib encoder was used previously, then reuse it here
		// to maintain zlib dictionary synchronization with the viewer.
		if ( m_hold_ultra2_encoder == NULL )
		{
			m_encoder = new vncEncodeUltra2;
		}
		else
		{
			m_encoder = m_hold_ultra2_encoder;
		}
		if (m_encoder == NULL)
			return FALSE;
		ultra2_encoder_in_use = true;//

		// sf@2003 - UltraEncoder Queing does not work with DSM - Disable it in this case until
		// some work is done to improve Queuing/DSM compatibility
		//EnableXCursor(false);
		//EnableRichCursor(false);
		//EnableCache(false);
		break;

	case rfbEncodingZSTDRLE:
		vnclog.Print(LL_INTINFO, VNCLOG("ZSTDRLE encoder requested\n"));			
		if (!zrleEncoder)
			zrleEncoder = new vncEncodeZRLE;
		m_encoder = zrleEncoder;
		((vncEncodeZRLE*)zrleEncoder)->m_use_zywrle = FALSE;
		((vncEncodeZRLE*)zrleEncoder)->set_use_zstd(true);
		break;
	case rfbEncodingZRLE:
		vnclog.Print(LL_INTINFO, VNCLOG("ZRLE encoder requested\n"));			
		if (!zrleEncoder)
			zrleEncoder = new vncEncodeZRLE;
		m_encoder = zrleEncoder;
		((vncEncodeZRLE*)zrleEncoder)->m_use_zywrle = FALSE;
		((vncEncodeZRLE*)zrleEncoder)->set_use_zstd(false);
		break;

	case rfbEncodingZSTDYWRLE:
		vnclog.Print(LL_INTINFO, VNCLOG("ZSTDYWRLE encoder requested\n"));		
		if (!zrleEncoder)
			zrleEncoder = new vncEncodeZRLE;
		m_encoder = zrleEncoder;
		((vncEncodeZRLE*)zrleEncoder)->m_use_zywrle = TRUE;
		((vncEncodeZRLE*)zrleEncoder)->set_use_zstd(true);
		break;
	case rfbEncodingZYWRLE:
		vnclog.Print(LL_INTINFO, VNCLOG("ZYWRLE encoder requested\n"));			
		if (!zrleEncoder)
			zrleEncoder = new vncEncodeZRLE;
		m_encoder = zrleEncoder;
		((vncEncodeZRLE*)zrleEncoder)->m_use_zywrle = TRUE;
		((vncEncodeZRLE*)zrleEncoder)->set_use_zstd(false);
		break;
#ifdef _XZ
	case rfbEncodingXZ:
		vnclog.Print(LL_INTINFO, VNCLOG("XZ encoder requested\n"));
		if ( m_hold_xz_encoder == NULL )
			m_encoder = new vncEncodeXZ;
		else
			m_encoder = m_hold_xz_encoder;
		((vncEncodeXZ*)m_encoder)->m_use_xzyw = FALSE;
		xz_encoder_in_use = true;
		break;

	case rfbEncodingXZYW:
		vnclog.Print(LL_INTINFO, VNCLOG("XZYW encoder requested\n"));
		if ( m_hold_xz_encoder == NULL )
			m_encoder = new vncEncodeXZ;
		else
			m_encoder = m_hold_xz_encoder;
		((vncEncodeXZ*)m_encoder)->m_use_xzyw = TRUE;
		xz_encoder_in_use = true;
		break;
#endif

	case rfbEncodingZlib:
		vnclog.Print(LL_INTINFO, VNCLOG("Zlib encoder requested\n"));		
		if (m_hold_zlib_encoder == NULL)
			m_encoder = new vncEncodeZlib;
		else
			m_encoder = m_hold_zlib_encoder;
		if (m_encoder == NULL)
			return FALSE;
		zlib_encoder_in_use = true;
		((vncEncodeZlib*)m_encoder)->set_use_zstd(false);
		break;

	case rfbEncodingZstd:
		vnclog.Print(LL_INTINFO, VNCLOG("Zstd encoder requested\n"));
		if ( m_hold_zlib_encoder == NULL )
			m_encoder = new vncEncodeZlib;
		else
			m_encoder = m_hold_zlib_encoder;
		if (m_encoder == NULL)
			return FALSE;
		zlib_encoder_in_use = true;//
		((vncEncodeZlib*)m_encoder)->set_use_zstd(true);
		break;

	case rfbEncodingZlibHex:
		vnclog.Print(LL_INTINFO, VNCLOG("ZlibHex encoder requested\n"));
		if (m_hold_zlibhex_encoder == NULL)
			m_encoder = new vncEncodeZlibHex;
		else
			m_encoder = m_hold_zlibhex_encoder;
		if (m_encoder == NULL)
			return FALSE;
		zlibhex_encoder_in_use = true;//
		((vncEncodeZlibHex*)m_encoder)->set_use_zstd(false);
		break;

	case rfbEncodingZstdHex:
		vnclog.Print(LL_INTINFO, VNCLOG("ZstdbHex encoder requested\n"));
		if ( m_hold_zlibhex_encoder == NULL )
			m_encoder = new vncEncodeZlibHex;
		else
			m_encoder = m_hold_zlibhex_encoder;
		if (m_encoder == NULL)
			return FALSE;
		zlibhex_encoder_in_use = true;//
		((vncEncodeZlibHex*)m_encoder)->set_use_zstd(true);
		break;

	case rfbEncodingTight:
		vnclog.Print(LL_INTINFO, VNCLOG("Tight encoder requested\n"));
		if ( m_hold_tight_encoder == NULL )
			m_encoder = new vncEncodeTight;
		else
			m_encoder = m_hold_tight_encoder;
		if (m_encoder == NULL)
			return FALSE;
		tight_encoder_in_use = true;
		((vncEncodeTight*)m_encoder)->set_use_zstd(false);
		break;

	case rfbEncodingTightZstd:
		vnclog.Print(LL_INTINFO, VNCLOG("TightZstd encoder requested\n"));
		if (m_hold_tight_encoder == NULL)
			m_encoder = new vncEncodeTight;
		else
			m_encoder = m_hold_tight_encoder;
		if (m_encoder == NULL)
			return FALSE;
		tight_encoder_in_use = true;
		((vncEncodeTight*)m_encoder)->set_use_zstd(true);
		break;

	default:
		// An unknown encoding was specified
		vnclog.Print(LL_INTERR, VNCLOG("unknown encoder requested: %i\n"), encoding);
		return FALSE;
	}

	// Initialise it and give it the pixel format
	if (m_encoder != NULL) {
		m_encoder->Init();
		m_encoder->SetLocalFormat(m_scrinfo.format, m_scrinfo.framebufferWidth, m_scrinfo.framebufferHeight);
		if (m_clientfmtset && !m_encoder->SetRemoteFormat(m_clientformat)) {
			vnclog.Print(LL_INTERR, VNCLOG("client pixel format is not supported\n"));
			return FALSE;
		}
	}
	if (reinitialize && m_encoder != NULL) {
		m_encoder->EnableXCursor(m_use_xcursor);
		m_encoder->EnableRichCursor(m_use_richcursor);
		m_encoder->SetCompressLevel(m_compresslevel);
		m_encoder->SetQualityLevel(m_qualitylevel);
		m_encoder->SetFineQualityLevel(m_finequalitylevel);
		m_encoder->SetSubsampling(m_subsampling);
		m_encoder->EnableLastRect(m_use_lastrect);
	}

	m_buffer->ClearCache();
	m_buffer->ClearBack();		
	m_encoder->SetBufferOffset(monitor_Offsetx, monitor_Offsety);
	return CheckBuffer();
}

// Predict how many update rectangles a given rect will encode to
// For Raw, RRE or Hextile, this is always 1.  For CoRRE, may be more,
// because each update rect is limited in size.
inline UINT
vncEncodeMgr::GetNumCodedRects(const rfb::Rect &rect)
{
	// sf@2002 - Tight encoding
	// TODO: Add the appropriate virtual NumCodedRects function to Tight encoder instead.
	if (tight_encoder_in_use || zlibhex_encoder_in_use)
	{
		RECT TRect;
		TRect.right = rect.br.x;
		TRect.left = rect.tl.x;
		TRect.top = rect.tl.y;
		TRect.bottom = rect.br.y;
		return m_encoder->NumCodedRects(TRect);
	}

	return m_encoder->NumCodedRects(rect);
}

//
// -=- Pixel format translation
//

// Update the local pixel format used by the encoder
inline BOOL
vncEncodeMgr::SetServerFormat()
{
	if (m_encoder) {
		return CheckBuffer() && m_encoder->SetLocalFormat(
			m_scrinfo.format,
			m_scrinfo.framebufferWidth,
			m_scrinfo.framebufferHeight);
	}
	return FALSE;
}

// Specify the client's pixel format
inline BOOL
vncEncodeMgr::SetClientFormat(rfbPixelFormat &format)
{
	vnclog.Print(LL_INTINFO, VNCLOG("SetClientFormat called\n"));
	
	// Save the desired format
	m_clientfmtset = TRUE;
	m_clientformat = format;

	// Tell the encoder of the new format
	if (m_encoder != NULL)
		m_encoder->SetRemoteFormat(format);

	// Check that the output buffer is sufficient
	if (!CheckBuffer())
		return FALSE;

	return TRUE;
}

// Translate a rectangle to the client's pixel format
inline UINT
vncEncodeMgr::EncodeRect(const rfb::Rect &rect,VSocket *outconn)
{
	//Fix crash after resolution change
	// vncencoder seems still to have to old resolution while other parts are already updated
	// Could not find it, but this extra check eliminate the out of buffer updates during transition
	if(rect.br.x>m_scrinfo.framebufferWidth) 
		return 0;
	if(rect.br.y>m_scrinfo.framebufferHeight) 
		return 0;
	// Call the encoder to encode the rectangle into the client buffer...
	/*if (!m_clientbackbuffif){*/
	if (!m_buffer->m_backbuff){
		vnclog.Print(LL_INTERR, "no client back-buffer available in EncodeRect\n");
		return 0;
	}
	if (zlib_encoder_in_use)
	{
		if (m_use_queue)
			return m_encoder->EncodeRect(m_buffer->m_backbuff, outconn ,m_clientbuff, rect, 1);
		else 
			return m_encoder->EncodeRect(m_buffer->m_backbuff, outconn ,m_clientbuff, rect, 0);
	}
	if (ultra_encoder_in_use)
	{
		return m_encoder->EncodeRect(m_buffer->m_backbuff, outconn ,m_clientbuff, rect);
	}
	if (ultra2_encoder_in_use)
	{
		return m_encoder->EncodeRect(m_buffer->m_backbuff, outconn ,m_clientbuff, rect);
	}

	// sf@2002 - Tight encoding
	if (tight_encoder_in_use || zlibhex_encoder_in_use  || ultra_encoder_in_use || ultra2_encoder_in_use)
	{
		RECT TRect;
		TRect.right = rect.br.x;
		TRect.left = rect.tl.x;
		TRect.top = rect.tl.y;
		TRect.bottom = rect.br.y;
		return m_encoder->EncodeRect(m_buffer->m_backbuff, outconn, m_clientbuff, TRect); // sf@2002 - For Tight...
	}

	return m_encoder->EncodeRect(m_buffer->m_backbuff, m_clientbuff, rect);
}

inline UINT
vncEncodeMgr::EncodeBulkRects(const rfb::RectVector &allRects, int nScale, VSocket *outconn)
{
	if (!m_buffer->m_backbuff){
		vnclog.Print(LL_INTERR, "no client back-buffer available in EncodeRect\n");
		return 0;
	}

	rfb::RectVector scaledRects;

	// Work through the list of rectangles, sending each one
	rfb::RectVector::const_iterator i;

	for (i=allRects.begin();i!=allRects.end();i++) {
		rfb::Rect ScaledRect = *i;
		if (nScale != 1) {
			ScaledRect.tl.y = ScaledRect.tl.y / nScale;
			ScaledRect.br.y = ScaledRect.br.y / nScale;
			ScaledRect.tl.x = ScaledRect.tl.x / nScale;
			ScaledRect.br.x = ScaledRect.br.x / nScale;
		}
		if(ScaledRect.br.x>m_scrinfo.framebufferWidth) return 0;
		if(ScaledRect.br.y>m_scrinfo.framebufferHeight) return 0;

		scaledRects.push_back(ScaledRect);
	}

	return m_encoder->EncodeBulkRects(scaledRects, m_buffer->m_backbuff, m_clientbuff, outconn);
}


rfb::Rect 
vncEncodeMgr::GetSize()
{
	return m_buffer->GetSize();
}
inline void
vncEncodeMgr::SetBufferOffset(int x,int y)
{
	monitor_Offsetx = x;
	monitor_Offsety = y;
	m_encoder->SetBufferOffset(x,y);
}

inline void
vncEncodeMgr::EnableXCursor(BOOL enable)
{
	m_use_xcursor = enable;
	if (m_encoder != NULL) {
		m_encoder->EnableXCursor(enable);
	}
	m_buffer->m_cursor_shape_cleared = TRUE;
}

inline void
vncEncodeMgr::EnableRichCursor(BOOL enable)
{
	m_use_richcursor = enable;
	if (m_encoder != NULL) {
		m_encoder->EnableRichCursor(enable);
	}
	m_buffer->m_cursor_shape_cleared = TRUE;
}

// Check if cursor shape update should be sent
inline BOOL
vncEncodeMgr::IsCursorUpdatePending()
{
	if (m_use_xcursor || m_use_richcursor) {
		return m_buffer->IsCursorUpdatePending();
	}
	return false;
}

inline BOOL
vncEncodeMgr::WasCursorUpdatePending()
{
	BOOL pending=m_buffer->IsCursorUpdatePending();
	m_buffer->SetCursorPending(FALSE);
	return pending;
}

inline BOOL
vncEncodeMgr::SendCursorShape(VSocket *outConn) {
	return m_encoder->SendCursorShape(outConn, m_buffer->m_desktop);
}

inline BOOL
vncEncodeMgr::SendEmptyCursorShape(VSocket *outConn) {
	return m_encoder->SendEmptyCursorShape(outConn);
}

inline BOOL
vncEncodeMgr::IsXCursorSupported() {
	return m_encoder->IsXCursorSupported();
}

// Tight
inline void
vncEncodeMgr::SetCompressLevel(int level)
{
	m_compresslevel = (level >= 0 && level <= 9) ? level : 6;
	if (m_encoder != NULL)
		m_encoder->SetCompressLevel(m_compresslevel);
}

inline void
vncEncodeMgr::SetQualityLevel(int level)
{
	m_qualitylevel = (level >= 0 && level <= 9) ? level : -1;
	m_finequalitylevel = coarsequal2finequal[level];
	m_subsampling = coarsequal2subsamp[level];
	if (m_encoder != NULL) {
		m_encoder->SetQualityLevel(m_qualitylevel);
		m_encoder->SetFineQualityLevel(m_finequalitylevel);
		m_encoder->SetSubsampling(m_subsampling);
	}
}

inline void
vncEncodeMgr::SetFineQualityLevel(int level)
{
	m_finequalitylevel = (level >= 0 && level <= 100) ? level : -1;
	if (m_encoder != NULL)
		m_encoder->SetFineQualityLevel(m_finequalitylevel);
}

inline void
vncEncodeMgr::SetSubsampling(subsamp_type subsamp)
{
	m_subsampling = subsamp;
	if (m_encoder != NULL)
		m_encoder->SetSubsampling(m_subsampling);
}

inline void
vncEncodeMgr::EnableLastRect(BOOL enable)
{
	m_use_lastrect = enable;
	if (m_encoder != NULL) {
		m_encoder->EnableLastRect(enable);
	}
}

inline BOOL
vncEncodeMgr::IsMouseWheelTight()
{
	if (m_use_tight && !m_use_queue) //tight client
		return true;
	return false;
}

// sf@2005
inline void vncEncodeMgr::EnableGreyPalette(BOOL enable)
{
	m_buffer->EnableGreyPalette(enable);
}

inline void
vncEncodeMgr::EnableCache(BOOL enabled)
{
	m_cache_enabled = enabled;
	// if (enabled)
	m_buffer->EnableCache(enabled);
}

inline BOOL
vncEncodeMgr::IsCacheEnabled()
{
	return m_cache_enabled;
}

inline void
vncEncodeMgr::LastRect(VSocket *outConn)
{
	m_encoder->LastRect(outConn);
}

#endif // _WINVNC_VNCENCODEMGR

