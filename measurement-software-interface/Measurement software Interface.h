#pragma once
#include <afx.h>
#include <afxwin.h>
#include <afxdisp.h>
#include <mmsystem.h>
#include <gdiplus.h>

#include "..\Command Handler\msxml6.tlh"
#include "ImageIDs.h"

#include <list>
#include <vector>
#include <map>
#include <string>

//#define MASSHUNTER_USE_COM

#ifdef MASSHUNTER_USE_COM
namespace ICP
{
#include "ICP\agtpicworklist.tlh"
}

namespace LCMS_TOF
{
#include "LCMS-TOF\agtpiclauncher.tlh"
#include "LCMS-TOF\agtpicengine.tlh"
#include "LCMS-TOF\agtpicworklist.tlh"
}
#endif


namespace SamiControl
{

class CExecutiveParameters
{
public:
	typedef MSXML2::IXMLDOMNodePtr SavePtr;
	typedef MSXML2::IXMLDOMDocumentPtr DocPtr;

	CExecutiveParameters()
		: Success(false)
		, JobID(++JobCounter)
		, Handler(NULL)
		, StartTime(timeGetTime())
		, Timeout(0)
		, MsgID(0)
	{ }
	virtual ~CExecutiveParameters()
	{ }
	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);
	virtual std::string ToXmlString() const;
	virtual bool FromXmlString(const char* Xml);

	virtual char* StreamName() const
	{
		return "ExecutiveParameters";
	}

	virtual bool IsTimeout()
	{
		return (Timeout>0) && ((timeGetTime() - StartTime) > Timeout);
	}

	bool Success;
	CString ErrorText;
	HWND Handler;
	unsigned int MsgID;
	unsigned int JobID;
	static unsigned int JobCounter;
	DWORD StartTime;
	DWORD Timeout;
};

typedef std::map<unsigned int, CExecutiveParameters*> ExecutiveJobs;

class CBaseLabwareData
{
public:
	CBaseLabwareData()
		: BarcodeUsed(false)
		, ID(0)
		, Height(0.0)
	{	}
	//*****************
	std::wstring SampleName;      // SampleName
	std::wstring Vial;            // Vial Position (z. B. A1)
	std::wstring Method_Path;
	std::wstring Typ;         // Typ (Sample, QC, Cal)
	std::wstring DataFile;
	std::wstring Level;     // Level
	std::wstring Dilution;  // Dilution
	std::wstring Volume;    // Volume
	std::wstring TrayName;  // Tray
	std::wstring Comment;   // Kommentar
	//*****************
	std::wstring Name;
	double Height;
	std::wstring ClassName;
	std::wstring Barcode;
	bool BarcodeUsed;
	unsigned int ID;
	std::wstring Position;
};

class CMethodRun : public CExecutiveParameters
{
public:
	enum
	{	MsgHandlerReply = (WM_APP + 4)
	};
	CMethodRun()
		: ProjectID(0)
		, MethodRevision(0)
		, Families(0)
	{	MsgID = MsgHandlerReply;
	}
	virtual ~CMethodRun()
	{ }
	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);

	virtual char* StreamName() const
	{
		return "MethodRun";
	}

	CString Project;
	long ProjectID;
	CString ProjectFileName; 
	CString Method;
	long MethodRevision;
	CString MethodFileName;
	CString NewBatchFileName;
	long Families;
	std::list<CBaseLabwareData> Labware;
	bool RunSequenceExecuted = false;
	CString DataPath;
	CString Path;
	CString PositionName;


};

class CPositionDescriptor
{
public:
	CString PositionName;
	CString PathName;
	std::vector<CBaseLabwareData> Labware;
};

class CMethodSchedule : public CExecutiveParameters
{
public:
	enum
	{	MsgHandlerReply = (WM_APP + 5)
	};
	CMethodSchedule()
		: ETC(0.0)
		, Families(0)
		, ProjectID(0)
		, MethodRevision(0)
	{	MsgID = MsgHandlerReply;
	}
	virtual ~CMethodSchedule()
	{ }
	virtual bool Save(SavePtr X, DocPtr D) const;
	virtual bool Load(SavePtr X, DocPtr D);

