#pragma once
#include <afx.h>
#include <afxwin.h>
#include <afxcmn.h>

#include "Resource.h"

#include <boost\thread.hpp>

#include "..\List View\List View.h"
#include "..\Agilent\MassHunter Interface.h"
#include "..\LIMS Interface\LIMS Interface.h"
#include "..\Service Interface\Boost Socket Commands.h"

#include "Operation Container.h"

#include <memory>

// ------------------ code to replace PMS interface --------------------------------

class CPMSActionData
{

public:
	enum PMSAction
	{	PMSActionMin = 0,

		PMSCompleteOperationOrder,
		PMSFailOperationOrder,

		PMSActionMax
	};
	typedef std::function<void (boost::system::error_code, CPMSActionData*)> PMSActionHandler;

	CPMSActionData(PMSAction A, std::shared_ptr<COperationContainer>& C, PMSActionHandler H = nullptr)
		: Action(A)
		, Handler(H)
		, OrderID(0)
	{
		OperationContainer.push_back(C);
	}

	std::list<std::shared_ptr<COperationContainer>> OperationContainer;

	PMSActionHandler Handler;
	PMSAction Action;
	unsigned __int64 OrderID;
};
// ---------------------------------------------------------------------------------

class CPmsLsaMassHunterServiceDialog : public CDialog
{
public:
	CString GetMassHunterStatus();
	CStatic m_PathDisplay;
	void RunSequenceComplete(bool success, const CString& message);
	CString Fertig;
	CTime m_LastRunStartTime;
	//std::wstring StatusMeldung ;
	CString StatusMeldung;
	enum
	{	MSG_SET_INFO_TEXT = (WM_APP + 100),
		MSG_TASKBAR_ICON_CLICK,
		MSG_OPEN_OPERATION_TREE_DIALOG,
		MSG_UPDATE_PMS_CONNECT_STATUS
	};

	CPmsLsaMassHunterServiceDialog(CWnd* pParent = nullptr);
	~CPmsLsaMassHunterServiceDialog();

	enum
	{ IDD = IDD_PMSLSAMASSHUNTERSERVICE_DIALOG
	};

	enum RunState
	{
		RunIdle = 1,
		RunReadProject,
		RunCheckMethod,
		RunReadMethod,
		Sequncetabelle,
		ParameterTabelleFertig, 
		RunStart,
		RunEnd,
		RunError,
		Runlaufend
	};

protected:

	static unsigned int m_TaskbarCreatedMsg;
	bool m_IsTaskbarIcon;
	bool m_IsMinimized;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();

	void SetInfoText(CString Text);
	void SetInfoTextExtern(CString Text);
	LRESULT OnSetInfoText(WPARAM wParam, LPARAM lParam);

	void TickMassHunter();
	void TickStatus();

	struct _Lims
	{	_Lims()
			: Port(0)
		{
		}
		CString NicToUse;
		unsigned int Port;
		std::shared_ptr<SamiControl::CLIMSInterfaceThread> WebService;
		std::shared_ptr<boost::thread> Thread;
		std::shared_ptr<COperationContainer> LastOperation;	// used for Status Polling (after a method completed)
	} m_Lims;
	void ConnectLims();
	void DisconnectLims();

	std::list<CProject> Lims_GetProjects(const std::string& Device);
	std::list<CMethod> Lims_GetMethods(const std::string& Device, const std::string& Project);
	std::string Lims_GetStatus(const std::string& Device, const std::string& Project, const std::string& Method);
	std::string Lims_StartMethod(const std::string& Device, const std::string& Project, const std::string& Method, q0__SAMIMethodParameter Parameter);
	std::list<CPosition> Lims_GetPositions(const std::string& Device, const std::string& Project, const std::string& Method);
	void ReportRunToLims(std::shared_ptr<COperationContainer> Run);
	void LimsReportRunHandler(boost::system::error_code Error, CPMSActionData* PmsData);

	// -------------------------------------- SAMI Interface -------------------------------------------------------------
	struct _Sami
	{	_Sami()
			: Port(0)
		{
		}
		CString NicToUse;
		unsigned int Port;
		std::shared_ptr<MassHunter::CMassHunterServer> Service;
		boost::asio::io_service IoService;
		std::shared_ptr<boost::thread> Thread;
		std::shared_ptr<COperationContainer> LastOperation;	// used for Status Polling (after a method completed)
	} m_Sami;
	void ConnectSami();
	void DisconnectSami();

