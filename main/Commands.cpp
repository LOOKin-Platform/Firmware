/*
*    Commands.cpp
*    Class to handle API /Commands
*
*/
#include "Globals.h"
#include "Commands.h"

uint8_t Command_t::GetEventCode(string Action) {
	if (Events.count(Action) > 0)
		return Events[Action];

	for(auto const &Event : Events)
		if (Event.second == Converter::UintFromHexString<uint8_t>(Action))
			return Event.second;

	return +0;
}

Command_t* Command_t::GetCommandByName(string CommandName) {
	for (auto& Command : Commands)
		if (Converter::ToLower(Command->Name) == Converter::ToLower(CommandName))
			return Command;

	return nullptr;
}

Command_t* Command_t::GetCommandByID(uint8_t CommandID) {
	for (auto& Command : Commands)
		if (Command->ID == CommandID)
			return Command;

	return nullptr;
}

uint8_t Command_t::GetDeviceTypeHex() {
	return Device.Type.Hex;
}

vector<Command_t*> Command_t::GetCommandsForDevice() {
	vector<Command_t*> Commands = {};

	switch (Device.Type.Hex) {
#ifdef BUILD1GEN
		case Settings.Devices.Remote:
			Commands = { new CommandIR_t(), new CommandBLE_t() };
			break;
#else
		case Settings.Devices.Plug:
			Commands = { new CommandSwitch_t() };
			break;
		case Settings.Devices.Duo:
			Commands = {new CommandMultiSwitch_t() };
			break;
		case Settings.Devices.Remote:
			Commands = { new CommandIR_t(), new CommandBLE_t() };
			break;
		case Settings.Devices.WindowOpener:
			if (Settings.eFuse.Model == 0xF0)
				Commands = { new CommandDriventWindowOpener_t() };
			break;
		case Settings.Devices.Motion:
			Commands = { };
			break;
#endif
	}

	for (auto& CommandItem : Commands)
		CommandItem->InitSettings();

	return Commands;
}

void Command_t::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
    // Вывести список всех комманд

	map<string, string> Params = Query.GetParams();

	if (strlen(Query.GetBody()) > 0)
		Params = JSON(Query.GetBody()).GetItems();

	if (Query.GetURLPartsCount() == 1 && Query.Type == QueryType::GET) {
		vector<string> Vector = vector<string>();
		for (auto& Command : Commands)
			Vector.push_back(Command->Name);

		Result.Body = JSON::CreateStringFromVector(Vector);
    }

    // Запрос списка действий конкретной команды
	if (Query.GetURLPartsCount() == 2 && Query.Type == QueryType::GET) {
		Command_t* Command = Command_t::GetCommandByName(Query.GetStringURLPartByNumber(1));

		if (Command!=nullptr)
			if (Command->Events.size() > 0) {

			vector<string> Vector = vector<string>();
			for (auto& Event: Command->Events)
				Vector.push_back(Event.first);

			Result.Body = JSON::CreateStringFromVector(Vector);
		}
    }

	// Работа с настройками команды
	if (Query.GetURLPartsCount() == 3) {
		if (Query.Type == QueryType::GET && Query.CheckURLPart("settings", 2)) {
			Command_t* Command = Command_t::GetCommandByName(Query.GetStringURLPartByNumber(1));

			if (Command == nullptr) {
				Result.SetInvalid();
				return;
			}

			Result.Body = Command->GetSettings();

			if (Result.Body == "")
				Result.Body = "{}";

			return;
		}
		else if (Query.Type == QueryType::PUT && Query.CheckURLPart("settings", 2)) {
			Command_t* Command = Command_t::GetCommandByName(Query.GetStringURLPartByNumber(1));

			if (Command == nullptr) {
				Result.SetInvalid();
				return;
			}

			Command->SetSettings(Result, Query);

			return;
		}
	}

	/*
	GET /switch/on
	GET /switch/on?operand=FFFFFF
	POST /switch/on
	POST command=switch&action=on
	*/
	if (Query.GetURLPartsCount() == 3 ||
			(Query.GetURLPartsCount() == 1 && Params.size() > 0 && Query.Type != QueryType::DELETE))
	{
		string CommandName = "";

		if (Query.GetURLPartsCount() > 1) CommandName = Query.GetStringURLPartByNumber(1);

		if (Params.find("command") != Params.end()) {
			CommandName = Params[ "command" ];
			Params.erase ( "command" );
		}

		string Action = "";

		if (Query.GetURLPartsCount() > 2)
			Action = Query.GetStringURLPartByNumber(2);

		if (Params.find("action") != Params.end()) {
			Action = Params[ "action" ];
			Params.erase ( "action" );
		}

		string Operand = "0";
		if (Params.find("operand") != Params.end()) {
			Operand = Params[ "operand" ];
			Params.erase ( "operand" );
		}
		else if (Query.Type == QueryType::POST) {
			Operand = string(Query.GetBody());
		}

		Command_t* Command = Command_t::GetCommandByName(CommandName);
		if (Command == nullptr)
			Command = Command_t::GetCommandByID(Converter::UintFromHexString<uint8_t>(CommandName));

		if (Command != nullptr) {
			Operand = Converter::StringURLDecode(Operand);
			if (Command->Execute(Command->GetEventCode(Action), Operand.c_str()))
				Result.SetSuccess();
			else
				Result.SetFail();
		}
    }

    /*
    GET /switch/on/FFFFFF
    POST /switch/on/FFFFFF
    */
    if (Query.GetURLPartsCount() == 4 && Query.Type != QueryType::DELETE) {
		string CommandName 	= Query.GetStringURLPartByNumber(1);
		string Action 		= Query.GetStringURLPartByNumber(2);

		Command_t* Command = Command_t::GetCommandByName(CommandName);
		if (Command == nullptr)
			Command = Command_t::GetCommandByID(Converter::UintFromHexString<uint8_t>(CommandName));

		if (Command != nullptr) {
			const char* OperandPointer = Query.GetLastURLPartPointer();
			if (OperandPointer == NULL) {
				Result.SetFail();
				return;
			}

			if (Command->Execute(Command->GetEventCode(Action), OperandPointer))
				Result.SetSuccess();
			else
				Result.SetFail();
		}
    }
}

void Command_t::SendLocalMQTT(string Payload, string Topic) {
	LocalMQTT.SendMessage(Payload, "/commands" + Topic);
}
