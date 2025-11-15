#include <afx.h>
#include <afxctl.h>

#include "MassHunter Interface.h"
#include "MemDc.h"

#include "..\Utf8\utf8.h"

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

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif
using namespace std;

namespace SamiControl
{

void FindProgID(IUnknown* X)
{
	HKEY CR;
	RegOpenKey(HKEY_CLASSES_ROOT, /*L"CLSID"*/L"Interface", &CR);
	DWORD Index = 0; 
	FILETIME FT;
	IID ID;
	wchar_t Name[100];
	DWORD NL = sizeof(Name) / sizeof(wchar_t);
	while (RegEnumKeyEx(CR, Index, Name, &NL, NULL, NULL, NULL, &FT)==ERROR_SUCCESS)
	{	if (CLSIDFromString(Name, &ID)==NOERROR)
		{	IUnknown* Y = nullptr;
			if (X->QueryInterface(ID, (void**)&Y)==S_OK)
			{	HKEY I;
				wchar_t Text[100];
				DWORD TL = sizeof(Text);
				if (RegOpenKeyEx(CR, Name, 0, KEY_READ, &I)==ERROR_SUCCESS)
				{	if (RegQueryValueEx(I, NULL, NULL, NULL, (LPBYTE)Text, &TL)==ERROR_SUCCESS)
					{	wprintf(L"CLSID:%s\tProgID[%08p]: ", Name, Y);
						wprintf(Text);
						wprintf(L"\n");
					}
					RegCloseKey(I);
				}
				Y->Release();
			}
		}
		++Index;
		NL = sizeof(Name) / sizeof(wchar_t);
	}
	RegCloseKey(CR);
}


const double SegmentReduction = 0.66;
const double InnerRadiusReduction = 0.35;
const double BorderReduction = 0.05;
const double RotationSpeed = 0.1;

unsigned int CExecutiveParameters::JobCounter = 0;

CString CMassHunterInterface::GetCurrentStatusText() const
{
	return m_StatusText; // oder dein eigener m_MassHunterStatusText

}

std::string CExecutiveParameters::ToXmlString() const
{
	DocPtr docptr;
	if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
	{	throw CString(L"Failed to create xml object");
	}
	docptr->async = VARIANT_FALSE;
	docptr->validateOnParse	= VARIANT_FALSE;
	docptr->resolveExternals = VARIANT_FALSE;
	MSXML2::IXMLDOMNodePtr Temp(docptr->appendChild(docptr->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"")));
		
	MSXML2::IXMLDOMNodePtr nodptr(docptr->createElement(StreamName()));
	Temp = docptr->appendChild(nodptr);
	if (!Save(nodptr, docptr)) return "";
	return BstrToUtf8(docptr->Getxml());
}

bool CExecutiveParameters::FromXmlString(const char* Xml)
{
	DocPtr docptr;
	if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
	{	throw CString(L"Failed to create xml object");
	}
	docptr->async = VARIANT_FALSE;
	docptr->validateOnParse = VARIANT_FALSE;
	docptr->resolveExternals = VARIANT_FALSE;
	if (!docptr->loadXML(Utf8ToBstr(Xml)))	return false;
	if (!Load(docptr->documentElement, docptr)) return false;
	return true;
}

bool CExecutiveParameters::Save(SavePtr X, DocPtr D) const
{
	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMElementPtr E(X);
	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"Error"));
	MSXML2::IXMLDOMNodePtr Temp(E->appendChild(Y));
	Y->setAttribute(L"Text", (LPCTSTR)ErrorText); 
	wchar_t T[60];
	Y = D->createElement(L"Timing");
	Temp = E->appendChild(Y);
	_swprintf_s_l(T, 60, L"%u", L, StartTime);
	Y->setAttribute(L"StartTime", T); 
	_swprintf_s_l(T, 60, L"%u", L, Timeout);
	Y->setAttribute(L"Timeout", T); 

	_free_locale(L);
	return true;
}

bool CExecutiveParameters::Load(SavePtr X, DocPtr D)
{
	JobID = 0;
	Handler = NULL;
	Success = false;
	ErrorText.Empty();

	MSXML2::IXMLDOMElementPtr	E(X);

	if (!E->hasChildNodes()) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMNodeListPtr NL(E->childNodes);
	for (int i=0;i<NL->length;i++)
	{
		MSXML2::IXMLDOMNodePtr N = NL->Getitem(i);
		E = N;
		if (N->nodeName==_bstr_t("Error"))
		{	BSTR A = E->getAttribute(L"Text").bstrVal;
			if (A) ErrorText = A;
			continue;
		}
		if (N->nodeName==_bstr_t("Timing"))
		{	BSTR A = E->getAttribute(L"StartTime").bstrVal;
			if (A) StartTime = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"Timeout").bstrVal;
			if (A) Timeout = wcstoul(A, NULL, 10);
			continue;
		}
	}

	_free_locale(L);
	return true;
}

bool CMethodSchedule::Save(SavePtr X, DocPtr D) const
{
	if (!CExecutiveParameters::Save(X, D)) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMElementPtr E(X);
	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"Method"));
	MSXML2::IXMLDOMNodePtr Temp(E->appendChild(Y));
	Y->setAttribute(L"Name", (LPCTSTR)Method); 
	Y->setAttribute(L"Revision", MethodRevision); 
	Y->setAttribute(L"File", (LPCTSTR)MethodFileName); 

	Y = D->createElement(L"Project");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"Name", (LPCTSTR)Project); 
	Y->setAttribute(L"ID", ProjectID); 
	Y->setAttribute(L"File", (LPCTSTR)ProjectFileName); 
	
	Y = D->createElement(L"Schedule");
	wchar_t T[60];
	_swprintf_s_l(T, 60, L"%0.2f", L, ETC);
	Y->setAttribute(L"ETC", T); 
	Y->setAttribute(L"Families", Families); 
	Temp = E->appendChild(Y);

	Y = D->createElement(L"Batch");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"File", (LPCTSTR)NewBatchFileName); 

	Y = D->createElement(L"Positions");
	Temp = E->appendChild(Y);
	for (std::list<CPositionDescriptor>::const_iterator i(Positions.begin());i!=Positions.end();++i)
	{
		MSXML2::IXMLDOMElementPtr	P(D->createElement(L"Position"));
		Temp = Y->appendChild(P);
		P->setAttribute(L"Name", (LPCTSTR)i->PositionName);
		P->setAttribute(L"Path", (LPCTSTR)i->PathName);
		MSXML2::IXMLDOMElementPtr	LW(D->createElement(L"Labwares"));
		Temp = P->appendChild(LW);
		unsigned int StackIndex = 1;
		for (std::vector<CBaseLabwareData>::const_iterator j(i->Labware.begin());j!=i->Labware.end();++j)
		{	P = D->createElement(L"Labware");
			Temp = LW->appendChild(P);
			P->setAttribute(L"StackIndex", StackIndex++);
			P->setAttribute(L"Name",j->Name.c_str());
			P->setAttribute(L"Class", j->ClassName.c_str());
			_swprintf_s_l(T, 60, L"%0.2f", L, j->Height);
			P->setAttribute(L"Height", T);
			P->setAttribute(L"Barcode", j->Barcode.c_str());
			P->setAttribute(L"BarcodeUsed", j->BarcodeUsed?L"True":L"False");
			P->setAttribute(L"Position", j->Position.c_str());
			P->setAttribute(L"ID", j->ID);
			//*****************
			P->setAttribute(L"SampleName", j->SampleName.c_str());
			P->setAttribute(L"Vial", j->Vial.c_str());
			P->setAttribute(L"Method_Path", j->Method_Path.c_str());
			P->setAttribute(L"Typ", j->Typ.c_str());
			P->setAttribute(L"DataFile", j->DataFile.c_str());
			P->setAttribute(L"Level", j->Level.c_str());
			P->setAttribute(L"Dilution", j->Dilution.c_str());
			P->setAttribute(L"Volume", j->Volume.c_str());
			P->setAttribute(L"TrayName", j->TrayName.c_str());
			P->setAttribute(L"Comment", j->Comment.c_str());
		}
	}
	Y = D->createElement(L"Paths");
	Temp = E->appendChild(Y);
	unsigned int PathIndex = 1;
	for (std::list<std::list<CPositionDescriptor>>::const_iterator k(Paths.begin());k!=Paths.end();++k)
	{
		MSXML2::IXMLDOMElementPtr	Path(D->createElement(L"Path"));
		Temp = Y->appendChild(Path);
		Path->setAttribute(L"Index", PathIndex++);
		for (std::list<CPositionDescriptor>::const_iterator i(k->begin());i!=k->end();++i)
		{	MSXML2::IXMLDOMElementPtr P(D->createElement(L"Position"));
			Temp = Path->appendChild(P);
			P->setAttribute(L"Name", (LPCTSTR)i->PositionName);
			P->setAttribute(L"Path", (LPCTSTR)i->PathName);
			MSXML2::IXMLDOMElementPtr	LW(D->createElement(L"Labwares"));
			Temp = P->appendChild(LW);
			unsigned int StackIndex = 1;
			for (std::vector<CBaseLabwareData>::const_iterator j(i->Labware.begin());j!=i->Labware.end();++j)
			{	P = D->createElement(L"Labware");
				Temp = LW->appendChild(P);
				P->setAttribute(L"StackIndex", StackIndex++);
				P->setAttribute(L"Name", j->Name.c_str());
				P->setAttribute(L"Class", j->ClassName.c_str());
				_swprintf_s_l(T, 60, L"%0.2f", L, j->Height);
				P->setAttribute(L"Height", T);
				P->setAttribute(L"Barcode", j->Barcode.c_str());
				P->setAttribute(L"BarcodeUsed", j->BarcodeUsed?L"True":L"False");
				P->setAttribute(L"Position", j->Position.c_str());
				P->setAttribute(L"ID", j->ID);
				//*****************
				P->setAttribute(L"SampleName", j->SampleName.c_str());
				P->setAttribute(L"Vial", j->Vial.c_str());
				P->setAttribute(L"Method_Path", j->Method_Path.c_str());
				P->setAttribute(L"Typ", j->Typ.c_str());
				P->setAttribute(L"DataFile", j->DataFile.c_str());
				P->setAttribute(L"Level", j->Level.c_str());
				P->setAttribute(L"Dilution", j->Dilution.c_str());
				P->setAttribute(L"Volume", j->Volume.c_str());
				P->setAttribute(L"TrayName", j->TrayName.c_str());
				P->setAttribute(L"Comment", j->Comment.c_str());
			}
		}
	}
	_free_locale(L);
	return true;
}

bool CMethodSchedule::Load(SavePtr X, DocPtr D)
{
	Project.Empty();
	ProjectID = 0;
	ProjectFileName.Empty();
	Method.Empty();
	MethodRevision = 0;
	MethodFileName.Empty();
	ETC = 0.0;
	Families = 0;
	NewBatchFileName.Empty();

	CExecutiveParameters::Load(X, D);

	MSXML2::IXMLDOMElementPtr	E(X);
	
	if (!E->hasChildNodes()) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");
	BSTR A;
	MSXML2::IXMLDOMNodeListPtr NL(E->childNodes);
	for (int i=0;i<NL->length;i++)
	{
		MSXML2::IXMLDOMNodePtr N = NL->Getitem(i);
		E = N;
		if (N->nodeName==_bstr_t("Method"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) Method = A;
			A = E->getAttribute(L"File").bstrVal;
			if (A) MethodFileName = A;
			A = E->getAttribute(L"Revision").bstrVal;
			if (A) MethodRevision = wcstoul(A, NULL, 10);
			continue;
		}
		if (N->nodeName==_bstr_t("Project"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) Project = A;
			A = E->getAttribute(L"File").bstrVal;
			if (A) ProjectFileName = A;
			A = E->getAttribute(L"ID").bstrVal;
			if (A) ProjectID = wcstoul(A, NULL, 10);
			continue;
		}
		if (N->nodeName==_bstr_t("Schedule"))
		{A = E->getAttribute(L"Families").bstrVal;
			if (A) Families = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"ETC").bstrVal;
			if (A) ETC = _wcstod_l(A, NULL, L);
			continue;
		}
		if (N->nodeName==_bstr_t("Batch"))
		{	A = E->getAttribute(L"File").bstrVal;
			if (A) NewBatchFileName = A;
			continue;
		}
		// TODO Positions
		// *****Labware in "Positions"
		if (N->nodeName == _bstr_t("Positions"))
		{
			MSXML2::IXMLDOMNodeListPtr PositionList = E->childNodes;
			for (int p = 0; p < PositionList->length; ++p)
			{
				MSXML2::IXMLDOMElementPtr P(PositionList->Getitem(p));
				if (!P || P->nodeName != _bstr_t("Position")) continue;

				CPositionDescriptor pos;
				A = P->getAttribute(L"Name").bstrVal;
				if (A) pos.PositionName = A;
				A = P->getAttribute(L"Path").bstrVal;
				if (A) pos.PathName = A;

				MSXML2::IXMLDOMNodeListPtr LabwareList = P->childNodes;
				for (int l = 0; l < LabwareList->length; ++l)
				{
					MSXML2::IXMLDOMElementPtr LW(LabwareList->Getitem(l));
					if (!LW || LW->nodeName != _bstr_t("Labware")) continue;

					CBaseLabwareData lab;
					A = LW->getAttribute(L"Name").bstrVal;
					if (A) lab.Name = A;
					A = LW->getAttribute(L"Class").bstrVal;
					if (A) lab.ClassName = A;
					A = LW->getAttribute(L"Height").bstrVal;
					if (A) lab.Height = static_cast<float>(_wtof(A));
					A = LW->getAttribute(L"Barcode").bstrVal;
					if (A) lab.Barcode = A;
					A = LW->getAttribute(L"BarcodeUsed").bstrVal;
					lab.BarcodeUsed = (A && wcscmp(A, L"True") == 0);
					A = LW->getAttribute(L"Position").bstrVal;
					if (A) lab.Position = A;
					A = LW->getAttribute(L"ID").bstrVal;
					if (A) lab.ID = wcstoul(A, NULL, 10);

					// Erweiterte Felder:
					A = LW->getAttribute(L"SampleName").bstrVal;
					if (A) lab.SampleName = A;
					A = LW->getAttribute(L"Vial").bstrVal;
					if (A) lab.Vial = A;
					A = LW->getAttribute(L"Method_Path").bstrVal;
					if (A) lab.Method_Path = A;
					A = LW->getAttribute(L"Typ").bstrVal;
					if (A) lab.Typ = A;
					A = LW->getAttribute(L"DataFile").bstrVal;
					if (A) lab.DataFile = A;
					A = LW->getAttribute(L"Level").bstrVal;
					if (A) lab.Level = A;
					A = LW->getAttribute(L"Dilution").bstrVal;
					if (A) lab.Dilution = A;
					A = LW->getAttribute(L"Volume").bstrVal;
					if (A) lab.Volume = A;
					A = LW->getAttribute(L"TrayName").bstrVal;
					if (A) lab.TrayName = A;
					A = LW->getAttribute(L"Comment").bstrVal;
					if (A) lab.Comment = A;

					pos.Labware.push_back(lab);
				}
				Positions.push_back(pos);
			}
			continue;
		}
	}

	_free_locale(L);
	return true;
}

