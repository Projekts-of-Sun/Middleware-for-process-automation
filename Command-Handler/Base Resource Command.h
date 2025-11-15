#pragma once

#include "Base Command.h"

#include <string>
#include <list>
#include <memory>

namespace CommandHandler
{

	class CBaseResourceInfo
	{
	public:
		CBaseResourceInfo(const CString& N)
			: Name(N)
		{ }

		virtual bool operator ==(const CBaseResourceInfo& P) const
		{
			if (Name!=P.Name) return false;
			return true;
		}

		CString Name;
	};

	class CBaseResourceCommand: public CBaseCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			static const wchar_t* TypeName;

			virtual bool operator ==(const CBaseResourceCommand& P) const;

			CBaseResourceCommand();
			virtual ~CBaseResourceCommand();
			
			virtual CString CommandName() const
			{	return TypeName;
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
			virtual std::string ToXmlString(unsigned int MsgID = 0) const;
			virtual std::string AsReplyToXmlString(unsigned int MsgID = 0) const
			{	return ToXmlString(MsgID);
			}
			virtual bool FromXmlString(const char* Xml);
			
			bool HasEntryWithName(const CString& Name) const;
			CBaseResourceInfo* EntryWithName(const CString& Name) const;

			static std::shared_ptr<CBaseResourceCommand> ObjectFromXmlString(const char* Xml);
			static std::shared_ptr<CBaseResourceCommand> ObjectFromXmlObject(SavePtr X, DocPtr D);
			
			std::list<std::shared_ptr<CBaseResourceInfo>> List;

			bool Success;				// Command was successful or not
			CString ErrorText;	// if command failed
			int ErrorCode;			// if command failed
	};

// --------------------- Status Info --------------------------------------------------------------

	class CBaseResourceStatusInfo : public CBaseResourceInfo
	{
	public:
		CBaseResourceStatusInfo(const CString& Name, const CString& _StatusAsText, int _StatusCode)
			: CBaseResourceInfo(Name)
			, StatusAsText(_StatusAsText)
			, StatusCode(_StatusCode)
		{ }
		CBaseResourceStatusInfo(const CString& Name)
			: CBaseResourceInfo(Name)
			, StatusAsText(L"")
			, StatusCode(0)
		{ }
		CBaseResourceStatusInfo()
			: CBaseResourceInfo(L"Request")
			, StatusAsText(L"")
			, StatusCode(0)
		{ }

		virtual bool operator ==(const CBaseResourceInfo& P) const
		{
			CBaseResourceStatusInfo* RSI = dynamic_cast<CBaseResourceStatusInfo*>((CBaseResourceInfo*)&P);
			if (!RSI) return false;
			return *this==*RSI;
		}

		virtual bool operator ==(const CBaseResourceStatusInfo& P) const
		{
			if (Name!=P.Name) return false;
			if (StatusAsText!=P.StatusAsText) return false;
			if (StatusCode!=P.StatusCode) return false;
			return true;
		}

		CString StatusAsText;
		int StatusCode;
		int FertigCode;

	};


// --------------------- Project Info --------------------------------------------------------------

	class CBaseResourceProjectInfo : public CBaseResourceInfo
	{
	public:
		CBaseResourceProjectInfo(const CString& Name)
			: CBaseResourceInfo(Name)
		{ }

		virtual bool operator ==(const CBaseResourceInfo& P) const
		{
			CBaseResourceProjectInfo* RPI = dynamic_cast<CBaseResourceProjectInfo*>((CBaseResourceInfo*)&P);
			if (!RPI) return false;
			return *this==*RPI;
		}

		virtual bool operator ==(const CBaseResourceProjectInfo& P) const
		{
			if (Name!=P.Name) return false;
			if (Projects!=P.Projects) return false;
			return true;
		}

		std::list<CString> Projects;
	};

// --------------------- Method Info --------------------------------------------------------------

	class CBaseResourceMethodInfo : public CBaseResourceInfo
	{
	public:
		CBaseResourceMethodInfo(const CString& Name, const CString& _Project)
			: CBaseResourceInfo(Name)
			, Project(_Project)
		{ }

		virtual bool operator ==(const CBaseResourceInfo& P) const
		{
			CBaseResourceMethodInfo* RMI = dynamic_cast<CBaseResourceMethodInfo*>((CBaseResourceInfo*)&P);
			if (!RMI) return false;
			return *this==*RMI;
		}

		virtual bool operator ==(const CBaseResourceMethodInfo& P) const
		{
			if (Name!=P.Name) return false;
			if (Project!=P.Project) return false;
			if (Methods!=P.Methods) return false;
			return true;
		}
		
		CString Project;
		std::list<CString> Methods;
	};

// --------------------- MasterBatch Info --------------------------------------------------------------

	class CBaseResourceMasterBatchInfo : public CBaseResourceInfo
	{
	public:
		CBaseResourceMasterBatchInfo(const CString& Name, const CString& _Project)
			: CBaseResourceInfo(Name)
			, Project(_Project)
		{ }

		virtual bool operator ==(const CBaseResourceInfo& P) const
		{
			CBaseResourceMasterBatchInfo* RMBI = dynamic_cast<CBaseResourceMasterBatchInfo*>((CBaseResourceInfo*)&P);
			if (!RMBI) return false;
			return *this==*RMBI;
		}

		virtual bool operator ==(const CBaseResourceMasterBatchInfo& P) const
		{
			if (Name!=P.Name) return false;
			if (Project!=P.Project) return false;
			if (MasterBatches!=P.MasterBatches) return false;
			return true;
		}
		
		CString Project;
		std::list<CString> MasterBatches;
	};

	class CBaseResourceStatusCommand: public CBaseResourceCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			virtual bool operator ==(const CBaseResourceStatusCommand& P) const;

			CBaseResourceStatusCommand();
			virtual ~CBaseResourceStatusCommand();
			
			virtual CString CommandName() const
			{	return L"Status";
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
	};

	class CBaseResourceProjectsCommand: public CBaseResourceCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			virtual bool operator ==(const CBaseResourceProjectsCommand& P) const;

			CBaseResourceProjectsCommand();
			virtual ~CBaseResourceProjectsCommand();
			
			virtual CString CommandName() const
			{	return L"List Projects";
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
	};

	class CBaseResourceMethodsCommand: public CBaseResourceCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			virtual bool operator ==(const CBaseResourceMethodsCommand& P) const;

			CBaseResourceMethodsCommand();
			virtual ~CBaseResourceMethodsCommand();
			
			virtual CString CommandName() const
			{	return L"List Methods";
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
	};

	class CBaseResourceMasterBatchesCommand: public CBaseResourceCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			virtual bool operator ==(const CBaseResourceMasterBatchesCommand& P) const;

			CBaseResourceMasterBatchesCommand();
			virtual ~CBaseResourceMasterBatchesCommand();
			
			virtual CString CommandName() const
			{	return L"List MasterBatches";
			}
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);
	};
}	//namespace