	bool Sami_GetProjectList(std::list<CString>& List);
	bool Sami_GetMethodList(const CString& Project, std::list<CString>& List);
	bool Sami_GetMasterBatchList(const CString& Project, std::list<CString>& List);
	bool Sami_GetStatus(CString& Text, int& Code);
	bool Sami_StartMethod(CString& ProjectPath, const CString& MasterBatch, 
		                                         const CString& MethodName,
												const CString& DataPath,

											const std::list<CommandHandler::CPositionData>& Labware,
		//const ::CommandHandler::CPositionData& PositionName,
		//const CString& PositionName,

	const boost::posix_time::time_duration Timeout);
	bool Sami_Abort();
	// --------------------------------------------------------------------------------------------------------------------

	CProgressCtrl m_RunProgress;
	enum
	{	MethodsMsgOffset = 100
	};
	Graphics::CColorList m_MassHunterProjectsList;
	Graphics::CColorList m_MassHunterMethodsList;
	CStatusBar  m_StatusBar;
	CString m_PmsMachineID;
	struct _MassHunterOptions
	{	CString ProjectPath;
		CString DataPath;
	} m_MassHunterOptions;
	HICON m_hIcon;
	bool m_OperationSchedulingStopped;
	BOOL m_ShowExecutiveStatus;
	BOOL m_AutoReplyToDialogs;
	HICON m_RedSignal, m_GreenSignal, m_GraySignal;
	CFont m_StatusFont;
	//***********
	CStatic m_StatusIndicator;

	union
	{	unsigned int UInt;
		struct
		{ unsigned int Running : 1;
		  unsigned int MassHunterLoaded : 1;
		  unsigned int MassHunterError : 1;
		  unsigned int WebServiceStarted : 1;
		  unsigned int WebServiceError : 1;
		  unsigned int SamiServiceStarted : 1;
		  unsigned int SamiServiceError : 1;
		};
	} m_Status;
	void RefreshStatus();

	boost::recursive_mutex m_Mutex;

	Logger::CFileLogger m_Logger;

	std::shared_ptr<COperationContainer> m_CurrentRunOperation;
	RunState m_RunState;

	std::list<std::shared_ptr<COperationContainer>> m_Operations;

	void Shutdown();
	bool m_ShuttingDown;

	std::shared_ptr<SamiControl::CMassHunterInterface> m_MassHunter;
	bool m_MassHunterIsReadyToRun;


	
	bool StartMethodRun(CRunOperationContainer& ROC);

	LRESULT OnMethodRunComplete(WPARAM wParam, LPARAM lParam);
	LRESULT OnMassHunterStatus(WPARAM wParam, LPARAM lParam);

	LRESULT OnRunMethod(WPARAM wParam, LPARAM lParam);
	LRESULT OnWriteMethod(WPARAM wParam, LPARAM lParam);
	LRESULT OnProjectSelectionChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnMethodSelectionChanged(WPARAM wParam, LPARAM lParam);

	LRESULT OnNotifyTaskbarCreated(WPARAM wParam, LPARAM lParam);
	LRESULT OnTaskBarIconClick(WPARAM wParam, LPARAM lParam);
	void TaskBarAddIcon(HICON hicon, const CString& lpszTip);
	void TaskBarIconText(const CString& lpszTip);
	void TaskBarDeleteIcon();

	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnSize(UINT nType, int cx, int cy);
	void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void OnTimer(UINT_PTR nIDEvent);
	void OnBnClickedShowExecutiveStatus();
	void OnBnClickedAutomaticReplyToDialogs();
	void OnBnClickedPauseMassHunter();
	void OnBnClickedResumeMassHunter();
	void OnBnClickedAbortMassHunter();
	void OnPaint();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedStaticMhStatus();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	afx_msg void OnStnClickedStaticMhStatusIndicator();
	afx_msg void OnBnClickedShowProjectPath();
	afx_msg void OnBnClickedShowDataPath();
	afx_msg void OnStnClickedPathDisplay();
};
