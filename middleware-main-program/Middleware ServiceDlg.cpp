#include "stdafx.h"
#include <WinSock2.h>
#include <ws2tcpip.h>

#include "PMS@LSA MassHunter ServiceDlg.h"
#include "Options.h"
#include "MemDC.h"
#include "..\Version Info\Version Info.h"
#include "..\Utf8\utf8.h"
#include "..\Agilent\ImageIDs.h"
#include <locale.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <UIAutomation.h>
#include <oleauto.h>  
#include <tlhelp32.h>  
#include <set>        
#include <thread>   

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

#include <limits>

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif


using namespace boost;
using namespace PMSLSA_LSA;
using namespace SamiControl;


class CAboutMassHunterServiceDialog : public CDialog
{
public:
	CAboutMassHunterServiceDialog();

	enum
	{ IDD = IDD_ABOUTBOX
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX); 

	DECLARE_MESSAGE_MAP()
};

CAboutMassHunterServiceDialog::CAboutMassHunterServiceDialog() : CDialog(CAboutMassHunterServiceDialog::IDD)
{
}

void CAboutMassHunterServiceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);


}

BEGIN_MESSAGE_MAP(CAboutMassHunterServiceDialog, CDialog)
	ON_WM_CTLCOLOR()

END_MESSAGE_MAP()


unsigned int CPmsLsaMassHunterServiceDialog::m_TaskbarCreatedMsg = RegisterWindowMessage(L"TaskbarCreated");

BEGIN_MESSAGE_MAP(CPmsLsaMassHunterServiceDialog, CDialog)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()

	ON_MESSAGE(MSG_SET_INFO_TEXT, &CPmsLsaMassHunterServiceDialog::OnSetInfoText)

	ON_MESSAGE(SamiControl::CMethodRun::MsgHandlerReply, &CPmsLsaMassHunterServiceDialog::OnMethodRunComplete)
	
	ON_MESSAGE(Graphics::CColorList::MsgSelectionChanged, &CPmsLsaMassHunterServiceDialog::OnProjectSelectionChanged)
	ON_MESSAGE(Graphics::CColorList::MsgRun + MethodsMsgOffset, &CPmsLsaMassHunterServiceDialog::OnRunMethod)
	ON_MESSAGE(Graphics::CColorList::MsgWrite + MethodsMsgOffset, &CPmsLsaMassHunterServiceDialog::OnWriteMethod)
	ON_MESSAGE(Graphics::CColorList::MsgSelectionChanged + MethodsMsgOffset, &CPmsLsaMassHunterServiceDialog::OnMethodSelectionChanged)

	ON_BN_CLICKED(IDC_SHOW_EXECUTIVE_STATUS, &CPmsLsaMassHunterServiceDialog::OnBnClickedShowExecutiveStatus)
	ON_BN_CLICKED(IDC_ANSWER_DIALOGS, &CPmsLsaMassHunterServiceDialog::OnBnClickedAutomaticReplyToDialogs)

	ON_BN_CLICKED(IDC_PAUSE_MASSHUNTER, &CPmsLsaMassHunterServiceDialog::OnBnClickedPauseMassHunter)
	ON_BN_CLICKED(IDC_RESUME_MASSHUNTER, &CPmsLsaMassHunterServiceDialog::OnBnClickedResumeMassHunter)
	ON_BN_CLICKED(IDC_ABORT_MASSHUNTER, &CPmsLsaMassHunterServiceDialog::OnBnClickedAbortMassHunter)

	ON_REGISTERED_MESSAGE(m_TaskbarCreatedMsg, &CPmsLsaMassHunterServiceDialog::OnNotifyTaskbarCreated)
	ON_MESSAGE(MSG_TASKBAR_ICON_CLICK, &CPmsLsaMassHunterServiceDialog::OnTaskBarIconClick)
	ON_STN_CLICKED(IDC_STATIC_MH_STATUS, &CPmsLsaMassHunterServiceDialog::OnStnClickedStaticMhStatus)
	ON_STN_CLICKED(IDC_STATIC_MH_STATUS2, &CPmsLsaMassHunterServiceDialog::OnStnClickedStaticMhStatus)

	ON_BN_CLICKED(IDC_ABORT_MASSHUNTER, &CPmsLsaMassHunterServiceDialog::OnBnClickedAbortMassHunter)



	


	ON_BN_CLICKED(IDC_SHOW_PROJECT_PATH, &CPmsLsaMassHunterServiceDialog::OnBnClickedShowProjectPath)
	ON_BN_CLICKED(IDC_SHOW_DATA_PATH, &CPmsLsaMassHunterServiceDialog::OnBnClickedShowDataPath)

	//ON_STN_CLICKED(IDC_PATH_DISPLAY, &CPmsLsaMassHunterServiceDialog::OnStnClickedPathDisplay)
END_MESSAGE_MAP()


#define ID_INDICATOR_ERRORTEXT 3400

static UINT indicators[] =
{
	ID_INDICATOR_ERRORTEXT
};

const wchar_t* RegConfigName = L"Settings";



CPmsLsaMassHunterServiceDialog::CPmsLsaMassHunterServiceDialog(CWnd* pParent)
	: CDialog(CPmsLsaMassHunterServiceDialog::IDD, pParent)
	, m_PmsMachineID(L"XX")
#ifndef NDEBUG
	, m_Logger(L"PMS@LSA MassHunter Service.Debug.log.txt")
#else
	, m_Logger(L"PMS@LSA MassHunter Service.Release.log.txt")
#endif
	, m_MassHunterIsReadyToRun(false)
	, m_RunState(RunIdle)
	, m_OperationSchedulingStopped(AfxGetApp()->GetProfileInt(RegConfigName, L"Scheduling Stopped", 0) != 0)
	, m_ShowExecutiveStatus(AfxGetApp()->GetProfileInt(RegConfigName, L"Show Executive Window", 0) != 0)
	, m_AutoReplyToDialogs(AfxGetApp()->GetProfileInt(RegConfigName, L"Auto Reply To Dialogs", 0) != 0)
	, m_MassHunterProjectsList(false, 0)
	, m_MassHunterMethodsList(true, MethodsMsgOffset, &m_MassHunterProjectsList)
	, m_ShuttingDown(false)
	, m_IsTaskbarIcon(false)
	, m_IsMinimized(false)
{
	CString LogFilePath;
	if (!SHGetSpecialFolderPath(NULL, LogFilePath.GetBuffer(MAX_PATH + 1), CSIDL_COMMON_DOCUMENTS, false))
	{	GetCurrentDirectory(MAX_PATH, LogFilePath.GetBuffer());
	}
	LogFilePath.ReleaseBuffer();
	if (LogFilePath.Right(1)!=L'\\') LogFilePath+=L'\\';
	CreateDirectory(LogFilePath, NULL);
	LogFilePath+=AfxGetApp()->m_pszAppName;
	CreateDirectory(LogFilePath, NULL);
	LogFilePath+=L"\\Logfiles";
	CreateDirectory(LogFilePath, NULL);
	LogFilePath+=L"\\";
	m_Logger.NewLogFilePath(LogFilePath);

	m_Status.UInt = 0;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_GreenSignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_GREEN_16x16);
	m_RedSignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_RED_16x16);
	m_GraySignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_GRAY_16x16);
	m_StatusFont.CreatePointFont(80, L"Arial");
}

CPmsLsaMassHunterServiceDialog::~CPmsLsaMassHunterServiceDialog()
{
	m_Logger.LogText(L"Program end");
}

void CPmsLsaMassHunterServiceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MASSHUNTERI_PROJECT_LIST, m_MassHunterProjectsList);
	DDX_Control(pDX, IDC_MASSHUNTERI_METHOD_LIST, m_MassHunterMethodsList);
	DDX_Check(pDX, IDC_SHOW_EXECUTIVE_STATUS, m_ShowExecutiveStatus);
	DDX_Check(pDX, IDC_ANSWER_DIALOGS, m_AutoReplyToDialogs);
	DDX_Control(pDX, IDC_RUN_PROGRESS, m_RunProgress);
	//DDX_Control(pDX, IDC_STATIC_MH_STATUS_INDICATOR, m_StatusIndicator);
	DDX_Control(pDX, IDC_PATH_DISPLAY, m_PathDisplay);



}

void CPmsLsaMassHunterServiceDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0)==IDM_ABOUTBOX)
	{	CAboutMassHunterServiceDialog dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0)==IDM_OPTIONS_DIALOG)

	{	CPmsLsaMassHunterServiceOptions Options;
	    Options.SetMassHunterInterface(m_MassHunter.get());  // Setze den Pointer!
		Options.SetMassHunterInterface2(m_MassHunter.get());  // Setze den Pointer!

		if (Options.DoModal()==IDOK)
		{	m_Logger.LogText(L"Settings changed - restart components");
			KillTimer(1);
			OnTimer(2);
		}
	}
	else
	{	CDialog::OnSysCommand(nID, lParam);
	}
}

BOOL CPmsLsaMassHunterServiceDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString status = m_MassHunter->GetInstrumentStatus();
	CString displayText = L"Instrument Status: " + status;
	GetDlgItem(IDC_STATIC_MH_STATUS)->SetWindowText(displayText);

	CString status2 = m_MassHunter->GetRunStatus();
	CString displayText2 = L"Run Status: " + status2;
	GetDlgItem(IDC_STATIC_MH_STATUS2)->SetWindowText(displayText2);


	ASSERT((IDM_ABOUTBOX & 0xFFF0)==IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX<0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu!=NULL)
	{	pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, L"&About PMS@LSA MassHunter Service...");
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_OPTIONS_DIALOG, L"&Options");
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_StatusBar.Create(this);
	m_StatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_StatusBar.SetPaneInfo(0, ID_INDICATOR_ERRORTEXT, SBPS_STRETCH, 0);
 
	CString Name(AfxGetApp()->m_pszAppName);
	CVersionInfo Version;
	CString Text(L" Version: " + Version.VersionAsString());
#ifndef NDEBUG
	SetInfoText(Name + Text + L" Debug");
#else
	SetInfoText(Name + Text + L" Release");
#endif

	CRect R;
	m_MassHunterProjectsList.GetClientRect(&R);
	m_MassHunterProjectsList.InsertColumn(0, L"Projects", LVCFMT_LEFT, R.Width() - 16);
	m_MassHunterMethodsList.GetClientRect(&R);
	m_MassHunterMethodsList.InsertColumn(0, L"Methoden", LVCFMT_LEFT, R.Width() - 16);

	int x = AfxGetApp()->GetProfileInt(RegConfigName, L"MWX", 100);
	x = (std::min)(x, GetSystemMetrics(SM_CXSCREEN));
	int y = AfxGetApp()->GetProfileInt(RegConfigName, L"MWY", 100);
	y = (std::min)(y, GetSystemMetrics(SM_CYSCREEN));
	int w = AfxGetApp()->GetProfileInt(RegConfigName, L"MWW", 648);
	w = (std::min)(w, GetSystemMetrics(SM_CYSCREEN));
	int h = AfxGetApp()->GetProfileInt(RegConfigName, L"MWH", 400);
	h = (std::min)(h, GetSystemMetrics(SM_CYSCREEN));
	SetWindowPos(0, x, y, w, h, SWP_NOZORDER);

	SetInfoText(L"Loading PMS/MassHunter .NET Components ...");
	SetTimer(2, 40, nullptr);
	SetTimer(3, 5000, NULL); // Timer-ID = 3, alle 5000 ms
	//SetTimer(4, 30000, NULL);
	return TRUE;
}


void CPmsLsaMassHunterServiceDialog::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 648;
	lpMMI->ptMinTrackSize.y = 280;

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CPmsLsaMassHunterServiceDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_MassHunterProjectsList.m_hWnd)
	{ CRect R;
		m_MassHunterProjectsList.GetWindowRect(&R);
		ScreenToClient(&R);
		R.bottom = cy - 16 - 12;
		m_MassHunterProjectsList.MoveWindow(&R);
	}
	if (m_MassHunterMethodsList.m_hWnd)
	{ CRect R;
		m_MassHunterMethodsList.GetWindowRect(&R);
		ScreenToClient(&R);
		R.bottom = cy - 16 - 12;
		m_MassHunterMethodsList.MoveWindow(&R);
	}
	CWnd* X = GetDlgItem(IDOK);
	if (X)
	{	CRect R;
		R.bottom = cy - 20;
		R.top = R.bottom - 24;
		R.right = cx - 12;
		R.left = R.right - 40;
		X->MoveWindow(&R);
	}
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, ID_INDICATOR_ERRORTEXT);
	
	if (SIZE_MINIMIZED==nType)
	{	ShowWindow(SW_HIDE);
		if (!m_IsMinimized)
		{	CString T;
			m_StatusBar.GetPaneText(0, T);
			TaskBarAddIcon(AfxGetApp()->LoadIcon(IDI_TASKBAR), T);
			m_IsMinimized = true;
		}
		return;
	}
	if (m_IsMinimized)
	{	TaskBarDeleteIcon();
		m_IsMinimized = false;
	}
}

void CPmsLsaMassHunterServiceDialog::TaskBarAddIcon(HICON hicon, const CString& lpszTip)
{
	NOTIFYICONDATA tnid{0};
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_hWnd;     
	tnid.uID = 0; 
	tnid.uFlags =  NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	tnid.uCallbackMessage = MSG_TASKBAR_ICON_CLICK;     
	tnid.hIcon = hicon; 
	if (!lpszTip.IsEmpty()) lstrcpyn(tnid.szTip, lpszTip.Left(127), sizeof(tnid.szTip) / sizeof(WCHAR)); 
	else tnid.szTip[0] = 0;  
	Shell_NotifyIcon(NIM_ADD, &tnid);      
	if (hicon) DestroyIcon(hicon);
	m_IsTaskbarIcon = true;
}

void CPmsLsaMassHunterServiceDialog::TaskBarIconText(const CString& lpszTip)
{
	NOTIFYICONDATA tnid{0};
	_ASSERTE(sizeof(tnid.szTip)==128*sizeof(wchar_t));
	if (!m_IsTaskbarIcon) return;
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_hWnd;     
	tnid.uID = 0; 
	tnid.uFlags =  NIF_TIP; 
	if (!lpszTip.IsEmpty()) lstrcpyn(tnid.szTip, lpszTip.Left(127), sizeof(tnid.szTip) / sizeof(WCHAR)); 
	else tnid.szTip[0] = 0;  
	Shell_NotifyIcon(NIM_MODIFY, &tnid);      
}

void CPmsLsaMassHunterServiceDialog::TaskBarDeleteIcon()
{	  
	NOTIFYICONDATA tnid{0};  
	tnid.cbSize = sizeof(NOTIFYICONDATA);     
	tnid.hWnd = m_hWnd; 
	tnid.uID = 0;              
	Shell_NotifyIcon(NIM_DELETE, &tnid); 
	m_IsTaskbarIcon = false;
}

LRESULT CPmsLsaMassHunterServiceDialog::OnNotifyTaskbarCreated(WPARAM wParam, LPARAM lParam)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::OnNotifyTaskbarCreated\n");
	CString wndTxt;
	GetWindowText(wndTxt);
	TaskBarAddIcon(AfxGetApp()->LoadIcon(IDI_TASKBAR), wndTxt);
  return Default();
}

LRESULT CPmsLsaMassHunterServiceDialog::OnTaskBarIconClick(WPARAM wParam, LPARAM lParam)  
{	
	switch (lParam)
	{	case WM_LBUTTONDOWN		:	
		case WM_LBUTTONDBLCLK	:	
									if (m_IsMinimized)
									{	TaskBarDeleteIcon();
										m_IsMinimized = false;
									}
									ShowWindow(SW_RESTORE);
									SetForegroundWindow();
									break;
		case WM_RBUTTONDOWN		:	
									{	//SetForegroundWindow();
										CMenu Menue;
										Menue.CreatePopupMenu();
										Menue.AppendMenu(MF_STRING, 1, L"Options");
										Menue.AppendMenu(MF_STRING, IDCANCEL, L"Close");
										CPoint Point;
										GetCursorPos(&Point);
										switch (Menue.TrackPopupMenu(/*TPM_RETURNCMD | */TPM_TOPALIGN | TPM_LEFTALIGN, Point.x, Point.y, this))
										{	case	1	:	//OnSysCommand(IDD_OPTIONEN_DIALOG, 0);	
															break;
											case	IDCANCEL	:	OnCancel();
															break;
										}
									}
									break;
	}
	return 1; 
}

