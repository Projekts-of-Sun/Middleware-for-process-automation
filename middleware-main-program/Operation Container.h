#pragma once
#include <afx.h>
#include <string>
#include <map>
//#import <msxml6.dll> named_guids
#include "..\Command Handler\msxml6.tlh"

#include <memory>

namespace SamiControl
{
class CMethodSchedule;
class CMethodRun;
};

class COperationContainer
{
public:
	typedef MSXML2::IXMLDOMNodePtr SavePtr;
	typedef MSXML2::IXMLDOMDocumentPtr DocPtr;

	enum OperationSourceType
	{	NoSourceOperation = 0,
		PmsOperation,
		LimsOperation,
		SamiOperation
	};

	COperationContainer(OperationSourceType Source = NoSourceOperation);
	virtual ~COperationContainer(); 
	
	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);
	virtual bool Load(const CString& Filename);
	virtual bool Save(const CString& Filename) const;
	virtual std::string ToXmlString() const;
	virtual bool FromXmlString(const char* Xml);
	
	virtual bool operator !=(const COperationContainer& OC) const;

	virtual char* StreamName() const
	{
		return "OperationContainer";
	}

	virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

	CString CommandName()
	{	return CName;
	}

	void CommandName(const CString& Name)
	{	CName = Name;
	}

	CString ProjectName;
	unsigned int ProjectID;
	CString MethodName;
	unsigned int MethodRevision;
	unsigned int Families;
	unsigned int MaxFamilies;
	double ETC;
	CString ErrorText;
	CString DataPath;
	CString PositionName;

	unsigned __int64 PmsId;

	OperationSourceType OperationSource;
protected:
	CString CName;
};

class CScheduleOperationContainer : public COperationContainer
{
public:
	CScheduleOperationContainer(OperationSourceType Source = NoSourceOperation);
	virtual ~CScheduleOperationContainer(); 
	
	virtual char* StreamName() const
	{
		return "ScheduleOperationContainer";
	}
	virtual bool operator !=(const CScheduleOperationContainer& SOC) const;
	virtual bool operator !=(const COperationContainer& OC) const;

	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);

	std::shared_ptr<SamiControl::CMethodSchedule> Schedule;
	CString BaseFilePath;
};

class CRunOperationContainer : public COperationContainer
{
public:
	bool RunSequenceCalled = false;
	bool HasRunSequenceStarted = false;
	bool RunSequenceStarted = false;


	CRunOperationContainer(OperationSourceType Source = NoSourceOperation);
	virtual ~CRunOperationContainer(); 
	
	virtual char* StreamName() const
	{
		return "RunOperationContainer";
	}
	virtual bool operator !=(const CRunOperationContainer& ROC) const;
	virtual bool operator !=(const COperationContainer& OC) const;

	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);

	std::shared_ptr<SamiControl::CMethodSchedule> Schedule;
	std::shared_ptr<SamiControl::CMethodRun> Run;
	CString BaseFilePath;
};
