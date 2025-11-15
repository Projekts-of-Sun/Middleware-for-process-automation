#include <afx.h>

#include "Operation Container.h"
#include "..\Utf8\utf8.h"
#include "..\Agilent\MassHunter Interface.h"

#include <locale.h>

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif


COperationContainer::COperationContainer(OperationSourceType Source)
	: PmsId(0)
	, ProjectID(0)
	, Families(0)
	, MaxFamilies(0)
	, ETC(0)
	, OperationSource(Source)
	, MethodRevision(0)
{
	TRACE(L"COperationContainer::COperationContainer\n");
}

COperationContainer::~COperationContainer()
{
	TRACE(L"COperationContainer::~COperationContainer\n");
}

CString COperationContainer::Dump(unsigned int Number, unsigned int Tabs) const
{	CString T, X;

	while (Tabs--) T+=L"\t";
	X.Format(L"Command Name: '%s', PMS ID: %I64u", (LPCTSTR)CName, PmsId);
	return T + X;
}

std::string COperationContainer::ToXmlString() const
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
#ifndef NDEBUG
	docptr->save(L"OperationContainer.xml");
#endif
	return BstrToUtf8(docptr->Getxml());
}

bool COperationContainer::FromXmlString(const char* Xml)
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

bool COperationContainer::Save(const CString& Filename) const
{
	TRACE(L"COperationContainer::Save: %s\n", Filename);
	DocPtr docptr;
	if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
	{	throw CString(L"Failed to create xml object");
	}
	docptr->async	= VARIANT_FALSE;
	docptr->validateOnParse	= VARIANT_FALSE;
	docptr->resolveExternals = VARIANT_FALSE;
	MSXML2::IXMLDOMNodePtr Temp(docptr->appendChild(docptr->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"")));
		
	MSXML2::IXMLDOMNodePtr nodptr(docptr->createElement(StreamName()));
	Temp = docptr->appendChild(nodptr);
	if (!Save(nodptr, docptr)) return false;
	docptr->save(CComVariant(Filename));
	return true;
}

bool COperationContainer::Save(SavePtr X, DocPtr D) const
{
	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMElementPtr E(X);
	E->setAttribute(L"CommandName", (LPCTSTR)CName);
	wchar_t T[30];
	_swprintf_s_l(T, sizeof(T)/sizeof(wchar_t), L"%I64u", L, PmsId);
	E->setAttribute(L"PMSID", T); 
	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"Method"));
	MSXML2::IXMLDOMNodePtr Temp(E->appendChild(Y));
	Y->setAttribute(L"Name", (LPCTSTR)MethodName); 
	Y->setAttribute(L"Revision", MethodRevision); 

	Y = D->createElement(L"Project");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"Name", (LPCTSTR)ProjectName); 
	Y->setAttribute(L"ID", ProjectID); 



	Y = D->createElement(L"Schedule");
	Y->setAttribute(L"MaxFamilies", MaxFamilies); 
	_swprintf_s_l(T, 30, L"%0.2f", L, ETC);
	Y->setAttribute(L"ETC", T); 
	Y->setAttribute(L"Families", Families); 
	Temp = E->appendChild(Y);

	Y = D->createElement(L"Error");
	Temp = E->appendChild(Y);
	Y->setAttribute(L"Text", (LPCTSTR)ErrorText); 

	



	_free_locale(L);
	return true;
}

bool COperationContainer::Load(const CString& Filename)
{
	TRACE(L"COperationContainer::Load: %s\n", Filename);
	DocPtr docptr;
	if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
	{	throw CString(L"Failed to create xml object");
	}
	docptr->async = VARIANT_FALSE;
	docptr->validateOnParse = VARIANT_FALSE;
	docptr->resolveExternals = VARIANT_FALSE;
	if (!docptr->load(CComVariant(Filename)))	return false;
	if (!Load(docptr->documentElement, docptr)) return false;
	return true;
}

bool COperationContainer::Load(SavePtr X, DocPtr D)
{
	MethodName.Empty();
	MethodRevision = 0;
	ProjectName.Empty();
	ProjectID = 0;
	ETC = 0.0;
	MaxFamilies = 0;
	Families = 0;
	ErrorText.Empty();

	MSXML2::IXMLDOMElementPtr	E(X);
	CName = E->getAttribute(L"CommandName");
	
	BSTR A = E->getAttribute(L"PMSID").bstrVal;
	if (A) PmsId = _wcstoui64(A, NULL, 10);
	else PmsId = 0;

	if (!E->hasChildNodes()) return false;

	_locale_t L = _create_locale(LC_CTYPE, "English");

	MSXML2::IXMLDOMNodeListPtr NL(E->childNodes);
	for (int i=0;i<NL->length;i++)
	{
		MSXML2::IXMLDOMNodePtr N = NL->Getitem(i);
		E = N;
		if (N->nodeName==_bstr_t("Method"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) MethodName = A;
			A = E->getAttribute(L"Revision").bstrVal;
			if (A) MethodRevision = wcstoul(A, NULL, 10);
			continue;
		}
		if (N->nodeName==_bstr_t("Project"))
		{	A = E->getAttribute(L"Name").bstrVal;
			if (A) ProjectName = A;
			A = E->getAttribute(L"ID").bstrVal;
			if (A) ProjectID = wcstoul(A, NULL, 10);
			continue;
		}
		if (N->nodeName==_bstr_t("Schedule"))
		{	A = E->getAttribute(L"MaxFamilies").bstrVal;
			if (A) MaxFamilies = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"Families").bstrVal;
			if (A) Families = wcstoul(A, NULL, 10);
			A = E->getAttribute(L"ETC").bstrVal;
			if (A) ETC = _wcstod_l(A, NULL, L);
			continue;
		}
		if (N->nodeName==_bstr_t("Error"))
		{	A = E->getAttribute(L"Text").bstrVal;
			if (A) ErrorText = A;
			continue;
		}
	}

	_free_locale(L);
	return true;
}