void CPmsLsaMassHunterServiceDialog::Shutdown()
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Shutdown\n");
	UpdateData();
	m_ShuttingDown = true;

	if (m_Lims.WebService) DisconnectLims();
	
	if (m_Sami.Service) DisconnectSami();

	if (m_MassHunter)
	{	
		AfxGetApp()->WriteProfileInt(RegConfigName, L"Show Executive Window", m_ShowExecutiveStatus);
		AfxGetApp()->WriteProfileInt(RegConfigName, L"Auto Reply To Dialogs", m_AutoReplyToDialogs);
	}
	AfxGetApp()->WriteProfileInt(RegConfigName, L"Scheduling Stopped", m_OperationSchedulingStopped);

	CRect R;
	GetWindowRect(&R);
	if (!m_IsMinimized && R.left>=0 && R.top>=0)
	{	AfxGetApp()->WriteProfileInt(RegConfigName, L"MWX", R.left);
		AfxGetApp()->WriteProfileInt(RegConfigName, L"MWW", R.Width());
		AfxGetApp()->WriteProfileInt(RegConfigName, L"MWY", R.top);
		AfxGetApp()->WriteProfileInt(RegConfigName, L"MWH", R.Height());
		if (m_MassHunter && m_ShowExecutiveStatus) m_MassHunter->SavePosition(RegConfigName);
	}
	if (m_MassHunter) m_MassHunter->DestroyWindow();

	TaskBarDeleteIcon();
	CDialog::OnClose();
}

void CPmsLsaMassHunterServiceDialog::OnCancel()
{
	Shutdown();
	CDialog::OnCancel();
}

void CPmsLsaMassHunterServiceDialog::OnOK()
{
	Shutdown();
	CDialog::OnOK();
}

void CPmsLsaMassHunterServiceDialog::OnTimer(UINT_PTR nIDEvent)
{
	

	if (nIDEvent==1)
	{	KillTimer(nIDEvent);
		_ASSERTE(!m_Sami.IoService.stopped());
		if (m_MassHunter) TickMassHunter();
		TickStatus();
		SetTimer(1, 500, NULL);
	}
	else if (nIDEvent==2)
	{	KillTimer(nIDEvent);
		UpdateData();
		CString StartInfo(L"0..");
		try
		{	DisconnectLims();
			DisconnectSami();

			m_PmsMachineID = AfxGetApp()->GetProfileString(RegConfigName, L"PMS Machine ID", L"M00000");
			
			if (m_MassHunter)
			{	MSG Msg;
				int MaxMsg = 100;
				while (--MaxMsg && PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
				{	TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
				if (m_MassHunter->m_hWnd) m_MassHunter->DestroyWindow();
				m_MassHunter.reset();
			}

			m_MassHunterOptions.ProjectPath = AfxGetApp()->GetProfileString(RegConfigName, L"MassHunter Project Path", L".");
			m_MassHunterOptions.DataPath = AfxGetApp()->GetProfileString(RegConfigName, L"MassHunter Data Path", L".");
			m_Logger.LogText(L"MassHunter project path: " + m_MassHunterOptions.ProjectPath);
			m_Logger.LogText(L"MassHunter data path: " + m_MassHunterOptions.DataPath);
			if (!m_MassHunter) m_MassHunter.reset(new SamiControl::CMassHunterInterface((m_MassHunterOptions.ProjectPath), (m_MassHunterOptions.DataPath)));
			//
			StartInfo+=L"1..";
			m_MassHunter->SetOwner(this);
			CRect R(m_MassHunter->LoadPosition(RegConfigName));
			m_MassHunter->OpenWindow(L"MassHunter Status", R, this, m_ShowExecutiveStatus!=0);
			m_MassHunter->LoadPosition(RegConfigName);
			m_MassHunter->SetReplyToDialogs(m_AutoReplyToDialogs!=0);

			StartInfo+=L"2..";
			m_MassHunterProjectsList.DeleteAllItems();
			if (m_MassHunter->ReadProjectList())
			{	for (std::list<std::pair<CString, long>>::iterator i(m_MassHunter->m_ProjectList.begin());i!=m_MassHunter->m_ProjectList.end();++i)
				{	m_MassHunterProjectsList.InsertItem(m_MassHunterProjectsList.GetItemCount(), i->first);
				}
			}
			StartInfo+=L"3..";
			CString T(AfxGetApp()->GetProfileString(RegConfigName, L"Project"));
			LVFINDINFO lvfi{0};
			lvfi.flags = LVFI_STRING;
			lvfi.psz = (LPCTSTR)T;
			int Index = m_MassHunterProjectsList.FindItem(&lvfi);
			if (Index!=-1)
			{	m_MassHunterProjectsList.EnsureVisible(Index, false);
				m_MassHunterProjectsList.SetItemState(Index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				T = AfxGetApp()->GetProfileString(RegConfigName, L"Method");
				lvfi.psz = (LPCTSTR)T;
				Index = m_MassHunterMethodsList.FindItem(&lvfi);
				if (Index!=-1)
				{	m_MassHunterMethodsList.EnsureVisible(Index, false);
					m_MassHunterMethodsList.SetItemState(Index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				}
			}
			StartInfo+=L"4..";
			m_MassHunterIsReadyToRun = m_MassHunter->ErrorText().IsEmpty();
			StartInfo+=L"5..";
			m_Status.MassHunterLoaded = true;
			m_Status.MassHunterError = !m_MassHunterIsReadyToRun;

			StartInfo+=L"6..";

			StartInfo+=L"7..";
			ConnectLims();
			
			StartInfo+=L"8..";
			ConnectSami();

			StartInfo+=L"9..";

			m_Logger.LogText(StartInfo + L"Ok");
			SetInfoText(L"basic startup complete");
			SetTimer(1, 500, nullptr);
		}
		catch (CString& E)
		{
			SetInfoText(L"Exception: " + StartInfo + L" - " + E);
		}
		catch (...)
		{
			SetInfoText(L"Exception: " + StartInfo);
		}
	}

	else if (nIDEvent == 3)
	{
		if (m_MassHunter)
		{
			CString status = m_MassHunter->GetInstrumentStatus();
			GetDlgItem(IDC_STATIC_MH_STATUS)->SetWindowText(L"Instrument Status: " + status);

			CString status2 = m_MassHunter->GetRunStatus();
			GetDlgItem(IDC_STATIC_MH_STATUS2)->SetWindowText(L"Run Status: " + status2);
		}
	}
	
	else if (nIDEvent == 4)
	{
		if (m_CurrentRunOperation)
		{
			HWND hwnd = NULL;
			EnumWindows([](HWND h, LPARAM lParam) -> BOOL {
				wchar_t title[256];
				::GetWindowTextW(h, title, 256);
				if (wcsstr(title, L"GC Status")) {
					*((HWND*)lParam) = h;
					return FALSE;
				}
				return TRUE;
				}, (LPARAM)&hwnd);

			if (hwnd != NULL) {
				m_CurrentRunOperation->ErrorText = L"Das Vial-Nummer ist falsch ";
				KillTimer(4);
				m_RunState = RunError;
				return;
			}
			else
			{

				CString guiStatus = m_MassHunter->GetInstrumentStatus();
				CString runStatus = m_MassHunter->GetRunStatus();

				if (guiStatus == L"Ready" && runStatus == L"Idle")
				{
					CTime now = CTime::GetCurrentTime();
					if (m_LastRunStartTime != 0 && now - m_LastRunStartTime > CTimeSpan(0, 0, 5, 0))  // über 2 Minute
					{

						m_CurrentRunOperation->ErrorText = L"Die Messung ist fertig..";
						SetInfoText(m_CurrentRunOperation->ErrorText);
						//Fertig == L"Die Messung ist fertig.";
						//SetInfoText(Fertig);
						// Timer beenden und ggf. Run-Status zurücksetzen
						KillTimer(4);
						m_RunState = RunEnd;
					}
				}
			}
		}
		}


	
	
		
		
		

		


	CDialog::OnTimer(nIDEvent);
}

void CPmsLsaMassHunterServiceDialog::SetInfoText(CString Text)
{
	_ASSERTE(!Text.IsEmpty());
	m_Logger.LogText(Text);
	Text = CTime::GetCurrentTime().Format(L"%c ") + Text;
	m_StatusBar.SetPaneText(0, Text);
	TRACE(Text + L"\n");
}

LRESULT CPmsLsaMassHunterServiceDialog::OnSetInfoText(WPARAM wParam, LPARAM /*lParam*/)
{
	CString* Text = (CString*)wParam;
	if (Text)
	{	SetInfoText(*Text);
		delete Text;
	}
	return 0;
}

void CPmsLsaMassHunterServiceDialog::SetInfoTextExtern(CString Text)
{
	CString* X = new CString(Text);
	PostMessage(MSG_SET_INFO_TEXT, (WPARAM)X);
}

LRESULT CPmsLsaMassHunterServiceDialog::OnProjectSelectionChanged(WPARAM wParam, LPARAM /*lParam*/)
{
	// wParam = Item index, lParam = Item data
	//::AfxMessageBox(L"Projekt wurde ausgewählt!");

	CString P(m_MassHunterProjectsList.GetItemText(int(wParam), 0));
	m_MassHunter->ReadMethodsForProject(P);
	m_MassHunterMethodsList.DeleteAllItems();
	for (std::list<CString>::iterator i(m_MassHunter->m_MethodList.begin());i!=m_MassHunter->m_MethodList.end();++i)
	{	m_MassHunterMethodsList.InsertItem(m_MassHunterMethodsList.GetItemCount(), *i);
	}
	AfxGetApp()->WriteProfileString(RegConfigName, L"Project", P);
	return 0;
	//**********+

	int sel = m_MassHunterProjectsList.GetNextItem(-1, LVNI_SELECTED);
	if (sel != -1)
	{
		CString P = m_MassHunterProjectsList.GetItemText(sel, 0);

		// Methodenliste lesen
		m_MassHunter->ReadMethodsForProject(P);

		// GUI Worklist (rechte Liste) leeren und neu füllen
		m_MassHunterMethodsList.DeleteAllItems();
		for (std::list<CString>::iterator i = m_MassHunter->m_MethodList.begin(); i != m_MassHunter->m_MethodList.end(); ++i)
		{
			m_MassHunterMethodsList.InsertItem(m_MassHunterMethodsList.GetItemCount(), *i);
		}
	}


}

LRESULT CPmsLsaMassHunterServiceDialog::OnMethodSelectionChanged(WPARAM wParam, LPARAM /*lParam*/)
{
	CString M(m_MassHunterMethodsList.GetItemText(int(wParam), 0));
	m_MassHunter->GetMethod(M, 0);
	AfxGetApp()->WriteProfileString(RegConfigName, L"Method", M);
	return 0;
}

bool CPmsLsaMassHunterServiceDialog::StartMethodRun(CRunOperationContainer& ROC)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::StartMethodRun\n");
	SetInfoText(L"Start Run " + ROC.MethodName + L" as " + ROC.Run->NewBatchFileName);
	// if we have data from PMS - we already have labware information
	// where we add some more information
	// if we come from a local run - we have to add the whole labware data
	for (std::list<SamiControl::CPositionDescriptor>::const_iterator i(ROC.Schedule->Positions.begin());i!=ROC.Schedule->Positions.end();++i)
	{	for (std::vector<SamiControl::CBaseLabwareData>::const_iterator j(i->Labware.begin());j!=i->Labware.end();++j)
		{	bool Found = false;
			for (std::list<SamiControl::CBaseLabwareData>::iterator k(ROC.Run->Labware.begin());k!=ROC.Run->Labware.end();++k)
			{	if (j->Name==k->Name)
				{	k->ClassName = j->ClassName;
					k->Height = j->Height;
					k->Position = i->PositionName;
					Found = true;
					break;
				}
			}
			if (Found) continue;
			ROC.Run->Labware.push_back(*j);
		}
	}
	
	SamiControl::CMethodRun* MR = new SamiControl::CMethodRun(*ROC.Run);
	MR->Handler = m_hWnd;

	if (!m_MassHunter->StartMethod/*2*/(MR, DWORD(ROC.ETC * 1000.0)))
	{	SetInfoText(m_MassHunter->ErrorText());
		return false;
	}
	m_Status.Running = true;
	RefreshStatus();
	return true;
}

LRESULT CPmsLsaMassHunterServiceDialog::OnMethodRunComplete(WPARAM wParam, LPARAM lParam)
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::OnMethodRunComplete\n");
	SamiControl::CMethodRun* MR = (SamiControl::CMethodRun*)lParam;
	_ASSERTE(MR && wParam==MR->JobID);
	m_Status.Running = false;
	RefreshStatus();
	if (!m_CurrentRunOperation)
	{	// local start run - no PMS
		m_RunState = RunIdle;
		delete MR;
		return 0;
	}
	CRunOperationContainer* ROC = dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get());
	_ASSERTE(ROC);
	ROC->Run.reset(new SamiControl::CMethodRun(*MR));
	if (!MR->Success)
	{	m_CurrentRunOperation->ErrorText = L"Run failed: " + MR->ErrorText;
		SetInfoText(ROC->Run->ErrorText);
		delete MR;
		m_RunState = RunError;
		return 0;
	}
	/*
	CString T;
	T.Format(L"Run complete");
	SetInfoText(T);*/
	/*
	if (CString StatusMeldung = L"Idle")
	{
		CString ST;
		ST.Format(L"Messung ist fertig ");
		m_Logger.LogText(ST);
		SetInfoText(ST);
	}*/

	delete MR;
	
	//m_RunState = RunEnd;
	return 0;
}

LRESULT CPmsLsaMassHunterServiceDialog::OnMassHunterStatus(WPARAM wParam, LPARAM /*lParam*/)
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::OnMassHunterStatus\n");
	SamiControl::CExecutiveStatus* ES = (SamiControl::CExecutiveStatus*)wParam;
	if (!ES) return 0;
	m_RunProgress.SetPos(int(1000.0 * ES->Time / ES->ETC));
	delete ES;
	return 0;
}