bool CMethodRun::Save(SavePtr X, DocPtr D) const
{
	if (!CExecutiveParameters::Save(X, D)) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMElementPtr E(X);
	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"Method"));
	MSXML2::IXMLDOMNodePtr Temp(E->appendChild(Y));
	Y->setAttribute(L"Name", (LPCTSTR)Method); 
	Y->setAttribute(L"Revision", MethodRevision); 
	Y->setAttribute(L"File", (LPCTSTR)MethodFileName); 
	
	Y = D->createElement(L"Project");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"Name", (LPCTSTR)Project); 
	Y->setAttribute(L"ID", ProjectID); 
	Y->setAttribute(L"File", (LPCTSTR)ProjectFileName); 
	
	Y = D->createElement(L"Schedule");
	Y->setAttribute(L"Families", Families); 
	Temp = E->appendChild(Y);

	Y = D->createElement(L"Batch");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"File", (LPCTSTR)NewBatchFileName); 

	Y = D->createElement(L"DataPath");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"Path", (LPCTSTR)DataPath);

	Y = D->createElement(L"Labwares");
	Temp = E->appendChild(Y);
	wchar_t T[60];
	for (std::list<CBaseLabwareData>::const_iterator j(Labware.begin());j!=Labware.end();++j)
	{
		MSXML2::IXMLDOMElementPtr LW(D->createElement(L"Labware"));
		Temp = Y->appendChild(LW);
		LW->setAttribute(L"Name", j->Name.c_str());
		LW->setAttribute(L"Class", j->ClassName.c_str());
		_swprintf_s_l(T, 60, L"%0.2f", L, j->Height);
		LW->setAttribute(L"Height", T);
		LW->setAttribute(L"Barcode", j->Barcode.c_str());
		LW->setAttribute(L"BarcodeUsed", j->BarcodeUsed?L"True":L"False");
		LW->setAttribute(L"Position", j->Position.c_str());
		LW->setAttribute(L"ID", j->ID);
		//*****************

		LW->setAttribute(L"SampleName", j->SampleName.c_str());
		LW->setAttribute(L"Vial", j->Vial.c_str());
		LW->setAttribute(L"Method_Path", j->Method_Path.c_str());
		LW->setAttribute(L"Typ", j->Typ.c_str());
		LW->setAttribute(L"DataFile", j->DataFile.c_str());
		LW->setAttribute(L"Level", j->Level.c_str());
		LW->setAttribute(L"Dilution", j->Dilution.c_str());
		LW->setAttribute(L"Volume", j->Volume.c_str());
		LW->setAttribute(L"TrayName", j->TrayName.c_str());
		LW->setAttribute(L"Comment", j->Comment.c_str());
	}
	_free_locale(L);
	return true;
}

bool CMethodRun::Load(SavePtr X, DocPtr D)
{
	Method.Empty();
	MethodRevision = 0;
	Project.Empty();
	ProjectID = 0;
	Families = 0;
	ErrorText.Empty();

	MSXML2::IXMLDOMElementPtr	E(X);

	if (!E->hasChildNodes()) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");
	BSTR A;
	MSXML2::IXMLDOMNodeListPtr NL(E->childNodes);
	for (int i=0;i<NL->length;i++)
	{
		MSXML2::IXMLDOMNodePtr N = NL->Getitem(i);
		E = N;
		if (N->nodeName==_bstr_t("Method"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) Method = A;
			A = E->getAttribute(L"Revision").bstrVal;
			if (A) MethodRevision = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"File").bstrVal;
			if (A) MethodFileName = A;
			continue;
		}
		if (N->nodeName==_bstr_t("Project"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) Project = A;
			A = E->getAttribute(L"ID").bstrVal;
			if (A) ProjectID = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"File").bstrVal;
			if (A) ProjectFileName = A;
			continue;
		}
		if (N->nodeName==_bstr_t("Schedule"))
		{	A = E->getAttribute(L"Families").bstrVal;
			if (A) Families = wcstoul(A, NULL, 10);
			continue;
		}
		if (N->nodeName==_bstr_t("Batch"))
		{	A = E->getAttribute(L"File").bstrVal;
			if (A) NewBatchFileName = A;
			continue;
		}
		if (N->nodeName == _bstr_t("DataPath"))
		{
			A = E->getAttribute(L"Path").bstrVal;
			if (A) DataPath = A;
			continue;
		}
	
	// TODO: Labware
			//**************
		if (N->nodeName == _bstr_t("Labwares"))
		{
			MSXML2::IXMLDOMNodeListPtr LWs = E->childNodes;
			for (int j = 0; j < LWs->length; ++j)
			{
				MSXML2::IXMLDOMElementPtr LW(LWs->Getitem(j));
				if (!LW || LW->nodeName != _bstr_t("Labware"))
					continue;

				CBaseLabwareData lab;

				A = LW->getAttribute(L"Name").bstrVal;
				if (A) lab.Name = A;

				A = LW->getAttribute(L"Class").bstrVal;
				if (A) lab.ClassName = A;

				A = LW->getAttribute(L"Height").bstrVal;
				if (A) lab.Height = static_cast<float>(_wtof(A));

				A = LW->getAttribute(L"Barcode").bstrVal;
				if (A) lab.Barcode = A;

				A = LW->getAttribute(L"BarcodeUsed").bstrVal;
				lab.BarcodeUsed = (A && wcscmp(A, L"True") == 0);

				A = LW->getAttribute(L"Position").bstrVal;
				if (A) lab.Position = A;

				A = LW->getAttribute(L"ID").bstrVal;
				if (A) lab.ID = wcstoul(A, NULL, 10);

				// Erweiterte Labware-Felder:
				A = LW->getAttribute(L"SampleName").bstrVal;
				if (A) lab.SampleName = A;

				A = LW->getAttribute(L"Vial").bstrVal;
				if (A) lab.Vial = A;

				A = LW->getAttribute(L"Method_Path").bstrVal;
				if (A) lab.Method_Path = A;

				A = LW->getAttribute(L"Typ").bstrVal;
				if (A) lab.Typ = A;

				A = LW->getAttribute(L"DataFile").bstrVal;
				if (A) lab.DataFile = A;

				A = LW->getAttribute(L"Level").bstrVal;
				if (A) lab.Level = A;

				A = LW->getAttribute(L"Dilution").bstrVal;
				if (A) lab.Dilution = A;

				A = LW->getAttribute(L"Volume").bstrVal;
				if (A) lab.Volume = A;

				A = LW->getAttribute(L"TrayName").bstrVal;
				if (A) lab.TrayName = A;

				A = LW->getAttribute(L"Comment").bstrVal;
				if (A) lab.Comment = A;

				Labware.push_back(lab);
			}
			continue;
		}
	}

	_free_locale(L);
	return true;
}

CExecutiveStatus::CExecutiveStatus(CString& _ETC, CString& _Time, bool _Paused)
	: ETC(wcstod(_ETC, NULL))
	, Time(wcstod(_Time, NULL))
	, Paused(_Paused)
{
	MsgID = MsgHandlerReply;
}

CExecutiveStatus::~CExecutiveStatus()
{
}


#ifdef MASSHUNTER_USE_COM
const IID IID_IcpAgtPicWklControlManagerEvents = __uuidof(ICP::AGTPICWORKLISTLib::IAgtPicWklControlManagerEvents);
const IID IID_IcpAgtPicWklControlManagerAcquisitionSink = __uuidof(ICP::AGTPICWORKLISTLib::IAgtPicWklControlManagerAcquisitionSink);
const IID IID_IcpAgtPicWklControlManagerDataAnalysisSink = __uuidof(ICP::AGTPICWORKLISTLib::IAgtPicWklControlManagerDataAnalysisSink);
const IID IID_IcpAgtPicWklDataManagerEvents = __uuidof(ICP::AGTPICWORKLISTLib::IAgtPicWklDataManagerEvents);

const IID IID_LcMsTofAgtPicWklControlManagerEvents = __uuidof(LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklControlManagerEvents);
const IID IID_LcMsTofAgtPicWklControlManagerAcquisitionSink = __uuidof(LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklControlManagerAcquisitionSink);
const IID IID_LcMsTofAgtPicWklControlManagerDataAnalysisSink = __uuidof(LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklControlManagerDataAnalysisSink);
const IID IID_LcMsTofAgtPicWklDataManagerEvents = __uuidof(LCMS_TOF::AGTPICWORKLISTLib::IAgtPicWklDataManagerEvents);

BEGIN_DISPATCH_MAP(CMassHunterInterface, CWnd)
 	DISP_FUNCTION_ID(CMassHunterInterface, "OnExecutionComplete", 1, OnExecutionComplete, VT_ERROR, VTS_I4 VTS_VARIANT VTS_WBSTR)
END_DISPATCH_MAP()
#endif


BEGIN_MESSAGE_MAP(CMassHunterInterface, CWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()

	ON_COMMAND_RANGE(BUTTON_ID_START, BUTTON_ID_END, &CMassHunterInterface::OnButtons)
END_MESSAGE_MAP()

//***************************************++
void Tastatur(int key1, int key2 = 2, char key3 = 'Q') {

	INPUT inputs[3];

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key1;
	inputs[0].ki.wScan = 0;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	if (key2 != 2)

		inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = key2;
	inputs[1].ki.wScan = 0;
	inputs[1].ki.dwFlags = 0;
	inputs[1].ki.time = 0;
	inputs[1].ki.dwExtraInfo = 0;

	if (key3 != 'Q')

		inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = key3;
	inputs[2].ki.wScan = 0;
	inputs[2].ki.dwFlags = 0;
	inputs[2].ki.time = 0;
	inputs[2].ki.dwExtraInfo = 0;

	SendInput(3, inputs, sizeof(INPUT));

	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(3, inputs, sizeof(INPUT));


}// Eine Text Eingeben              

// Hilfsfunktion: Suche nach einem Fenster, dessen Titel einen bestimmten Teil enthält
BOOL IsWindowTitleContains(HWND hwnd, LPCWSTR partialTitle) {
	wchar_t title[256];
	GetWindowText(hwnd, title, sizeof(title) / sizeof(wchar_t));
	return wcsstr(title, partialTitle) != nullptr;
}

// Funktion zum Abrufen des Root-Elements des Teilfensters anhand seiner Handle
IUIAutomationElement* GetRootElementFromHandle(IUIAutomation* automation, HWND hwnd) {
	IUIAutomationElement* element = NULL;
	HRESULT hr = automation->ElementFromHandle(hwnd, &element);
	if (FAILED(hr) || !element) {
		std::wcout << L"Root-Element konnte nicht abgerufen werden!" << std::endl;
	}
	return element;
}

// Funktion zur Prüfung des Status im Root-Element des Teilfensters
void CheckRunStatus(IUIAutomation* automation, IUIAutomationElement* parent, LPCWSTR automationId) {
	IUIAutomationElement* statusElement = NULL;

	// Verwende FindFirst, um das erste Kind des Parent-Elements zu finden
	IUIAutomationCondition* condition = NULL;
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = SysAllocString(automationId);
	automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, var, &condition);

	// Durchsuche alle untergeordneten Elemente
	IUIAutomationElement* currentElement = NULL;
	HRESULT hr = parent->FindFirst(TreeScope_Descendants, condition, &currentElement);

	// Wenn das Element gefunden wird, überprüfen wir es
	if (SUCCEEDED(hr) && currentElement) {
		BSTR name;
		currentElement->get_CurrentName(&name);
		std::wcout << L"Status gefunden: " << name << std::endl;

		// Prüfen des Status
		if (wcscmp(name, L"Offline") == 0) {
			std::wcout << L"Status ist: Offline" << std::endl;
		}
		else if (wcscmp(name, L"Ready") == 0) {
			std::wcout << L"Status ist: Ready" << std::endl;
		}
		else if (wcscmp(name, L"Idle") == 0) {
			std::wcout << L"Status ist: Idle" << std::endl;
		}
		else {
			std::wcout << L"Unbekannter Status: " << name << std::endl;
		}

		SysFreeString(name);
		currentElement->Release();
	}
	else {
		std::wcout << L"Element mit AutomationId '" << automationId << L"' nicht gefunden!" << std::endl;
	}

	SysFreeString(var.bstrVal);
	condition->Release();
}

// Hilfsfunktion: Suche nach einem Fenster mit einem bestimmten Namen (UIAutomation)
IUIAutomationElement* FindWindowByName(IUIAutomation* automation, IUIAutomationElement* parent, LPCWSTR windowName) {
	VARIANT varName;
	varName.vt = VT_BSTR;
	varName.bstrVal = SysAllocString(windowName);

	IUIAutomationCondition* condition = NULL;
	automation->CreatePropertyCondition(UIA_NamePropertyId, varName, &condition);

	IUIAutomationElement* foundElement = NULL;
	HRESULT hr = parent->FindFirst(TreeScope_Descendants, condition, &foundElement);

	condition->Release();
	SysFreeString(varName.bstrVal);

	if (SUCCEEDED(hr) && foundElement) {
		return foundElement;
	}
	return NULL;
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

std::wstring getActiveWindowTitle() {
	HWND hwnd = GetForegroundWindow(); // Aktives Fenster abrufen
	if (hwnd == NULL) return L"";      // Kein aktives Fenster

	wchar_t title[256];
	GetWindowText(hwnd, title, sizeof(title) / sizeof(wchar_t));
	return std::wstring(title);



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

CString CMassHunterInterface::GetInstrumentStatus()
{
	CString InstrumenstatusText;
	// COM initialisieren
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	IUIAutomation* automation = NULL;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		CoUninitialize();
		return L"UI Automation Init Failed";
	}

	HWND hwnd = NULL;
	EnumWindows([](HWND h, LPARAM lParam) -> BOOL {
		wchar_t title[256];
		::GetWindowTextW(h, title, 256);
		if (wcsstr(title, L"Enhanced MassHunter")) {
			*((HWND*)lParam) = h;
			return FALSE;
		}
		return TRUE;
		}, (LPARAM)&hwnd);

	if (!hwnd) {
		automation->Release();
		CoUninitialize();
		return L"MassHunter not found";
	}

	IUIAutomationElement* rootElement = NULL;
	if (FAILED(automation->ElementFromHandle(hwnd, &rootElement)) || !rootElement) {
		automation->Release();
		CoUninitialize();
		return L"UI Element not found";
	}

	// Status-Element suchen
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = SysAllocString(L"labelInstrumentReadinessIndicator");

	IUIAutomationCondition* condition = NULL;
	automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, var, &condition);

	IUIAutomationElement* statusElement = NULL;
	rootElement->FindFirst(TreeScope_Descendants, condition, &statusElement);

	if (statusElement) {
		BSTR name;
		statusElement->get_CurrentName(&name);
		InstrumenstatusText = name;
		SysFreeString(name);
		statusElement->Release();
	}

	SysFreeString(var.bstrVal);
	condition->Release();
	rootElement->Release();
	automation->Release();
	CoUninitialize();

	return InstrumenstatusText;
}

