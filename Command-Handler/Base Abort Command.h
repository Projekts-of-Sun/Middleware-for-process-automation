#pragma once

#include "Base Command.h"
#include <string>

namespace CommandHandler
{

	class CBaseAbortCommand: public CBaseCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			static const wchar_t* TypeName;

			virtual bool operator ==(const CBaseAbortCommand& P) const;

			CBaseAbortCommand();

			virtual ~CBaseAbortCommand();
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
			virtual std::string ToXmlString(unsigned int MsgID = 0) const;
			virtual bool FromXmlString(const char* Xml);
	};

}	//namespace