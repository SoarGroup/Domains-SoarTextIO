/*****************************************************
*  Author: Taylor Lafrinere
*  
*  Soar SoarTextIO
*
*  SoarTextIO.cpp
*
*  The purpose of this application is to allow the user
*  to freely type random words and sentences and have them
*  appear on the input-link.
*
*  Start Date: 06.23.2005
*
*****************************************************/

#include <portability.h>

#include "sml_Utils.h"
#include "SoarTextIO.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cctype>

#include <errno.h>
#include <time.h>

using namespace sml;

//void MyUpdateEventHandler(sml::smlUpdateEventId id, void* pUserData, sml::Kernel* pKernel, sml::smlRunFlags runFlags);
void MyStartSystemEventHandler(smlSystemEventId id, void* pUserData, Kernel* pKernel);
void MyStopSystemEventHandler(smlSystemEventId id, void* pUserData, Kernel* pKernel);
void MyAgentEventHandler(smlAgentEventId id, void* pUserData, Agent* pAgent) ;
void MyRunEventHandler(smlRunEventId id, void* pUserData, Agent* pAgent, smlPhase phase);
void enact_init_soar(sml::smlAgentEventId id, void* pUserData, sml::Agent* pAgent);
int callBackId;

#ifndef _WIN32
void* RunForever( void* info);
#endif

SoarTextIO::SoarTextIO()
{
	init_soar = false;
	RemoteConnect();
	cout << endl;	
}

SoarTextIO::~SoarTextIO()
{
	pKernel->Shutdown();
	delete pKernel;
}

void
SoarTextIO::init()
{
	//******INITIALIZE VARIABLES******
	sentenceNum = 1; 
	wordNum = 0;
	//if(!init_soar)
	//{
	pInputLink.holder = pAgent->GetInputLink();
	pOutputLink = pAgent->GetOutputLink();
	//}



	loadPlease = false;
	subtractOne = false;

	agentName = pAgent->GetAgentName();
	m_IsRunning = false;
	m_StopNow = false;
	initiateRes = false;
	initiateRem = false;
	printNow = true;
	ShouldPrintNow = false;
	PrintNothing = false;
	getnextline = false;
	print_hack = 0; // 0 indicates print normally

	if(!init_soar)
	{

		WhenReady();
		//callBackId = pKernel->RegisterForUpdateEvent(sml::smlEVENT_AFTER_ALL_OUTPUT_PHASES,MyUpdateEventHandler,this);
		callBackId = pKernel->RegisterForSystemEvent(smlEVENT_SYSTEM_START,MyStartSystemEventHandler, this );
		callBackId = pKernel->RegisterForSystemEvent(smlEVENT_SYSTEM_STOP, MyStopSystemEventHandler, this);
		callBackId = pKernel->RegisterForAgentEvent(smlEVENT_BEFORE_AGENT_REINITIALIZED, MyAgentEventHandler, this);
		callBackId = pKernel->RegisterForAgentEvent(smlEVENT_AFTER_AGENT_REINITIALIZED, enact_init_soar, this);
		callBackId = pAgent->RegisterForRunEvent(smlEVENT_BEFORE_INPUT_PHASE, MyRunEventHandler, this);
		inFile.open("!@#$%^&*");

		run();
	}

	init_soar = false;

	//pKernel->SetAutoCommit(false);

}



void
SoarTextIO::run()
{
	while(checker != "--QUIT" && checker != "--EXIT")
	{
		sentStore.resize(0);
		if(initiateRem)
		{
			initiateRem = false;
			//KillKernel();
			RemoteConnect();
		}
		else if(initiateRes)
		{
			initiateRes = false;
			//KillKernel();
			ResetConnect();
		}
		WriteCycle(&cin);

	}
}

