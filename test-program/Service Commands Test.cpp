#include "stdafx.h"

#include <locale.h>

#include "Service Commands Test.h"

#include "..\..\Service Interface\Method Command.h"
#include "..\..\Service Interface\Resource Command.h"
#include "..\..\Service Interface\Version Command.h"
#include "..\..\Service Interface\KeepAlive Command.h"
#include "..\..\Service Interface\Abort Command.h"
#include "..\..\Service Interface\Stop Command.h"
#include "..\..\Service Interface\Continue Command.h"
#include "..\..\Service Interface\Command List.h"
#include "..\..\Utf8\Utf8.h"

#include <conio.h>
#include <memory>

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

using namespace MassHunter;

HANDLE hStdout = INVALID_HANDLE_VALUE;

void Wait(const CString& E)
{
	SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
	wprintf(L"\r\nError: %s\r\nPress any key to end the test\r\n", (LPCTSTR)E);
	while (!_kbhit())
	{	Sleep(20);
	}
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	_wsetlocale(LC_ALL, L"");
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return 0;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

	system("cls");
	SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	wprintf(L"MassHunter Service Command Test start\n");

	try
	{	// --------------------- CMethodCommand ---------------------------------------------------------
		std::list<CommandHandler::CPositionData> Labware;
		CommandHandler::CPositionData PD;
		PD.PositionName = L"P1";
		PD.Labware.push_back(CommandHandler::CLabwareData(L"Labw1", L"BC 1", L"GC Probe", L"1", L"C:\\MassHunter\\GCMS\\1\\methods", L"Sample", L"Plate One", L"", L"", L"", L"", L""));
		//PD.Labware.push_back(CommandHandler::CLabwareData(L"LabwX-9", L"BC 1-Tango-Emil"));
		//PD.Labware.back().
		Labware.push_back(PD);
		CString ProjectPath(L"C:\\Rostock");
		CString MasterBatch(L"Analytik1.b");
		CString MethodName(L"Analytik1.M");
		CString DataPath(L"C:\\Semester 5\\Masterarbeit\\Boost\\LsaToMassHunter\\LsaToMassHunter\\Test Data\\");
		// Method Command
		wprintf(L"Test CMethodCommand Create: ");
		CMethodCommand MC1(ProjectPath, MasterBatch, MethodName, DataPath, Labware), MC2;
		MC1.WellResultList.push_back(std::make_pair(L"A1", L"98.45"));
		MC1.WellResultList.push_back(std::make_pair(L"A9", L"Affe"));
		MC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		MC2 = MC1;
		CString FileName(L"MethodCommand.xml");
		unsigned int ID = 77;
		CString Client(L"MessageTest");
		MC1.MessageID = ID;
		MC1.Client = Client;
		std::string Xml(MC1.ToXmlString(88));
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		CFile F;
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();
		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		unsigned int L = (unsigned int)F.GetLength();
		char* Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		bool R = MC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CMethodCommand ID persistence: ");
		if (MC1.MessageID!=ID || MC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CMethodCommand operator ==: ");
		MC2.MessageID = ID;
		if (!(MC1==MC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CResourceStatusCommand ---------------------------------------------------------
		wprintf(L"Test CResourceStatusCommand Create: ");
		CResourceStatusCommand RSC1, RSC2;
		std::list<std::shared_ptr<CommandHandler::CBaseResourceInfo>> BCs;
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceStatusInfo(L"Oma", L"Status A", 7)));
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceStatusInfo(L"Opa", L"B Status", 456)));
		RSC1.List = BCs;
		RSC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		RSC2 = RSC1;
		FileName = L"ResourceStatusCommand.xml";
		ID = 77;
		RSC1.MessageID = ID;
		RSC1.Client = Client;
		Xml = RSC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();
		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = RSC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CResourceStatusCommand ID persistence: ");
		if (RSC1.MessageID!=ID || RSC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CResourceStatusCommand operator ==: ");
		RSC2.MessageID = ID;
		if (!(RSC1==RSC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");



		// --------------------- CResourceProjectsCommand ---------------------------------------------------------
		wprintf(L"Test CResourceProjectsCommand Create: ");
		CResourceProjectsCommand RPC1, RPC2;
		BCs.clear();
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceProjectInfo(L"Oma")));
		CommandHandler::CBaseResourceProjectInfo* PI = dynamic_cast<CommandHandler::CBaseResourceProjectInfo*>(BCs.back().get());
		PI->Projects.push_back(L"A");
		PI->Projects.push_back(L"B");
		PI->Projects.push_back(L"C");
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceProjectInfo(L"Opa")));
		PI = dynamic_cast<CommandHandler::CBaseResourceProjectInfo*>(BCs.back().get());
		PI->Projects.push_back(L"Z");
		PI->Projects.push_back(L"Y");
		PI->Projects.push_back(L"X");
		RPC1.List = BCs;
		RPC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		RPC2 = RPC1;
		FileName = L"ResourceProjectsCommand.xml";
		ID = 77;
		RPC1.MessageID = ID;
		RPC1.Client = Client;
		Xml = RPC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();
		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = RPC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CResourceProjectsCommand ID persistence: ");
		if (RPC1.MessageID!=ID || RPC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CResourceProjectsCommand operator ==: ");
		RPC2.MessageID = ID;
		if (!(RPC1==RPC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");



		// --------------------- CResourceMethodCommand ---------------------------------------------------------
		wprintf(L"Test CResourceMethodsCommand Create: ");
		CResourceMethodsCommand RMC1, RMC2;
		BCs.clear();
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceMethodInfo(L"Oma", ProjectPath)));
		CommandHandler::CBaseResourceMethodInfo* MI = dynamic_cast<CommandHandler::CBaseResourceMethodInfo*>(BCs.back().get());
		MI->Methods.push_back(L"A");
		MI->Methods.push_back(L"B");
		MI->Methods.push_back(L"C");
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceMethodInfo(L"Opa", ProjectPath)));
		MI = dynamic_cast<CommandHandler::CBaseResourceMethodInfo*>(BCs.back().get());
		MI->Methods.push_back(L"Z");
		MI->Methods.push_back(L"Y");
		MI->Methods.push_back(L"X");
		RMC1.List = BCs;
