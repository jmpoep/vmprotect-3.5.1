#ifndef CONSOLE_H
#define CONSOLE_H

class ConsoleLog : public ILog
{
public:
	ConsoleLog();
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "");
	virtual void StartProgress(const std::string &caption, unsigned long long max);
	virtual void StepProgress(unsigned long long value, bool is_project);
	virtual void EndProgress();
	virtual void set_warnings_as_errors(bool value) { warnings_as_errors_ = value; }
	virtual void set_arch_name(const std::string &arch_name) { arch_name_ = arch_name; }
	ConsoleLog &operator<<(ConsoleLog & (* function)(ConsoleLog &));
	ConsoleLog &operator<<(const std::string &text);
private:
	enum {
		NEED_PRINT_PERCENT = (size_t)-1
	};
	void PrintArch();
	void PrintPercent();
	unsigned long long pos_;
	unsigned long long max_;
	size_t percent_value_;
	size_t percent_chars_;
	std::string caption_;
	bool warnings_as_errors_;
	bool show_progress_;
	std::string arch_name_;
};

ConsoleLog &endl(ConsoleLog &log);

#endif