CString CMassHunterInterface::GetRunStatus()
{
	CString RunstatusText;
	// COM initialisieren
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	IUIAutomation* automation = NULL;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		CoUninitialize();
		return L"UI Automation Init Failed";
	}

	HWND hwnd = NULL;
	EnumWindows([](HWND h, LPARAM lParam) -> BOOL {
		wchar_t title[256];
		::GetWindowTextW(h, title, 256);
		if (wcsstr(title, L"Enhanced MassHunter")) {
			*((HWND*)lParam) = h;
			return FALSE;
		}
		return TRUE;
		}, (LPARAM)&hwnd);

	if (!hwnd) {
		automation->Release();
		CoUninitialize();
		return L"MassHunter not found";
	}

	IUIAutomationElement* rootElement = NULL;
	if (FAILED(automation->ElementFromHandle(hwnd, &rootElement)) || !rootElement) {
		automation->Release();
		CoUninitialize();
		return L"UI Element not found";
	}

	// Status-Element suchen
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = SysAllocString(L"labelInstrumentRunStatusIndicator");

	IUIAutomationCondition* condition = NULL;
	automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, var, &condition);

	IUIAutomationElement* statusElement = NULL;
	rootElement->FindFirst(TreeScope_Descendants, condition, &statusElement);

	if (statusElement) {
		BSTR name;
		statusElement->get_CurrentName(&name);
		RunstatusText = name;
		SysFreeString(name);
		statusElement->Release();
	}

	SysFreeString(var.bstrVal);
	condition->Release();
	rootElement->Release();
	automation->Release();
	CoUninitialize();

	return RunstatusText;
}




CMassHunterInterface::CMassHunterInterface(CString& ProjectPath, CString& DataPath)

	: m_ProjectPath(ProjectPath)
	,DataPathFest(DataPath) 
	, m_StatusFont(NULL)
	, m_InfoFont(NULL)
	, m_Color(NULL)
	, m_IsVisible(NULL)
	, m_LastStatusETC(0.0)
	, m_ReplyToDialogs(true)
	, m_WorklistIsSupposedToRun(false)
	, m_Abort(false)
	, m_hIcon(NULL)
	, m_AngleRotation(0.0)
	, m_PointerIndex(0)
	, m_Lastvalue(0.0)
	, m_ColorShift(0.0)
	, m_ColorFactor(1.0)
	, m_Elements(1)
{
	TRACE(L"CMassHunterInterface::CMassHunterInterface\n");
#ifdef MASSHUNTER_USE_COM
	Icp.ControlManagerEvents = NULL; 
	Icp.ControlManagerCookie = 0;
	Icp.DataManagerEvents = NULL;
	Icp.DataManagerCookie = 0;
	LcMsTof.ControlManagerEvents = NULL; 
	LcMsTof.ControlManagerCookie = 0;
	LcMsTof.DataManagerEvents = NULL;
	LcMsTof.DataManagerCookie = 0;
#endif
	Icp.OnlineFound = false;
	Icp.OfflineFound = false;
	LcMsTof.OnlineFound = false;
	LcMsTof.OfflineFound = false;

	Gdiplus::GdiplusStartup(&m_GdiPlusToken, &m_GdiPlusStartupInput, nullptr);
	m_StatusFont = ::new Gdiplus::Font(L"Tahoma", 20.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	m_InfoFont = ::new Gdiplus::Font(L"Arial", 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

	m_GreenSignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_GREEN_16x16);
	m_RedSignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_RED_16x16);
	m_GraySignal = AfxGetApp()->LoadIcon(IMAGE_SIGNAL_GRAY_16x16);
	SetPointer(0xffffffff);
	SetRange(0, 10, 23);

	EnableAutomation();

#ifdef MASSHUNTER_USE_COM
	//AllocConsole(); 
  //freopen("conout$", "w", stderr);
 	//freopen("conin$", "r", stdin); 
  //freopen("conout$", "w", stdout); 
	try
	{
		if (FAILED(LcMsTof.DataManager.CreateInstance(L"Agtpic.AgtPicWklDataManager")))
		{
			GetErrorText();
			throw L"Failed to create Agtpic.AgtPicWklDataManager: " + m_ErrorText;
		}
#ifndef NDEBUG
		//FindProgID(LcMsTof.DataManager.GetInterfacePtr());
#endif
		if (FAILED(LcMsTof.DataManager->Initialize()))
		{
			GetErrorText();
			throw L"Failed to initialize data manager: " + m_ErrorText;
		}
		LcMsTof.DataManagerEvents = GetIDispatch(false);
		if (!AfxConnectionAdvise(LcMsTof.DataManager.GetInterfacePtr(), IID_IcpAgtPicWklDataManagerEvents, LcMsTof.DataManagerEvents, true, &LcMsTof.DataManagerCookie))
		{
			GetErrorText();
			TRACE(L"\tNo data manager events connection: %08X\n", GetLastError());
		}
	
		LCMS_TOF::AGTPICLAUNCHERLib::IPicLauncherPtr Launcher;
		if (FAILED(Launcher.CreateInstance(L"agtpic.agtpicLauncher.piclauncher")))
		{
			GetErrorText();
			throw L"Failed to create agtpic.agtpicLauncher.piclauncher: " + m_ErrorText;
		}

		IDispatch* A = NULL;
		if (FAILED(Launcher->GetEngine(L"WkLstEng", &A)))
		{
			GetErrorText();
			throw L"Failed to get engine 'WkLstEng': " + m_ErrorText;
		}
		LCMS_TOF::AGTPICENGINELib::IAgtPicHostCmdPtr HC(A);
		if (!HC)
		{
			GetErrorText();
			throw L"Failed to get HostCmdPtr for 'WkLstEng': " + m_ErrorText;
		}

		IUnknownPtr UP(HC->GetHostObject(L"WklControlMgr", L"WkLstEng"));
		if (!UP)
		{
			GetErrorText();
			throw L"Failed to get 'WklControlMgr' from 'WkLstEng': " + m_ErrorText;
		}
		if (FAILED(LcMsTof.ControlManager.CreateInstance(L"Agtpic.AgtPicWklControlManager")))
		{
			GetErrorText();
			throw L"Failed to create Agtpic.AgtPicWklControlManager: " + m_ErrorText;
		}
#ifndef NDEBUG
		//FindProgID(LcMsTof.ControlManager.GetInterfacePtr());
#endif
		LcMsTof.ControlManagerEvents = GetIDispatch(false);
		if (!AfxConnectionAdvise(LcMsTof.ControlManager.GetInterfacePtr(), IID_IcpAgtPicWklControlManagerEvents, LcMsTof.ControlManagerEvents, true, &LcMsTof.ControlManagerCookie))
		{
			GetErrorText();
			TRACE(L"\tNo control manager events connection: %08X\n", GetLastError());
		}

		/*if (FAILED(LcMsTof.ControlManager->Initialize()))
		{
			throw L"Failed to initialize control manager: " + m_ErrorText;
		}*/

		TRACE(L"DataManager: %08X ControlManager: %08X\n", LcMsTof.DataManager, LcMsTof.ControlManager); 
		LcMsTof.OnlineFound = LcMsTof.ControlManager != NULL && LcMsTof.DataManager != NULL;
	}
	catch (_com_error E)
	{
		TRACE(L"_com_error:  ");
		TRACE(E.ErrorMessage());
		TRACE(L" %08X\n", E.Error());
		if (E.Description().GetBSTR())
		{	TRACE(E.Description().GetBSTR());
			TRACE(L"\n");
		}
	}
#endif

	if (FindMassHunterWindow()) m_ErrorText.Empty();
	else m_ErrorText = L"no running MassHunter was found";

}




CMassHunterInterface::~CMassHunterInterface()
{
	TRACE(L"CMassHunterInterface::~CMassHunterInterface\n");
#ifdef MASSHUNTER_USE_COM
	AfxConnectionUnadvise(Icp.ControlManager.GetInterfacePtr(), IID_IcpAgtPicWklControlManagerEvents, Icp.ControlManagerEvents, true, Icp.ControlManagerCookie);
	AfxConnectionUnadvise(Icp.DataManager.GetInterfacePtr(), IID_IcpAgtPicWklDataManagerEvents, Icp.DataManagerEvents, true, Icp.DataManagerCookie);
#endif
	if (m_Color) delete m_Color;
	if (m_IsVisible) delete m_IsVisible;
	for (std::map<unsigned int, CButton*>::iterator i(m_Buttons.begin());i!=m_Buttons.end();++i)
	{	delete i->second;
	}
	for (ExecutiveJobs::iterator i(m_Jobs.begin());i!=m_Jobs.end();++i)
	{
		delete i->second;
	}

	if (m_StatusFont) ::delete m_StatusFont;
	if (m_InfoFont) ::delete m_InfoFont;

	Gdiplus::GdiplusShutdown(m_GdiPlusToken);
}

void CMassHunterInterface::GetErrorText(HRESULT HR)
{
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						HR!=0?HR:GetLastError(), GetUserDefaultLangID(),
						m_ErrorText.GetBuffer(1000), 1000, nullptr))
	{	m_ErrorText.ReleaseBuffer();
	}
	else
	{	m_ErrorText.ReleaseBuffer();
		m_ErrorText = L"Unknown Error";
	}
	TRACE(m_ErrorText + L"\n");
}

void CMassHunterInterface::OpenWindow(CString Title, RECT& Size, CWnd* Parent, bool Visible)
{	static CString WindowClass;
	
	if (WindowClass.IsEmpty())
	{	WindowClass = AfxRegisterWndClass(CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS | CS_DBLCLKS,
												LoadCursor(NULL, IDC_ARROW),
												/*(HBRUSH)GetStockObject(WHITE_BRUSH)*/0,
												LoadIcon(AfxGetInstanceHandle(),
												MAKEINTRESOURCE(/*IDR_MAINFRAME*/102)));
	}

	CreateEx(WS_EX_TOPMOST | WS_EX_STATICEDGE, WindowClass, Title, WS_OVERLAPPEDWINDOW | (Visible?WS_VISIBLE:0), Size, /*Parent*/NULL, NULL);
	ShowWindow(Visible?SW_SHOW:SW_HIDE);
	CMenu* M = GetSystemMenu(false);
	M->EnableMenuItem(SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);
	M->EnableMenuItem(SC_SIZE, MF_GRAYED | MF_BYCOMMAND);
	M->EnableMenuItem(SC_MOVE, MF_GRAYED | MF_BYCOMMAND);
	M->EnableMenuItem(SC_MINIMIZE, MF_ENABLED | MF_BYCOMMAND);
	M->EnableMenuItem(SC_MAXIMIZE, MF_GRAYED | MF_BYCOMMAND);
	m_hIcon = AfxGetApp()->LoadIcon(/*IDR_MAINFRAME*/128);
	SetIcon(m_hIcon, false);

	SetTimer(TimerID_Timeout, 1000, nullptr);
}

void CMassHunterInterface::SetStatusText(LPCTSTR Text)
{
	m_StatusText = Text;
	InvalidateRect(nullptr);
}

void CMassHunterInterface::SetInfoText(LPCTSTR Text)
{
	m_InfoText = Text;
	InvalidateRect(nullptr);
}

void CMassHunterInterface::SetText(LPCTSTR Status, LPCTSTR Info)
{
	m_StatusText = Status;
	m_InfoText = Info;
	InvalidateRect(nullptr);
}

void CMassHunterInterface::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 330;
	lpMMI->ptMinTrackSize.y = 200;

	CWnd::OnGetMinMaxInfo(lpMMI);
}

void CMassHunterInterface::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

	int l = cy;
	if (cx < cy) l = cx;
	if (l>142) l = 142;
	int x = int(double(l) * BorderReduction);
	m_CircleRect.left = x;
	m_CircleRect.right = l - x;
	m_CircleRect.top = x;
	m_CircleRect.bottom = l - x;

	m_StatusRect.top = 0;
	m_StatusRect.bottom = l;
	m_StatusRect.left = l;
	m_StatusRect.right = m_StatusRect.left + 170;

	m_InfoRect.bottom = m_StatusRect.bottom;
	m_InfoRect.top = m_InfoRect.bottom - 48;
	m_InfoRect.left = m_StatusRect.right;
	m_InfoRect.right = cx;
}

