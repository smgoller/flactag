/* --------------------------------------------------------------------------

   flactag -- A tagger for single album FLAC files with embedded CUE sheets
   						using data retrieved from the MusicBrainz service

   Copyright (C) 2006-2012 Andrew Hawkins

   This file is part of flactag.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

   Flactag is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

     $Id$

----------------------------------------------------------------------------*/

#include "CoverArt.h"

#include "ErrorLog.h"

#include <setjmp.h>

#include "base64.h"

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_output_message (j_common_ptr cinfo)
{
	  char buffer[JMSG_LENGTH_MAX];

	  /* Create the message */

	  (*cinfo->err->format_message) (cinfo, buffer);

	  CErrorLog::Log(buffer);
}

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

CCoverArt::CCoverArt(const unsigned char *Data, size_t Length)
:	m_Data(0),
	m_Length(0),
	m_Width(0),
	m_Height(0)
{
	SetArt(Data,Length,true);
}

CCoverArt::CCoverArt(const CCoverArt& Other)
:	m_Data(0),
	m_Length(0),
	m_Width(0),
	m_Height(0)
{
	if (this!=&Other)
		*this=Other;
}

CCoverArt::~CCoverArt()
{
	Free();
}

void CCoverArt::Free()
{
	if (m_Data)
		delete[] m_Data;

	m_Data=0;
	m_Length=0;
}

CCoverArt& CCoverArt::operator =(const CCoverArt& Other)
{
	m_Width=Other.m_Width;
	m_Height=Other.m_Height;
	SetArt(Other.m_Data,Other.m_Length,false);

	return *this;
}

bool CCoverArt::operator ==(const CCoverArt& Other) const
{
	bool RetVal=false;

	if (m_Length==0 && Other.m_Length==0)
		RetVal=true;
	else if (m_Length==Other.m_Length && 0==memcmp(m_Data,Other.m_Data,m_Length))
		RetVal=true;

	return RetVal;
}

bool CCoverArt::operator !=(const CCoverArt& Other) const
{
	return !(*this==Other);
}

CCoverArt::operator std::string() const
{
	return rfc822_binary(m_Data,m_Length);
}

CCoverArt::operator bool() const
{
	return m_Length!=0;
}

void CCoverArt::Clear()
{
	Free();
}

void CCoverArt::SetArt(const unsigned char *Data, size_t Length, bool RetrieveDimensions)
{
	Free();

	if (Data && Length)
	{
		m_Data=new unsigned char[Length];
		m_Length=Length;
		memcpy(m_Data,Data,Length);

		if (RetrieveDimensions)
			GetDimensions();
	}
}

unsigned char *CCoverArt::Data() const
{
	return m_Data;
}

size_t CCoverArt::Length() const
{
	return m_Length;
}


typedef struct
{
	struct jpeg_source_mgr pub; /* public fields */
	JOCTET eoi_buffer[2]; /* a place to put a dummy EOI */
} my_source_mgr;

typedef my_source_mgr *my_src_ptr;

void CCoverArt::InitSource(j_decompress_ptr /*cinfo*/)
{
}


boolean CCoverArt::FillInputBuffer(j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	WARNMS(cinfo, JWRN_JPEG_EOF);

	src->eoi_buffer[0] = (JOCTET) 0xFF;
	src->eoi_buffer[1] = (JOCTET) JPEG_EOI;
	src->pub.next_input_byte = src->eoi_buffer;
	src->pub.bytes_in_buffer = 2;

	return true;
}

void CCoverArt::SkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	if (num_bytes > 0)
	{
		while (num_bytes > (long) src->pub.bytes_in_buffer)
		{
			num_bytes -= (long) src->pub.bytes_in_buffer;
			FillInputBuffer(cinfo);
		}

		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

void CCoverArt::TermSource(j_decompress_ptr /*cinfo*/)
{
}

void CCoverArt::JPEGMemorySource(j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
{
	my_src_ptr src;

	if (cinfo->src == NULL)
	{
		cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)
											((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = CCoverArt::InitSource;
	src->pub.fill_input_buffer = CCoverArt::FillInputBuffer;
	src->pub.skip_input_data = CCoverArt::SkipInputData;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = CCoverArt::TermSource;

	src->pub.next_input_byte = buffer;
	src->pub.bytes_in_buffer = bufsize;
}

void CCoverArt::GetDimensions()
{
	if (!m_Width && !m_Height)
	{
		struct jpeg_decompress_struct cinfo;
		struct my_error_mgr jerr;

		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		jerr.pub.output_message = my_output_message;

		if (setjmp(jerr.setjmp_buffer))
		{
			/* If we get here, the JPEG code has signaled an error.
			* We need to clean up the JPEG object, close the input file, and return.
			*/
			jpeg_destroy_decompress(&cinfo);
			return;
		}

		jpeg_create_decompress(&cinfo);
		JPEGMemorySource(&cinfo,m_Data,m_Length);
		jpeg_read_header(&cinfo, TRUE);

		m_Width=cinfo.image_width;
		m_Height=cinfo.image_height;

		jpeg_destroy_decompress(&cinfo);
	}
}

int CCoverArt::Width() const
{
	return m_Width;
}

int CCoverArt::Height() const
{
	return m_Height;
}