void
SoarTextIO::runner()
{
#ifdef _WIN32
	_beginthread( RunForever, 0, this); //thread for the RunForever() command
#else
	pthread_create(&newThread, NULL, RunForever, this);
#endif
}
#ifdef _WIN32
void
SoarTextIO::RunForever( void* info )
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(info);
	if(!STIO->pKernel->IsSoarRunning())
	{
		STIO->m_IsRunning = true;
		STIO->pKernel->RunAllAgentsForever();
	}
	STIO->m_StopNow = false;
	
}
#else
void *
RunForever( void* info )
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(info);
	if(!STIO->pKernel->IsSoarRunning())
	{
		STIO->m_IsRunning = true;
		STIO->pKernel->RunAllAgentsForever();
	}
	STIO->m_StopNow = false;
	
	pthread_exit(NULL);
}
#endif


/*void
MyUpdateEventHandler(smlUpdateEventId id, void* pUserData, Kernel* pKernel, smlRunFlags runFlags)
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
	STIO->make_buffered_changes();
	if(STIO->m_StopNow == true)
	{
		pKernel->StopAllAgents();
		STIO->m_StopNow = false;
		STIO->m_IsRunning = false;
	}
	if(STIO->pAgent->GetNumberOutputLinkChanges()!=0)
	{
		STIO->RespondCycle();

	}	
	//STIO->pAgent->Commit();
}*/

void
MyStopSystemEventHandler(smlSystemEventId id, void* pUserData, Kernel* pKernel)
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
	STIO->m_IsRunning = false;
}

void
MyStartSystemEventHandler(smlSystemEventId id, void* pUserData, Kernel* pKernel)
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
	STIO->m_IsRunning = true;
}

void
MyAgentEventHandler(smlAgentEventId id, void* pUserData, Agent* pAgent)
{
	if(id == smlEVENT_BEFORE_AGENT_REINITIALIZED)
	{
		SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
		if(STIO->LastSent.size() > 0)
		{
			//STIO->pAgent->DestroyWME(STIO->LastSent[0]->holder);
			STIO->LastSent.clear();
			
		}
	}
	//	if(STIO->pTextInput != NULL)
	//		STIO->pAgent->DestroyWME(STIO->pTextInput);
	//	STIO->init_soar = true;
}

void 
MyRunEventHandler(smlRunEventId id, void* pUserData, Agent* pAgent, smlPhase phase)
{
	SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
	STIO->make_buffered_changes();
	if(STIO->m_StopNow == true)
	{
		pAgent->GetKernel()->StopAllAgents();
		STIO->m_StopNow = false;
		STIO->m_IsRunning = false;
	}
	if(STIO->pAgent->GetNumberOutputLinkChanges()!=0)
	{
		STIO->RespondCycle();

	}	
}

void enact_init_soar(sml::smlAgentEventId id, void* pUserData, sml::Agent* pAgent)
{
	if(id == smlEVENT_AFTER_AGENT_REINITIALIZED)
	{
		SoarTextIO* STIO = static_cast<SoarTextIO*>(pUserData);
		STIO->init_soar = true;
		STIO->init();
		STIO->changes.clear();
	}
}

void
SoarTextIO::make_buffered_changes()
{
	while(!changes.empty())
	{
		changes.front().make_change(pAgent);
		changes.pop_front();
	}
}

void
SoarTextIO::WriteCycle(istream* getFrom)
{
	wordNum = 0;
	NextWord.resize(0);
	bool printRead = false;


	word = "", forMem = "";
	checker = "";
	while(!printNow) { sml::Sleep(0,1); }
	if(getFrom == &cin)
	{
		if(print_hack != 2)
			cout << endl << endl << endl << endl << endl << "> ";
		print_hack = 0;
#ifndef _WIN32 
		print_position = 5;
#endif
	}
	if(ShouldPrintNow)
	{
		cout << "READ FROM FILE: ";
		if(!PrintNothing)
			cout << memory[memory.size() - 1] << endl;
		else
			cout << "<NOTHING>" << endl;
		ShouldPrintNow = false;
		print_hack = 1;
	}
	else
		printRead = true; 


	CarryOutCommand(getFrom);
}

