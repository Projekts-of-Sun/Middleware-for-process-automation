#include "stdafx.h"
#include "Options.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "../Agilent/MassHunter Interface.h"


#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

using namespace SamiControl;

BEGIN_MESSAGE_MAP(CPmsLsaMassHunterServiceOptions, CDialog)
	ON_EN_CHANGE(IDC_OPTION_MASSHUNTER_DATA_PATH, &CPmsLsaMassHunterServiceOptions::OnEnChangeOptionMasshunterDataPath)
END_MESSAGE_MAP()




extern const wchar_t* RegConfigName;

CPmsLsaMassHunterServiceOptions::CPmsLsaMassHunterServiceOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CPmsLsaMassHunterServiceOptions::IDD, pParent)
{
	LimsWebService.Port = 9000;
	SamiService.Port = 9002;
	MassHunter.ProjectPath = L"C:\\";
}

CPmsLsaMassHunterServiceOptions::~CPmsLsaMassHunterServiceOptions()
{
}

void CPmsLsaMassHunterServiceOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPTION_LIMS_WEB_SERVICE_URI, LimsWebService.Address);
	DDX_Text(pDX, IDC_OPTION_LIMS_WEB_SERVICE_PORT, LimsWebService.Port);
	DDX_Text(pDX, IDC_OPTION_MASSHUNTER_PROJECT_PATH, MassHunter.ProjectPath);
	DDX_Text(pDX, IDC_OPTION_MASSHUNTER_DATA_PATH, MassHunter.DataPath);
	DDX_Control(pDX, IDC_OPTION_SAMI_SERVICE_URI, SamiService.Address);
	DDX_Text(pDX, IDC_OPTION_SAMI_SERVICE_PORT, SamiService.Port);
	DDX_Control(pDX, IDC_OPTION_MASSHUNTER_PROJECT_PATH, m_ProjectPathEdit);
	DDX_Control(pDX, IDC_OPTION_MASSHUNTER_DATA_PATH, m_DataPathEdit);

}

BOOL CPmsLsaMassHunterServiceOptions::OnInitDialog()
{
	TRACE(L"CPmsLsaMassHunterServiceOptions::OnInitDialog\n");
	CDialog::OnInitDialog();

	LimsWebService.Address.Clear();
	SamiService.Address.Clear();
	char hn[128];
	gethostname(hn, 128);
	TRACE(L"\tgethostname: %S\n", hn);
	addrinfo *ai, *aii;
	int A = getaddrinfo(hn, nullptr, nullptr, &ai);
	TRACE(L"\tgetaddrinfo: %d\n", A);
	aii = ai;
	unsigned int AdressIndex = 0;
	while (aii)
	{	wchar_t T[128];
		DWORD TL = sizeof(T) / sizeof(wchar_t);
		TRACE(L"\tNIC: 0x%p = %u\n", aii->ai_addr, aii->ai_addrlen);
		if (!WSAAddressToString(aii->ai_addr, DWORD(aii->ai_addrlen), NULL, T, &TL))
		{	// http://en.wikipedia.org/wiki/IPv6
			// ipv6: "::1" entspricht "127.0.0.1" bei ipv4
			LimsWebService.Address.InsertString(AdressIndex, T);
			SamiService.Address.InsertString(AdressIndex, T);
		}
		aii = aii->ai_next;
	}
	freeaddrinfo(ai); 
	LimsWebService.Address.InsertString(0, L"0.0.0.0");
	SamiService.Address.InsertString(0, L"0.0.0.0");


	CString T(AfxGetApp()->GetProfileString(RegConfigName, L"LIMS NIC To Use", L"0.0.0.0"));
	int Index = LimsWebService.Address.FindString(-1, T);
	if (Index!=CB_ERR) LimsWebService.Address.SetCurSel(Index);
	else LimsWebService.Address.SetCurSel(0);
	LimsWebService.Port = AfxGetApp()->GetProfileInt(RegConfigName, L"LIMS Port", 9000);

	T = AfxGetApp()->GetProfileString(RegConfigName, L"SAMI NIC To Use", L"0.0.0.0");
	Index = SamiService.Address.FindString(-1, T);
	if (Index!=CB_ERR) SamiService.Address.SetCurSel(Index);
	else SamiService.Address.SetCurSel(0);
	SamiService.Port = AfxGetApp()->GetProfileInt(RegConfigName, L"SAMI Port", 9002);

	MassHunter.ProjectPath = AfxGetApp()->GetProfileString(RegConfigName, L"MassHunter Project Path", L"C:\\");

	MassHunter.DataPath = AfxGetApp()->GetProfileString(RegConfigName, L"MassHunter Data Path", L"C:\\");

	UpdateData(false);

	return TRUE;
}

void CPmsLsaMassHunterServiceOptions::OnOK()
{
	UpdateData();
	CString T;
	int Index = LimsWebService.Address.GetCurSel();
	if (Index>-1)
	{	LimsWebService.Address.GetLBText(Index, T);
		AfxGetApp()->WriteProfileString(RegConfigName, L"LIMS NIC To Use", T);
	}
	AfxGetApp()->WriteProfileInt(RegConfigName, L"LIMS Port", LimsWebService.Port);

	Index = SamiService.Address.GetCurSel();
	if (Index>-1)
	{	SamiService.Address.GetLBText(Index, T);
		AfxGetApp()->WriteProfileString(RegConfigName, L"SAMI NIC To Use", T);
	}
	AfxGetApp()->WriteProfileInt(RegConfigName, L"SAMI Port", SamiService.Port);

	if (MassHunter.ProjectPath.Right(1)!=L'\\' && MassHunter.ProjectPath.Right(1)!=L'/')
	{	MassHunter.ProjectPath+=L'\\';
	}
	if (MassHunter.DataPath.Right(1)!=L'\\' && MassHunter.DataPath.Right(1)!=L'/')
	{	MassHunter.DataPath+=L'\\';
	}
	AfxGetApp()->WriteProfileString(RegConfigName, L"MassHunter Project Path", MassHunter.ProjectPath);
	AfxGetApp()->WriteProfileString(RegConfigName, L"MassHunter Data Path", MassHunter.DataPath);

	UpdateData(TRUE);
	if (m_pMassHunterInterface)
	{
		CString NewPath;
		m_ProjectPathEdit.GetWindowText(NewPath);
		m_pMassHunterInterface->SetProjectPath(NewPath);;
	}
	if (m_pMassHunterInterface2)
	{
		CString NewPath2;
		m_DataPathEdit.GetWindowText(NewPath2);
	    //std::wstring NewPath2 = path.GetString();

		m_pMassHunterInterface2->SetCurrentDatatPath(NewPath2);
		
	}

	CDialog::OnOK();
}


void CPmsLsaMassHunterServiceOptions::OnEnChangeOptionMasshunterDataPath()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
