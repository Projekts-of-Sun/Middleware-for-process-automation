#pragma once

#include <afxwin.h>
#include <afxcmn.h>

#include <boost\thread.hpp>

#include "..\..\List View\List View.h"
#include "..\..\Service Interface\Boost Socket Commands.h"

#include <memory>

#include "resource.h"

class CServiceClientDlg : public CDialog
{
public:
	CServiceClientDlg(CWnd* pParent = NULL);
	CStatic m_StatusBox;


	enum
	{ IDD = IDD_SERVICECLIENT_DIALOG
	};
	enum
	{	MSG_INFO_TEXT = (WM_APP + 0)
	};

	void SetInfoText(CString Text);
	void SetInfoTextExtern(CString Text);

protected:
	bool Shutdown;
	LRESULT OnInfoText(WPARAM wParam, LPARAM lParam);

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void OnBnClickedServerConnect();
	void OnBnClickedServerDisconnect();
	void OnBnClickedListProjects();
	void OnBnClickedListMasterBatches();
	void OnBnClickedRunMasterBatch();
	void OnBnClickedListMethods();
	void OnBnClickedAbort();
	void OnBnClickedRunMethod();
	void OnBnClickedGetStatus();

	HICON m_hIcon;
	Graphics::CColorList ProjectList;
	Graphics::CColorList MasterBatchList;
	Graphics::CColorList MethodList;

	struct _MassHunter
	{	CString ServerIP;
		short ServerPort;
		std::shared_ptr<boost::asio::io_service> IoService;
		std::shared_ptr<boost::thread> Thread;
		std::shared_ptr<MassHunter::CMassHunterPeerConnection> Peer;
	} MassHunter;

	boost::recursive_mutex m_Mutex;
	void ActivateComplete(boost::system::error_code Error, MassHunter::CMassHunterPeerConnection* Peer);
	void PeerTimeout(boost::system::error_code Error, MassHunter::CMassHunterPeerConnection* Peer);
	void PeerError(boost::system::error_code Error, MassHunter::CMassHunterPeerConnection* Peer);

	void ProjectListCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void MasterBatchListCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void MethodListCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void RunMethodCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void StatusCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);


	

	void MasterBatchRunCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void MethodRunCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);
	void AbortCommandComplete(boost::system::error_code Error, CommandHandler::CCommandOnTheFly* X);

	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedGetStatus();
	afx_msg void OnLvnItemchangedProjectList(NMHDR* pNMHDR, LRESULT* pResult);
};