void CMassHunterInterface::OnPaint() 
{	
	using namespace Gdiplus;

	CPaintDC dc(this); 
	dc.SetBkColor(GetSysColor(COLOR_3DFACE));
	{
		CRect R(dc.m_ps.rcPaint);
		CMemDC	mdc(&dc, true, &R);
		{
			Graphics* G = Graphics::FromHDC(mdc);
			G->SetSmoothingMode(SmoothingModeAntiAlias);
			G->SetCompositingMode(CompositingModeSourceOver);

			Color C;
			C.SetFromCOLORREF(GetSysColor(COLOR_3DFACE));
			SolidBrush B(C);

			double AngleStep = 700 / m_Elements;
			m_AngleRotation+=AngleStep * RotationSpeed;
			if (!(m_AngleRotation<700)) m_AngleRotation = 0.0;
			double Angle = 180 + m_AngleRotation;
			Angle-=AngleStep * SegmentReduction / 2;
			Color BkColor;
			BkColor.SetFromCOLORREF(GetSysColor(COLOR_3DFACE));
			for (unsigned int Index=0;Index<m_Elements;Index++)
			{	if (m_IsVisible[Index])
				{	if (Index==m_PointerIndex)
					{	Color X(m_Color[Index].GetA(), m_Color[Index].GetR(), m_Color[Index].GetG(), 255);
						B.SetColor(X);
					}
					else
					{	B.SetColor(m_Color[Index]);
					}
					const REAL Spiral = 0.80f;
					G->FillPie(&B, REAL(m_CircleRect.left + Spiral * Index), REAL(m_CircleRect.top + Spiral * Index),
													REAL(m_CircleRect.right - m_CircleRect.left - 2 * Spiral * Index),
													REAL(m_CircleRect.bottom - m_CircleRect.top - 2 * Spiral * Index),
													REAL(Angle), REAL(AngleStep * SegmentReduction));

				/*	B.SetColor(BkColor);
					G->FillPie(&B, REAL(m_CircleRect.left + Spiral * Index + 10), REAL(m_CircleRect.top + Spiral * Index + 10),
													REAL(m_CircleRect.right - m_CircleRect.left - 2 * Spiral * Index - 20),
													REAL(m_CircleRect.bottom - m_CircleRect.top - 2 * Spiral * Index - 20),
													REAL(Angle - 2), REAL(AngleStep));*/
				}
				Angle-=AngleStep;
			}
			double Radius = min(m_CircleRect.right - m_CircleRect.left, m_CircleRect.bottom - m_CircleRect.top) * InnerRadiusReduction;
			//Color BkColor;
			//BkColor.SetFromCOLORREF(GetSysColor(COLOR_3DFACE));
			//B.SetColor(BkColor);
			RectF IR(REAL(m_CircleRect.left + 0.5 * (m_CircleRect.right - m_CircleRect.left) - Radius),
							REAL(m_CircleRect.top + 0.5 * (m_CircleRect.bottom - m_CircleRect.top) - Radius),
							REAL(2 * Radius), REAL(2 * Radius));
			//G->FillPie(&B, IR, 0.0, 360.0);

			B.SetColor(Color(255, 255, 255, 255));
			StringFormat SF;
			SF.SetAlignment(StringAlignmentCenter);
			SF.SetLineAlignment(StringAlignmentCenter);
			//G->DrawString(m_CenterText, -1, m_StatusFont, IR, &SF, &B);
			Pen P(Color(255, 0, 0, 0), 2);
			GraphicsPath Path;
			Path.AddString(m_CenterText, -1, &FontFamily(L"Tahoma"), FontStyleBold, 24, IR, &SF);
			G->DrawPath(&P, &Path);
			G->FillPath(&B, &Path);

			//mdc.FillSolidRect(&m_StatusRect, RGB(255, 0, 0));
			B.SetColor(Color(255, 200, 0, 0));
			IR.X = REAL(m_StatusRect.left);
			IR.Y = REAL(m_StatusRect.top) + 80.0f;
			IR.Width = REAL(m_StatusRect.right - m_StatusRect.left);
			IR.Height = REAL(m_StatusRect.bottom - m_StatusRect.top - 80.0f);
			G->DrawString(m_StatusText, -1, m_StatusFont, IR, &SF, &B);


			//mdc.FillSolidRect(&m_InfoRect, RGB(0, 255, 0));	
			B.SetColor(Color(255, 0, 0, 0));
			IR.X = REAL(m_InfoRect.left);
			IR.Y = REAL(m_InfoRect.top);
			IR.Width = REAL(m_InfoRect.right - m_InfoRect.left);
			IR.Height = REAL(m_InfoRect.bottom - m_InfoRect.top);
			SF.SetAlignment(StringAlignmentNear);
			G->DrawString(m_InfoText, -1, m_InfoFont, IR, &SF, &B);

			C.SetFromCOLORREF(GetSysColor(COLOR_BTNTEXT));
			B.SetColor(C);
			IR.X = m_StatusRect.left + 20.0f;
			IR.Y = m_StatusRect.top + 8.0f;
			IR.Width = 160.0f;
			IR.Height = 16.0f;
			const REAL IconShift = -20.0f;
			G->DrawString(L"Online ICP-MS", -1, m_InfoFont, IR, &SF, &B);
			IR.Offset(0, 20);
			G->DrawString(L"Offline ICP-MS", -1, m_InfoFont, IR, &SF, &B);
			IR.Offset(0, 20);
			G->DrawString(L"Online GC-MS", -1, m_InfoFont, IR, &SF, &B);
			IR.Offset(0, 20);
			G->DrawString(L"Offline GC-MS", -1, m_InfoFont, IR, &SF, &B);

			delete G;
		}
		CRect IR;
		IR.left = m_StatusRect.left + 20;
		IR.top = m_StatusRect.top + 8;
		IR.right = IR.left + 16;
		IR.bottom = IR.top + 16;
		const int IconShift = -20;
		DrawIconEx(mdc, IR.left + IconShift, IR.top, Icp.OnlineFound?m_GreenSignal:m_GraySignal, 16, 16, 0, NULL, DI_NORMAL);
		IR.OffsetRect(0, 20);
		DrawIconEx(mdc, IR.left + IconShift, IR.top, Icp.OfflineFound?m_GreenSignal:m_GraySignal, 16, 16, 0, NULL, DI_NORMAL);
		IR.OffsetRect(0, 20);
		DrawIconEx(mdc, IR.left + IconShift, IR.top, LcMsTof.OnlineFound?m_GreenSignal:m_GraySignal, 16, 16, 0, NULL, DI_NORMAL);
		IR.OffsetRect(0, 20);
		DrawIconEx(mdc, IR.left + IconShift, IR.top, LcMsTof.OfflineFound?m_GreenSignal:m_GraySignal, 16, 16, 0, NULL, DI_NORMAL);
	}
}

Gdiplus::Color CMassHunterInterface::SectionColor(double Color)
{	long c;

	int f = int(m_ColorFactor * (Color - m_ColorShift));
	if (f<0x100) c = f << 8;
	else if (f<0x200) c = ((f & 0x00ff) << 16) | 0x00ff00;
	else c = 0x00ffff00 - ((f&0x00ff) << 8);
	c|=0xff000000;
	return c;
}

void CMassHunterInterface::SetRange(double Minimum, double Maximum, unsigned int Elements)
{
	if (m_Color) ::delete m_Color;
	if (m_IsVisible) delete m_IsVisible;

	m_Elements = Elements;
	m_Color = ::new Gdiplus::Color[m_Elements];
	m_IsVisible = new bool[m_Elements];

	m_ColorShift = Minimum;
	if ((Maximum - Minimum) != 0.0) m_ColorFactor = 767.0 / (Maximum - Minimum);
	else m_ColorFactor = 511.0;

	for (unsigned int Index=0;Index<m_Elements;Index++)
	{	m_IsVisible[Index] = true;
		m_Color[Index] = SectionColor(Maximum / m_Elements);
	}
	m_PointerIndex = 0xffffffff;	//no pointer
	m_AngleRotation = 0.0;
}

void CMassHunterInterface::SetValue(double Value, bool Visible, unsigned int Element)
{
	if (Element>=m_Elements) return;
	m_IsVisible[Element] = Visible;
	m_Color[Element] = SectionColor(Value);
	if (IsWindow(m_hWnd)) InvalidateRect(NULL);
}

void CMassHunterInterface::SetPointer(unsigned int Pointer)
{
	if (Pointer>m_Elements) return;
	m_PointerIndex = Pointer;
}

void CMassHunterInterface::OnButtons(UINT nID)
{
	std::map<unsigned int, CButton*>::iterator B(m_Buttons.find(nID));
	_ASSERTE(B!=m_Buttons.end());
	if (B==m_Buttons.end()) return;
	CString T;
	B->second->GetWindowText(T);
	if (T==L"Start")
	{	SetInfoText(L"Click button Finished after worklist run is complete");
		SetStatusText(L"Wait for Worklist");
		m_SelfReplyBuffer.first = nID;
		m_SelfReplyBuffer.second = L"Finished";
		B->second->SetWindowText(m_SelfReplyBuffer.second);
		SetTimer(TimerID_Buttons, 500, NULL);
		return;
	}

	bool WasAborted = false;
	if (T==L"Finished")
	{	
		m_WorklistIsSupposedToRun = false;
	}
	else if (T==L"Abort")
	{	WasAborted = true;
		m_WorklistIsSupposedToRun = false;
	}
	SetInfoText(L"");
	SetStatusText(L"");
	ButtonsClear();

	// TODO: identify this job
	ExecutiveJobs::iterator Job(m_Jobs.begin());
	if (Job->second->Handler)
	{	Job->second->Success = !WasAborted;
		if (WasAborted) Job->second->ErrorText = L"Aborted";
		else Job->second->ErrorText.Empty();
		::PostMessage(Job->second->Handler, Job->second->MsgID, Job->first, (LPARAM)Job->second);
	}
	else delete Job->second;
	m_Jobs.erase(Job);
}

void CMassHunterInterface::ButtonsClear()
{
		for (std::map<unsigned int, CButton*>::iterator i(m_Buttons.begin());i!=m_Buttons.end();++i)
		{	i->second->DestroyWindow();
			delete i->second;
		}
		m_Buttons.clear();
}

void CMassHunterInterface::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent==TimerID_Buttons)
	{	if (m_ReplyToDialogs)
		{	KillTimer(nIDEvent);
			if (m_SelfReplyBuffer.first) OnButtons(m_SelfReplyBuffer.first);
		}
	}
	else if (nIDEvent==TimerID_Timeout)
	{
		CheckJobTimeouts();
		if (m_WorklistIsSupposedToRun)
		{
			// TODO: identify this job
			ExecutiveJobs::iterator Job(m_Jobs.begin());
			if (Job!=m_Jobs.end())
			{	DWORD T = timeGetTime() - Job->second->StartTime;
				m_CenterText.Format(L"%u", (Job->second->Timeout - T) / 1000);
				SetValue(T / 1000, true, int(23.0 * T / Job->second->Timeout));
			}
			else
			{	DWORD T = abs(rand() / 100);
				m_CenterText.Format(L"%u", T);
				SetValue(T, true, int(23.0 * T / 330));
			}
		}
	}
	CWnd::OnTimer(nIDEvent);
}

RECT CMassHunterInterface::LoadPosition(CString ConfigurationName)
{
//	TRACE(L"CMassHunterInterface::LoadPosition: %s\n", ConfigurationName);
	RECT R;
	R.top = AfxGetApp()->GetProfileInt(ConfigurationName, L"EI Top", 10);
	R.left = AfxGetApp()->GetProfileInt(ConfigurationName, L"EI Left", 10);
	R.bottom = R.top + AfxGetApp()->GetProfileInt(ConfigurationName, L"EI Height", 100);
	R.right = R.left + AfxGetApp()->GetProfileInt(ConfigurationName, L"EI Width", 200);
	return R;
}

void CMassHunterInterface::SavePosition(CString ConfigurationName)
{
//	TRACE(L"CMassHunterInterface::SavePosition: %s\n", ConfigurationName);
	RECT R;
	GetWindowRect(&R);
	if (R.left>=0 && R.top>=0)
	{	AfxGetApp()->WriteProfileInt(ConfigurationName, L"EI Top", R.top);
		AfxGetApp()->WriteProfileInt(ConfigurationName, L"EI Left", R.left);
		AfxGetApp()->WriteProfileInt(ConfigurationName, L"EI Height", R.bottom - R.top);
		AfxGetApp()->WriteProfileInt(ConfigurationName, L"EI Width", R.right - R.left);
	}
}

//***************************************************

bool Überprüfen_Ordner2(wstring Folder_Path2) {
	DWORD attributes = GetFileAttributes(Folder_Path2.c_str());

	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}



void CMassHunterInterface::SetProjectPath(const CString& Path)
{

	 m_ProjectPath = Path;

	 std::wstring wstrPath(Path.GetString());
	

	 if (!Überprüfen_Ordner2(wstrPath))
	 {

		 AfxMessageBox(_T("Bitte einen gültigen Pfad auswählen!"), MB_ICONWARNING);
	 }

}


void CMassHunterInterface::SetCurrentDatatPath(const CString& path2) {

	DataPathFest = path2;

	std::wstring wstrPath2(path2.GetString());


	if (!Überprüfen_Ordner2(wstrPath2))
	{

		AfxMessageBox(_T("Bitte einen gültigen Pfad auswählen!"), MB_ICONWARNING);
	}
}


bool CMassHunterInterface::ReadProjectList()
{
	TRACE(L"CMassHunterInterface::ReadProjectList\n");
	m_ProjectList.clear();



	//m_ProjectPath.IsEmpty()
	if (m_ProjectPath.IsEmpty() || m_ProjectPath.CompareNoCase(L"C:\\") == 0)

	{
		m_ProjectPath = L"D:\\MassHunter\\GCMS\\1\\";
	
	
	}

	if (DataPathFest.IsEmpty() || DataPathFest.CompareNoCase(L"C:\\") == 0) {

		DataPathFest = L"D:\\MassHunter\\GCMS\\1\\data\\";
		


	}
//	AfxMessageBox(L"DataPathFest ist: " + DataPathFest);

	WIN32_FIND_DATA findFileData;


	//m_ProjectPath = L"C:\\Semester 5\\Masterarbeit\\Boost\\LsaToMassHunter\\LsaToMassHunter\\Test Data\\";
	CString searchPath = m_ProjectPath + L"*" ;  // z. B. C:\Rostock\*
	


	HANDLE hFind = FindFirstFile(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return false;
	}

	do {
		CString folderName = findFileData.cFileName;

		if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			folderName != L"." && folderName != L".." && folderName.Right(2) != L".M")
		{
			m_ProjectList.push_back(std::make_pair(folderName, 0));
		}

	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
	return true;
}