	virtual char* StreamName() const
	{
		return "MethodSchedule";
	}

	CString Project;
	long ProjectID;
	CString ProjectFileName;
	CString Method;
	long MethodRevision;
	CString MethodFileName;
	CString NewBatchFileName;
	double ETC;
	long Families;
	std::list<CPositionDescriptor> Positions;
	std::list<std::list<CPositionDescriptor>> Paths;
};

class CExecutiveStatus : public CExecutiveParameters
{
public:
	

	enum
	{	MsgHandlerReply = (WM_APP + 2)
	};
	CExecutiveStatus(CString& ETC, CString& Time, bool Paused);
	virtual ~CExecutiveStatus();

	double Time;
	double ETC;
	bool Paused;
};

class CWindowToFind
{
public:
	CWindowToFind(const wchar_t* T, const wchar_t* C)
		: Title(T)
		, Class(C)
		, Window(NULL)
		, MaxWindows(1000)
	{
	}
	HWND Window;
	CString Title;
	CString Class;
	unsigned int MaxWindows;
};

class CMassHunterInterface : public CWnd
{
public:
	CString GetCurrentStatusText() const;
	CString GetInstrumentStatus();
	CString GetCurrentProjectPath() const { return m_ProjectPath; }
	CString GetCurrentDatatPath()  const {return DataPathFest; }

	CString RunSequenceFertig() ;
	CString GetRunStatus();

	enum
	{	BUTTON_ID_START = 100,
		BUTTON_ID_END = 200
	};

	enum
	{	TimerID_Buttons = 1,
		TimerID_Timeout
	};

	CMassHunterInterface(CString& ProjectPath,CString& DataPat);
	virtual ~CMassHunterInterface();

	bool ReadProjectList();
	void SetProjectPath(const CString& Path);

	void SetCurrentDatatPath(const CString& path2);


	bool ReadMethodsForProject(const CString& ProjectName);
	bool GetMethod(const CString& MethodName, unsigned int MethodRevision);
	bool GetETCForMethod(const CString& MethodName, unsigned int MethodRevision, double& ETC);

	bool MethodExists(CString& MethodName, unsigned int MethodRevision);

	std::list<std::pair<CString, long>> m_ProjectList;
	std::list<CString> m_MethodList;
	CString m_ProjectPath;
	CString DataPathFest ;
	std::wstring DataPAth_Final;
	CString RunstatusText;
	CString InstrumenstatusText;
	

	


	HWND FindMassHunterWindow(bool Icp = true);
	bool ScheduleMethod(CMethodSchedule* Method, DWORD MilliSeconds);
	bool ScheduleMethod2(CMethodSchedule* Method, DWORD MilliSeconds);
	bool StartMethod(CMethodRun* Method, DWORD MilliSeconds);
	bool StartMethod2(CMethodRun* Method, DWORD MilliSeconds);
	bool WriteMethodToFile(CMethodRun* Method, DWORD MilliSeconds);
	bool WriteMethodToFile2(CMethodRun* Method, DWORD MilliSeconds);
	bool RunSequence(CMethodRun* Method, DWORD MilliSeconds);


	static const wchar_t* IcpMassHunterOnlineWindowTitle;
	static const wchar_t* IcpMassHunterOfflineWindowTitle;
	static const wchar_t* IcpMassHunterWindowClass;
	static const wchar_t* LcMsMassHunterOnlineWindowTitle;
	static const wchar_t* LcMsMassHunterOfflineWindowTitle;
	static const wchar_t* LcMsMassHunterWindowClass;

	CString& ErrorText()
	{	return m_ErrorText;
	}

	void SetReplyToDialogs(bool Reply)
	{
		m_ReplyToDialogs = Reply;
	}

	void Abort();
	void Resume();
	void Pause();

	void CheckJobTimeouts();

