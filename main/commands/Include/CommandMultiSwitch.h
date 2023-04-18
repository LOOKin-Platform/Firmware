/*
*    CommandSwitch.h
*    CommandSwitch_t implementation
*
*/

#ifndef COMMANDS_MULTISWITCH
#define COMMANDS_MULTISWITCH

class CommandMultiSwitch_t : public Command_t {
	private:
		vector<gpio_num_t> GPIOS = vector<gpio_num_t>();

	public:
		CommandMultiSwitch_t();
		void Overheated() override;

		bool IsOn(string Operand);
		bool IsOff(string Operand);

		bool Execute(uint8_t EventCode, const char *StringOperand) override;

	private:
		bool SetPin(gpio_num_t Pin, string StringOperand);
};


#endif