bool CMassHunterInterface::ReadMethodsForProject(const CString& ProjectName)
{
	m_MethodList.clear();
	CString path = m_ProjectPath + ProjectName + L"\\*.M";

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(path, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		//::AfxMessageBox(L"FindFirstFile fehlgeschlagen");
		return false;
	}

	do {
		if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			wcscmp(findFileData.cFileName, L".") != 0 &&
			wcscmp(findFileData.cFileName, L"..") != 0)
		{
			m_MethodList.push_back(findFileData.cFileName);
			//::AfxMessageBox(L"Gefundene Methode: " + CString(findFileData.cFileName));
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
	return !m_MethodList.empty();
}


bool CMassHunterInterface::GetMethod(const CString& MethodName, unsigned int MethodRevision)
{
	TRACE(L"CMassHunterInterface::GetMethod\n");
	return true;
}

bool CMassHunterInterface::GetETCForMethod(const CString& MethodName, unsigned int MethodRevision, double& ETC)
{
	TRACE(L"CMassHunterInterface::GetETCForMethod\n");
	return true;
}

bool CMassHunterInterface::MethodExists(CString& MethodName, unsigned int /*MethodRevision*/)
{
	TRACE(L"CMassHunterInterface::MethodExists: %s\n", MethodName);
	if (MethodName.IsEmpty()) return false;
	return GetFileAttributes(MethodName)!=INVALID_FILE_ATTRIBUTES;
}

bool CMassHunterInterface::StartMethod2(CMethodRun* Method, DWORD MilliSeconds)
{
	TRACE(L"CMassHunterInterface::StartMethod2\n");
	m_Abort = false;
	Method->Timeout = MilliSeconds;
	m_Jobs.insert(std::make_pair(Method->JobID, Method));

	SetStatusText(L"Start Worklist/Batch");
	SetInfoText(L"Start worklist '" + Method->NewBatchFileName + L"' and click button Start");
	int BID = BUTTON_ID_START;
	ButtonsClear();
	RECT R;
	R.top = 10;
	R.bottom = R.top + 25;
	R.left = 142 / 2 + m_StatusRect.right + 10;
	R.right = R.left + 80;
	int Pos = 0;
	int LastPos = 0;
	std::pair<std::map<unsigned int, CButton*>::iterator, bool> B(m_Buttons.insert(std::make_pair(BID, new CButton())));
	m_SelfReplyBuffer.first = BID++;
	m_SelfReplyBuffer.second = L"Start";
	B.first->second->Create(m_SelfReplyBuffer.second, BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, R, this, m_SelfReplyBuffer.first);
	B.first->second->ShowWindow(SW_SHOW);
	R.top+=30;
	R.bottom = R.top + 25;
	B = m_Buttons.insert(std::make_pair(BID, new CButton()));
	B.first->second->Create(L"Abort", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER, R, this, BID++);
	B.first->second->ShowWindow(SW_SHOW);

	SetRange(0.0, MilliSeconds / 1000.0, 23);
	m_WorklistIsSupposedToRun = true;

	SetTimer(TimerID_Buttons, 500, nullptr);
	return true;
}

bool CMassHunterInterface::ScheduleMethod(CMethodSchedule* Method, DWORD MilliSeconds)
{
	TRACE(L"CMassHunterInterface::ScheduleMethod\n");
	return true;
}

bool CMassHunterInterface::WriteMethodToFile(CMethodRun* Method, DWORD MilliSeconds)
{
	TRACE(L"CMassHunterInterface::WriteMethodToFile: %s\n", Method->NewBatchFileName);

#ifdef MASSHUNTER_USE_COM
	std::list<LCMS_TOF::AGTPICWORKLISTLib::SWkl_AttributeInfo> AtributeInfo;

	SAFEARRAY* Y = LcMsTof.DataManager->CreateWorklist(L"");
	long LB, UB;
	HRESULT hr = SafeArrayGetLBound(Y, 1, &LB);
	if (FAILED(hr)) throw _com_error(hr);
	hr = SafeArrayGetUBound(Y, 1, &UB);
	if (FAILED(hr)) throw _com_error(hr);

	LCMS_TOF::AGTPICWORKLISTLib::SWkl_AttributeInfo* AI;
	hr = SafeArrayAccessData(Y, (void**)&AI);
	if (FAILED(hr)) throw _com_error(hr);
	for (long i=LB;i<UB;++i)
	{	AtributeInfo.push_back(AI[i]);
	}
	hr = SafeArrayUnaccessData(Y);
	if (FAILED(hr)) throw _com_error(hr);

	hr = SafeArrayDestroy(Y);
	if (FAILED(hr)) throw _com_error(hr);

	LCMS_TOF::AGTPICWORKLISTLib::SWkl_WorklistExecutionInfo X;
	X = LcMsTof.DataManager->GetWorklistExecutionInfo();
  X.m_bstrMethodExecutionType = L"Acquisition Only";
  X.m_lThresholdDiskSpace = 0;
	LcMsTof.DataManager->PutWorklistExecutionInfo(&X);

	LcMsTof.DataManager->SaveWorklist(L"Worklist.xml");
	LcMsTof.DataManager->SaveInteractiveSample(L"InteractiveSample.xml");

	LCMS_TOF::AGTPICWORKLISTLib::SWkl_InteractiveSampleExecutionInfo EI;
	LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleAttributeInfo SAI;
	long S = LcMsTof.DataManager->GetInteractiveSampleWithAttribute(&SAI, &EI);

	LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleInfo SI = LcMsTof.DataManager->GetSampleInfoDefaultValue();
	SI.m_bstrAcqMethod = L"DrinkingWater";
  SI.m_bstrDataFileName = L"eumel";
  SI.m_bstrSamplePosition = L"-1";
  SI.m_bstrMethodExecutionType = L"Acquisition Only";
  SI.m_bstrBarcode = L"";
  SI.m_bstrDAMethod = L"";      //Doing acquisition only, so setting DA to null
  SI.m_bstrSampleType = L"Sample";
  SI.m_bstrDescription = L"";   // Don't care
  SI.m_bstrLabel = L"";         // Don't care
  SI.m_bstrPlateCode = L"";     // Don't care
  SI.m_bstrPlatePosition = L"";
  SI.m_bstrRackCode = L"";
  SI.m_bstrRackPosition = L"";
  SI.m_bstrReserved1 = L"";
  SI.m_bstrReserved2 = L"";
  SI.m_bstrReserved3 = L"";
  SI.m_dReserved4 = 0;
  SI.m_dReserved5 = 0;
  SI.m_dReserved6 = 0;
  SI.m_bstrSampleID = L"";
  SI.m_bstrSampleName = L"";
  SI.m_dDilutionFactor = 1;
  SI.m_dEquilibrationTime = 0;
  SI.m_dInjectionVolume = -1;
  SI.m_dWeightPerVolume = 0;
  SI.m_bstrBalanceType = L"No Override";
  SI.m_bstrCalibLevelName = L"";
	SAFEARRAYBOUND B[1];
	B[0].cElements = 2;
	B[0].lLbound = 0;
	IRecordInfo *RI;
	hr = GetRecordInfoFromGuids(__uuidof(LCMS_TOF::AGTPICWORKLISTLib::__AGTPICWORKLISTLib), 1, 0, LOCALE_USER_DEFAULT,
		__uuidof(LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleData), &RI);
	SI.m_pSampleDataArray = SafeArrayCreateEx(VT_RECORD, 1, B, RI);
	LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleData* SD;
	hr = SafeArrayAccessData(SI.m_pSampleDataArray, (void**)&SD);
	if (FAILED(hr)) throw _com_error(hr);
	for (unsigned long i=B[0].lLbound;i<B[0].cElements;++i)
	{	SD[i].m_lAttributeID = 45;
		SD[i].m_varDataValue.vt = VT_BSTR;
		SD[i].m_varDataValue.bstrVal = NULL;
	}
	hr = SafeArrayUnaccessData(SI.m_pSampleDataArray);
	if (FAILED(hr)) throw _com_error(hr);

	
	try
	{	Y = NULL;
		hr = LcMsTof.DataManager->GetAllBatchIDs(&Y);
		if (Y)
		{
			hr = SafeArrayGetLBound(Y, 1, &LB);
			if (FAILED(hr)) throw _com_error(hr);
			hr = SafeArrayGetUBound(Y, 1, &UB);
			if (FAILED(hr)) throw _com_error(hr);
		}


		LCMS_TOF::AGTPICWORKLISTLib::SWkl_BatchInfo BI;
		BI.m_bstrLabel = L"Spucke";
		BI.m_pAtrributeInfoArray = NULL;
		BI.m_pBatchDataArray = NULL;
		//long BID = LcMsTof.DataManager->AddBatch(&BI);
		long SID = LcMsTof.DataManager->AddSample(-1, &SI);
	}
	catch (_com_error E)
	{
		TRACE(L"_com_error:  ");
		TRACE(E.ErrorMessage());
		TRACE(L" %08X\n", E.Error());
		if (E.Description().GetBSTR())
		{	TRACE(E.Description().GetBSTR());
			TRACE(L"\n");
		}
	}

	LcMsTof.DataManager->SaveWorklist(L"eumel.xml");

	LCMS_TOF::AGTPICWORKLISTLib::SWkl_JobInfo Job = LcMsTof.DataManager->LoadWorklist(L"eumel.xml", &X); 
#endif
	return true;
}


// funktion: Suche nach einem Element mit AutomationId oder Name

IUIAutomationElement* FindElementByNameOrAutomationId(IUIAutomation* automation, IUIAutomationElement* parent, LPCWSTR name, LPCWSTR automationId) {
	IUIAutomationCondition* conditionName = NULL;
	IUIAutomationCondition* conditionId = NULL;
	IUIAutomationCondition* combinedCondition = NULL;

	// Suchbedingungen erstellen
	if (name) {
		VARIANT varName;
		varName.vt = VT_BSTR;
		varName.bstrVal = SysAllocString(name);
		automation->CreatePropertyCondition(UIA_NamePropertyId, varName, &conditionName);
		SysFreeString(varName.bstrVal);
	}

	if (automationId) {
		VARIANT varAutomationId;
		varAutomationId.vt = VT_BSTR;
		varAutomationId.bstrVal = SysAllocString(automationId);
		automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, varAutomationId, &conditionId);
		SysFreeString(varAutomationId.bstrVal);
	}

	// Bedingungen kombinieren
	if (conditionName && conditionId) {
		automation->CreateAndCondition(conditionName, conditionId, &combinedCondition);
		conditionName->Release();
		conditionId->Release();
	}
	else if (conditionName) {
		combinedCondition = conditionName;
	}
	else if (conditionId) {
		combinedCondition = conditionId;
	}

	// Falls kein Parent angegeben ist, benutze das Root-Element
	if (!parent) {
		automation->GetRootElement(&parent);
	}

	// Suche das Element
	IUIAutomationElement* foundElement = NULL;
	if (SUCCEEDED(parent->FindFirst(TreeScope_Descendants, combinedCondition, &foundElement))) {
		combinedCondition->Release();
		return foundElement;
	}

	// Bedingungen freigeben
	if (combinedCondition) combinedCondition->Release();

	return 0;
}
// Funktion: Klick auf eine Button

bool Button_Drucken(IUIAutomation* automation, IUIAutomationElement* parent, LPCWSTR name_Button, LPCWSTR automationId_Button) {
	// Suche nach dem Button und klicken
	IUIAutomationElement* Button = FindElementByNameOrAutomationId(automation, parent, name_Button, automationId_Button);
	if (Button) {
		IUIAutomationInvokePattern* Button_Invoke = NULL;
		if (SUCCEEDED(Button->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&Button_Invoke)))) {
			Button_Invoke->Invoke(); //  klicken
			Button_Invoke->Release();
		}
		Button->Release();
		Sleep(1000);
	}
	return true;
}





void ClickButtonByAutomationIdAndName1(const wchar_t* automationId, const wchar_t* name)
{
	//HRESULT hr;
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (FAILED(hr)) {
		std::wcerr << L"CoInitializeEx failed\n";
		return;
	}
	IUIAutomation* pAutomation = nullptr;

	hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pAutomation));
	if (FAILED(hr)) {
		std::wcerr << L"CoCreateInstance failed\n";
		return;
	}

	// Root Element (Desktop)
	IUIAutomationElement* pRoot = nullptr;
	hr = pAutomation->GetRootElement(&pRoot);
	if (FAILED(hr)) {
		std::wcerr << L"GetRootElement failed\n";
		pAutomation->Release();
		return;
	}

	// Conditions für AutomationId und Name
	IUIAutomationCondition* pConditionId = nullptr;
	IUIAutomationCondition* pConditionName = nullptr;
	IUIAutomationCondition* pAndCondition = nullptr;

	pAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, _variant_t(automationId), &pConditionId);
	pAutomation->CreatePropertyCondition(UIA_NamePropertyId, _variant_t(name), &pConditionName);
	pAutomation->CreateAndCondition(pConditionId, pConditionName, &pAndCondition);

	// Button suchen
	IUIAutomationElement* pButton = nullptr;
	hr = pRoot->FindFirst(TreeScope_Subtree, pAndCondition, &pButton);

	if (SUCCEEDED(hr) && pButton != nullptr) {
		// Button klicken
		IUIAutomationInvokePattern* pInvoke = nullptr;
		hr = pButton->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&pInvoke));
		if (SUCCEEDED(hr)) {
			pInvoke->Invoke();  // Klick
			std::wcout << L"Button wurde geklickt\n";
			pInvoke->Release();
		}
		else {
			std::wcerr << L"InvokePattern konnte nicht geladen werden\n";
		}
		pButton->Release();
	}
	else {
		std::wcerr << L"Button nicht gefunden\n";
	}

	// Aufräumen
	if (pConditionId) pConditionId->Release();
	if (pConditionName) pConditionName->Release();
	if (pAndCondition) pAndCondition->Release();
	if (pRoot) pRoot->Release();
	pAutomation->Release();
	CoUninitialize();
}


void CMassHunterInterface::Abort()
{
	TRACE(L"CMassHunterInterface::Abort\n");

	// UI Automation vorbereiten
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	IUIAutomation* automation = NULL;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		CoUninitialize();
		return;
	}

	// Sicherstellen, dass das Fenster offen ist
	if (!BringWindowToFront(L"Enhanced MassHunter"))
	{
		automation->Release();
		CoUninitialize();
		return;
	}

	// Hier ggf. ID mit Inspect.exe prüfen!
	//Button_Drucken(automation, NULL, NULL, L"buttonStop");

	ClickButtonByAutomationIdAndName1(L"buttonStop",NULL);

	Sleep(20000);
	ClickButtonByAutomationIdAndName1(L"buttonYes", L"Yes");

//	Button_Drucken(automation, NULL, L"Yes", L"buttonYes");

	Sleep(20000);
	ClickButtonByAutomationIdAndName1(L"2", L"OK");

	//Button_Drucken(automation, NULL, L"OK", L"2");

	automation->Release();
	CoUninitialize();
	m_Abort = true;
}


void CMassHunterInterface::Resume()
{
	TRACE(L"CMassHunterInterface::Resume\n");
}

void CMassHunterInterface::Pause()
{
	TRACE(L"CMassHunterInterface::Pause\n");
}

void CMassHunterInterface::CheckJobTimeouts()
{
	for (ExecutiveJobs::iterator i(m_Jobs.begin());i!=m_Jobs.end();)
	{
		if (!i->second->IsTimeout())
		{	++i;
			continue;
		}
		TRACE(L"CMassHunterInterface::CheckJobTimeouts: %u\n", i->first);
		if (i->second->Handler)
		{	i->second->Success = false;
			i->second->ErrorText = L"Timeout (Maybe nobody came to push the button)";
			::PostMessage(i->second->Handler, i->second->MsgID, i->first, (LPARAM)i->second);
		}
		else delete i->second;
		i = m_Jobs.erase(i);
		SetInfoText(L"");
		SetStatusText(L"");
		// TODO: job specific
		m_WorklistIsSupposedToRun = false;
		ButtonsClear();
	}
}

#ifdef MASSHUNTER_USE_COM
HRESULT CMassHunterInterface::OnAcquisitionSampleRunEnd(CComVariant varWklBatchID, long lJobID )
{
	return 0;
}

