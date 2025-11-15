#include <afx.h>

#include "Base Command.h"
#include "..\Utf8\utf8.h"

#include <locale.h>

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif


namespace CommandHandler
{
	const wchar_t* CBaseCommand::TypeName = L"Command";
	const wchar_t* CBaseCommand::MessageTypeCommand = L"Command";
	const wchar_t* CBaseCommand::MessageTypeACK = L"Ack";
	const wchar_t* CBaseCommand::MessageTypeNACK = L"NoAck";
	const wchar_t* CBaseCommand::MessageTypeACKReply = L"AckReply";
	const wchar_t* CBaseCommand::MessageTypeReply = L"Reply";
	const wchar_t* CBaseCommand::MessageTypeReplyACK = L"ReplyAck";

	CBaseCommand::CBaseCommand()
		: MessageID(0)
		, MessageType(MessageTypeCommand)
		, NumberOfSends(0)
		, NumberOfRepliesSends(0)
	{
	}

	CBaseCommand::~CBaseCommand()
	{
	}

	CString CBaseCommand::Dump(unsigned int /*Number*/, unsigned int Tabs) const
	{	CString T, X;

		while (Tabs--) T+=L"\t";
		X.Format(L"ID=%u Type=%s Sends=%u: CBaseCommand", MessageID, (LPCTSTR)MessageType, NumberOfSends);
		return T + X;
	}

	bool CBaseCommand::Save(SavePtr X, DocPtr /*D*/, unsigned int MsgID) const
	{
		MSXML2::IXMLDOMElementPtr E(X);
		E->setAttribute(L"ID", MsgID);
		E->setAttribute(L"Type", (LPCTSTR)MessageType);
		E->setAttribute(L"Client", (LPCTSTR)Client);
		return true;
	}

	bool CBaseCommand::SaveDummyData(SavePtr X, DocPtr D) const
	{
		MSXML2::IXMLDOMElementPtr E(D->createElement(TypeName));
		E->setAttribute(L"CommandName", L"Dummy");
		MSXML2::IXMLDOMNodePtr Temp1(X->appendChild(E));
		E->setAttribute(L"Text", L"UltraDummy");
		return true;
	}

	bool CBaseCommand::Load(SavePtr X, DocPtr D)
	{
		MSXML2::IXMLDOMElementPtr E(X);
		MessageType = E->getAttribute(L"Type");
		Client = E->getAttribute(L"Client");
		BSTR A(E->getAttribute(L"ID").bstrVal);
		if (!A) return false;
		MessageID = wcstoul(A, NULL, 10);
		return true;
	}

	std::string CBaseCommand::ToXmlString(unsigned int _MessageID) const
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
		if (!SaveDummyData(nodptr, docptr)) return "";
		_bstr_t X(docptr->Getxml());
		return BstrToUtf8(X);
	}

	std::string CBaseCommand::AsReplyToXmlString(unsigned int _MessageID) const
	{
		_ASSERTE(MessageType==MessageTypeACK || MessageType==MessageTypeACKReply
						|| MessageType==MessageTypeReply || MessageType==MessageTypeNACK
						|| MessageType==MessageTypeReplyACK);
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
		if (!CBaseCommand::Save(nodptr, docptr, _MessageID)) return "";
		_bstr_t X(docptr->Getxml());
		return BstrToUtf8(X);
	}

	bool CBaseCommand::FromXmlString(const char* Xml)
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

	CString CBaseCommand::ObjectTypeFromXmlString(const char* ExpectedStreamName, const char* Xml)
	{
		DocPtr docptr;
		docptr.CreateInstance(CLSID_DOMDocument);
		docptr->async = VARIANT_FALSE;
		docptr->validateOnParse = VARIANT_FALSE;
		docptr->resolveExternals = VARIANT_FALSE;
		if (!docptr->loadXML(Utf8ToBstr(Xml)))	return L"";
		if (strcmp((char*)docptr->documentElement->nodeName, ExpectedStreamName))
		{	return L"";
		}
		if (docptr->documentElement->hasChildNodes())
		{	return docptr->documentElement->firstChild->nodeName.GetBSTR();
		}
		return TypeName;
	}

	bool CBaseCommand::operator == (const CBaseCommand& P) const
	{
		// do not compare the message type
		if (MessageID!=P.MessageID) return false;
		if (Client!=P.Client) return false;
		return true;
	}

	bool CBaseCommand::IsResponse()
	{
		return MessageType==MessageTypeACK || MessageType==MessageTypeNACK
				|| MessageType==MessageTypeACKReply || MessageType==MessageTypeReply;
	}
	