bool COperationContainer::operator !=(const COperationContainer& OC) const
{
	if (OC.CName!=CName) return true;
	if (OC.ErrorText!=ErrorText) return true;
	if (OC.ETC!=ETC) return true;
	if (OC.Families!=Families) return true;
	if (OC.MaxFamilies!=MaxFamilies) return true;
	if (OC.MethodName!=MethodName) return true;
	if (OC.MethodRevision!=MethodRevision) return true;
	if (OC.ProjectName!=ProjectName) return true;
	if (OC.ProjectID!=ProjectID) return true;
	if (OC.PmsId!=PmsId) return true;
	return false;
}


CScheduleOperationContainer::CScheduleOperationContainer(OperationSourceType Source)
	: COperationContainer(Source)
{
	TRACE(L"CScheduleOperationContainer::CScheduleOperationContainer\n");
}

CScheduleOperationContainer::~CScheduleOperationContainer()
{
	TRACE(L"CScheduleOperationContainer::~CScheduleOperationContainer\n");
}

bool CScheduleOperationContainer::Save(SavePtr X, DocPtr D) const
{
	bool E = COperationContainer::Save(X, D);
	if (Schedule)
	{	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"SamiSchedule"));
		MSXML2::IXMLDOMNodePtr Temp(X->appendChild(Y));
		E = E && Schedule->Save(Y, D);
	}
	return E;
}

bool CScheduleOperationContainer::Load(SavePtr X, DocPtr D)
{
	bool E = COperationContainer::Load(X, D);
	if (Schedule)
	{	MSXML2::IXMLDOMElementPtr	Y(X);
		MSXML2::IXMLDOMNodeListPtr NL(Y->getElementsByTagName(L"SamiSchedule"));
		if (NL->length>0)
		{	E = E && Schedule->Load(NL->item[0], D);
		}
	}
	return E;
}

bool CScheduleOperationContainer::operator !=(const CScheduleOperationContainer& SOC) const
{
	if (*static_cast<const COperationContainer*>(this)!=*static_cast<const COperationContainer*>(&SOC)) return true;
	if (SOC.Schedule!=Schedule) return true;
	return false;
}

bool CScheduleOperationContainer::operator !=(const COperationContainer& OC) const
{
	return COperationContainer::operator!=(OC);
}


CRunOperationContainer::CRunOperationContainer(OperationSourceType Source)
	: COperationContainer(Source)
{
	TRACE(L"CRunOperationContainer::CRunOperationContainer\n");
}

CRunOperationContainer::~CRunOperationContainer()
{
	TRACE(L"CRunOperationContainer::~CRunOperationContainer\n");
}

bool CRunOperationContainer::Save(SavePtr X, DocPtr D) const
{
	bool E = COperationContainer::Save(X, D);
	if (Schedule)
	{	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"SamiSchedule"));
		MSXML2::IXMLDOMNodePtr Temp(X->appendChild(Y));
		E = E && Schedule->Save(Y, D);
	}
	if (Run)
	{	MSXML2::IXMLDOMElementPtr Y(D->createElement(L"SamiRun"));
		MSXML2::IXMLDOMNodePtr Temp(X->appendChild(Y));
		E = E && Run->Save(Y, D);
	}
	return E;
}

bool CRunOperationContainer::Load(SavePtr X, DocPtr D)
{
	bool E = COperationContainer::Load(X, D);
	if (Schedule)
	{	MSXML2::IXMLDOMElementPtr	Y(X);
		MSXML2::IXMLDOMNodeListPtr NL(Y->getElementsByTagName(L"SamiSchedule"));
		if (NL->length>0)
		{	E = E && Schedule->Load(NL->item[0], D);
		}
	}
	if (Run)
	{	MSXML2::IXMLDOMElementPtr	Y(X);
		MSXML2::IXMLDOMNodeListPtr NL(Y->getElementsByTagName(L"SamiRun"));
		if (NL->length>0)
		{	E = E && Run->Load(NL->item[0], D);
		}
	}
	return E;
}

bool CRunOperationContainer::operator !=(const CRunOperationContainer& ROC) const
{
	if (*static_cast<const COperationContainer*>(this)!=*static_cast<const COperationContainer*>(&ROC)) return true;
	if ((ROC.Run!=0) ^ (Run!=0)) return true;
	if ((ROC.Schedule!=0) ^ (Schedule!=0)) return true;
	if (ROC.Run!=Run) return true;
	if (ROC.Schedule!=Schedule) return true;
	return false;
}

bool CRunOperationContainer::operator !=(const COperationContainer& OC) const
{
	return COperationContainer::operator!=(OC);
}