HRESULT CMassHunterInterface::OnAcquisitionSampleRunStart(CComVariant varWklBatchID, long lJobID, LCMS_TOF::AGTPICWORKLISTLib::SWkl_SampleAttributeInfo *pSampleInfo)
{
	return 0;
}

HRESULT CMassHunterInterface::OnDAJobRunStart(long lJobID, LCMS_TOF::AGTPICWORKLISTLib::EnumJobType enumJobType)
{
	return 0;
}

HRESULT CMassHunterInterface::OnJobRunStart(CComVariant varWklBatchID, LCMS_TOF::AGTPICWORKLISTLib::SWkl_NewJobData *pJobData)
{
	return 0;
}

HRESULT CMassHunterInterface::OnExecutionComplete(LCMS_TOF::AGTPICWORKLISTLib::EnumWklRunResultState enumRunState, HRESULT hrValue, CComBSTR bstrErrorMsg)
{
	return 0;
}

HRESULT CMassHunterInterface::OnRunStart(CComVariant varWklBatchID)
{
	return 0;
}
#endif

BOOL CALLBACK CMassHunterInterface::EnumChildProc(HWND hwnd, LPARAM lParam)
{
	CWindowToFind* WTF = (CWindowToFind*)lParam;
	if (!--WTF->MaxWindows) return false;
	wchar_t T[MAX_PATH + 200];
	if (!WTF->Title.IsEmpty())
	{	if (!::GetWindowText(hwnd, T, sizeof(T) / sizeof(wchar_t))) return true;
		if (wcsncmp(WTF->Title, T, WTF->Title.GetLength())) return true;
	}
	if (!::GetClassName(hwnd, T, sizeof(T) / sizeof(wchar_t))) return true;
	wchar_t X[200];
	::GetWindowText(hwnd, X, 200);
	//TRACE(L"\t%s = %s\n", T, X);
	if (wcsncmp(WTF->Class, T, WTF->Class.GetLength())) return true;
	WTF->Window = hwnd;
	return false;
}

const wchar_t* CMassHunterInterface::IcpMassHunterOnlineWindowTitle = L"Online ICP-MS MassHunter";
const wchar_t* CMassHunterInterface::IcpMassHunterOfflineWindowTitle = L"Offline ICP-MS MassHunter";
const wchar_t* CMassHunterInterface::IcpMassHunterWindowClass = L"WindowsForms10.Window.8.app.0.";	//"13a5ba8";

HWND CMassHunterInterface::FindMassHunterWindow(bool _Icp)
{
	if (_Icp)
	{	CWindowToFind WTF(IcpMassHunterOnlineWindowTitle, IcpMassHunterWindowClass);
		while (EnumWindows(EnumChildProc, (LPARAM)&WTF));
		Icp.OnlineFound = WTF.Window != NULL;
		Icp.OfflineFound = false;
		if (!WTF.Window)
		{	WTF.MaxWindows = 1000;
			WTF.Title = IcpMassHunterOfflineWindowTitle;
			while (EnumWindows(EnumChildProc, (LPARAM)&WTF));
			Icp.OfflineFound = WTF.Window != NULL;
		}
		return WTF.Window;
	}
	// TODO LC-TOF
	return NULL;
}

bool CMassHunterInterface::StartMethod(CMethodRun* Method, DWORD MilliSeconds)
{
	TRACE(L"CMassHunterInterface::StartMethod\n");
	if (m_hWnd) ShowWindow(SW_SHOW);
	m_Abort = false;
	Method->Timeout = MilliSeconds;
	DWORD StartTime = timeGetTime();
	HWND MassHunterWindow = FindMassHunterWindow();
	if (!MassHunterWindow) return false;
	//::BringWindowToTop(MassHunterWindow);
	//::SetFocus(MassHunterWindow);
	SetForegroundWindowInternal2(MassHunterWindow);

	// ALT-D / "Default Layout" clicken
	
	INPUT IP[4]{0};
	/*IP[0].type = INPUT_KEYBOARD;
	IP[0].ki.dwFlags = 0;
	IP[0].ki.wScan = 0;
	IP[0].ki.wVk = VK_MENU;
	IP[0].ki.time = 0;
	IP[1].type = INPUT_KEYBOARD;
	IP[1].ki.dwFlags = 0;
	IP[1].ki.wScan = 0;
	IP[1].ki.wVk = L'D';
	IP[1].ki.time = 0;
	IP[2].type = INPUT_KEYBOARD;
	IP[2].ki.dwFlags = KEYEVENTF_KEYUP;
	IP[2].ki.wScan = 0;
	IP[2].ki.wVk = L'D';
	IP[2].ki.time = 0;
	IP[3].type = INPUT_KEYBOARD;
	IP[3].ki.dwFlags = KEYEVENTF_KEYUP;
	IP[3].ki.wScan = 0;
	IP[3].ki.wVk = VK_MENU;
	IP[3].ki.time = 0;
	SendInput(4, IP, sizeof(INPUT));

	CWindowToFind WTF(L"Peak Pattern:", L"WindowsForms10.Window.8.app.0." + CString(IcpMassHunterWindowClassNumber));
	do
	{	WTF.MaxWindows = 100;
		while (EnumChildWindows(MassHunterWindow, EnumChildProc, (LPARAM)&WTF));
		if ((timeGetTime() - StartTime)>MilliSeconds)
		{
			return false;
		}
		if (m_Abort)
		{	m_ErrorText = L"Aborted";
			return false;
		}
		if (!WTF.Window)
		{	TimedSleep(20);
		}
	} while (!WTF.Window);
	_ASSERTE(WTF.MaxWindows);
	TRACE(L"\tWartezeit: %ums\n", timeGetTime() - StartTime);
	RECT R;
	::GetWindowRect(WTF.Window, &R);*/

	// ALT-Q / "Add to Queue" clicken

	IP[0].type = INPUT_KEYBOARD;
	IP[0].ki.dwFlags = 0;
	IP[0].ki.wScan = 0;
	IP[0].ki.wVk = VK_MENU;
	IP[0].ki.time = 0;
	IP[1].type = INPUT_KEYBOARD;
	IP[1].ki.dwFlags = 0;
	IP[1].ki.wScan = 0;
	IP[1].ki.wVk = L'Q';
	IP[1].ki.time = 0;
	IP[2].type = INPUT_KEYBOARD;
	IP[2].ki.dwFlags = KEYEVENTF_KEYUP;
	IP[2].ki.wScan = 0;
	IP[2].ki.wVk = L'Q';
	IP[2].ki.time = 0;
	IP[3].type = INPUT_KEYBOARD;
	IP[3].ki.dwFlags = KEYEVENTF_KEYUP;
	IP[3].ki.wScan = 0;
	IP[3].ki.wVk = VK_MENU;
	IP[3].ki.time = 0;
	SendInput(4, IP, sizeof(INPUT));
	TRACE(L"\tWartezeit: %ums\n", timeGetTime() - StartTime);

	return StartMethod2(Method, MilliSeconds);
}

bool CMassHunterInterface::ScheduleMethod2(CMethodSchedule* Method, DWORD MilliSeconds)
{
	TRACE(L"CMassHunterInterface::ScheduleMethod2\n");
	m_Abort = false;
	return true;
}



//******************************************************************+++


// Eine Text Eingeben

void Texteingeben(wstring text) {
	for (char c : text) {
		// Die Tastatureingaben simulieren
		INPUT ip;
		ip.type = INPUT_KEYBOARD;
		ip.ki.wVk = 0;
		ip.ki.wScan = c;
		ip.ki.dwFlags = KEYEVENTF_UNICODE;
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;
		SendInput(1, &ip, sizeof(INPUT));
	}

}

// Eine Number Eingeben

void Number_eingeben(int number) {
	// Die Tastatureingaben simulieren
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = 0;
	ip.ki.wScan = number;
	ip.ki.dwFlags = KEYEVENTF_UNICODE;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	SendInput(1, &ip, sizeof(INPUT));
}

//  den Ordner zu erstellen

bool Neue_Ordner(const wstring& Folder_Path) {
	// Versuche, das Verzeichnis zu erstellen

	bool success = CreateDirectoryW(Folder_Path.c_str(), NULL);
	if (success) { // NULL: Standard-Sicherheitsattribute
		wcout << L"Der Ordner '" << Folder_Path << L"' wurde erfolgreich erstellt.\n";
		return true;
	}
	else {
		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS) {
			wcout << L"Der Ordner '" << Folder_Path << L"' existiert bereits.\n";
		}
		else {
			wcerr << L"Fehler beim Erstellen des Ordners. Fehlercode: " << error << L"\n";
		}
		return false;
	}
}

// Prüfen, ob einer Ordner existiert und es ein Verzeichnis ist

bool Überprüfen_Ordner(const wstring Folder_Path2) {
	DWORD attributes = GetFileAttributes(Folder_Path2.c_str());

	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Funktion, um Text in die Zwischenablage zu kopieren

void CopyTextToClipboard(const std::wstring& text) {

	// Öffne die Zwischenablage
	OpenClipboard(NULL);

	// Lösche den aktuellen Inhalt der Zwischenablage
	EmptyClipboard();

	// Konvertiere den Text in ein globales Speicherobjekt
	size_t size = (text.size() + 1) * sizeof(wchar_t); // Platz für Nullterminierung
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);

	// Kopiere den Text in den globalen Speicher
	memcpy(GlobalLock(hGlobal), text.c_str(), size);
	GlobalUnlock(hGlobal);

	// Setze den Text in die Zwischenablage
	SetClipboardData(CF_UNICODETEXT, hGlobal);

	// Schließe die Zwischenablage
	GlobalFree(hGlobal);
	CloseClipboard();
	// std::wcout << L"Text wurde in die Zwischenablage kopiert!" << std::endl;
}



// Funktion: Klick auf die Box und Auswahl eines Element

void SetDataPath(IUIAutomation* automation, IUIAutomationElement* table, LPCWSTR name_element, LPCWSTR automationId_element) {

	// Wähle den gewünschten Ordner aus 
	IUIAutomationElement* folderNode = FindElementByNameOrAutomationId(automation, table, name_element, automationId_element);
	if (folderNode) {
		IUIAutomationSelectionItemPattern* selectPattern = NULL;
		if (SUCCEEDED(folderNode->GetCurrentPatternAs(UIA_SelectionItemPatternId, IID_PPV_ARGS(&selectPattern)))) {
			selectPattern->Select(); // Ordner auswählen
			selectPattern->Release();
		}
		folderNode->Release();
	}
}


// Funktion für das Abgeben die Ordner in einer bestimmte Path

void ListDirectories(const std::wstring& path) {
	WIN32_FIND_DATA findFileData;
	// Suche nach allen Dateien und Ordnern im angegebenen Verzeichnis
	HANDLE hFind = FindFirstFile((path + L"*").c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcout << L"Fehler beim Öffnen des Verzeichnisses!" << std::endl;
		return;
	}

	do {
		const std::wstring fileName = findFileData.cFileName;
		// Überprüfe, ob es sich um einen Ordner handelt
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Ignoriere "." und ".."
			if (fileName != L"." && fileName != L"..") {
				std::wcout << L"Ordner: " << fileName << std::endl;
			}
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

bool CMassHunterInterface::WriteMethodToFile2(CMethodRun* Method, DWORD MilliSeconds)
{
	TRACE(L"WriteMethodToFile2 Projekt: %s Methode: %s", Method->Project, Method->Method);

	// --- Automatisierung Teil ---

	/*std::wstring project = Method->Project;
	std::wstring method = Method->Method;
	const std::list<SamiControl::CBaseLabwareData>& labwareList = Method->Labware;

	// MassHunter-Fenster in den Vordergrund holen
	if (!BringWindowToFront(L"Enhanced MassHunter"))
	{
		//m_Logger.LogText(L"[WriteMethodToFile2] MassHunter-Fenster nicht gefunden.");
		Method->ErrorText = L"MassHunter window not found.";

		return false;
	}
	Sleep(3000); // Warte auf GUI-Stabilität

	// Starte Sequence-Menü (Alt → S → N)
	Tastatur(VK_MENU); Sleep(1000);
	Tastatur('S'); Sleep(1000);
	Tastatur('N'); Sleep(4000); // Fenster öffnet sich

	// Jetzt simulieren wir das Ausfüllen der Tabelle
	for (const auto& lw : labwareList)
	{

		// Beispiel für Eingabe von SampleName → Tab → Vial → Tab usw.
		if (lw.SampleName.empty())
		{
			m_ErrorText = L"SampleName ist leer.";
			return false;
		}
		else 
		{
			Texteingeben(lw.SampleName.c_str()); Sleep(500);
			Tastatur(VK_TAB); Sleep(200);
		}
		
		for (size_t i = 0; i < lw.Vial.length(); ++i)
		{
			if (!iswdigit(lw.Vial[i]))
			{
				m_ErrorText = L"Vial enthält ungültige Zeichen: ";
				m_ErrorText += CString(lw.Vial.c_str());
				return false;
			}
		}


		Texteingeben(lw.Vial.c_str()); Sleep(500);
		Tastatur(VK_TAB); Sleep(200);


		Texteingeben(lw.Level.c_str()); Sleep(500);
		Tastatur(VK_TAB); Sleep(200);
		Texteingeben(lw.Dilution.c_str()); Sleep(500);
		Tastatur(VK_TAB); Sleep(200);
		Texteingeben(lw.Volume.c_str()); Sleep(500);
		Tastatur(VK_TAB); Sleep(200);
		Texteingeben(lw.TrayName.c_str()); Sleep(500);
		Tastatur(VK_TAB); Sleep(200);
		Texteingeben(lw.Comment.c_str()); Sleep(500);
		Tastatur(VK_RETURN); // Neue Zeile für nächsten Eintrag
		Sleep(800);
	}

	// Fertig mit der Eingabe
	//m_Logger.LogText(L"[WriteMethodToFile2] Automatische Eingabe erfolgreich abgeschlossen.");
	*/return true;
}

// Hilfsfunktion: Schreibwert in ein UI-Element, wenn ValuePattern verfügbar
void SetValueInCell(IUIAutomationElement* pElement, const wchar_t* value) {
	VARIANT varProp;
	pElement->GetCurrentPropertyValue(UIA_IsValuePatternAvailablePropertyId, &varProp);
	if (varProp.boolVal) {
		IUIAutomationValuePattern* pValuePattern = nullptr;
		HRESULT hr = pElement->GetCurrentPatternAs(UIA_ValuePatternId,
			IID_PPV_ARGS(&pValuePattern));
		if (SUCCEEDED(hr) && pValuePattern) {
			BSTR bstrValue = SysAllocString(value); // Umwandlung
			pValuePattern->SetValue(bstrValue); // Übergabe
			SysFreeString(bstrValue); // Aufräumen
			pValuePattern->Release();
		}
	}
}



//Zweite Version 

void PasteToFieldByName(IUIAutomationElementArray* pElements, const std::wstring& fieldName, const std::wstring& textToPaste) {
	int length = 0;
	pElements->get_Length(&length);

	for (int i = 0; i < length; ++i) {
		IUIAutomationElement* pElement11 = nullptr;
		pElements->GetElement(i, &pElement11);
		if (!pElement11) continue;

		BSTR name;
		pElement11->get_CurrentName(&name);

		if (fieldName == name) {
			// 1. Maus-Klick auf das Element (statt nur SetFocus)
			RECT rect;
			HRESULT hr = pElement11->get_CurrentBoundingRectangle(&rect);
			if (SUCCEEDED(hr)) {

				int x = (rect.left + rect.right) / 2;
				int y = (rect.top + rect.bottom) / 2;
				Sleep(100);

				SetCursorPos(x, y);
				Sleep(100);
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				Sleep(50);
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				Sleep(300);
			}

			// 2. Text in Zwischenablage kopieren
			CopyTextToClipboard(textToPaste);
			Sleep(300);

			// 3. STRG+V senden
			INPUT input[4] = {};
			input[0].type = INPUT_KEYBOARD; input[0].ki.wVk = VK_CONTROL;
			input[1].type = INPUT_KEYBOARD; input[1].ki.wVk = 'V';
			input[2].type = INPUT_KEYBOARD; input[2].ki.wVk = 'V'; input[2].ki.dwFlags = KEYEVENTF_KEYUP;
			input[3].type = INPUT_KEYBOARD; input[3].ki.wVk = VK_CONTROL; input[3].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(4, input, sizeof(INPUT));
			Sleep(500);


			SysFreeString(name);
			pElement11->Release();
			break;
		}

		//SysFreeString(name);
		//pElement11->Release();
	}
}

void OpenComboBox(IUIAutomationElement* pElement) {
	IUIAutomationInvokePattern* pInvoke = nullptr;
	if (SUCCEEDED(pElement->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&pInvoke))) && pInvoke) {
		pInvoke->Invoke();
		pInvoke->Release();
		Sleep(500); // Zeit geben zum Öffnen
	}
}