// Version der 13.04.20205

void CPmsLsaMassHunterServiceDialog::TickMassHunter()
{
	//if (!m_MassHunterIsReadyToRun) return;

	// Kein MassHunter oder kein aktueller Auftrag → nichts zu tun
	if (!m_MassHunter || !m_CurrentRunOperation)
		return;

	CRunOperationContainer& X = *dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get());
	if(m_CurrentRunOperation){
	switch (m_RunState)
	{
	case RunReadProject:
		if (!m_MassHunter->ReadMethodsForProject(X.ProjectName))
		{
			m_CurrentRunOperation->ErrorText = L"Fehler beim Lesen des Projekts: " + X.ProjectName;
			SetInfoText(m_CurrentRunOperation->ErrorText);
			m_RunState = RunError;
			break;
		}

		m_RunState = Runlaufend;
		break;

	
	case Runlaufend:
		SetInfoText(L"Starte RunSequence...");

		m_LastRunStartTime = CTime::GetCurrentTime();

		if (!m_MassHunter->RunSequence(X.Run.get(), 60000)) {
			m_CurrentRunOperation->ErrorText = m_MassHunter->ErrorText();
			SetInfoText(m_CurrentRunOperation->ErrorText);
			m_RunState = RunError;
			break;
		}
		else {

			m_CurrentRunOperation->ErrorText = L"Die Messung ist startet.";
			SetInfoText(m_CurrentRunOperation->ErrorText);
			m_Status.Running = true;
			RefreshStatus();
			//m_RunState = RunEnd;
			SetTimer(4, 30000, NULL);
			m_RunState = RunStart;


			break;
		}
	

	case	RunEnd: {
#ifndef NDEBUG
		try
		{
			CString T;
			T.Format(L"RunOperation%08I64X.xml", m_CurrentRunOperation->PmsId);
			m_CurrentRunOperation->Save(m_MassHunterOptions.DataPath + T);
			m_Logger.LogText(L"Saved Run to: " + m_MassHunterOptions.DataPath + T);
		}
		catch (...)
		{
		}
#endif
		if (m_CurrentRunOperation->PmsId)
		{
			if (m_CurrentRunOperation->OperationSource == COperationContainer::PmsOperation)
			{
			}
			
			else if (m_CurrentRunOperation->OperationSource == COperationContainer::SamiOperation)
			{	// Nothing to do
			}
		}

		

		m_CurrentRunOperation.reset();
		m_RunState = RunIdle;
	}
				  break;
	case	RunError: {
#ifndef NDEBUG
		try
		{
			CString T;
			T.Format(L"RunOperation%08I64X.xml", m_CurrentRunOperation->PmsId);
			m_CurrentRunOperation->Save(m_MassHunterOptions.DataPath + T);
			m_Logger.LogText(L"Saved Run to: " + m_MassHunterOptions.DataPath + T);
		}
		catch (...)
		{
		}
#endif
		if (m_CurrentRunOperation->PmsId)
		{
			if (m_CurrentRunOperation->OperationSource == COperationContainer::PmsOperation)
			{
			}
			
			else if (m_CurrentRunOperation->OperationSource == COperationContainer::SamiOperation)
			{	// Nothing to do
			}
		}
		m_CurrentRunOperation.reset();
		m_RunState = RunIdle;
	}
					break;
	}
}
}


