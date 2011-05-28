#include "Medium.h"

MusicBrainzADH::CDisc::CDisc(const XMLNode& Node)
{
	if (!Node.isEmpty())
	{
		//std::cout << "Disc node: " << std::endl << Node.createXMLString(true) << std::endl;
			
		if (Node.isAttributeSet("id"))			
			m_ID=Node.getAttribute("id");
		
		for (int count=0;count<Node.nChildNode();count++)
		{
			XMLNode ChildNode=Node.getChildNode(count);
			std::string NodeName=ChildNode.getName();
			std::string NodeValue;
			if (ChildNode.getText())
				NodeValue=ChildNode.getText();
			
			if ("sectors"==NodeName)
			{
				m_Sectors=NodeValue;
			}
			else
			{
				std::cerr << "Unrecognised disc node: '" << NodeName << "'" << std::endl;
			}
		}
	}
}

std::string MusicBrainzADH::CDisc::ID() const
{
	return m_ID;
}

std::string MusicBrainzADH::CDisc::Sectors() const
{
	return m_Sectors;
}

std::ostream& operator << (std::ostream& os, const MusicBrainzADH::CDisc& Disc)
{
	os << "Disc:" << std::endl;
		
	os << "\tID:      " << Disc.ID() << std::endl;
	os << "\tSectors: " << Disc.Sectors() << std::endl;
		
	return os;
}