void GetCurrentDate(std::wstring& year, std::wstring& month, std::wstring& day)
{
	SYSTEMTIME st;
	GetLocalTime(&st); // Aktuelle lokale Zeit abfragen

	// Jahr, Monat und Tag separat als String speichern
	year = std::to_wstring(st.wYear);
	month = (st.wMonth < 10 ? L"0" : L"") + std::to_wstring(st.wMonth);
	day = (st.wDay < 10 ? L"0" : L"") + std::to_wstring(st.wDay);
}

// Führt InvokePattern aus, wenn verfügbar
void InvokeIfPossible(IUIAutomationElement* pElement) {
	VARIANT varProp;
	pElement->GetCurrentPropertyValue(UIA_IsInvokePatternAvailablePropertyId, &varProp);

	if (varProp.boolVal) {
		IUIAutomationInvokePattern* pInvoke = nullptr;
		HRESULT hr = pElement->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&pInvoke));
		if (SUCCEEDED(hr) && pInvoke) {
			pInvoke->Invoke();
			pInvoke->Release();
		}
	}
}



void ClickElement(IUIAutomationElement* pElement) {
	if (!pElement) return;

	RECT rect;
	HRESULT hr = pElement->get_CurrentBoundingRectangle(&rect);
	if (FAILED(hr)) return;

	// Position berechnen
	int x = (rect.left + rect.right) / 2;
	int y = (rect.top + rect.bottom) / 2;

	// Mausposition setzen
	SetCursorPos(x, y);
	Sleep(100);

	// Linksklick ausführen
	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	Sleep(50);
	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	Sleep(200);
}



bool IsValueEmpty(IUIAutomationElement* pElement) {
	if (!pElement) return true;

	IUIAutomationValuePattern* pValuePattern = nullptr;
	HRESULT hr = pElement->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&pValuePattern));

	if (FAILED(hr) || !pValuePattern)
		return true; // Wenn kein ValuePattern → als leer behandeln

	BSTR currentValue = nullptr;
	hr = pValuePattern->get_CurrentValue(&currentValue);
	bool isEmpty = false;

	if (FAILED(hr) || !currentValue) {
		isEmpty = true;
	}
	else {
		// Prüfen auf "(null)" oder leeren String
		isEmpty = (wcscmp(currentValue, L"(null)") == 0 || wcslen(currentValue) == 0);
	}

	if (currentValue) SysFreeString(currentValue);
	pValuePattern->Release();

	return isEmpty;
}


bool IstStartSequenceDialogOffen() {
	CoInitialize(NULL);

	IUIAutomation* pAutomation = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pAutomation)))) {
		CoUninitialize();
		return false;
	}

	HWND hwnd = GetForegroundWindow();
	if (!hwnd) {
		pAutomation->Release();
		CoUninitialize();
		return false;
	}

	IUIAutomationElement* pRoot = nullptr;
	if (FAILED(pAutomation->ElementFromHandle(hwnd, &pRoot)) || !pRoot) {
		pAutomation->Release();
		CoUninitialize();
		return false;
	}

	// Bedingungen
	IUIAutomationCondition* condText = nullptr;
	IUIAutomationCondition* condId = nullptr;
	IUIAutomationCondition* condCombined = nullptr;

	pAutomation->CreatePropertyCondition(UIA_NamePropertyId, _variant_t(L"start sequence now?"), &condText);
	pAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, _variant_t(L"65535"), &condId);
	pAutomation->CreateAndCondition(condText, condId, &condCombined);

	// Suche das Textelement im UI
	IUIAutomationElement* pText = nullptr;
	bool gefunden = false;

	if (SUCCEEDED(pRoot->FindFirst(TreeScope_Subtree, condCombined, &pText)) && pText) {
		gefunden = true;
		pText->Release();
	}

	// Aufräumen
	if (condCombined) condCombined->Release();
	if (condText) condText->Release();
	if (condId) condId->Release();
	if (pRoot) pRoot->Release();
	pAutomation->Release();
	CoUninitialize();

	return gefunden;
}



std::wstring GetFehlertext()
{
	std::wstring Fehlertext;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) return L"COM Init failed";

	IUIAutomation* automation = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		CoUninitialize();
		return L"UI Automation Init Failed";
	}

	HWND hwnd = GetForegroundWindow();  // Wichtig!
	if (!hwnd) {
		automation->Release();
		CoUninitialize();
		return L"Window not found";
	}

	IUIAutomationElement* rootElement = nullptr;
	if (FAILED(automation->ElementFromHandle(hwnd, &rootElement)) || !rootElement) {
		automation->Release();
		CoUninitialize();
		return L"UI Element not found";
	}

	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = SysAllocString(L"65535");

	IUIAutomationCondition* condition = nullptr;
	automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, var, &condition);
	SysFreeString(var.bstrVal);

	IUIAutomationElement* statusElement = nullptr;
	if (SUCCEEDED(rootElement->FindFirst(TreeScope_Descendants, condition, &statusElement)) && statusElement) {
		BSTR name = nullptr;
		if (SUCCEEDED(statusElement->get_CurrentName(&name)) && name != nullptr) {
			if (wcscmp(name, L"start sequence now?") != 0) {
				Fehlertext = name;
			}
			SysFreeString(name);
		}
		statusElement->Release();
	}

	if (condition) condition->Release();
	rootElement->Release();
	automation->Release();
	CoUninitialize();

	return Fehlertext;
}


