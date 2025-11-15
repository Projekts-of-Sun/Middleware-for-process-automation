#include <afx.h>
#include "Base Abort Command.h"
#include <locale.h>
#include "..\Utf8\utf8.h"

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

namespace CommandHandler
{
	const wchar_t* CBaseAbortCommand::TypeName = L"Abort";


	CBaseAbortCommand::CBaseAbortCommand()
	{
	}

	CBaseAbortCommand::~CBaseAbortCommand()
	{
	}

	CString CBaseAbortCommand::Dump(unsigned int /*Number*/, unsigned int Tabs) const
	{	CString T, X;

		while (Tabs--) T+=L"\t";
		X.Format(L"ID=%u Type=%s : Abort", MessageID, (LPCTSTR)MessageType);
		T+=X;
		return T;
	}


	bool CBaseAbortCommand::Save(SavePtr X, DocPtr D, unsigned int MsgID) const
	{
		if (!CBaseCommand::Save(X, D, MsgID)) return false;

		_locale_t L = _create_locale(LC_CTYPE, "English");

		MSXML2::IXMLDOMElementPtr E(D->createElement(TypeName));
		E->setAttribute(L"CommandName", TypeName);
		MSXML2::IXMLDOMNodePtr Temp1(X->appendChild(E));

		_free_locale(L);
		return true;
	}

	bool CBaseAbortCommand::Load(SavePtr X, DocPtr D)
	{
		if (!CBaseCommand::Load(X, D)) return false;

		MSXML2::IXMLDOMElementPtr E(X->firstChild);
		
		BSTR A(E->getAttribute(L"CommandName").bstrVal);
		if (wcscmp(A, TypeName)) return false;

		_locale_t L = _create_locale(LC_CTYPE, "English");

		MSXML2::IXMLDOMNodeListPtr nodlist(E->childNodes);

		_free_locale(L);

		return true;
	}

	std::string CBaseAbortCommand::ToXmlString(unsigned int _MessageID) const
	{
		MSXML2::IXMLDOMDocumentPtr docptr;
		if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
		{	throw CString(L"Failed to create xml object");
		}
		docptr->async = VARIANT_FALSE;
		docptr->validateOnParse	= VARIANT_FALSE;
		docptr->resolveExternals = VARIANT_FALSE;
		MSXML2::IXMLDOMNodePtr Temp(docptr->appendChild(docptr->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"")));
			
		MSXML2::IXMLDOMNodePtr nodptr(docptr->createElement(StreamName()));
		Temp = docptr->appendChild(nodptr);
		if (!Save(nodptr, docptr, _MessageID)) return "";
		_bstr_t X(docptr->Getxml());
		return BstrToUtf8(X);
	}

	bool CBaseAbortCommand::FromXmlString(const char* Xml)
	{
		DocPtr docptr;
		if (!SUCCEEDED(docptr.CreateInstance(CLSID_DOMDocument)))
		{	throw CString(L"Failed to create xml object");
		}
		docptr->async = VARIANT_FALSE;
		docptr->validateOnParse = VARIANT_FALSE;
		docptr->resolveExternals = VARIANT_FALSE;
		if (!docptr->loadXML(Utf8ToBstr(Xml))) return false;
		return Load(docptr->documentElement, docptr);
	}

	bool CBaseAbortCommand::operator == (const CBaseAbortCommand& P) const
	{
		if (!(*static_cast<const CBaseCommand*>(this)==*static_cast<const CBaseCommand*>(&P))) return false;
		return true;
	}
	
	
}//namespace
