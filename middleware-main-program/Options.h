#pragma once
#include <afx.h>
#include "resource.h"
#include "../Agilent/MassHunter Interface.h"
#include "../Service Interface\Boost Socket Commands.h"



class CPmsLsaMassHunterServiceOptions : public CDialog
{
public:
	CPmsLsaMassHunterServiceOptions(CWnd* pParent = nullptr);
	virtual ~CPmsLsaMassHunterServiceOptions();

	//void SetMassHunterInterface(CMassHunterInterface* pInterface) { m_pMassHunterInterface = pInterface; }

	//void SetMassHunterInterface(std::shared_ptr<SamiControl::CMassHunterInterface> m_MassHunter* pInterface) { m_pMassHunterInterface = pInterface; }
	//shared_ptr<SamiControl::CMassHunterInterface> m_MassHunter

	/*void SetMassHunterInterface(std::shared_ptr<SamiControl::CMassHunterInterface> pInterface)
	{
		m_pMassHunterInterface = pInterface;

	}*/

	void SetMassHunterInterface(SamiControl::CMassHunterInterface* pInterface)
	{
		m_pMassHunterInterface = pInterface;
	}

	void SetMassHunterInterface2(SamiControl::CMassHunterInterface* pInterface2)
	{
		m_pMassHunterInterface2 = pInterface2;
	}

	enum
	{ IDD = IDD_OPTIONS_DIALOG
	};

	CEdit m_ProjectPathEdit;
	CEdit m_DataPathEdit;



protected:
	//std::shared_ptr<SamiControl::CMassHunterInterface> m_pMassHunterInterface;

	SamiControl::CMassHunterInterface* m_pMassHunterInterface = nullptr;
	SamiControl::CMassHunterInterface* m_pMassHunterInterface2 = nullptr;


	struct _LimsWebService
	{	_LimsWebService()
			: Port(0)
		{
		}
		CComboBox Address;
		unsigned int Port;
	} LimsWebService;

	struct _SamiService
	{	_SamiService()
			: Port(0)
		{
		}
		CComboBox Address;
		unsigned int Port;
	} SamiService;

	struct _MassHunter
	{	CString ProjectPath;
		CString DataPath;
	} MassHunter;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeOptionMasshunterDataPath();
};