bool CMassHunterInterface::RunSequence(CMethodRun* Method, DWORD MilliSeconds) {

	std::wstring targetProcess = L"msinsctl.exe";
	if (IsProcessRunning(targetProcess)) {
		std::wcout << L"MassHunter APP ist schon angefangen." << std::endl;
	}
	else {
		Method->ErrorText = L"MassHunter APP ist nicht angefangen" ;

	}
	
	std::wstring fixedPart = L"Enhanced MassHunter"; // Fester Teil des Fenstertitels
	if (BringWindowToFront(fixedPart)) {
		std::wcout << L"Das Fenster mit '" << fixedPart << L"' wurde in den Vordergrund gebracht."<< std::endl;
	}
	else {
		std::wcout << L"Kein Fenster mit '" << fixedPart << L"' gefunden." <<std::endl;
		Method->ErrorText = L"Kein Fenster mit Enhanced MassHunter gefunden.";

	}

	//******************************************************************************************

		// Initialisiere COM 

	HRESULT mr1 = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(mr1)) { return -1; }

	IUIAutomation* pAutomation2 = nullptr;
	mr1 = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pAutomation2));
	if (FAILED(mr1))
	{
		CoUninitialize();
		return -1;
	}
	// Nutze aktives Fenster 
	HWND hwnd2 = ::GetForegroundWindow();
	if (!hwnd2)
	{
		pAutomation2->Release();
		CoUninitialize();
		return -1;
	}
	IUIAutomationElement* pRoot2 = nullptr;
	mr1 = pAutomation2->ElementFromHandle(hwnd2, &pRoot2);

	// Finde "Sequence"-Menüpunkt 
	IUIAutomationCondition* condSequence = nullptr;
	pAutomation2->CreatePropertyCondition(UIA_NamePropertyId,_variant_t(L"Sequence"), &condSequence);

	IUIAutomationElement* pSequenceMenu = nullptr;
	pRoot2->FindFirst(TreeScope_Subtree, condSequence, &pSequenceMenu);

	if (pSequenceMenu) {
		std::wcout << L"Sequence' gefunden, klicke..." << std::endl;
		InvokeIfPossible(pSequenceMenu);
		Sleep(1000); // Zeit geben, damit Menü aufgeht 

		// Jetzt suchen wir global nach "New Sequence..." 
		IUIAutomationCondition* condName = nullptr;
		IUIAutomationCondition* condType = nullptr;
		IUIAutomationCondition* condCombined = nullptr;

		// Name = "New Sequence..." 
		pAutomation2->CreatePropertyCondition(UIA_NamePropertyId,
			_variant_t(L"New Sequence..."), &condName);

		// Typ = Menüpunkt (MenuItem) 
		pAutomation2->CreatePropertyCondition(UIA_ControlTypePropertyId,
			_variant_t(UIA_MenuItemControlTypeId), &condType);

		// Kombinierte Bedingung 
		pAutomation2->CreateAndCondition(condName, condType, &condCombined);

		IUIAutomationElement* pNewSeq = nullptr;
		HRESULT mr1 = pRoot2->FindFirst(TreeScope_Subtree, condCombined,&pNewSeq);

		if (SUCCEEDED(mr1) && pNewSeq) {
			std::wcout << L"New Sequence...' gefunden, klicke..." <<
				std::endl;
			InvokeIfPossible(pNewSeq);
			pNewSeq->Release();
		}
		else {
			Method->ErrorText = L"New Sequence...' konnte nicht gefunden werden.";
			
		}

		// Aufräumen 
		if (condName) condName->Release();
		if (condType) condType->Release();
		if (condCombined) condCombined->Release();
		pSequenceMenu->Release();
	}
	else {
		Method->ErrorText = L"Sequence'-Menü nicht gefunden!";

		CoUninitialize();

	}


	// COM-Aufräumen 
	if (condSequence) condSequence->Release();
	if (pRoot2) pRoot2->Release();
	if (pAutomation2) pAutomation2->Release();
	CoUninitialize();

	//******************************************************************************************

	Sleep(500);
	
	std::vector<int> positions;

	// Erste Schleife: Speichere Positionen in einem Vektor
	for (const auto& lab : Method->Labware) {
		int rowIndex1 = std::stoi(lab.Position);  // Position in Integer umwandeln
		positions.push_back(rowIndex1);  // In den Vektor einfügen
	}

	// Anzahl der Positionen
	int numPositions = positions.size();


	// Nächste Schleife: Anstatt rowIndex1 verwenden wir numPositions
	if (numPositions > 2) {
		for (int i = 2; i <= numPositions; ++i) {
			Tastatur(VK_MENU, 'N'); // Tastendruck simulieren
			Sleep(500); // Warten für 500 ms
		}
	}
	else {
		for (int i = 3; i >= numPositions; --i) {

			Tastatur(VK_SHIFT, VK_SPACE); // Tastendruck simulieren
			Sleep(500); // Warten für 500 ms
			Tastatur(VK_DELETE); // Tastendruck simulieren
			Sleep(500); // Warten für 500 ms
		}


	}
	
		//********************************************************************************

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		std::wcerr << L"COM-Initialisierung fehlgeschlagen!" << std::endl;
		return -1;
	}
	IUIAutomation* pAutomation = nullptr;
	hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pAutomation));
	if (FAILED(hr)) {
		std::wcerr << L"UIAutomation konnte nicht initialisiert werden!" <<
			std::endl;
		CoUninitialize();
		return -1;
	}
	// ?? Aktives Fenster als Root-Element verwenden 
	HWND hwnd = ::GetForegroundWindow();
	if (!hwnd) {
		std::wcerr << L"Aktives Fenster konnte nicht ermittelt werden!" <<
			std::endl;
		pAutomation->Release();
		CoUninitialize();
		return -1;
	}
	IUIAutomationElement* pRoot = nullptr;
	hr = pAutomation->ElementFromHandle(hwnd, &pRoot);
	if (FAILED(hr) || !pRoot) {
		std::wcerr << L"UI Root konnte nicht geladen werden!" << std::endl;
		pAutomation->Release();
		CoUninitialize();
		return -1;
	}
	// Suche alle Edit-Felder (Zellen) 

	IUIAutomationCondition* condEdit = nullptr;
	IUIAutomationCondition* condCombo = nullptr;

	pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId,_variant_t(UIA_EditControlTypeId), &condEdit);
	pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId,_variant_t(UIA_ComboBoxControlTypeId), &condCombo);

	IUIAutomationCondition* pCondition = nullptr;
	pAutomation->CreateOrCondition(condEdit, condCombo, &pCondition);

	IUIAutomationElementArray* pElements = nullptr;
	hr = pRoot->FindAll(TreeScope_Subtree, pCondition, &pElements);
	if (SUCCEEDED(hr) && pElements)
	{
		int length = 0;
		pElements->get_Length(&length);


		std::wstring DataPathFest2(DataPathFest.GetString());

		std::wstring Jahr, Monat, Tag;
		GetCurrentDate(Jahr, Monat, Tag);
		std::wstring pathJahr = DataPathFest2 + Jahr;
		std::wstring pathJahrMonat = pathJahr + L"\\" + Jahr + Monat;
		
		std::wstring DataPathFinal;

	


		if (std::wstring(Method->DataPath).empty()) 
		{


		    // 1) Jahr-Ordner
	    	std::wstring pathJahr = DataPathFest2  + Jahr;
	        	if (!Überprüfen_Ordner(pathJahr)) {
	        		Neue_Ordner(pathJahr);
					Sleep(100);

	        	}

	     	// 2) JahrMonat-Ordner
	    	std::wstring pathJahrMonat = pathJahr + L"\\" + Jahr + Monat;
	        	if (!Überprüfen_Ordner(pathJahrMonat)) {
			       Neue_Ordner(pathJahrMonat);
				   Sleep(100);

		        }
    
		     // Loop A: Tages-Ordner mit Nummern-Suffix
	    	
		    	std::wstring base = pathJahrMonat + L"\\" + Jahr + Monat + Tag;
	      		int n = 0;
	        		while (true) 
					{
						std::wstring candidate = base;
						if (n > 0) candidate += L"(" + std::to_wstring(n) + L")";
						
							if (!Überprüfen_Ordner(candidate))
							{
								Neue_Ordner(candidate);
								Sleep(100);

								DataPathFinal = candidate;

								break;
							}
							++n;
						
			         }
		     
	}
		else 
		{
		         	// Loop B: Fest-Name (z.B. "202504016") mit Nummern-Suffix
			
			
				std::wstring base2 = pathJahrMonat + L"\\" + std::wstring(Method->DataPath) ;
				int n2 = 0;
				while (true) 
				{
					std::wstring candidate2 = base2;

					if (n2 > 0) 
						candidate2 += L"(" + std::to_wstring(n2) + L")";
					
					    if (!Überprüfen_Ordner(candidate2))
					    {
						  Neue_Ordner(candidate2);
						  Sleep(100);

						  DataPathFinal = candidate2;

						  break;
					    }
					     ++n2;
					
				}
			
		}


		Sleep(200);
		for (const auto& lab : Method->Labware)
		{
			int rowIndex = 0;


			rowIndex = std::stoi(lab.Position);

			rowIndex -= 1;  // von 1-basiert auf 0-basiert 



			// Position: 
			std::wstring nameCell = L"Name Row " + std::to_wstring(rowIndex);
			std::wstring vialCell = L"Vial Row " + std::to_wstring(rowIndex);
			std::wstring methodPathCell = L"Method Path Row " + std::to_wstring(rowIndex);
			std::wstring Method_File_RowCell = L"Method File Row " + std::to_wstring(rowIndex);
			std::wstring Data_Path_RowCell = L"Data Path Row " + std::to_wstring(rowIndex);
			std::wstring Data_File_RowCell = L"Data File Row " + std::to_wstring(rowIndex);
			std::wstring Type_Row_RowCell = L"Type Row " + std::to_wstring(rowIndex);
			std::wstring Level_Row_RowCell = L"Level Row " + std::to_wstring(rowIndex);
			std::wstring Dil_Row_RowCell = L"Dil. Row " + std::to_wstring(rowIndex);
			std::wstring Vol_Row_RowCell = L"Vol. Row " + std::to_wstring(rowIndex);
			std::wstring Sample_Prep_Row_RowCell = L"Sample Prep Path Name Row " + std::to_wstring(rowIndex);
			std::wstring Sample_Prep_File_Row_RowCell = L"Sample Prep File Name Row " + std::to_wstring(rowIndex);
			std::wstring Tray_Name_Row_RowCell = L"Tray Name Row " + std::to_wstring(rowIndex);
			std::wstring Comment_RowCell = L"Comment Row " + std::to_wstring(rowIndex);


			for (int i = 0; i < length; ++i) {
				IUIAutomationElement* pElement = nullptr;
				pElements->GetElement(i, &pElement);
				if (!pElement) continue;

				BSTR name;
				pElement->get_CurrentName(&name);

				std::wstring cellName(name);

				if (cellName == nameCell) {
					if (lab.SampleName.empty()) {
						Method->ErrorText = L"Keine SampleName eingegeben.";
						return false; // Wichtig: abbrechen!
					}
					else {
						SetValueInCell(pElement, lab.SampleName.c_str());
						
					}
				}

				else if (cellName == vialCell) 
				{

					if (lab.Vial.empty()) {
						Method->ErrorText = L"Keine Vial eingegeben.";
						return false; // Wichtig: abbrechen!
					}
					else
					{

								SetValueInCell(pElement, lab.Vial.c_str());
								
							
					}
				
				}
					

				
				else if (cellName == methodPathCell) {
					SetValueInCell(pElement, m_ProjectPath + Method->Project);
					
				}
					

				else if (cellName == Method_File_RowCell) {
					SetValueInCell(pElement, Method->Method);
					
				}
				
				else if (cellName == Data_Path_RowCell)
				{
					const int maxTries = 4;
					bool success = false;

					for (int attempt = 1; attempt <= maxTries; ++attempt) {
						std::wcout << L" Versuch " << attempt << L" für Data Path" << std::endl;

						PasteToFieldByName(pElements, Data_Path_RowCell, DataPathFinal);
						Sleep(300); // Warten, damit UI aktualisieren kann

						// Wert prüfen
						if (!IsValueEmpty(pElement)) {
							success = true;
							break;
						}
						else {
							std::wcout << L" Data Path noch leer!" << std::endl;
						}
					}

					if (!success) {
						std::wcerr << L" Data Path konnte nicht gesetzt werden!" << std::endl;
					}
				
				
				}
                else if (cellName == Data_File_RowCell)
				{
					std::wstring DataFile_Index = L"00" + std::to_wstring(rowIndex + 1);
					if (lab.DataFile.empty()) {

						wstring DataFile2 = Jahr + Monat + Tag + DataFile_Index;
						SetValueInCell(pElement, DataFile2.c_str());
						
					}
					else {

						SetValueInCell(pElement, lab.DataFile.c_str());
						
					}
				}

				else if (name && std::wstring(name).find(Type_Row_RowCell) != std::wstring::npos)
				{
					OpenComboBox(pElement); // öffnet das Dropdown


					if (wcscmp(lab.Typ.c_str(), L"Blank") == 0) { Tastatur('B'); }
					if (wcscmp(lab.Typ.c_str(), L"Sample") == 0) { Tastatur('S'); }
					if (wcscmp(lab.Typ.c_str(), L"Cal") == 0) { Tastatur('C'); }
					if (wcscmp(lab.Typ.c_str(), L"QC") == 0) { Tastatur('Q'); }
					if (wcscmp(lab.Typ.c_str(), L"Keyword") == 0) { Tastatur('K'); }
					if (wcscmp(lab.Typ.c_str(), L"TuneCheck") == 0) { Tastatur('T'); }
					if (wcscmp(lab.Typ.c_str(), L"MatrixBlank") == 0) { Tastatur('M'); }
					if (wcscmp(lab.Typ.c_str(), L"MatrixSpike") == 0) { Tastatur('M'); Sleep(1000);Tastatur(VK_DOWN); }
					if (wcscmp(lab.Typ.c_str(), L"MatrixSpikeDup") == 0) { Tastatur('M'); Sleep(1000);Tastatur(VK_DOWN);Sleep(1000);Tastatur(VK_DOWN); }
					if (wcscmp(lab.Typ.c_str(), L"DoubleBlank") == 0) { Tastatur('D'); }
					if (wcscmp(lab.Typ.c_str(), L"CC") == 0) { Tastatur('D');Sleep(1000);Tastatur(VK_DOWN); }
					if (wcscmp(lab.Typ.c_str(), L"ResponseCheck") == 0) { Tastatur('R'); }
				}



				else if (cellName == Level_Row_RowCell)
				{


					if (wcscmp(lab.Typ.c_str(), L"Cal") == 0)

					{
						if (lab.Level.empty())
						{
							Method->ErrorText = L"Das Typ ist Cal , Eingabe der Level ist erforderlich ";
							return false; // Wichtig: abbrechen!
						}
						else
						{
							SetValueInCell(pElement, lab.Level.c_str());
						}
					}
					else
					{
						SetValueInCell(pElement, lab.Level.c_str());
					}
				}

				else if (cellName == Dil_Row_RowCell)
					SetValueInCell(pElement, lab.Dilution.c_str());

				else if (cellName == Vol_Row_RowCell)
					SetValueInCell(pElement, lab.Volume.c_str());

				else if (cellName == Sample_Prep_Row_RowCell)
					SetValueInCell(pElement, L"");

				else if (cellName == Sample_Prep_File_Row_RowCell)
					SetValueInCell(pElement, L"");

				else if (cellName == Tray_Name_Row_RowCell)
					SetValueInCell(pElement, lab.TrayName.c_str());

				else if (cellName == Comment_RowCell)
					SetValueInCell(pElement, lab.Comment.c_str());


				// usw. für andere Felder 

				SysFreeString(name);
				pElement->Release();
			}
		}

		//int rowIndex = 0; 

		pElements->Release();
	}
	// Aufräumen 
	if (pCondition) pCondition->Release();
	if (pRoot) pRoot->Release();
	pAutomation->Release();
	CoUninitialize();

	SetStatusText(L"Sequence automatisch eingetragen.");



	HRESULT CoInitialize(NULL);
	//UI Automation initialisieren 
	IUIAutomation* automation = NULL;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		return -1;
	}
	// Drucken der OK  

	
	Sleep(500);
	
	if (!Button_Drucken(automation, NULL, L"OK", L"m_buttonOk")) 
	{
		Method->ErrorText = L"Das OK Button ist nicht gedrückt!";
		return false;
	}
	Sleep(500);



	std::wstring fehlertext3 = GetFehlertext();
	std::wstring trimmed2 = fehlertext3;
	trimmed2.erase(0, trimmed2.find_first_not_of(L" \n\r\t"));
	trimmed2.erase(trimmed2.find_last_not_of(L" \n\r\t") + 1);
	if (!trimmed2.empty()) {
		CString msg = L" Fehlerfenster erkannt: " + CString(fehlertext3.c_str());
		Method->ErrorText = msg;
	}
	else {

		if (IstStartSequenceDialogOffen()) {

			if (!Button_Drucken(automation, NULL, L"Yes", L"6")) 
			{
				Method->ErrorText = L"Das Yes Button ist nicht gedrückt!";
				return false;
			}

		}

		Sleep(1000);
	}


	std::wstring fehlertext2 = GetFehlertext();
	std::wstring trimmed = fehlertext2;
	trimmed.erase(0, trimmed.find_first_not_of(L" \n\r\t"));
	trimmed.erase(trimmed.find_last_not_of(L" \n\r\t") + 1);
	if (!trimmed.empty()) {
		CString msg = L" Fehlerfenster erkannt: " + CString(fehlertext2.c_str());
		Method->ErrorText = msg;
	}
	else {
		std::wstring fixedPart2 = L"Start Sequence"; // Fester Teil des Fenstertitels
		if (BringWindowToFront(fixedPart2)) {

			if (!Button_Drucken(automation, NULL, L"Run Sequence", L"buttonRunSequence")) {
				Method->ErrorText = L"Das Run Sequence Button ist nicht gedrückt!";
				return false;
			}
		}
		Sleep(1000);
	}

	//Button_Drucken(automation, NULL, L"Run Sequence", L"buttonRunSequence"); 
	automation->Release();
	CoUninitialize();
	return true;
}

CString  CMassHunterInterface::RunSequenceFertig()
{
	CString RunstatusText;
	// COM initialisieren
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	IUIAutomation* automation = NULL;
	if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation)))) {
		CoUninitialize();
		return L"UI Automation Init Failed";
	}

	HWND hwnd = NULL;
	EnumWindows([](HWND h, LPARAM lParam) -> BOOL {
		wchar_t title[256];
		::GetWindowTextW(h, title, 256);
		if (wcsstr(title, L"Enhanced MassHunter")) {
			*((HWND*)lParam) = h;
			return FALSE;
		}
		return TRUE;
		}, (LPARAM)&hwnd);

	if (!hwnd) {
		automation->Release();
		CoUninitialize();
		return L"MassHunter not found";
	}

	IUIAutomationElement* rootElement = NULL;
	if (FAILED(automation->ElementFromHandle(hwnd, &rootElement)) || !rootElement) {
		automation->Release();
		CoUninitialize();
		return L"UI Element not found";
	}

	// Status-Element suchen
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = SysAllocString(L"labelDataFileNameValue");

	IUIAutomationCondition* condition = NULL;
	automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, var, &condition);

	IUIAutomationElement* statusElement = NULL;
	rootElement->FindFirst(TreeScope_Descendants, condition, &statusElement);

	if (statusElement) {
		BSTR name;
		statusElement->get_CurrentName(&name);
		RunstatusText = name;
		SysFreeString(name);
		statusElement->Release();
	}

	SysFreeString(var.bstrVal);
	condition->Release();
	rootElement->Release();
	automation->Release();
	CoUninitialize();

	return RunstatusText;
}


void CMassHunterInterface::TimedSleep(DWORD MilliSeconds)
{
	DWORD StartTime = timeGetTime();
	MSG msg;
	do
	{	if (AfxGetMainWnd() && ::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
		{	AfxPumpMessage();
		}
		else Sleep(20);
	} while ((timeGetTime() - StartTime)<MilliSeconds);
}

// http://www.codeproject.com/Tips/76427/How-to-bring-window-to-top-with-SetForegroundWindow
void  CMassHunterInterface::SetForegroundWindowInternal1(HWND hWnd)
{
	if (!::IsWindow(hWnd)) return;
	BYTE keyState[256] = {0};
	//to unlock SetForegroundWindow we need to imitate Alt pressing
	if (::GetKeyboardState((LPBYTE)&keyState))
	{
		if (!(keyState[VK_MENU] & 0x80))
		{
			::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
		}
	}
	::SetForegroundWindow(hWnd);
	if (::GetKeyboardState((LPBYTE)&keyState))
	{
		if (!(keyState[VK_MENU] & 0x80))
		{
			::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}
}


// http://www.codeproject.com/Tips/76427/How-to-bring-window-to-top-with-SetForegroundWindow
void  CMassHunterInterface::SetForegroundWindowInternal2(HWND hWnd)
{
	if (!::IsWindow(hWnd)) return;
 
	//relation time of SetForegroundWindow lock
	DWORD lockTimeOut = 0;
	HWND  hCurrWnd = ::GetForegroundWindow();
	DWORD dwThisTID = ::GetCurrentThreadId(),
	      dwCurrTID = ::GetWindowThreadProcessId(hCurrWnd, 0);
 
	//we need to bypass some limitations from Microsoft :)
	if (dwThisTID!=dwCurrTID)
	{
		::AttachThreadInput(dwThisTID, dwCurrTID, TRUE);
 
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
 
		::AllowSetForegroundWindow(ASFW_ANY);
	}
	::SetForegroundWindow(hWnd);
	if (dwThisTID!=dwCurrTID)
	{
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)(DWORD_PTR)lockTimeOut, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
		::AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
	}
}


}	// namespace
