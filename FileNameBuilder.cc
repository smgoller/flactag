/* --------------------------------------------------------------------------

   flactag -- A tagger for single album FLAC files with embedded CUE sheets
   						using data retrieved from the MusicBrainz service

   Copyright (C) 2006 Andrew Hawkins
   
   This file is part of flactag.
   
   Flactag is free software; you can redistribute it and/or
   modify it under the terms of v2 of the GNU Lesser General Public
   License as published by the Free Software Foundation.
   
   Flactag is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

     $Id$

----------------------------------------------------------------------------*/

#include "FileNameBuilder.h"

CFileNameBuilder::CFileNameBuilder(const tTagMap& Tags, const std::string& BasePath, const std::string& SingleDiskFileName, const std::string& MultiDiskFileName)
:	m_Tags(Tags),
	m_BasePath(BasePath),
	m_SingleDiskFileName(SingleDiskFileName),
	m_MultiDiskFileName(MultiDiskFileName)
{
	BuildPath();
}
	
std::string CFileNameBuilder::FileName() const
{
	return m_FileName;
}

void CFileNameBuilder::BuildPath()
{
	std::string Template;
	
	if (m_Tags[CTagName("DISCNUMBER")].empty())
		Template=m_SingleDiskFileName;
	else
		Template=m_MultiDiskFileName;
		
	m_FileName=m_BasePath+"/"+Template;
	
	ReplaceString("%A","ARTIST");
	ReplaceString("%S","ARTISTSORT");
	ReplaceString("%T","ALBUM");
	ReplaceString("%D","DISCNUMBER");
	ReplaceString("%Y","YEAR");
	ReplaceString("%G","GENRE");
}

void CFileNameBuilder::ReplaceString(const std::string& Search, const std::string& ReplaceTag)
{
	std::string Replace="NO"+ReplaceTag;
	
	tTagMap::const_iterator ThisTag=m_Tags.find(CTagName(ReplaceTag));
	if (m_Tags.end()!=ThisTag)
	{
		std::string Value=(*ThisTag).second;
		if (!Value.empty())
			Replace=FixString(Value);
	}
	
	std::string::size_type SearchPos=m_FileName.find(Search);
	while(std::string::npos!=SearchPos)
	{
		m_FileName=m_FileName.substr(0,SearchPos)+Replace+m_FileName.substr(SearchPos+Search.length());
		SearchPos=m_FileName.find(Search);
	}
}

std::string CFileNameBuilder::FixString(const std::string& String) const
{
	std::string Fixed=String;
	std::string BadChars="/:\"'`;?";
			
	for (std::string::size_type count=0;count<Fixed.length();count++)
	{
		if (BadChars.find(Fixed[count])!=std::string::npos)
			Fixed.replace(count,1,"-");
	}
	
	return Fixed;
}