void
SoarTextIO::CarryOutCommand(istream* getFrom)
{
	bool created = false;
	while(checker != "--STEP" && checker != "--CLEAR" && checker != "--RESET" && checker != "--REMOTE" && checker != "--CMDLIN" && checker != "--DEBUG" && checker != "--RUN" && checker != "--STOP" && checker != "--EXIT" && checker != "--QUIT" && checker != "--SAVE" && checker != "--LOAD" && getFrom->peek() != '\n' && getFrom->peek() != EOF && checker != "#&#&" )
	{
		wordNum++;
		word = "";
		if(!getnextline)
		{
			*getFrom >> word;
			if(init_soar)
				init();  //reintializes SoarTextIO
			while(word == "")
			{
				cout << "> ";
				if(!*getFrom)
					getFrom = &cin;
				*getFrom >> word;
			}
			if(pKernel)
				checker = word;
			makeUpper(checker);
		}
		if(checker == "--NEXTLINE" || getnextline)
		{
			ShouldPrintNow = true;
			getFrom = &inFile;
			GetFileInput(word);
			if(!*getFrom)
			{
				CloseFile();
				checker = "#&#&";  // this just gets us out of next if statement, this is bad and needs to be changed
			}
			getnextline = false;
		}		
		if(checker != "--STEP" && checker != "--CLEAR" && checker != "--RESET" && checker != "--REMOTE" && checker != "--CMDLIN" && checker != "--DEBUG" && checker != "--RUN" && checker != "--EXIT" && checker != "--QUIT" && checker != "--SAVE" && checker != "--LOAD" && checker != "#&#&" && checker != "--STOP")
		{
			if(!created)
			{
				createSentId();	
				//int temp = sentStore.size();
				//pAgent->CreateIntWME(sentStore[0],"text-input-number", sentenceNum);
				changes.push_back(buffered_change_t(CREATE, NULL, sentStore[0], "text-input-number", string_make(sentenceNum), INTEGER));
				//sml::Identifier* tmp3 = NULL; // = pAgent->CreateIdWME(NextWord[0], "next");
				WMEpointer* tmp3 = new WMEpointer();
				changes.push_back(buffered_change_t(CREATE, tmp3, NextWord[0], "next", "", ID));
				NextWord.push_back(tmp3);
				sentStore.push_back(tmp3);

				created = true;
			}
			CreateWord();
			forMem += (word + " ");
		}
		else 
		{
			print_hack = 1; // who ever has a chance to print next should
			if(checker == "--SAVE")
				saveMem();
			else if(checker == "--LOAD")
				loadMem();
			else if(checker == "--STOP")
				m_StopNow = true;
			else if(checker == "--RUN")
				runner();
			else if(checker == "--DEBUG")
				spawnDebugger();
			else if(checker == "--CMDLIN")
			{
				string command;
				getline(*getFrom, command);
				pAgent->ExecuteCommandLine(command.c_str(), true);
			}
			else if(checker == "--NEWTHREAD")
			{
				initiateRes = true;
			}
			else if(checker == "--REMOTE")
			{
				initiateRem = true;
			}
			else if(checker == "--STEP")
				step();
			else if(checker == "--CLEAR")
			{
				if(LastSent.size() > 0)
				{
					changes.push_back(buffered_change_t(DESTROY, &pTextInput, NULL, "", "", ID));
					NextWord.clear();
					LastSent.clear();
				}
			}
			
			
		}
	}
	if(checker != "--STEP" && checker != "--CLEAR" && checker != "--RESET" && checker != "--REMOTE" && checker != "--CMDLIN" && checker != "--DEBUG" && checker != "--RUN" && checker != "--SAVE" && checker != "--LOAD" && !loadPlease && checker != "--EXIT" && checker != "--QUIT" && checker != "--STOP")
	{
		if(getFrom != &cin)
		{
			wordNum--;
		}
		if(forMem != "")
			memory.push_back(forMem.substr(0, forMem.size() - 1));
		if(sentStore.size() >0)
		{
			//pAgent->CreateIntWME(sentStore[0], "length", wordNum); //add counter for num words
			changes.push_back(buffered_change_t(CREATE, NULL, sentStore[0], "length", string_make(wordNum), INTEGER));
		}
	}
	char garbage;
	if(checker != "--CMDLIN")
		getFrom->get(garbage);
	if(getFrom != &cin && getFrom->peek() == '\n')
	{
		while(isspace(getFrom->peek()))
		{
			char temp = getFrom->get();
			temp = temp;
		}
	}
	if(checker != "--STEP" && checker != "--CLEAR" && checker != "--RESET" && checker != "--REMOTE" && checker != "--CMDLIN" && checker != "--DEBUG" && checker != "--RUN" && checker != "--STOP" && checker != "--EXIT" && checker != "--QUIT" && checker != "--SAVE" && checker != "--LOAD")
	{
		if(NextWord.size() > 0)
		{
			//pAgent->DestroyWME(NextWord[NextWord.size()-1]); //gets rid of null-pointing next-word identifier
			changes.push_back(buffered_change_t(DESTROY, NextWord[NextWord.size()-1], NULL, "", "", STRING));
			//pAgent->CreateStringWME(NextWord[NextWord.size()-2],"next","nil");
			changes.push_back(buffered_change_t(CREATE, NULL, NextWord[NextWord.size()-2], "next", "nil", STRING));
		}
		sentenceNum++;		
	}	
	//pAgent->Commit();
}