//------------------------------------------------------------------------------------------
	const wchar_t* CBaseError::TypeName = L"Error";

	CBaseError::CBaseError()
		: SourceStreamName("Command-Handler")
		, ErrorText(L"CBaseError: Unknown Error")
		, ErrorCode(1)
	{
	}

	CBaseError::CBaseError(const CBaseCommand& P, const CString Text, const int Code)
		: CBaseCommand(P)
		, SourceStreamName(P.StreamName())
		, ErrorText(Text)
		, ErrorCode(Code)
	{
	}

	CBaseError::~CBaseError()
	{
	}

	CString CBaseError::Dump(unsigned int /*Number*/, unsigned int Tabs) const
	{	CString T, X;

		while (Tabs--) T+=L"\t";
		X.Format(L"Code=%d Text=%s : CBaseError", ErrorCode, (LPCTSTR)ErrorText);
		return T + X;
	}

	bool CBaseError::Save(SavePtr X, DocPtr D, unsigned int MsgID) const
	{
		if (!CBaseCommand::Save(X, D, MsgID)) return false;

		_locale_t L = _create_locale(LC_CTYPE, "English");

		MSXML2::IXMLDOMElementPtr E(D->createElement(TypeName));
		E->setAttribute(L"Text", (LPCTSTR)ErrorText);
		wchar_t t[40];
		swprintf_s(t, 40, L"%d", ErrorCode);
		E->setAttribute(L"Code", t);
		MSXML2::IXMLDOMNodePtr Temp1(X->appendChild(E));
		
		_free_locale(L);
		return true;
	}

	bool CBaseError::Load(SavePtr X, DocPtr D)
	{
		if (!CBaseCommand::Load(X, D)) return false;

		MSXML2::IXMLDOMElementPtr E(X->firstChild);

		_locale_t L = _create_locale(LC_CTYPE, "English");

		BSTR A(E->getAttribute(L"Text").bstrVal);
		if (A) ErrorText = A;
		else ErrorText = E->text.GetBSTR();
		A = E->getAttribute(L"Code").bstrVal;
		if (A)	ErrorCode = wcstol(A, NULL, 10);
		else ErrorCode = 0;

		_free_locale(L);
		return true;
	}

	std::string CBaseError::ToXmlString(unsigned int _MessageID) const
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

	
	std::string CBaseError::AsReplyToXmlString(unsigned int _MessageID) const
	{
		_ASSERTE(MessageType==MessageTypeACK || MessageType==MessageTypeACKReply
						|| MessageType==MessageTypeReply || MessageType==MessageTypeNACK
						|| MessageType==MessageTypeReplyACK);
		return ToXmlString(_MessageID);
	}

	bool CBaseError::FromXmlString(const char* Xml)
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

	bool CBaseError::operator == (const CBaseError& P) const
	{
		if (!(*static_cast<const CBaseCommand*>(this)==*static_cast<const CBaseCommand*>(&P))) return false;
		if (ErrorText!=P.ErrorText) return false;
		if (ErrorCode!=P.ErrorCode) return false;
		return true;
	}

	void CBaseError::operator = (const CBaseCommand& P)
	{
		*static_cast<CBaseCommand*>(this) = *static_cast<const CBaseCommand*>(&P);
		ErrorText.Empty();
		ErrorCode = 0;
	}

