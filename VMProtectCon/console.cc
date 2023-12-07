#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/lang.h"
#include "console.h"

/**
 * ConsoleLog
 */

ConsoleLog::ConsoleLog()
	: pos_(0), max_(0), percent_value_(0), percent_chars_(0), warnings_as_errors_(false)
{
	show_progress_ = (isatty(fileno(stdout)) != 0);
}

static const char *compilation_type[] = {
	"V",
	"M",
	"U"
};

bool IsUserFunction(const IFunction *func)
{
	IArchitecture *file = func->owner()->owner();
	return (file && file->function_list()->IndexOf(func) != NOT_ID);
}

void ConsoleLog::Notify(MessageType type, IObject *sender, const std::string &message)
{
	static const LangString message_lang_type[] = {
		lsInformation,
		lsWarning,
		lsError,
		lsLoading,
		lsChanging,
		lsDeleting,
		lsScript,
	};

	bool need_throw;
	if (type == mtWarning && warnings_as_errors_) {
		type = mtError;
		need_throw = true;
	} else {
		need_throw = false;
	}

	std::string log_message, add;
	std::string message_type = language[message_lang_type[type]];
	if (sender) {
		if (IFunction *func = dynamic_cast<IFunction *>(sender)) {
			if (IsUserFunction(func) && (type == mtAdded || type == mtChanged || type == mtDeleted)) {
				std::string compilation_type_str = func->need_compile() ? compilation_type[func->compilation_type()] : "-";
				std::string address_str = func->type() != otUnknown ? string_format("%.8llX", func->address()) : "????????";
				log_message = string_format("%s [%s] %s %s", message_type.c_str(), compilation_type_str.c_str(), address_str.c_str(), func->display_name().c_str());
			}
		} else if (ICommand *command = dynamic_cast<ICommand *>(sender)) {
			if (IsUserFunction(command->owner())) {
				std::string func_name;
				if (command->owner()) {
					func_name = command->owner()->display_name();
					if (!func_name.empty())
						func_name += ".";
				}
				add = string_format("%s%.8llX: ", func_name.c_str(), command->address());
			}
		}
	}
	if (type == mtInformation || type == mtWarning || type == mtError || type == mtScript)
		log_message = string_format("[%s] %s%s", message_type.c_str(), add.c_str(), message.c_str());

	if (!log_message.empty()) {
		EndProgress();
		PrintArch();
		operator << (log_message) << endl;
	}

	if (need_throw)
		throw abort_error("");
}

void ConsoleLog::StartProgress(const std::string &caption, unsigned long long max)
{
	EndProgress();
	caption_ = caption;
	pos_ = 0;
	max_ = max;
	percent_value_ = 0;
}

void ConsoleLog::PrintArch()
{
	if (!arch_name_.empty())
		operator << (arch_name_) << "> ";
}

void ConsoleLog::PrintPercent()
{
	std::string percent_str = string_format(" %d%%", percent_value_);
	operator << (percent_str);
	percent_chars_ = percent_str.size();
}

void ConsoleLog::StepProgress(unsigned long long value, bool is_project)
{
	if (is_project)
		return;

	pos_ += value;
	size_t old_percent_value = percent_value_;
	percent_value_ = static_cast<size_t>(100.0 * pos_ / max_);

	if (percent_chars_) {
		if (percent_value_ == old_percent_value)
			return;

		if (percent_chars_ != NEED_PRINT_PERCENT) {
			std::string back_str;
			size_t i;
			for (i = 0; i < percent_chars_; i++) {
				back_str += '\b';
			}
			for (i = 0; i < percent_chars_; i++) {
				back_str += ' ';
			}
			for (i = 0; i < percent_chars_; i++) {
				back_str += '\b';
			}
			operator << (back_str);
		}
	} else {
		PrintArch();
		operator << (caption_);
	}

	if (show_progress_)
		PrintPercent();
	else
		percent_chars_ = NEED_PRINT_PERCENT;
}

void ConsoleLog::EndProgress()
{
	if (!percent_chars_)
		return;
	if (percent_chars_ == NEED_PRINT_PERCENT)
		PrintPercent();
	operator << (endl);
	percent_chars_ = 0;
}

ConsoleLog &ConsoleLog::operator<<(ConsoleLog & (* function)(ConsoleLog &))
{
	(*function)(*this);
	return *this;
}

ConsoleLog &ConsoleLog::operator<<(const std::string &text)
{
	os::Print(text.c_str());
	return *this;
}

ConsoleLog &endl(ConsoleLog &log)
{
	return log << "\n";
}