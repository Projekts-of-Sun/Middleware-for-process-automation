#pragma once
#include <afx.h>

#include "msxml6.tlh"

#include <string>
#include <list>

namespace CommandHandler
{
	typedef MSXML2::IXMLDOMNodePtr SavePtr;
	typedef MSXML2::IXMLDOMDocumentPtr DocPtr;

	/// <summary>Base class for all network commands.</summary>
	class CBaseCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			static const wchar_t* TypeName;
			static const wchar_t* MessageTypeCommand;
			static const wchar_t* MessageTypeACK;
			static const wchar_t* MessageTypeNACK;
			static const wchar_t* MessageTypeACKReply;
			static const wchar_t* MessageTypeReply;
			static const wchar_t* MessageTypeReplyACK;

			/// <summary>Get the transport stream name.</summary>
			/// <returns>Transport stream name</returns>
			virtual char* StreamName() const
			{
				return "Command-Handler";
			}

			CBaseCommand();
			virtual ~CBaseCommand();
			
			/// <summary>Append the content of the this command object to another xml document object.</summary>
			/// <param name="SavePtr">Parent object for this xml operation.</param>
			/// <param name="DocPtr">Pointer to the xml document object.</param>
			/// <param name="MsgID">message id to use for this conversion.</param>
			/// <returns>True if the command was successfully converted to xml and saved to X</returns>
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			/// <summary>Append some extra data object to another xml document object.</summary>
			/// <param name="SavePtr">Parent object for this xml operation.</param>
			/// <param name="DocPtr">Pointer to the xml document object.</param>
			/// <returns>True if the command was successfully converted to xml and saved to X</returns>
			virtual bool SaveDummyData(SavePtr X, DocPtr D) const;
			/// <summary>Load content xml document object into this command.</summary>
			/// <param name="SavePtr">Parent object for this xml operation.</param>
			/// <param name="DocPtr">Pointer to the xml document object.</param>
			/// <returns>True if the command was successfully converted to xml and saved to X</returns>
			virtual bool Load(SavePtr X, DocPtr D);
			/// <summary>Check if this command represents a resonse.</summary>
			/// <returns>True if it is a response</returns>
			virtual bool IsResponse();
			
			/// <summary>Get the command name.</summary>
			/// <returns>Command name</returns>
			virtual CString CommandName() const
			{	return TypeName;
			}

			/// <summary>Convert the command content into xml string.</summary>
			/// <param name="MessageID">Message id to use in this xml string.</param>
			/// <returns>Xml string</returns>
			virtual std::string ToXmlString(unsigned int MessageID = 0) const;
			/// <summary>Convert the command content as a reply into xml string.</summary>
			/// <param name="MessageID">Message id to use in this xml string.</param>
			/// <returns>Xml string</returns>
			virtual std::string AsReplyToXmlString(unsigned int MessageID = 0) const;
			/// <summary>Read command data into this object from xml string.</summary>
			/// <param name="Xml">Xml string.</param>
			/// <returns>True if read successfully</returns>
			virtual bool FromXmlString(const char* Xml);

			/// <summary>Compare the content of two CBaseCommand object (excluding the message type).</summary>
			/// <param name="P">CBaseCommand reference.</param>
			/// <returns>True if equal</returns>
			virtual bool operator ==(const CBaseCommand& P) const;

			/// <summary>Get the object type (tag) from xml string.</summary>
			/// <param name="ExpectedStreamName">Expected transport stream name</param>
			/// <param name="Xml">Xml string.</param>
			/// <returns>Object type name</returns>
			static CString ObjectTypeFromXmlString(const char* ExpectedStreamName, const char* Xml);
			/// <summary>ID to identify this message and its responses.</summary>
			unsigned int MessageID;
			/// <summary>Message type, one of: ACK, NACK, ACKReply, Reply, ReplyACK.</summary>
			CString MessageType;
			/// <summary>The name of the client which sends this message.</summary>
			CString Client;