//------------------------------------------------------------------------------------------

	const wchar_t* CLabwareData::TypeName = L"LWData";

	bool CLabwareData::Save(SavePtr X, DocPtr D) const
	{
		MSXML2::IXMLDOMElementPtr E(D->createElement(TypeName));
		MSXML2::IXMLDOMNodePtr Temp1(X->appendChild(E));
		E->setAttribute(L"Name", (LPCTSTR)Name);
		E->setAttribute(L"BC", (LPCTSTR)Barcode);


		// Erweiterte Attribute:
		E->setAttribute(L"SampleName", (LPCTSTR)SampleName);
		E->setAttribute(L"Vial", (LPCTSTR)Vial);
		E->setAttribute(L"Method_Path", (LPCTSTR)Method_Path);
		E->setAttribute(L"Type", (LPCTSTR)Typ);
		E->setAttribute(L"DataFile", (LPCTSTR)DataFile);
		E->setAttribute(L"Level", (LPCTSTR)Level);
		E->setAttribute(L"Dilution", (LPCTSTR)Dilution);
		E->setAttribute(L"Volume", (LPCTSTR)Volume);
		E->setAttribute(L"TrayName", (LPCTSTR)TrayName);
		E->setAttribute(L"Comment", (LPCTSTR)Comment);
		//E->setAttribute(L"Height", Height);
		return true;
	}

	bool CLabwareData::SaveDummyData(SavePtr /*X*/, DocPtr /*D*/) const
	{
		return true;
	}

	bool CLabwareData::Load(SavePtr X, DocPtr /*D*/)
	{
		MSXML2::IXMLDOMElementPtr E(X);

		BSTR A = E->getAttribute(L"Name").bstrVal;
		if (A) Name = A;
		else Name.Empty();
		A = E->getAttribute(L"BC").bstrVal;
		if (A) Barcode = A;
		else Barcode.Empty();
		A = E->getAttribute(L"SampleName").bstrVal;
		if (A) SampleName = A;
		A = E->getAttribute(L"Vial").bstrVal;
		if (A) Vial = A;
		else Vial.Empty();
		A = E->getAttribute(L"Method_Path").bstrVal;
		if (A) Method_Path = A;
		else Method_Path.Empty();
		A = E->getAttribute(L"Type").bstrVal;
		if (A) Typ = A;
		else Typ.Empty();
		A = E->getAttribute(L"DataFile").bstrVal;
		if (A) DataFile = A;
		else DataFile.Empty();
		A = E->getAttribute(L"Level").bstrVal;
		if (A) Level = A;
		else Level.Empty();
		A = E->getAttribute(L"Dilution").bstrVal;
		if (A) Dilution = A;
		else Dilution.Empty();
		A = E->getAttribute(L"Volume").bstrVal;
		if (A) Volume = A;
		else Volume.Empty();
		A = E->getAttribute(L"TrayName").bstrVal;
		if (A) TrayName = A;
		else TrayName.Empty();
		A = E->getAttribute(L"Comment").bstrVal;
		if (A) Comment = A;
		else Comment.Empty();

		return true;
	}

	bool CLabwareData::operator ==(const CLabwareData& P) const
	{
		return Name==P.Name && 
			Barcode==P.Barcode && 
			SampleName==P.SampleName && 
			Vial == P.Vial &&
			Method_Path == P.Method_Path &&
			Typ==P.Typ && 
			DataFile == P.DataFile &&
			Level ==P.Level && 
			Dilution==P.Dilution && 
			Volume==P.Volume && 
			TrayName==P.TrayName &&
			Comment==P.Comment;

	}


	const wchar_t* CPositionData::TypeName = L"PosData";

	bool CPositionData::Save(SavePtr X, DocPtr D) const
	{
		MSXML2::IXMLDOMElementPtr E(D->createElement(TypeName));
		MSXML2::IXMLDOMNodePtr Temp1(X->appendChild(E));
		E->setAttribute(L"Name", (LPCTSTR)PositionName);
		
		for (std::list<CLabwareData>::const_iterator i(Labware.begin());i!=Labware.end();++i)
		{	if (!i->Save(E, D)) return false;
		}

		return true;
	}

	bool CPositionData::SaveDummyData(SavePtr X, DocPtr D) const
	{
		return true;
	}

	bool CPositionData::Load(SavePtr X, DocPtr D)
	{
		Labware.clear();
		MSXML2::IXMLDOMElementPtr	E(X);

		PositionName = E->getAttribute(L"Name");
		MSXML2::IXMLDOMNodeListPtr nodlist(X->childNodes);

		for(int i=0;i<nodlist->length;i++)
		{	MSXML2::IXMLDOMNodePtr nodptr(nodlist->Getitem(i));
								
			CLabwareData A;
			if (A.Load(nodptr, D))
			{	Labware.push_back(A);
			}
		}
		return true;
	}

	bool CPositionData::operator ==(const CPositionData& P) const
	{
		if (PositionName!=P.PositionName) return false;
		if (Labware.size()!=P.Labware.size()) return false;
		for (std::list<CLabwareData>::const_iterator i(Labware.begin()), j(P.Labware.begin());i!=Labware.end();++i, ++j)
		{	if (!(*i==*j)) return false;
		}
		return true;
	}


}	// namespace
