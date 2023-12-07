#ifndef MAIN_H
#define MAIN_H

class ConsoleApplication
{
public:
	ConsoleApplication(const std::vector<std::string> &args);
	int Run();
private:
	std::vector<std::string> args_;
	ConsoleLog log_;
};

#endif