//erste Version der 10.04.20205



void CPmsLsaMassHunterServiceDialog::OnBnClickedShowExecutiveStatus()
{
	UpdateData();
	if (m_MassHunter) m_MassHunter->ShowWindow(m_ShowExecutiveStatus?SW_SHOW:SW_HIDE);
}

void CPmsLsaMassHunterServiceDialog::OnBnClickedAutomaticReplyToDialogs()
{
	UpdateData();
	if (m_MassHunter) m_MassHunter->SetReplyToDialogs(m_AutoReplyToDialogs!=0);
}

void CPmsLsaMassHunterServiceDialog::OnBnClickedPauseMassHunter()
{
	UpdateData();
	if (m_MassHunter) m_MassHunter->Pause();
}

void CPmsLsaMassHunterServiceDialog::OnBnClickedResumeMassHunter()
{
	UpdateData();
	if (m_MassHunter) m_MassHunter->Resume();
}

void CPmsLsaMassHunterServiceDialog::OnBnClickedAbortMassHunter()
{
	UpdateData();
	if (m_MassHunter) m_MassHunter->Abort();
}

void CPmsLsaMassHunterServiceDialog::OnPaint()
{
	CPaintDC dc(this);
	dc.SetBkColor(GetSysColor(COLOR_3DFACE));
	CRect RES, R;
	GetDlgItem(IDC_SHOW_EXECUTIVE_STATUS)->GetWindowRect(&RES);
	ScreenToClient(&RES);
	{	CMemDC mdc(&dc);	
		mdc.SelectObject(m_StatusFont);
		mdc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
		mdc.SetBkColor(GetSysColor(COLOR_3DFACE));
		mdc.SetBkMode(TRANSPARENT);
		R.left = RES.left + 22;
		R.top = RES.bottom + 28;
		R.right = R.left + 100;
		R.bottom = R.top + 16;
		const int IconShift = -22;
		DrawIconEx(mdc, R.left + IconShift, R.top, m_Status.MassHunterError?m_RedSignal:(m_Status.MassHunterLoaded?m_GreenSignal:m_GraySignal), 16, 16, 0, NULL, DI_NORMAL);
		mdc.DrawText(L"MassHunter Loaded", -1, R, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);
		R.OffsetRect(0, 20);
		DrawIconEx(mdc, R.left + IconShift, R.top, m_Status.WebServiceError?m_RedSignal:(m_Status.WebServiceStarted?m_GreenSignal:m_GraySignal), 16, 16, 0, NULL, DI_NORMAL);
		mdc.DrawText(L"Webservice", -1, R, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);
		R.OffsetRect(0, 20);
		DrawIconEx(mdc, R.left + IconShift, R.top, m_Status.SamiServiceError?m_RedSignal:(m_Status.SamiServiceStarted?m_GreenSignal:m_GraySignal), 16, 16, 0, NULL, DI_NORMAL);
		mdc.DrawText(L"SAMI Service", -1, R, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);
		R.OffsetRect(0, 20);
		DrawIconEx(mdc, R.left + IconShift, R.top, m_Status.Running?m_GreenSignal:m_GraySignal, 16, 16, 0, NULL, DI_NORMAL);
		mdc.DrawText(L"Run Worklist", -1, R, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER);
	}
}


void CPmsLsaMassHunterServiceDialog::RefreshStatus()
{
	CRect RES, R;
	GetDlgItem(IDC_SHOW_EXECUTIVE_STATUS)->GetWindowRect(&RES);
	ScreenToClient(&RES);
	R.left = RES.left;
	R.top = RES.bottom + 8;
	R.right = RES.right;
	R.bottom = R.top + 16 + 20 * 5;
	InvalidateRect(R, false);
}

void CPmsLsaMassHunterServiceDialog::TickStatus()
{
}
	
LRESULT CPmsLsaMassHunterServiceDialog::OnRunMethod(WPARAM wParam, LPARAM lParam)
{
	if (!m_MassHunter) return 0;
	
	if (!m_MassHunterIsReadyToRun)
	{	SetInfoText(L"MassHunter is not ready to run a method");
		return 0;
	}

	_ASSERTE(!m_CurrentRunOperation);
	m_CurrentRunOperation.reset(new CRunOperationContainer());
	CRunOperationContainer& X = *dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get());

	X.Run.reset(new SamiControl::CMethodRun());
	X.Schedule.reset(new SamiControl::CMethodSchedule());
	X.ProjectName = *(CString*)wParam;
	X.Run->Project = X.ProjectName;
	X.Schedule->Project = X.ProjectName;
	X.ProjectID = 0;
	X.MethodName = *(CString*)lParam;
	X.MethodRevision = 0;
	X.Run->Method = X.MethodName;
	X.Schedule->Method = X.MethodName;
	X.Families = 1;
	delete (CString*)wParam;
	delete (CString*)lParam;

	int Index = m_MassHunterProjectsList.GetSelectionMark();
	if (Index>=0) X.Run->Project = m_MassHunterProjectsList.GetItemText(Index, 0);
	TRACE(L"CPmsLsaMassHunterServiceDialog::OnRunMethod: %s.%s\n", X.ProjectName, X.MethodName);

	X.Run->ProjectFileName = m_MassHunterOptions.ProjectPath + X.Run->Project;
	X.Run->MethodFileName = m_MassHunterOptions.ProjectPath + X.Run->Project + L"\\" + X.Run->Method;
	X.Run->NewBatchFileName = m_MassHunterOptions.DataPath + CTime::GetCurrentTime().Format(L"TODO NAME %Y-%m-%d-%H-%M-%S.b");

	if (Index>=0)
	{	m_RunState = RunReadProject;
	}
	else m_CurrentRunOperation.reset();
	return 0;
}

LRESULT CPmsLsaMassHunterServiceDialog::OnWriteMethod(WPARAM wParam, LPARAM lParam)
{
	if (!m_MassHunter) return 0;
	
	if (!m_MassHunterIsReadyToRun)
	{	SetInfoText(L"MassHunter is not ready to write a method");
		return 0;
	}

	SamiControl::CMethodRun X;
	X.Project = *(CString*)wParam;
	X.Method = *(CString*)lParam;
	
	delete (CString*)wParam;
	delete (CString*)lParam;

	int Index = m_MassHunterProjectsList.GetSelectionMark();
	if (Index>=0) X.Project = m_MassHunterProjectsList.GetItemText(Index, 0);
	TRACE(L"CPmsLsaMassHunterServiceDialog::OnWriteMethod: %s.%s\n", X.Project, X.Method);

	X.ProjectFileName = m_MassHunterOptions.ProjectPath + X.Project;
	X.MethodFileName = m_MassHunterOptions.ProjectPath + X.Project + L"\\" + X.Method;
	X.NewBatchFileName = m_MassHunterOptions.DataPath + CTime::GetCurrentTime().Format(L"TODO NAME %Y-%m-%d-%H-%M-%S.b");

	if (!m_MassHunter->WriteMethodToFile2(&X, 60000))
	{
		SetInfoText(m_MassHunter->ErrorText());
	}

	return 0;
}

std::list<CProject> CPmsLsaMassHunterServiceDialog::Lims_GetProjects(const std::string& Device)
{	
	TRACE(L"CTest::GetProjects: %S\n", Device.c_str());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString TL;
	TL.Format(L"Project list query from LIMS. Device: '%s'", (LPCTSTR)Utf8ToCString(Device));
	m_Logger.LogText(TL);

	std::list<CProject> X;
	// TODO: does the LIMS also use PmsID ?
	//if (Utf8ToCString(Device)==m_PmsMachineID)
	{	if (!m_MassHunter) return X;
		if (m_MassHunter->ReadProjectList())
		{	CProject P;
			P.Revision = 1;
			for (std::list<std::pair<CString, long>>::iterator i(m_MassHunter->m_ProjectList.begin());i!=m_MassHunter->m_ProjectList.end();++i)
			{	P.Name = CStringToUtf8(i->first);
				X.push_back(P);
			}
		}
	}
	return X;
}