bool
SoarTextIO::GetFileInput(string& word)
{
	if(!inFile)
		return false;
	while(true)
	{
		inFile >> word;
		if(word[0] != '#')  // not a comment
			break;
		getline(inFile, word);  // clears the line of comments		
	}
	return true;
}

void
SoarTextIO::CloseFile()
{
	inFile.clear();
	inFile.close();
	getnextline = false;
	cout << pAgent->GetAgentName() << " asked to get another line, but there is no file open\nor the file has ended." << endl;
	PrintNothing = true;
}

void
SoarTextIO::RespondCycle()
{
	top_level = "";
	int numberCommands2 = pAgent->GetNumberCommands();
	int numberCommands = pAgent->GetNumberOutputLinkChanges() ;
	for(int i = 0; i < numberCommands; i++)
	{
		if(pAgent->IsOutputLinkChangeAdd(i))
		{
			triple trip;
			sml::WMElement* tmp =pAgent->GetOutputLinkChange(i) ;
			trip.name = tmp->GetIdentifierName() ;
			trip.att = tmp->GetAttribute();
			trip.val = tmp->GetValueAsString();
			if(trip.att == "text")
				top_level = trip.val;
			if(trip.att == "get" && trip.val == "next-line")
				getnextline = true;

			trip.printed = false;
			storeO.push_back(trip);
			//cout << "(" << storeO[i].name << "   " << storeO[i].att << "   " << storeO[i].val << ")" << endl;
		}		
	}
	if(storeO.size()>0)
		PrintOutput();

	for(int i = 0; i< numberCommands2; i++)  //add's status complete
	{
		sml::Identifier* tmp2 = pAgent->GetCommand(i);
		WMEpointer* tempHolder = new WMEpointer();
		tempHolder->holder = tmp2;
		changes.push_back(buffered_change_t(CREATE, NULL, tempHolder, "status", "complete", STRING));
	}
	storeO.resize(0);

	if(getnextline)
	{
		GetNextLine();
	}
}