//		RMC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		RMC2 = RMC1;
		FileName = L"ResourceMethodsCommand.xml";
		ID = 77;
		RMC1.MessageID = ID;
		RMC1.Client = Client;
		Xml = RMC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();
		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = RMC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CResourceMethodsCommand ID persistence: ");
		if (RMC1.MessageID!=ID || RMC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CResourceMethodsCommand operator ==: ");
		RMC2.MessageID = ID;
		if (!(RMC1==RMC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");



		// --------------------- CResourceMasterBatchesCommand ---------------------------------------------------------
		wprintf(L"Test CResourceMasterBatchesCommand Create: ");
		CResourceMasterBatchesCommand RMBC1, RMBC2;
		BCs.clear();
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceMasterBatchInfo(L"Oma", ProjectPath)));
		CommandHandler::CBaseResourceMasterBatchInfo* MBI = dynamic_cast<CommandHandler::CBaseResourceMasterBatchInfo*>(BCs.back().get());
		MBI->MasterBatches.push_back(L"A");
		MBI->MasterBatches.push_back(L"B");
		MBI->MasterBatches.push_back(L"C");
		BCs.push_back(std::shared_ptr<CommandHandler::CBaseResourceInfo>(new CommandHandler::CBaseResourceMasterBatchInfo(L"Opa", ProjectPath)));
		MBI = dynamic_cast<CommandHandler::CBaseResourceMasterBatchInfo*>(BCs.back().get());
		MBI->MasterBatches.push_back(L"Z");
		MBI->MasterBatches.push_back(L"Y");
		MBI->MasterBatches.push_back(L"X");
		RMBC1.List = BCs;
		RMBC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		RMBC2 = RMBC1;
		FileName = L"ResourceMasterBatchesCommand.xml";
		ID = 77;
		RMBC1.MessageID = ID;
		RMBC1.Client = Client;
		Xml = RMBC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();
		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = RMBC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CResourceMasterBatchesCommand ID persistence: ");
		if (RMBC1.MessageID!=ID || RMBC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CResourceMasterBatchesCommand operator ==: ");
		RMBC2.MessageID = ID;
		if (!(RMBC1==RMBC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CVersionCommand ---------------------------------------------------------
		wprintf(L"Test CVersionCommand Create: ");
		CVersionCommand VC1, VC2;
		VC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		VC2 = VC1;
		FileName = L"VersionCommand.xml";
		ID = 77;
		VC1.MessageID = ID;
		VC1.Client = Client;
		Xml = VC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = VC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CVersionCommand ID persistence: ");
		if (VC1.MessageID!=ID || VC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CVersionCommand operator ==: ");
		VC2.MessageID = ID;
		if (!(VC1==VC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CKeepAliveCommand ---------------------------------------------------------
		wprintf(L"Test CKeepAliveCommand Create: ");
		CKeepAliveCommand KALC1, KALC2;
		KALC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		KALC2 = KALC1;
		FileName = L"KeepAliveCommand.xml";
		ID = 77;
		KALC1.MessageID = ID;
		KALC1.Client = Client;
		Xml = KALC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = KALC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}

		wprintf(L"Test CKeepAliveCommand ID persistence: ");
		if (KALC1.MessageID!=ID || KALC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");

		wprintf(L"Test CKeepAliveCommand operator ==: ");
		KALC2.MessageID = ID;
		if (!(KALC1==KALC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CAbortCommand ---------------------------------------------------------
		wprintf(L"Test CAbortCommand Create: ");
		CAbortCommand AC1, AC2;
		AC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		AC2 = AC1;
		FileName = L"AbortCommand.xml";
		ID = 77;
		AC1.MessageID = ID;
		AC1.Client = Client;
		Xml = AC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = AC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CAbortCommand ID persistence: ");
		if (AC1.MessageID!=ID || AC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CAbortCommand operator ==: ");
		AC2.MessageID = ID;
		if (!(AC1==AC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CStopCommand ---------------------------------------------------------
		wprintf(L"Test CStopCommand Create: ");
		CStopCommand SC1, SC2;
		SC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		SC2 = SC1;
		FileName = L"StopCommand.xml";
		ID = 77;
		SC1.MessageID = ID;
		SC1.Client = Client;
		Xml = SC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = SC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CStopCommand ID persistence: ");
		if (SC1.MessageID!=ID || SC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CStopCommand operator ==: ");
		SC2.MessageID = ID;
		if (!(SC1==SC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");


		// --------------------- CContinueCommand ---------------------------------------------------------
		wprintf(L"Test CContinueCommand Create: ");
		CContinueCommand CC1, CC2;
		CC1.MessageType = CommandHandler::CBaseCommand::MessageTypeACKReply;
		CC2 = CC1;
		FileName = L"ContinueCommand.xml";
		ID = 77;
		CC1.MessageID = ID;
		CC1.Client = Client;
		Xml = CC1.ToXmlString(88);
		wprintf(L"Ok, Data length %u bytes\n", Xml.length());
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = CC2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		wprintf(L"Test CContinueCommand ID persistence: ");
		if (CC1.MessageID!=ID || CC2.MessageID!=88)
		{	Wait(L"ID persistence failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CContinueCommand operator ==: ");
		CC2.MessageID = ID;
		if (!(CC1==CC2))
		{	Wait(L"== failed or failed to load data from file '" + FileName + L"'");
			return 1;
		}
		wprintf(L"Ok\n");

		// --------------------- CBaseCommand ---------------------------------------------------------
		wprintf(L"Test CBaseCommand empty stream: ");
		char* Ack("<MassHunter-Commands ID=\"88\" Type=\"AckReply\" Client=\"MessageTest\" />");
		std::list<std::shared_ptr<CommandHandler::CBaseCommand>> CL(CCommandList::ObjectsFromXmlString(Ack));
		if (CL.empty())
		{	Wait(L" failed");
			return 1;
		}
		wprintf(L"Ok\n");

		// --------------------- CCommandList ---------------------------------------------------------
		wprintf(L"Test CCommandList : ");
		Xml = MC1.ToXmlString();
		CL = CCommandList::ObjectsFromXmlString(Xml.c_str());
		if (CL.empty())
		{	Wait(L" failed");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CCommandList Error Handler: ");
		CString X = Utf8ToCString(Xml);
		X.Replace(L"BC", L"BÖ");
		Xml = CStringToUtf8(X);
		CL = CCommandList::ObjectsFromXmlString(Xml.c_str());
		if (CL.empty())
		{	Wait(L" failed");
			return 1;
		}
		wprintf(L"Ok\n");
		
		// --------------------- CBaseError ---------------------------------------------------------
		wprintf(L"Test CBaseError Create: ");
		CommandHandler::CBaseError BE1(MC1, L"Test Error", 7);
		wprintf(L"Ok\n");

		wprintf(L"Test CBaseError operator ==: ");
		CommandHandler::CBaseError BE2(BE1);
		if (!(BE1==BE2))
		{	Wait(L" failed");
			return 1;
		}
		wprintf(L"Ok\n");
		wprintf(L"Test CBaseError operator =: ");
		CommandHandler::CBaseError BE3;
		BE3 = MC1;
		wprintf(L"Ok\n");
		wprintf(L"Test CBaseError convert from to/xml: ");
		Xml = BE1.ToXmlString();
		FileName = L"ErrorMessage.xml";
		if (!F.Open(FileName, CFile::modeCreate | CFile::modeWrite))
		{	Wait(L"Failed to create file '" + FileName + L"'");
			return 1;
		}
		F.Write(Xml.c_str(), unsigned int(Xml.length()));
		F.Close();

		if (!F.Open(FileName, CFile::modeRead))
		{	Wait(L"Failed to open file '" + FileName + L"'");
			return 1;
		}
		L = (unsigned int)F.GetLength();
		Data = new char[L + 1];
		if (!F.Read(Data, L))
		{	Wait(L"Failed to read data from file '" + FileName + L"'");
			return 1;
		}
		F.Close();
		Data[L] = 0;
		R = BE2.FromXmlString(Data);
		delete[] Data;
		if (!R)
		{	Wait(L"Failed to convert data from XML");
			return 1;
		}
		BE2.MessageID = BE1.MessageID;
		if (!(BE1==BE2))
		{	Wait(L" failed");
			return 1;
		}
		wprintf(L"Ok\n");
	}
	catch (CString& E)
	{	Wait(E);
	}
	catch (_com_error& E)
	{	//system("cls");
		Wait(E.ErrorMessage());
	}
	catch (...)
	{	//system("cls");
		Wait(L"Error: ...");
	}
	CoUninitialize();
	SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	wprintf(L"MassHunter Service Command Test end\n");
	return 0;
}