			// these counters will not be writtten into xml
			// or used to compare messsages
			/// <summary>Number of times this message has been sent. This number gets not written into xml 
			/// and is not used for comparing commands.</summary>
			unsigned int NumberOfSends;
			/// <summary>Number of times this message has been sent as a reply. This number gets not written into xml 
			/// and is not used for comparing commands.</summary>
			unsigned int NumberOfRepliesSends;
	};

	/// <summary>Error class for responding to other command messages.</summary>
	class CBaseError : public CBaseCommand
	{
		public:

			virtual CString Dump(unsigned int Number = 0, unsigned int Tabs = 0) const;

			static const wchar_t* TypeName;

			virtual char* StreamName() const
			{
				return (char*)SourceStreamName.c_str();
			}
			
			virtual CString CommandName() const
			{	return TypeName;
			}

			CBaseError();
			CBaseError(const CBaseCommand& P, const CString Text, const int Code);
			virtual ~CBaseError();
			
			virtual	bool Save(SavePtr X, DocPtr D, unsigned int MsgID) const;
			virtual bool Load(SavePtr X, DocPtr D);

			virtual std::string ToXmlString(unsigned int MessageID = 0) const;
			virtual bool FromXmlString(const char* Xml);
			virtual std::string AsReplyToXmlString(unsigned int MessageID = 0) const;

			virtual bool operator ==(const CBaseError& P) const;
			virtual void operator =(const CBaseCommand& P);

			/// <summary>Holds the original transport stream name in case this object was
			/// constructed from another command object.</summary>
			std::string SourceStreamName;
			CString ErrorText;
			int ErrorCode;
	};

	/// <summary>Class for labware data within a transport command.</summary>
	class CLabwareData
	{
	public:
		CLabwareData(CString LabwareName, CString LabwareBarcode, CString LabwareSampleName, CString LabwareVial, CString LabwareMethod_Path,
			CString LabwareTyp	, CString LabwareDataFile, CString LabwareLevel, CString LabwareDilution, CString LabwareVolume, CString LabwareTrayName, CString LabwareComment)
			: Name(LabwareName)
			, Barcode(LabwareBarcode)
			, SampleName(LabwareSampleName)
			, Vial(LabwareVial)
			, Method_Path(LabwareMethod_Path)
		    , Typ(LabwareTyp)
			, DataFile(LabwareDataFile)
		    , Level(LabwareLevel)
		    , Dilution (LabwareDilution)
		    , Volume (LabwareVolume)
		    , TrayName (LabwareTrayName)
		   , Comment (LabwareComment)
		{	}
		CLabwareData()
		{	}

		static const wchar_t* TypeName;

		virtual	bool Save(SavePtr X, DocPtr D) const;
		virtual bool SaveDummyData(SavePtr X, DocPtr D) const;
		virtual bool Load(SavePtr X, DocPtr D);
		virtual bool operator ==(const CLabwareData& P) const;

		CString Barcode;
		CString Name;
		CString SampleName;      // SampleName
		CString Vial;            // Vial Position (z. B. A1)
		CString Method_Path ;
		CString Typ;         // Typ (Sample, QC, Cal)
		CString DataFile;
		CString Level;     // Level
		CString Dilution;  // Dilution
		CString Volume;    // Volume
		CString TrayName;  // Tray
		CString Comment;   // Kommentar
	};

	/// <summary>Class for position data including labware data within a transport command.</summary>
	class CPositionData
	{
	public:
		static const wchar_t* TypeName;

		virtual	bool Save(SavePtr X, DocPtr D) const;
		virtual bool SaveDummyData(SavePtr X, DocPtr D) const;
		virtual bool Load(SavePtr X, DocPtr D);
		virtual bool operator ==(const CPositionData& P) const;

		CString PositionName;
		std::list<CLabwareData> Labware;
	};

	/// <summary>Class for data to check.</summary>
	class CItemToCheck
	{
		public:
			CItemToCheck(unsigned __int64 _ID, CString& _Description, CString& _HelpLink)
				: ID(_ID)
				, Description(_Description)
				, HelpLink(_HelpLink)
			{}
			CItemToCheck()
				: ID(0)
			{}
			unsigned __int64 ID;
			CString Description;
			CString HelpLink;
	};

	/// <summary>Class for location data. Used in transport/check commands.</summary>
	class CLocation
	{
	public:
		CLocation(const CString& L, const CString& LBC, const CString& D, const CString& DBC, const CString& P, const CString& PBC)
			: Lab(L)
			, LabBC(LBC)
			, Device(D)
			, DeviceBC(DBC)
			, Position(P)
			, PositionBC(PBC)
		{	}
		CLocation()
		{ }
		CString Lab;
		CString LabBC;
		CString Position;
		CString PositionBC;
		CString Device;
		CString DeviceBC;
	};

	/// <summary>Class for rack data. Used in transport commands.</summary>
	class CRack
	{
	public:
		CRack(CString& N, CString& BC, CString& S,  CString& V, CString ME, CString& T, CString& DA, CString& L, CString& D, CString& VI, CString& TR,
			CString& CO, bool UseIt = false)
			: Name(N)
			, Barcode(BC)
			, SampleName(S)
			, Vial(V)
			, Method_Path(ME)
			, Typ(T)
			, DataFile(DA)
			, Level(L)
			, Dilution(D)
			, Volume(VI)
			, TrayName(TR)
			, Comment(CO)
			, Used(UseIt)
		{	}
		CRack()
			: Name(L"")
			, Barcode(L"")
			, SampleName(L"")
			, Vial(L"")
			, Method_Path(L"")
			, Typ(L"")
			, DataFile(L"")
			, Level(L"")
			, Dilution(L"")
			, Volume(L"")
			, TrayName(L"")
			, Comment(L"")
			, Used(false)
		{	}
		bool operator ==(const CRack& P) const
		{
			if (Used!=P.Used) return false;
			if (!Used) return true;
			if (Name!=P.Name) return false;
			if (Barcode!=P.Barcode) return false;
			if (SampleName!= P.SampleName) return false;
			if (Vial!= P.Vial) return false;
			if (Typ != P.Typ) return false;
			if (Method_Path != P.Method_Path) return false;
			if (DataFile != P.DataFile) return false;
			if (Level!= P.Level) return false;
			if (Dilution!= P.Dilution) return false;
			if (Volume!= P.Volume) return false;
			if (TrayName!= P.TrayName) return false;
			if (Comment!= P.Comment) return false;


			return true;
		}
		CString Name;
		CString Barcode;
		//*****
		CString SampleName;      // SampleName
		CString Vial;            // Vial Position (z. B. A1)
		CString Method_Path;
		CString Typ;         // Typ (Sample, QC, Cal)
		CString DataFile;
		CString Level;     // Level
		CString Dilution;  // Dilution
		CString Volume;    // Volume
		CString TrayName;  // Tray
		CString Comment ;
		bool Used;
	};


}	//namespace