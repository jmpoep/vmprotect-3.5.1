#include "precompiled.h"
#include "../VMProtectCon/CommandLineParser.h"

typedef std::vector<std::string> Strings;

TEST(CommandLineParser, Empty)
{
	Strings vEmpty;
	CommandLineParser parser(vEmpty);
	ASSERT_FALSE(parser.IsOK());
}

TEST(CommandLineParser, JustFileName)
{
	Strings v;
	v.push_back("filename.exe");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename.exe", parser.GetFile());
	ASSERT_EQ("", parser.GetOutFile());
	ASSERT_EQ("", parser.GetProjectFile());
	ASSERT_EQ("", parser.GetScriptFile());
	ASSERT_EQ("", parser.GetWatermarkName());
	ASSERT_FALSE(parser.GetWarningsAsErrors());
}

TEST(CommandLineParser, FileNamePlusOutFileName)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("filename2.exe");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename1.exe", parser.GetFile());
	ASSERT_EQ("filename2.exe", parser.GetOutFile());
	ASSERT_EQ("", parser.GetProjectFile());
	ASSERT_EQ("", parser.GetScriptFile());
	ASSERT_EQ("", parser.GetWatermarkName());
	ASSERT_FALSE(parser.GetWarningsAsErrors());
}

//////////////////////////////////////////////////////////////////////////

TEST(CommandLineParser, FileNamePlusProjectName)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-pf");
	v.push_back("filename1.vmp");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename1.exe", parser.GetFile());
	ASSERT_EQ("", parser.GetOutFile());
	ASSERT_EQ("filename1.vmp", parser.GetProjectFile());
	ASSERT_EQ("", parser.GetScriptFile());
	ASSERT_EQ("", parser.GetWatermarkName());
	ASSERT_FALSE(parser.GetWarningsAsErrors());
}

TEST(CommandLineParser, FileNamePlusProjectNameBad)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-pf");
	CommandLineParser parser(v);
	ASSERT_FALSE(parser.IsOK());
}

//////////////////////////////////////////////////////////////////////////

TEST(CommandLineParser, FileNamePlusScriptName)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-sf");
	v.push_back("filename1.py");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename1.exe", parser.GetFile());
	ASSERT_EQ("", parser.GetOutFile());
	ASSERT_EQ("", parser.GetProjectFile());
	ASSERT_EQ("filename1.py", parser.GetScriptFile());
	ASSERT_EQ("", parser.GetWatermarkName());
	ASSERT_FALSE(parser.GetWarningsAsErrors());
}

TEST(CommandLineParser, FileNamePlusScriptNameBad)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-sf");
	CommandLineParser parser(v);
	ASSERT_FALSE(parser.IsOK());
}

//////////////////////////////////////////////////////////////////////////

TEST(CommandLineParser, FileNamePlusWatermarkName)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-wm");
	v.push_back("watermark");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename1.exe", parser.GetFile());
	ASSERT_EQ("", parser.GetOutFile());
	ASSERT_EQ("", parser.GetProjectFile());
	ASSERT_EQ("", parser.GetScriptFile());
	ASSERT_EQ("watermark", parser.GetWatermarkName());
	ASSERT_FALSE(parser.GetWarningsAsErrors());
}

TEST(CommandLineParser, FileNamePlusWatermarkNameBad)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-wm");
	CommandLineParser parser(v);
	ASSERT_FALSE(parser.IsOK());
}

//////////////////////////////////////////////////////////////////////////

TEST(CommandLineParser, FileNamePlusWarningsAsErrors)
{
	Strings v;
	v.push_back("filename1.exe");
	v.push_back("-we");
	CommandLineParser parser(v);
	ASSERT_TRUE(parser.IsOK());
	ASSERT_EQ("filename1.exe", parser.GetFile());
	ASSERT_EQ("", parser.GetOutFile());
	ASSERT_EQ("", parser.GetProjectFile());
	ASSERT_EQ("", parser.GetScriptFile());
	ASSERT_EQ("", parser.GetWatermarkName());
	ASSERT_TRUE(parser.GetWarningsAsErrors());
}

//////////////////////////////////////////////////////////////////////////

TEST(CommandLineParser, NotAnOptionAfterTheSecondFileName)
{
	Strings v;
	v.push_back("filename1.exe"); // input
	v.push_back("filename2.exe"); // output
	v.push_back("filename3.exe"); // not an option parameter
	CommandLineParser parser(v);
	ASSERT_FALSE(parser.IsOK());
}