void
SoarTextIO::GetNextLine()
{
	string temp = checker;
	string temp_word = word;
	NextWord.resize(0);
	sentStore.resize(0);
	checker = "--NEXTLINE";
	getnextline = true;
	if(inFile.is_open() && inFile)
		CarryOutCommand(&inFile);
	else
		CloseFile();
	cout << endl << endl << endl << endl << endl << "> ";
#ifndef _WIN32
	print_position = 5;
#endif
	
	if(forMem != "")
	{
		cout << "READ FROM FILE: ";
		if(!PrintNothing)
			cout << forMem << endl;
		else
			cout << "<NOTHING>" << endl;
		print_hack = 1;
	}
	forMem = "";
	ShouldPrintNow = false;
	checker = temp;
	word = temp_word;
	getnextline = false;
}
void
SoarTextIO::PrintOutput()
{
	string toPrint = "";
	string NextName = "";
	bool found = false;
	for(unsigned int i = 0; i < storeO.size() && !found ; i++)
	{
		if(storeO[i].att == "value")// && storeO[i].name == top_level)
		{
			toPrint += storeO[i].val;
			NextName = storeO[i].name;
			NextName = FindNextParent(NextName);
			found = true;
		}
	}
	bool wordLeft = true;
	while(wordLeft)
	{
		wordLeft = false;
		for(unsigned int j = 0; j < storeO.size(); j++)
		{
			if(storeO[j].name == NextName && storeO[j].att == "value")
			{
				toPrint += (" " + storeO[j].val);
				NextName = FindNextParent(NextName);
				wordLeft = true;
			}
		}		
	}	
	if(toPrint.size()>0)
	{
		printNow = false;
		if(print_hack == 1) {
			cout << endl << endl << endl << endl << endl << "> ";
			print_hack = 2;
		}
		
#ifdef _WIN32
		char buffer[1000];
		CONSOLE_SCREEN_BUFFER_INFO info;
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if(!GetConsoleScreenBufferInfo( hStdout , &info ))
			cout << "GetInfo failed: " << GetLastError() << endl;
		COORD place;
		place.X = 0;
		place.Y = (info.dwCursorPosition.Y - 3); // get the current y position and move it up 3 lines
		SetConsoleCursorPosition(hStdout,place); // set the cursor up 3 lines
		cout << ": ";
		cout << toPrint << endl << endl;
		
		place.X = 0;
		place.Y = info.dwCursorPosition.Y;
		DWORD dummy;
		ReadConsoleOutputCharacter( hStdout , buffer /* holds what is read */ , 100 , place /* place to start reading */, &dummy );

		string holder = buffer;
		cout << "                                                                              " << endl ;
		cout << "                                                                              " << endl ;
		cout << "                                                                              " << endl ;

		cout << GetRelevant( holder );
#else
		int ret = system("tput sc"); // save the cursor position
		if (ret < 0) {
			// TODO: system call failed
		}
		if(print_position == 5) {
			ret = system("tput cuu 5"); // move the cursor 3 lines up
			if (ret < 0) {
				// TODO: system call failed
			}
		} else if(print_position == 4) {
			ret = system("tput cuu 4"); // move the cursor up 2 lines
			if (ret < 0) {
				// TODO: system call failed
			}
		} else if(print_position == 3) { // move the cursor up one line
			ret = system("tput cuu 3");
			if (ret < 0) {
				// TODO: system call failed
			}
		} else if(print_position == 2) {
			ret = system("tput cuu 2");
			if (ret < 0) {
				// TODO: system call failed
			}
		} else if(print_position == 1) {
			ret = system("tput cuu 1");
			if (ret < 0) {
				// TODO: system call failed
			}
		}
		
		ret = system("tput hpa 0"); // move the cursor to the start of the row
		if (ret < 0) {
			// TODO: system call failed
		}
		if(print_position == 0) // now we have to erase stuff in our way
		{
			ret = system("tput el"); // clear to the end of the line
			if (ret < 0) {
				// TODO: system call failed
			}

			ret = system("tput hpa 0"); // move the cursor back to the start
			if (ret < 0) {
				// TODO: system call failed
			}
		}

		cout << ": ";
		cout << toPrint << endl;

		if(print_position != 0)
		{
			ret = system("tput rc"); // return the cursor to its saved situation
			if (ret < 0) {
				// TODO: system call failed
			}
			print_position--;
		}
		else
		{
			cout << endl << endl << endl << endl << endl << "> ";
			cout.flush();
			print_position = 5;
		}
#endif // _WIN32
		

		printNow = true;
	}

}

string
SoarTextIO::FindNextParent(string name)
{
	for(unsigned int k = 0 ; k < storeO.size(); k++)
	{
		if(storeO[k].name == name && storeO[k].att == "next")
			return storeO[k].val;
	}
	return "error";

}