std::list<CMethod> CPmsLsaMassHunterServiceDialog::Lims_GetMethods(const std::string& Device, const std::string& Project)
{
	TRACE(L"CTest::GetMethods: %S %S\n", Device.c_str(), Project.c_str());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString TL;
	TL.Format(L"Method list query from LIMS. Device: '%s' Project: '%s'",
				(LPCTSTR)Utf8ToCString(Device), (LPCTSTR)Utf8ToCString(Project));
	m_Logger.LogText(TL);

	std::list<CMethod> X;
	// TODO: does the LIMS also use PmsID ?
	//if (Utf8ToCString(Device)==m_PmsMachineID)
	{	if (!m_MassHunter) return X;
		if (m_MassHunter->ReadMethodsForProject(Utf8ToCString(Project)))
		{	CMethod M;
			M.Revision = 0;
			for (std::list<CString>::iterator i(m_MassHunter->m_MethodList.begin());i!=m_MassHunter->m_MethodList.end();++i)
			{	
				M.Name = CStringToUtf8(*i);
				X.push_back(M);
			}
		}
	}
	return X;
}

std::string CPmsLsaMassHunterServiceDialog::Lims_GetStatus(const std::string& Device, const std::string& Project, const std::string& Method)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::GetStatus: %S %S\n", Device.c_str(), Method.c_str());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString T;
	CString MethodName(Utf8ToCString(Method));
	CString ProjectName(Utf8ToCString(Project));
	T.Format(L"Status query from LIMS. Device: '%s' Project: '%s' Method: '%s'",
			 (LPCTSTR)Utf8ToCString(Device), (LPCTSTR)ProjectName, (LPCTSTR)MethodName);
	m_Logger.LogText(T);
	if (CRunOperationContainer* RO = dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get()))
	{	if (RO->OperationSource==COperationContainer::LimsOperation	&& RO->ProjectName==ProjectName  && RO->MethodName==MethodName)
		{	return "Running";
		}
		else
		{	if (m_Lims.LastOperation && m_Lims.LastOperation->ProjectName==ProjectName && m_Lims.LastOperation->MethodName==MethodName)
			{	if (m_Lims.LastOperation->ErrorText.IsEmpty())
				{	return "Complete";
				}
				return CStringToUtf8(L"Error: " + m_Lims.LastOperation->ErrorText);
			}
			return "Error: The current method run is not for method '" + Project + "." + Method + "'";
		}
	}
	else if (m_Lims.LastOperation && m_Lims.LastOperation->ProjectName==ProjectName && m_Lims.LastOperation->MethodName==MethodName)
	{	if (m_Lims.LastOperation->ErrorText.IsEmpty())
		{	if (CScheduleOperationContainer* SO = dynamic_cast<CScheduleOperationContainer*>(m_Lims.LastOperation.get()))
			{	CString T;
				T.Format(L"Complete: ETC: %0.2f", SO->ETC);
				return CStringToUtf8(T);
			}
			else if (CRunOperationContainer* RO = dynamic_cast<CRunOperationContainer*>(m_Lims.LastOperation.get()))
			{	return "Complete";
			}
		}
		return CStringToUtf8(L"Error: " + m_Lims.LastOperation->ErrorText);
	}

	return "Idle";
}

std::string CPmsLsaMassHunterServiceDialog::Lims_StartMethod(const std::string& Device, const std::string& Project, const std::string& Method, q0__SAMIMethodParameter Parameter)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::StartMethod: %S %S %S\n", Device.c_str(), Project.c_str(), Method.c_str());

	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString TL;
	TL.Format(L"Start method request from LIMS. Device: '%s' Project: '%s' Method: '%s'",
						(LPCTSTR)Utf8ToCString(Device), (LPCTSTR)Utf8ToCString(Project), (LPCTSTR)Utf8ToCString(Method));
	m_Logger.LogText(TL);


	// TODO: does the LIMS also use PmsID ?
	/*if (Utf8ToCString(Device)==m_PmsMachineID)
	{
		return "Error: Unknown device.";
	}*/

	if (m_OperationSchedulingStopped)
	{	return "Error: Stopped";
	}

	if (m_CurrentRunOperation)
	{	CRunOperationContainer* X = dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get());
		_ASSERTE(X);
		if (X) return "Error: MassHunter is busy with method '" + CStringToUtf8(X->MethodName) +  "'";
		else return "Error: MassHunter is busy";
	}

	if (!m_MassHunterIsReadyToRun)
	{	return "Error: MassHunter is not ready to run";
	}

	m_CurrentRunOperation.reset(new CRunOperationContainer());
	CRunOperationContainer& X = *dynamic_cast<CRunOperationContainer*>(m_CurrentRunOperation.get());

	X.CommandName(L"MassHunter: Run Method");
	X.Run.reset(new SamiControl::CMethodRun());
	X.Schedule.reset(new SamiControl::CMethodSchedule());
	X.ProjectName = Utf8ToCString(Project);
	X.ProjectID = 0;
	X.MethodName = Utf8ToCString(Method);
	X.MethodRevision = 0;
	X.Families = Parameter.q0__Families;
	X.OperationSource = COperationContainer::LimsOperation;
	X.PmsId = (std::numeric_limits<unsigned __int64>::max)();
	m_Lims.LastOperation = m_CurrentRunOperation;

	X.BaseFilePath = m_MassHunterOptions.ProjectPath;// + X.ProjectName + L"." + X.MethodName;

	m_RunState = RunReadProject;
	
	X.Run->Project = X.ProjectName;
	X.Run->Method = X.MethodName;

	X.Run->ProjectFileName = m_MassHunterOptions.ProjectPath + X.Run->Project;
	X.Run->MethodFileName = m_MassHunterOptions.ProjectPath + X.Run->Project + L"\\" + X.Run->Method;
	//if (DataPath.IsEmpty())
	{	X.Run->NewBatchFileName = m_MassHunterOptions.DataPath + CTime::GetCurrentTime().Format(L"TODO HWMS RUN NAME %Y-%m-%d-%H-%M-%S.b");
	}
	//else
	//{	X.Run->NewBatchFileName = m_MassHunterOptions.DataPath + DataPath;
	//}
	return "Ok: Running";
}

std::list<CPosition> CPmsLsaMassHunterServiceDialog::Lims_GetPositions(const std::string& Device, const std::string& Project, const std::string& Method)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Positions: %S %S %S\n", Device.c_str(), Project.c_str(), Method.c_str());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_Logger.LogText(L"Position list query from LIMS");
	std::list<CPosition> X;
	char T[20];
	for (unsigned int i=25;i<31;++i)
	{	CPosition Y;
		sprintf_s(T, 20, "Position-%02u", i);
		Y.Name = T;
		X.push_back(Y);
	}
	return X;
}

void CPmsLsaMassHunterServiceDialog::ReportRunToLims(std::shared_ptr<COperationContainer> Run)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::ReportRunToLims\n");
	_ASSERTE(Run);
	_ASSERTE(dynamic_cast<CLIMSInterface*>((CLIMSInterface*)Run->OperationSource));
	m_Lims.LastOperation = Run;
	if (Run->ErrorText.IsEmpty())
	{	CRunOperationContainer* X = dynamic_cast<CRunOperationContainer*>(Run.get());
		_ASSERTE(X);
		_ASSERTE(X->Run);
		Run->ErrorText = X->Run->ErrorText;
	}
	//TODO Send Message to LIMS
	if (Run->ErrorText.IsEmpty())
	{	/*CPMSActionData A(CPMSActionData::PMSCompleteOperationOrder, Run,
							bind(&CPmsLsaMassHunterServiceDialog::LimsReportRunHandler, this, _1, _2));
		A.OrderID = Run->PmsId;
		if (m_Pms.Server) m_Pms.Server->AddPMSAction(A);*/
	}
	else
	{	/*CPMSActionData A(CPMSActionData::PMSFailOperationOrder, Run,
							bind(&CPmsLsaMassHunterServiceDialog::LimsReportRunHandler, this, _1, _2));
		A.OrderID = Run->PmsId;
		if (m_Pms.Server) m_Pms.Server->AddPMSAction(A);*/
	}
}

void CPmsLsaMassHunterServiceDialog::LimsReportRunHandler(system::error_code Error, CPMSActionData* PmsData)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::LimsReportRunHandler: %u\n", Error.value());
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	_ASSERTE(PmsData);
	_ASSERTE(!PmsData->OperationContainer.empty());
	try
	{	if (Error)
		{	SetInfoTextExtern(L"Report Schedule '" + PmsData->OperationContainer.front()->CommandName() + L"' to LIMS Server: " + PmsData->OperationContainer.front()->ErrorText);
		}
		else
		{	SetInfoTextExtern(L"Schedule '" + PmsData->OperationContainer.front()->CommandName() + L"' reported to LIMS");
		}
	}
	catch (...)
	{}
}