	// -------------- Window Functions ------------------------------
	RECT LoadPosition(CString ConfigurationName);
	void SavePosition(CString ConfigurationName);
	void OpenWindow(CString Titel, RECT& Size, CWnd* Parent, bool Visible = true);
	void SetStatusText(LPCTSTR Text);
	void SetInfoText(LPCTSTR Text);
	void SetText(LPCTSTR Status, LPCTSTR Info);
	void OnButtons(UINT nID);
	void ButtonsClear();
	// --------- Circle Diagram ------------------------------
	void SetPointer(unsigned int Pointer);
	void SetValue(double Value, bool Visible, unsigned int Element);
	void SetRange(double Minimum, double Maximum, unsigned int Elements);
	// -------------------------------------------------------


protected:
	HICON m_hIcon;
	void SetForegroundWindowInternal1(HWND hWnd);
	void SetForegroundWindowInternal2(HWND hWnd);
	void TimedSleep(DWORD MilliSeconds);
	bool m_Abort;
	void GetErrorText(HRESULT HR = 0);
	struct _ICP
	{
#ifdef MASSHUNTER_USE_COM
		ICP::AGTPICWORKLISTLib::IAgtPicWklControlManagerPtr ControlManager;
		ICP::AGTPICWORKLISTLib::IAgtPicWklDataManagerPtr DataManager;
		IUnknown*	ControlManagerEvents; 
		DWORD			ControlManagerCookie; 
		IUnknown*	DataManagerEvents; 
		DWORD			DataManagerCookie;
#endif
		bool OnlineFound;
		bool OfflineFound;
	} Icp;

#ifdef MASSHUNTER_USE_COM
	HRESULT OnAcquisitionSampleRunEnd(CComVariant varWklBatchID, long lJobID );
	HRESULT OnAcquisitionSampleRunStart(CComVariant varWklBatchID, long lJobID, LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleAttributeInfo *pSampleInfo);
	HRESULT OnDAJobRunStart(long lJobID, LCMS_TOF::AGTPICWORKLISTLib::EnumJobType enumJobType);
	HRESULT OnJobRunStart(CComVariant varWklBatchID, LCMS_TOF::AGTPICWORKLISTLib::SWkl_NewJobData *pJobData);
	HRESULT OnExecutionComplete(LCMS_TOF::AGTPICWORKLISTLib::EnumWklRunResultState enumRunState, HRESULT hrValue, CComBSTR bstrErrorMsg);
	HRESULT OnRunStart(CComVariant varWklBatchID);
#endif

	struct _LcMsTof
	{
#ifdef MASSHUNTER_USE_COM
		LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklControlManagerPtr ControlManager;
		LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklDataManagerPtr DataManager;
		IUnknown*	ControlManagerEvents; 
		DWORD		ControlManagerCookie; 
		IUnknown*	DataManagerEvents; 
		DWORD		DataManagerCookie; 
#endif
		bool OnlineFound;
		bool OfflineFound;
	} LcMsTof;

	// -------------- Window Functions ------------------------------
	CRect m_StatusRect, m_InfoRect;
	Gdiplus::Font *m_StatusFont, *m_InfoFont;
	CString m_StatusText, m_InfoText;
	HICON m_RedSignal, m_GreenSignal, m_GraySignal;
	std::map<unsigned int, CButton*> m_Buttons;
	Gdiplus::GdiplusStartupInput m_GdiPlusStartupInput;
	ULONG_PTR m_GdiPlusToken;

	// --------- circle diagram ------------------------------
	double m_AngleRotation;
	CFont m_CircleFont;
	CString m_CenterText;
	unsigned int m_PointerIndex;
	double m_Lastvalue;
	Gdiplus::Color SectionColor(double Color);
	double m_ColorShift;
	double m_ColorFactor;
	Gdiplus::Color* m_Color;
	bool* m_IsVisible;
	unsigned int m_Elements;
	CRect m_CircleRect;
	double m_LastStatusETC;
	// -------------------------------------------------------

	CString m_ErrorText;
	bool m_ReplyToDialogs;
	ExecutiveJobs m_Jobs;
	bool m_WorklistIsSupposedToRun;
	std::pair<unsigned int, CString> m_SelfReplyBuffer;

	void OnPaint();
	void OnSize(UINT nType, int cx, int cy);
	void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void OnTimer(UINT_PTR nIDEvent);

	static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
#ifdef MASSHUNTER_USE_COM
	DECLARE_DISPATCH_MAP()
#endif
};


}