void
SoarTextIO::createSentId()
{
	if(LastSent.size() > 0)
	{
		//pAgent->DestroyWME(LastSent[0]);
		changes.push_back(buffered_change_t(DESTROY, LastSent[0], NULL, "", "", STRING));
		LastSent.resize(0);
	}
	//pTextInput = pAgent->CreateIdWME(pInputLink,"text");
	changes.push_back(buffered_change_t(CREATE, &pTextInput, &pInputLink, "text", "", ID));
	sentStore.push_back(&pTextInput);
	NextWord.push_back(&pTextInput);
	LastSent.push_back(&pTextInput);
	//pAgent->Commit();
}

void
SoarTextIO::CreateWord()
{

	//Create word			
	//sml::StringElement* tmp4 = NULL;// = pAgent->CreateStringWME(NextWord[NextWord.size()-1],"value",word.c_str());
	WMEpointer* tmp4 = new WMEpointer();
	changes.push_back(buffered_change_t(CREATE, tmp4, NextWord[NextWord.size()-1], "value", word, decipher_type(word)));
	dontlose.push_back(tmp4);

	//Create next-word identifier
	//sml::Identifier* tmp = NULL; // = pAgent->CreateIdWME(NextWord[NextWord.size()-1], "next");
	WMEpointer* tmp = new WMEpointer();
	changes.push_back(buffered_change_t(CREATE, tmp, NextWord[NextWord.size()-1], "next", "", ID));
	sentStore.push_back(tmp);
	NextWord.push_back(tmp);
	//pAgent->Commit();
}

void
SoarTextIO::saveMem()  
{
	//	locFinder();
	cin >> loc;
	ofstream outFile;
	outFile.open(loc.c_str());

	if(!outFile)
	{
		cout << "***FILE FAILED TO OPEN***";
		WhenReady();
	}
	else
	{
		for(unsigned int i = 0; i < memory.size(); i++)
		{
			outFile << memory[i];
			outFile << endl;
		}
		//outFile << "&&&&" ; //used as delimeter for file
		cout << "***YOUR FILE HAS BEEN SAVED***";
		outFile.close();
		//	WhenReady();
	}	
}

void
SoarTextIO::loadMem()
{

	cin >> loc;
	inFile.close();
	inFile.clear();
	inFile.open(loc.c_str());

	if(!inFile)
		cout << "***FILE FAILED TO OPEN***";
	else
		PrintNothing = false;

	GetNextLine();

}



void
SoarTextIO::ResetConnect()
{
	pKernel = sml::Kernel::CreateKernelInNewThread();
	if (pKernel->HadError())
	{
		cout << pKernel->GetLastErrorDescription() << endl;
		return ;
	}
	pAgent = pKernel->CreateAgent("SoarTextIO");
	if (pKernel->HadError())
	{
		cout << pKernel->GetLastErrorDescription() << endl;
		return ;
	}
	spawnDebugger();
	init();
}

void
SoarTextIO::RemoteConnect()
{
	pKernel = sml::Kernel::CreateRemoteConnection(true,NULL,12121);
	while (pKernel->HadError())
	{
		cout << endl << pKernel->GetLastErrorDescription() << endl;
		cout << endl << "There is no kernel on port 12121 to connect to, please create a kernel." << endl;
		cout << "When you are ready, press any non white-space key + enter: ";
		string stuff;
		cin >> stuff;
		pKernel = sml::Kernel::CreateRemoteConnection( true , NULL , 12121 );
	}
	cout << endl << "A connection to the kernel has been made." << endl;
	if(pKernel->GetNumberAgents() == 1)
	{
		pAgent = pKernel->GetAgentByIndex(0);
		cout << "Your are connected to agent " << pAgent->GetAgentName() << endl;
	}
	else
	{
		cout << endl << "What is the name of the agent you are trying to connect to: ";
		string name;
		cin >> name;
		pAgent = pKernel->GetAgent(name.c_str());
		while(!pKernel->IsAgentValid(pAgent))
		{
			cout << "That agent name is invalid, please enter a new one: ";
			cin >> name;
			pAgent = pKernel->GetAgent(name.c_str());
		}
		if (pKernel->HadError())
		{
			cout << pKernel->GetLastErrorDescription() << endl;
			return ;
		}	
		char junk;
		cin.get(junk);
	}
	init();


}