void CPmsLsaMassHunterServiceDialog::ConnectLims()
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::ConnectLIMS\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_Status.WebServiceError = false;
	m_Status.WebServiceStarted = false;

	m_Lims.Port = AfxGetApp()->GetProfileInt(RegConfigName, L"LIMS Port", 9000);
	m_Lims.NicToUse = AfxGetApp()->GetProfileString(RegConfigName, L"LIMS NIC To Use", L"0.0.0.0");
	CString T;
	T.Format(L"LIMS webservice on %s port %u", (LPCTSTR)m_Lims.NicToUse, m_Lims.Port);
	m_Logger.LogText(T);

	if (!m_Lims.Port)
	{	_ASSERTE(!m_Lims.WebService);
		return;
	}

	m_Lims.WebService.reset(new CLIMSInterfaceThread(m_Lims.NicToUse, m_Lims.Port));

	m_Lims.WebService->ProjectsHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_GetProjects, this,
											  placeholders::_1);
	m_Lims.WebService->MethodsHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_GetMethods, this,
											 placeholders::_1, placeholders::_2);
	m_Lims.WebService->StatusHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_GetStatus, this,
											placeholders::_1, placeholders::_2, placeholders::_3);
	m_Lims.WebService->StartHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_StartMethod, this,
										   placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
	m_Lims.WebService->SchedulePositionsHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_GetPositions, this,
											   placeholders::_1, placeholders::_2, placeholders::_3);
	m_Lims.WebService->RunPositionsHandler = bind(&CPmsLsaMassHunterServiceDialog::Lims_GetPositions, this,
											   placeholders::_1, placeholders::_2, placeholders::_3);

	m_Lims.Thread.reset(new thread(bind(&CLIMSInterfaceThread::Run, m_Lims.WebService.get())));
	m_Status.WebServiceStarted = true;
	RefreshStatus();
}

void CPmsLsaMassHunterServiceDialog::DisconnectLims()
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::DisconnectLIMS\n");
	m_Logger.LogText(L"Disconnect LIMS begin");
//		interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	if (m_Lims.WebService) 
	{	m_Lims.WebService->Abort();
		if (m_Lims.Thread) m_Lims.Thread->join();
		m_Lims.WebService.reset();
		m_Lims.Thread.reset();
	}
	m_Lims.LastOperation.reset();
	m_Status.WebServiceError = false;
	m_Status.WebServiceStarted = false;
	RefreshStatus();

	m_Logger.LogText(L"Disconnect LIMS end");
}


void CPmsLsaMassHunterServiceDialog::ConnectSami()
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::ConnectSami\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_Status.SamiServiceError = false;
	m_Status.SamiServiceStarted = false;
	
	m_Sami.LastOperation.reset();

	m_Sami.Port = AfxGetApp()->GetProfileInt(RegConfigName, L"SAMI Port", 9002);
	m_Sami.NicToUse = AfxGetApp()->GetProfileString(RegConfigName, L"SAMI NIC To Use", L"0.0.0.0");
	CString T;
	T.Format(L"SAMI service on %s port %u", (LPCTSTR)m_Sami.NicToUse, m_Sami.Port);
	m_Logger.LogText(T);

	if (!m_Sami.Port)
	{	_ASSERTE(!m_Sami.Service);
		return;
	}

	m_Sami.Service.reset(new MassHunter::CMassHunterServer(m_Sami.IoService, m_Sami.NicToUse, m_Sami.Port, AfxGetApp()->m_pszAppName));

	m_Sami.Service->SetPeerProjectListHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_GetProjectList, this,
												   placeholders::_1));
	m_Sami.Service->SetPeerMethodListHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_GetMethodList, this,
												  placeholders::_1, placeholders::_2));
	m_Sami.Service->SetPeerMasterBatchListHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_GetMasterBatchList, this,
													   placeholders::_1, placeholders::_2));
	m_Sami.Service->SetPeerStatusListHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_GetStatus, this,
												  placeholders::_1, placeholders::_2));
	m_Sami.Service->SetPeerRunMethodHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_StartMethod, this,
												 placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6));
	m_Sami.Service->SetPeerAbortHandler(bind(&CPmsLsaMassHunterServiceDialog::Sami_Abort, this));

	m_Sami.Service->Start();
	m_Sami.IoService.reset();
	m_Sami.Thread.reset(new thread(bind(&asio::io_service::run, &m_Sami.IoService)));
	m_Status.SamiServiceStarted = true;
	RefreshStatus();
}

void CPmsLsaMassHunterServiceDialog::DisconnectSami()
{
//	TRACE(L"CPmsLsaMassHunterServiceDialog::DisconnectSami\n");
	m_Logger.LogText(L"Disconnect SAMI begin");
//		interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	if (m_Sami.Service) 
	{	m_Sami.Service->Abort();
		if (m_Sami.Thread)
		{	m_Sami.Thread->join();
		}
		m_Sami.Service.reset();
		m_Sami.Thread.reset();
	}
	m_Sami.LastOperation.reset();
	m_Status.SamiServiceError = false;
	m_Status.SamiServiceStarted = false;
	RefreshStatus();

	m_Logger.LogText(L"Disconnect SAMI end");
}

bool CPmsLsaMassHunterServiceDialog::Sami_GetProjectList(std::list<CString>& List)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Sami_GetProjectList\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	m_Logger.LogText(L"Project list query from SAMI");

	if (!m_MassHunter)
	{	List.clear();
		List.push_back(L"No active MassHunter object");
		return false;
	}
	if (m_MassHunter->ReadProjectList())
	{	for (std::list<std::pair<CString, long>>::iterator i(m_MassHunter->m_ProjectList.begin());i!=m_MassHunter->m_ProjectList.end();++i)
		{	List.push_back(i->first);
		}
	}
	return true;
}



bool CPmsLsaMassHunterServiceDialog::Sami_GetMethodList(const CString& Project, std::list<CString>& List)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Sami_GetMethodList: Project'%s'\n", Project);
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString TL;
	TL.Format(L"Method list query from SAMI. Project: '%s'", (LPCTSTR)Project);
	m_Logger.LogText(TL);

	if (!m_MassHunter)
	{	List.clear();
		List.push_back(L"No active MassHunter object");
		return false;
	}
	// TODO: read Methods
	if (m_MassHunter->ReadMethodsForProject(Project))
	{	List = m_MassHunter->m_MethodList;
	}
	return true;
}




bool CPmsLsaMassHunterServiceDialog::Sami_GetMasterBatchList(const CString& Project, std::list<CString>& List)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Sami_GetMasterBatchList\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	CString TL;
	TL.Format(L"Master Batch list query from SAMI. Project: '%s'", (LPCTSTR)Project);
	m_Logger.LogText(TL);

	if (!m_MassHunter)
	{	List.clear();
		List.push_back(L"No active MassHunter object");
		return false;
	}
	// TODO: read Master Batches
	if (m_MassHunter->ReadMethodsForProject(Project))
	{	List = m_MassHunter->m_MethodList;
	}
	return true;
}

