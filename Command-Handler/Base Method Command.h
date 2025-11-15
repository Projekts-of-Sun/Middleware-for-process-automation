#pragma once

#include "Base Command.h"
#include <string>
#include <list>

namespace CommandHandler
{
	class CBaseMethodCommand: public CBaseCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			static const wchar_t* TypeName;

			bool operator ==(const CBaseMethodCommand& P) const;

			void Demo();
			CBaseMethodCommand();
			CBaseMethodCommand(const CString& ProjectPath, const CString& MasterBatch, const CString& MethodName,
													const CString& DataPath, std::list<CommandHandler::CPositionData>& Labware);
			virtual ~CBaseMethodCommand();
			
			virtual CString CommandName() const
			{	return TypeName;
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
			virtual std::string ToXmlString(unsigned int MsgID = 0) const;
			virtual std::string AsReplyToXmlString(unsigned int MessageID = 0) const;
			virtual bool FromXmlString(const char* Xml);
			CString ProjectPath;
			CString MasterBatch;
			CString MethodName;
			CString DataPath;
			CRack Rack;
			int EstimateTime;		// Seconds, do not add to message if -1
			bool Success;				// Command was successful or not
			CString ErrorText;	// if command failed
			int ErrorCode;			// if command failed
			std::list<CPositionData> LabwareList;
			// Position, Data (A1, 0.3)
			std::list<std::pair<CString, CString>> WellResultList;

			enum RunStates
			{ RunState_CommandReceived = 1,
				RunState_MethodStarted,
				RunState_MethodError,
				RunState_MethodFinished
			};
			RunStates RunState;
	};

}	//namespace