void
SoarTextIO::KillKernel()
{
	pKernel->Shutdown();
	delete pKernel;
}

void
SoarTextIO::spawnDebugger()
{
#if defined _WIN32

	// spawn the debugger asynchronously
	
	
	intptr_t ret = _spawnlp(_P_NOWAIT, "javaw.exe", "javaw.exe", "-jar", "SoarJavaDebugger.jar", "-remote", NULL);
	if(ret == -1) {
		switch (errno) {
				case E2BIG:
					cout << "arg list too long" << endl;
					break;
				case EINVAL:
					cout << "illegal mode" << endl;
					break;
				case ENOENT:
					cout << "file/path not found" << endl;
					break;
				case ENOEXEC:
					cout << "specified file not an executable" << endl;
					break;
				case ENOMEM:
					cout << "not enough memory" << endl;
					break;
				default:
					cout << ret << endl;
		}
	}
	

#else // linux spawnning

	pid_t pid = fork();

	if (pid < 0)
		cout << "fork failed" << endl;
	else if (pid == 0)
	{
		int ret = system("java -jar SoarJavaDebugger.jar -remote");
		if (ret < 0) {
			// TODO: system call failed
			exit(1);
		}
		pKernel->CheckForIncomingCommands();
		exit(1); // this forked process dies
	}
	else
		return;// parent process continues as normal

#endif
	// wait until we are notified that the debugger is spawned
	pKernel->GetAllConnectionInfo();
	char const * java_debugger = "java-debugger";
	char const * ready = "ready";


	while(1)
	{
		sml::Sleep(1,0);
		pKernel->GetAllConnectionInfo();
		char const * status = pKernel->GetAgentStatus(java_debugger);
		if(status && !strcmp(status,ready)) break;
	}
}

void
SoarTextIO::step()
{
	pKernel->RunAllAgents(1);
}

void
SoarTextIO::makeUpper(string & tosmall)
{
	for(unsigned int ii = 0; ii < tosmall.size(); ii++)
		tosmall[ii]=toupper(tosmall[ii]);
	return;
}

void
SoarTextIO::WhenReady()
{
	cout << endl << "Please make sure your productions are loaded before giving Soar input.";
	cout << endl << endl;
}

int main()
{
	SoarTextIO STIO;
}

string
SoarTextIO::GetRelevant( string toShorten )
{
	int index = 0;
	int count = 0;
	while( index < 100 && count < 6 )
	{
		if( toShorten[index] == ' ' )
			count++;
		else
			count = 0;
		index++;
	}
	string toReturn = "";
	for(int i = 0; i < index - 5; i++)
	{
		toReturn += toShorten[i];
	}

	if( toReturn.size() > 2 )
		toReturn = toReturn.substr( 0 , toReturn.size() - 1 );

	return toReturn;
}

// decide the type of the given string
SoarTextIO::buf_type SoarTextIO::decipher_type(const string& str)
{
	// look at each member of the string.  If we see a character we know it is a string
	// otherwise, if we see a period, and all numbers we know it is a float
	int num_periods = 0;
	for(int i = 0; i < int(str.length()) ; i++)
	{
		if (str[i] == '.')
			num_periods++;
		else if(isalpha(str[i]) || ispunct(str[i])) // we know it is a string
			return STRING;
	}
	if(num_periods == 0) // we haven't seen a character and it doesn't have a period - int
		return INTEGER;
	else if(num_periods == 1 && (!isalpha(str[0]) || !isalpha(str[str.length()-1]))) // - float
		return FLOAT;

	// otherwise just return string
	return STRING;
}