// Funktion zur Überprüfung, ob ein bestimmter Prozess läuft
bool IsProcessRunning(const std::wstring& processName) {
	// Erstelle einen Snapshot aller laufenden Prozesse
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Fehler beim Erstellen des Prozess-Snapshots!" << std::endl;
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Durchlaufe alle Prozesse
	if (Process32First(hProcessSnap, &pe32)) {
		do {
			// Vergleiche den Namen des aktuellen Prozesses mit dem gesuchten Namen
			if (processName == pe32.szExeFile) {
				CloseHandle(hProcessSnap);
				return true; // Prozess gefunden
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);
	return false; // Prozess nicht gefunden
}

// Funktion, um Fenster zu finden und in den Vordergrund zu bringen
bool BringWindowToFront(const std::wstring& windowTitlePart) {
	HWND hwnd = GetTopWindow(NULL); // Starte bei oberstem Fenster
	while (hwnd) {
		wchar_t title[256]; // Speicher für den Fenstertitel
		GetWindowText(hwnd, title, sizeof(title) / sizeof(wchar_t)); // Fenstertitel abrufen

		// Prüfen, ob der Fenstertitel den festen Teil enthält
		if (std::wstring(title).find(windowTitlePart) != std::wstring::npos) {
			// Fenster gefunden, in den Vordergrund bringen
			if (IsIconic(hwnd)) {
				// Wenn das Fenster minimiert ist, wiederherstellen
				ShowWindow(hwnd, SW_RESTORE);
			}
			SetForegroundWindow(hwnd); // Fenster in den Vordergrund bringen
			return true;
		}

		// Nächstes Fenster abrufen
		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
	}

	return false; // Fenster nicht gefunden
}

bool CPmsLsaMassHunterServiceDialog::Sami_GetStatus(CString& Text, int& Code)
{
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);

	std::wstring targetProcess = L"msinsctl.exe";

	if (!IsProcessRunning(targetProcess))
	{
		Text = L"MassHunter APP ist nicht angefangen";
		Code = 72;
		return false;
	}
	/*
	else {

		std::wstring fixedPart = L"Enhanced MassHunter"; // Fester Teil des Fenstertitels
		BringWindowToFront(fixedPart);
	}*/
	// Standard: Fehlerstatus
	

	if (!m_MassHunter)
	{
		Text = L"MassHunter not connected";
	    Code = 1;
		return false;
	}
	if (m_CurrentRunOperation)
	{
		Text = L"Busy";
		Code = 1;
	}

	else
	{
		if (m_Sami.LastOperation)
		{
			_ASSERTE(m_Sami.LastOperation->OperationSource == COperationContainer::SamiOperation);
			if (m_Sami.LastOperation->ErrorText.IsEmpty())
			{
				Text = L"Idle";
				Code = 0;
			}
			else
			{
				Text = m_Sami.LastOperation->ErrorText;
				Code = 2;
			}
			m_Sami.LastOperation.reset();
		}
		else
		{
			CString guiStatus = m_MassHunter->GetInstrumentStatus();  // z. B. "Ready"
			CString runStatus = m_MassHunter->GetRunStatus();         // z. B. "Idle"

			// Falls beides "bereit" ist, prüfen ob noch in Pufferzeit
			if (guiStatus == L"Ready" && runStatus == L"Idle") {
				CTime now = CTime::GetCurrentTime();
				if (m_LastRunStartTime != 0 && now - m_LastRunStartTime < CTimeSpan(0, 0, 2, 0)) {

					Text = L"Busy ";
					Code = 1;
					return true;
				}
				else {
					
					CString StatusMeldung = L"Idle";

					Text = L"Idle";
					Code = 0;
					return true;
				}

			}
			else {

				Text = L"Busy ";
				Code = 1;
			}
			
		}
	}
	/*
	if( Fertig == L"Die Messung ist fertig.")
	{

		Text = Fertig;
	}*/
	
	return true;
}


//Zweite Version 


bool CPmsLsaMassHunterServiceDialog::Sami_StartMethod(CString& ProjectPath,
	const CString& MasterBatch,
	const CString& MethodName,
	const CString& DataPath,
	const std::list<CommandHandler::CPositionData>& Labware,
	const boost::posix_time::time_duration Timeout)
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Sami_StartMethod\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);

	CString TL;
	TL.Format(L"Start method request from SAMI. Project: '%s' Method: '%s'", (LPCTSTR)ProjectPath, (LPCTSTR)MethodName);
	m_Logger.LogText(TL);

	if (!m_MassHunter)
	{
		ProjectPath = L"MassHunter not connected";
		return false;
	}



	if (m_CurrentRunOperation)
	{
		ProjectPath = L"A method is already running";
		return false;
	}

	// Neue Run-Operation vorbereiten
	std::shared_ptr<CRunOperationContainer> ROC(new CRunOperationContainer());
	CRunOperationContainer& X(*dynamic_cast<CRunOperationContainer*>(ROC.get()));

	// Basisinformationen
	X.CommandName(L"MassHunter: Run Method");
	X.Run.reset(new SamiControl::CMethodRun());
	X.Schedule.reset(new SamiControl::CMethodSchedule());
	X.ProjectName = ProjectPath;
	X.ProjectID = 0;
	X.MethodName = MethodName;
	X.DataPath = DataPath;
	X.MethodRevision = 0;
	X.Families = 1;
	X.OperationSource = COperationContainer::SamiOperation;
	X.PmsId = (std::numeric_limits<unsigned __int64>::max)();
	m_Sami.LastOperation = ROC;
	X.Run->RunSequenceExecuted = false;

	// Projekt- und Methodendateien setzen
	X.BaseFilePath = m_MassHunterOptions.ProjectPath;
	X.Run->Project = X.ProjectName;
	X.Run->Method = X.MethodName;
	X.Run->ProjectFileName = m_MassHunterOptions.ProjectPath + X.Run->Project;
	X.Run->MethodFileName = X.Run->ProjectFileName + L"\\" + X.Run->Method;
	X.Run->DataPath = X.DataPath;

	// Labware-Parameter übernehmen
	for (const auto& pos : Labware)
	{
		for (const auto& lw : pos.Labware)
		{
			SamiControl::CBaseLabwareData lab;
			lab.Name = lw.Name;
			lab.Barcode = lw.Barcode;
			lab.SampleName = lw.SampleName;
			lab.Vial = lw.Vial;
			lab.Method_Path = lw.Method_Path;
			lab.Typ = lw.Typ;
			lab.DataFile = lw.DataFile;
			lab.Level = lw.Level;
			lab.Dilution = lw.Dilution;
			lab.Volume = lw.Volume;
			lab.TrayName = lw.TrayName;
			lab.Comment = lw.Comment;
			lab.Position = pos.PositionName;

			X.Run->Labware.push_back(lab);
		}
	}

	// Batch-Dateiname erzeugen
	if (DataPath.IsEmpty()) {
		X.Run->NewBatchFileName = m_MassHunterOptions.DataPath + CTime::GetCurrentTime().Format(L"TODO_SAMI_%Y-%m-%d-%H-%M-%S.b");
	}
	else {
		X.Run->NewBatchFileName = m_MassHunterOptions.DataPath + DataPath;
	}

	// Jetzt für TickMassHunter vorbereiten
	m_CurrentRunOperation = ROC;
	m_RunState = RunReadProject;


	

	return true;
}


bool CPmsLsaMassHunterServiceDialog::Sami_Abort()
{
	TRACE(L"CPmsLsaMassHunterServiceDialog::Sami_Abort\n");
	interprocess::scoped_lock<recursive_mutex> SL(m_Mutex);
	if (!m_MassHunter)
	{	return false;
	}
	m_MassHunter->Abort();
	return true;
}

void CPmsLsaMassHunterServiceDialog::OnStnClickedStaticMhStatus()
{
	// TODO: Add your control notification handler code here
}


void CPmsLsaMassHunterServiceDialog::OnStnClickedStaticMhStatusIndicator()
{
	// TODO: Add your control notification handler code here
}

CString CPmsLsaMassHunterServiceDialog::GetMassHunterStatus()
{
	if (!m_MassHunter) return L"MassHunter-Verbindung fehlt";

	return m_MassHunter->GetInstrumentStatus() 
		,  m_MassHunter->GetRunStatus() ;
}


void CPmsLsaMassHunterServiceDialog::OnBnClickedShowProjectPath()
{
	//m_PathDisplay.SetWindowText(m_MassHunterOptions.ProjectPath);


	if (m_MassHunter)
	{
		CString path = m_MassHunter->GetCurrentProjectPath();

		m_PathDisplay.SetWindowText(L"Projekt Path:" + path);
	}
	else
	{
		m_PathDisplay.SetWindowText(L"MassHunter interface not initialized");
	}
}


void CPmsLsaMassHunterServiceDialog::OnBnClickedShowDataPath()
{
	if (m_MassHunter)
	{
		CString path2 = m_MassHunter->GetCurrentDatatPath();
		m_PathDisplay.SetWindowText(L"Data Path:" + path2);
	}
	else
	{
		m_PathDisplay.SetWindowText(L"MassHunter interface not initialized");
	}
}


void CPmsLsaMassHunterServiceDialog::OnStnClickedPathDisplay()
{
	// TODO: Add your control notification handler code here
}



//Letzte Version 
void CPmsLsaMassHunterServiceDialog::RunSequenceComplete(bool success, const CString& message)
{
	if (!m_CurrentRunOperation)
		return;

	
	if (!success)
	{
		m_CurrentRunOperation->ErrorText = message;
		m_RunState = RunError;
		m_Logger.LogText(L"SequnceTable ist nihct erfolgreich ausgefullt !");

	}
	else
	{
		m_Logger.LogText(L"SequnceTable ist erfolgreich ausgefullt !");

		m_Status.Running = true;
		RefreshStatus();
		m_RunState = RunEnd;
	}
}




