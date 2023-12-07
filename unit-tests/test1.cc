#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/intel.h"

#include "testfile.h"

static uint8_t test_data[] = "test data";
const size_t TEST_DATA_SIZE = 9ul;

TEST(AbstractStreamTest, FileStream_Test)
{
	bool status;
	for (size_t bufSize = TEST_DATA_SIZE + 3; bufSize > 3; bufSize -= 3)
	{
		FileStream s(bufSize);
		uint8_t buf[16];
		size_t rc;
		uint64_t rc1, rc2;

		status = s.Open("test.bin", fmOpenReadWrite | fmShareDenyNone | fmCreate);
		ASSERT_TRUE(status);
		// Check .Write()
		rc = s.Write(reinterpret_cast<const uint8_t*>(test_data), TEST_DATA_SIZE);
		ASSERT_EQ(rc, TEST_DATA_SIZE);
		// Check .Seek() with various origins
		rc2 = s.Seek(1, soBeginning);
		ASSERT_EQ(rc2, 1ull);
		rc2 = s.Seek(2, soCurrent);
		ASSERT_EQ(rc2, 3ull);
		rc2 = s.Seek(1 - (int64_t)TEST_DATA_SIZE, soEnd);
		ASSERT_EQ(rc2, 1ull);
		// Check .Tell()
		rc1 = s.Tell();
		ASSERT_EQ(rc1, rc2);
		// Check .Read(), validate data
		rc = s.Read(buf, TEST_DATA_SIZE - 1);
		ASSERT_EQ(rc, TEST_DATA_SIZE - 1);
		ASSERT_EQ(0, memcmp(buf, &test_data[1], TEST_DATA_SIZE - 1)); //-V512
		s.Seek(0, soBeginning);
		rc = s.Read(buf, TEST_DATA_SIZE / 2);
		ASSERT_EQ(rc, TEST_DATA_SIZE / 2);
		ASSERT_EQ(0, memcmp(buf, &test_data[0], TEST_DATA_SIZE / 2)); //-V512
		rc = s.Read(buf, TEST_DATA_SIZE / 2);
		ASSERT_EQ(rc, TEST_DATA_SIZE / 2);
		ASSERT_EQ(0, memcmp(buf, &test_data[TEST_DATA_SIZE / 2], TEST_DATA_SIZE / 2)); //-V512
		// Check .GetSize()
		rc2 = s.Size();
		ASSERT_EQ(TEST_DATA_SIZE, rc2);
		// Check .Resize
		rc2 = s.Resize(2 * TEST_DATA_SIZE);
		ASSERT_EQ(TEST_DATA_SIZE * 2, rc2);
		rc2 = s.Seek(-((int64_t)TEST_DATA_SIZE), soEnd);
		ASSERT_EQ(TEST_DATA_SIZE, rc2);
		ASSERT_EQ(s.Size(), TEST_DATA_SIZE * 2);
		rc = s.Write(test_data, TEST_DATA_SIZE);
		ASSERT_EQ(rc, TEST_DATA_SIZE);
		rc = s.Write("\n12", 4);
		ASSERT_EQ(rc, 4ul);
		rc2 = s.Seek(TEST_DATA_SIZE, soBeginning);
		ASSERT_EQ(rc2, TEST_DATA_SIZE);
		std::string teststr;
		ASSERT_EQ(s.ReadLine(teststr), true);
		ASSERT_STREQ(teststr.c_str(), (const char *)test_data);
		s.Close();
	}
}

TEST(AbstractStreamTest, MemoryStream_Test)
{
	MemoryStream s;
	uint8_t buf[16];
	size_t rc;
	uint64_t rc1, rc2;

	// Check .Write()
	rc = s.Write(reinterpret_cast<const uint8_t*>(test_data), TEST_DATA_SIZE);
	ASSERT_EQ(rc, TEST_DATA_SIZE);
	// Check .Seek() with various origins
	rc2 = s.Seek(1, soBeginning);
	ASSERT_EQ(rc2, 1ull);
	rc2 = s.Seek(2, soCurrent);
	ASSERT_EQ(rc2, 3ull);
	rc2 = s.Seek(1 - (int64_t)TEST_DATA_SIZE, soEnd);
	ASSERT_EQ(rc2, 1ull);
	// Check .Tell()
	rc1 = s.Tell();
	ASSERT_EQ(rc1, rc2);
	// Check .Read(), validate data
	rc = s.Read(buf, TEST_DATA_SIZE - 1);
	ASSERT_EQ(rc, TEST_DATA_SIZE - 1);
	ASSERT_EQ(0, memcmp(buf, &test_data[1], TEST_DATA_SIZE - 1)); //-V512
	// Check .GetSize()
	rc2 = s.Size();
	ASSERT_EQ(TEST_DATA_SIZE, rc2);
}

TEST(AbstractStreamTest, MemoryStream_Copy_Test)
{
	MemoryStream s1, s2;
	uint8_t buf[16];
	size_t rc;
	uint64_t rc2;

	rc = s1.Write(reinterpret_cast<const uint8_t*>(test_data), TEST_DATA_SIZE);
	ASSERT_EQ(rc, TEST_DATA_SIZE);
	rc2 = s1.Seek(0, soBeginning);
	ASSERT_EQ(rc2, 0ull);
	rc = s2.CopyFrom(s1, TEST_DATA_SIZE);
	ASSERT_EQ(rc, TEST_DATA_SIZE);
	rc2 = s2.Seek(1, soBeginning);
	ASSERT_EQ(rc2, 1ull);
	rc = s2.Read(buf, TEST_DATA_SIZE - 1);
	ASSERT_EQ(rc, TEST_DATA_SIZE - 1);
	ASSERT_EQ(0, memcmp(buf, &test_data[1], TEST_DATA_SIZE - 1)); //-V512
}

TEST(AbstractStreamTest, FileStream_Copy_Test)
{
	bool status;
	for (size_t bufSize = TEST_DATA_SIZE + 3; bufSize > 3; bufSize -= 3)
	{
		FileStream s1(bufSize), s2(bufSize), s3(bufSize);
		status = s1.Open("test1.bin", fmOpenReadWrite | fmShareDenyNone | fmCreate);
		ASSERT_TRUE(status);
		status = s2.Open("test2.bin", fmOpenReadWrite | fmShareDenyNone | fmCreate);
		ASSERT_TRUE(status);
		uint8_t buf[16];
		size_t rc;
		uint64_t rc2;

		rc = s1.Write(reinterpret_cast<const uint8_t*>(test_data), TEST_DATA_SIZE);
		ASSERT_EQ(rc, TEST_DATA_SIZE);
		rc2 = s1.Seek(0, soBeginning);
		ASSERT_EQ(rc2, 0ull);
		rc = s2.CopyFrom(s1, TEST_DATA_SIZE);
		ASSERT_EQ(rc, TEST_DATA_SIZE);
		status = s3.Open("test2.bin", fmOpenRead | fmShareDenyNone);
		ASSERT_TRUE(status);
		ASSERT_STREQ(s3.ReadAll().c_str(), (const char *)(test_data));
		rc2 = s2.Seek(1, soBeginning);
		ASSERT_EQ(rc2, 1ull);
		rc = s2.Read(buf, TEST_DATA_SIZE - 1);
		ASSERT_EQ(rc, TEST_DATA_SIZE - 1);
		ASSERT_EQ(0, memcmp(buf, &test_data[1], TEST_DATA_SIZE - 1)); //-V512
		s1.Close();
		s2.Close();
		s3.Close();
	}
}

TEST(Demangle, Demangle_Test)
{
	const struct {
		const char *mangled;
		const char *demangled;
	} names[] = {
		// Check some names from Qt.
		{"?newCol@QColorPicker@?A0x3be3cb80@@QEAAXHH@Z", "`anonymous namespace'::QColorPicker::newCol(int,int)"},
		{"_ZN9QSettings10beginGroupERK7QString", "QSettings::beginGroup(QString const&)"},
		{"_ZNK10QTableView14verticalHeaderEv", "QTableView::verticalHeader() const"},
		{"_ZNK12QTableWidget20horizontalHeaderItemEi", "QTableWidget::horizontalHeaderItem(int) const"},

		// Apple names.
		{"St9bad_alloc", "std::bad_alloc"},
		{"_ZN1f1fE", "f::f"},
		{"_Z1fv", "f()"},
		{"_Z1fi", "f(int)"},
		{"_Z3foo3bar", "foo(bar)"},
		{"_Zrm1XS_", "operator%(X, X)"},
		{"_ZplR1XS0_", "operator+(X&, X&)"},
		{"_ZlsRK1XS1_", "operator<<(X const&, X const&)"},
		{"_ZN3FooIA4_iE3barE", "Foo<int [4]>::bar"},
		{"_Z1fIiEvi", "void f<int>(int)"},
		{"_Z5firstI3DuoEvS0_", "void first<Duo>(Duo)"},
		{"_Z5firstI3DuoEvT_", "void first<Duo>(Duo)"},
		{"_Z3fooIiFvdEiEvv", "void foo<int, void (double), int>()"},
		{"_ZN1N1fE", "N::f"},
		{"_ZN6System5Sound4beepEv", "System::Sound::beep()"},
		{"_ZN5Arena5levelE", "Arena::level"},
		{"_ZN5StackIiiE5levelE", "Stack<int, int>::level"},
		{"_Z1fI1XEvPVN1AIT_E1TE", "void f<X>(A<X>::T volatile*)"},
		{"_ZngILi42EEvN1AIXplT_Li2EEE1TE", "void operator-<42>(A<(42)+(2)>::T)"},
		{"_Z4makeI7FactoryiET_IT0_Ev", "Factory<int> make<Factory, int>()"},
		{"_Z3foo5Hello5WorldS0_S_", "foo(Hello, World, World, Hello)"},
		{"_Z3fooPM2ABi", "foo(int AB::**)"},
		{"_ZlsRSoRKSs", "operator<<(std::ostream&, std::string const&)"},
		{"_ZTI7a_class", "typeinfo for a_class"},
		{"U4_farrVKPi", "int* const volatile restrict _far"},
		{"_Z3fooILi2EEvRAplT_Li1E_i", "void foo<2>(int (&) [(2)+(1)])"},
		{"_Z1fM1AKFvvE", "f(void (A::*)() const)"},
		{"_Z3fooc", "foo(char)"},
		{"2CBIL_Z3foocEE", "CB<foo(char)>"},
		{"2CBIL_Z7IsEmptyEE", "CB<IsEmpty>"},
		{"_ZZN1N1fEiE1p", "N::f(int)::p"},
		{"_ZZN1N1fEiEs", "N::f(int)::string literal"},
		{"_Z1fPFvvEM1SFvvE", "f(void (*)(), void (S::*)())"},
		{"_ZN1N1TIiiE2mfES0_IddE", "N::T<int, int>::mf(N::T<double, double>)"},
		{"_ZSt5state", "std::state"},
		{"_ZNSt3_In4wardE", "std::_In::ward"},
		{"_Z1fKPFiiE", "f(int (* const)(int))"},
		{"_Z1fAszL_ZZNK1N1A1fEvE3foo_0E_i", "f(int [sizeof (N::A::f() const::foo)])"},
		{"_Z1fA37_iPS_", "f(int [37], int (*) [37])"},
		{"_Z1fM1AFivEPS0_", "f(int (A::*)(), int (*)())"},
		{"_Z1fPFPA1_ivE", "f(int (*(*)()) [1])"},
		{"_Z1fPKM1AFivE", "f(int (A::* const*)())"},
		{"_Z1jM1AFivEPS1_", "j(int (A::*)(), int (A::**)())"},
		{"_Z1sPA37_iPS0_", "s(int (*) [37], int (**) [37])"},
		{"_Z3fooA30_A_i", "foo(int [30][])"},
		{"_Z3kooPA28_A30_i", "koo(int (*) [28][30])"},
		{"_ZlsRKU3fooU4bart1XS0_", "operator<<(X bart foo const&, X bart)"},
		{"_ZlsRKU3fooU4bart1XS2_", "operator<<(X bart foo const&, X bart foo const)"},
		{"_Z1fM1AKFivE", "f(int (A::*)() const)"},
		{"_Z3absILi11EEvv", "void abs<11>()"},
		{"_ZN1AIfEcvT_IiEEv", "A<float>::operator int<int>()"},
		{"_ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_", "void libcw_app_ct::add_option<libcw_app_ct>(void (libcw_app_ct::*)(char const*), char const*, char, char const*, char const*)"},
		{"_ZGVN5libcw24_GLOBAL__N_cbll.cc0ZhUKa23compiler_bug_workaroundISt6vectorINS_13omanip_id_tctINS_5debug32memblk_types_manipulator_data_ctEEESaIS6_EEE3idsE", "guard variable for libcw::(anonymous namespace)::compiler_bug_workaround<std::vector<libcw::omanip_id_tct<libcw::debug::memblk_types_manipulator_data_ct>, std::allocator<libcw::omanip_id_tct<libcw::debug::memblk_types_manipulator_data_ct> > > >::ids"},
		{"_ZN5libcw5debug13cwprint_usingINS_9_private_12GlobalObjectEEENS0_17cwprint_using_tctIT_EERKS5_MS5_KFvRSt7ostreamE", "libcw::debug::cwprint_using_tct<libcw::_private_::GlobalObject> libcw::debug::cwprint_using<libcw::_private_::GlobalObject>(libcw::_private_::GlobalObject const&, void (libcw::_private_::GlobalObject::*)(std::ostream&) const)"},
		{"_ZNKSt14priority_queueIP27timer_event_request_base_ctSt5dequeIS1_SaIS1_EE13timer_greaterE3topEv", "std::priority_queue<timer_event_request_base_ct*, std::deque<timer_event_request_base_ct*, std::allocator<timer_event_request_base_ct*> >, timer_greater>::top() const"},
		{"_ZNKSt15_Deque_iteratorIP15memory_block_stRKS1_PS2_EeqERKS5_", "std::_Deque_iterator<memory_block_st*, memory_block_st* const&, memory_block_st* const*>::operator==(std::_Deque_iterator<memory_block_st*, memory_block_st* const&, memory_block_st* const*> const&) const"},
		{"_ZNKSt17__normal_iteratorIPK6optionSt6vectorIS0_SaIS0_EEEmiERKS6_", "std::__normal_iterator<option const*, std::vector<option, std::allocator<option> > >::operator-(std::__normal_iterator<option const*, std::vector<option, std::allocator<option> > > const&) const"},
		{"_ZNSbIcSt11char_traitsIcEN5libcw5debug27no_alloc_checking_allocatorEE12_S_constructIPcEES6_T_S7_RKS3_", "char* std::basic_string<char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator>::_S_construct<char*>(char*, char*, libcw::debug::no_alloc_checking_allocator const&)"},
		{"_Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_", "void f<A, A*, A const*>(A, A*, A const*, A const* (*) [4], A const* (* C::*) [4])"},
		{"_Z3fooiPiPS_PS0_PS1_PS2_PS3_PS4_PS5_PS6_PS7_PS8_PS9_PSA_PSB_PSC_", "foo(int, int*, int**, int***, int****, int*****, int******, int*******, int********, int*********, int**********, int***********, int************, int*************, int**************, int***************)"},
		{"_ZSt1BISt1DIP1ARKS2_PS3_ES0_IS2_RS2_PS2_ES2_ET0_T_SB_SA_PT1_", "std::D<A*, A*&, A**> std::B<std::D<A*, A* const&, A* const*>, std::D<A*, A*&, A**>, A*>(std::D<A*, A* const&, A* const*>, std::D<A*, A* const&, A* const*>, std::D<A*, A*&, A**>, A**)"},
		{"_X11TransParseAddress", "_X11TransParseAddress"},
		{"_ZNSt13_Alloc_traitsISbIcSt18string_char_traitsIcEN5libcw5debug9_private_17allocator_adaptorIcSt24__default_alloc_templateILb0ELi327664EELb1EEEENS5_IS9_S7_Lb1EEEE15_S_instancelessE", "std::_Alloc_traits<std::basic_string<char, std::string_char_traits<char>, libcw::debug::_private_::allocator_adaptor<char, std::__default_alloc_template<false, 327664>, true> >, libcw::debug::_private_::allocator_adaptor<std::basic_string<char, std::string_char_traits<char>, libcw::debug::_private_::allocator_adaptor<char, std::__default_alloc_template<false, 327664>, true> >, std::__default_alloc_template<false, 327664>, true> >::_S_instanceless"},
		{"_GLOBAL__I__Z2fnv", "global constructors keyed to fn()"},
		{"_Z1rM1GFivEMS_KFivES_M1HFivES1_4whatIKS_E5what2IS8_ES3_", "r(int (G::*)(), int (G::*)() const, G, int (H::*)(), int (G::*)(), what<G const>, what2<G const>, int (G::*)() const)"},
		{"_Z10hairyfunc5PFPFilEPcE", "hairyfunc5(int (*(*)(char*))(long))"},
		{"_Z1fILi1ELc120EEv1AIXplT_cviLd810000000000000000703DAD7A370C5EEE", "void f<1, (char)120>(A<(1)+((int)((double)[810000000000000000703DAD7A370C5]))>)"},
		{"_Z1fILi1EEv1AIXplT_cvingLf3f800000EEE", "void f<1>(A<(1)+((int)(-((float)[3f800000])))>)"},
		{"_ZNK11__gnu_debug16_Error_formatter14_M_format_wordImEEvPciPKcT_", "void __gnu_debug::_Error_formatter::_M_format_word<unsigned long>(char*, int, char const*, unsigned long) const"},
		{"_ZSt18uninitialized_copyIN9__gnu_cxx17__normal_iteratorIPSt4pairISsPFbP6sqlitePPcEESt6vectorIS9_SaIS9_EEEESE_ET0_T_SG_SF_", "__gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > > std::uninitialized_copy<__gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > >, __gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > > >(__gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > >, __gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > >, __gnu_cxx::__normal_iterator<std::pair<std::string, bool (*)(sqlite*, char**)>*, std::vector<std::pair<std::string, bool (*)(sqlite*, char**)>, std::allocator<std::pair<std::string, bool (*)(sqlite*, char**)> > > >)"},
		{"_Z1fP1cIPFiiEE", "f(c<int (*)(int)>*)"},
		{"_Z4dep9ILi3EEvP3fooIXgtT_Li2EEE", "void dep9<3>(foo<((3)>(2))>*)"},
		{"_ZStltI9file_pathSsEbRKSt4pairIT_T0_ES6_", "bool std::operator< <file_path, std::string>(std::pair<file_path, std::string> const&, std::pair<file_path, std::string> const&)"},
		{"_Z9hairyfuncM1YKFPVPFrPA2_PM1XKFKPA3_ilEPcEiE", "hairyfunc(int (* const (X::** (* restrict (* volatile* (Y::*)(int) const)(char*)) [2])(long) const) [3])"},
		{"_Z1fILin1EEvv", "void f<-1>()"},
		{"_ZNSdD0Ev", "std::basic_iostream<char, std::char_traits<char> >::~basic_iostream()"},
		{"_ZNK15nsBaseHashtableI15nsUint32HashKey8nsCOMPtrI4IFooEPS2_E13EnumerateReadEPF15PLDHashOperatorRKjS4_PvES9_", "nsBaseHashtable<nsUint32HashKey, nsCOMPtr<IFoo>, IFoo*>::EnumerateRead(PLDHashOperator (*)(unsigned int const&, IFoo*, void*), void*) const"},
		{"_ZNK1C1fIiEEPFivEv", "int (*C::f<int>() const)()"},
		{"_ZZ3BBdI3FooEvvENK3Fob3FabEv", "BBd<Foo>()::Fob::Fab() const"},
		{"_ZZZ3BBdI3FooEvvENK3Fob3FabEvENK3Gob3GabEv", "BBd<Foo>()::Fob::Fab() const::Gob::Gab() const"},
		{"_ZNK5boost6spirit5matchI13rcs_deltatextEcvMNS0_4impl5dummyEFvvEEv", "boost::spirit::match<rcs_deltatext>::operator void (boost::spirit::impl::dummy::*)()() const"},
		{"_Z3fooIA6_KiEvA9_KT_rVPrS4_", "void foo<int const [6]>(int const [9][6], int restrict const (* volatile restrict) [9][6])"},
		{"_Z3fooIA3_iEvRKT_", "void foo<int [3]>(int const (&) [3])"},
		{"_Z3fooIPA3_iEvRKT_", "void foo<int (*) [3]>(int (* const&) [3])"},
		{"_ZN13PatternDriver23StringScalarDeleteValueC1ERKNS_25ConflateStringScalarValueERKNS_25AbstractStringScalarValueERKNS_12TemplateEnumINS_12pdcomplementELZNS_16complement_namesEELZNS_14COMPLEMENTENUMEEEE", "PatternDriver::StringScalarDeleteValue::StringScalarDeleteValue(PatternDriver::ConflateStringScalarValue const&, PatternDriver::AbstractStringScalarValue const&, PatternDriver::TemplateEnum<PatternDriver::pdcomplement, PatternDriver::complement_names, PatternDriver::COMPLEMENTENUM> const&)"},
		{"ALsetchannels", "ALsetchannels"},
		{"_Z4makeI7FactoryiET_IT0_Ev", "Factory<int> make<Factory, int>()"},
		{"_Z1fM1AKiPKS1_", "f(int const A::*, int const A::* const*)"},
		{"_ZSA", "_ZSA"},
		{"_ZN1fIL_", "_ZN1fIL_"},
		{"_Za", "_Za"},
		{"_ZNSA", "_ZNSA"},
		{"_ZNT", "_ZNT"},
		{"_Z1aMark", "_Z1aMark"},
		{"_ZL3foo_2", "foo"},
		{"_ZZL3foo_2vE4var1", "foo()::var1"},
		{"_ZZL3foo_2vE4var1_0", "foo()::var1"},
		{"_ZZN7myspaceL3foo_1EvEN11localstruct1fEZNS_3fooEvE16otherlocalstruct", "myspace::foo()::localstruct::f(myspace::foo()::otherlocalstruct)"},

		// Check some names from MFC.
		{"?InitWorkspacesCollection@CDaoWorkspace@@IAEXXZ", "CDaoWorkspace::InitWorkspacesCollection(void)"},
		{"?Invoke@XEventSink@COleControlSite@@UAGJJABU_GUID@@KGPAUtagDISPPARAMS@@PAUtagVARIANT@@PAUtagEXCEPINFO@@PAI@Z",
			"COleControlSite::XEventSink::Invoke(long,struct _GUID const &,unsigned long,unsigned short,struct tagDISPPARAMS *,struct tagVARIANT *,struct tagEXCEPINFO *,unsigned int *)"},
		{"?IsNullValue@CDaoFieldExchange@@SGHPAXK@Z", "CDaoFieldExchange::IsNullValue(void *,unsigned long)"},
		{"??$?0VQObject@@@?$QWeakPointer@VQObject@@@@AEAA@PEAVQObject@@_N@Z", "QWeakPointer<class QObject>::<class QObject>::<class QObject>(class QObject *,bool)"},

		// Check not mangled names
		{"__SomeNotMangledName", "__SomeNotMangledName"}
	};
	size_t i;

	for (i = 0; i < _countof(names); i++) {
		const std::string actual = DemangleName(names[i].mangled).name();
		const char *expected = names[i].demangled;
		EXPECT_TRUE(actual.compare(expected) == 0) << actual << std::endl << expected;
	}
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	int rc;
#ifdef WIN
	/* Set up heap verifying. */
	_CrtSetDbgFlag(
		_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF);
#elif MACOSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif
	std::cout << "Unit test application #1\n";
	testing::InitGoogleTest(&argc, argv);
	rc = RUN_ALL_TESTS();
	remove("test.bin");
	remove("test1.bin");
	remove("test2.bin");
    
#ifdef MACOSX
    [pool release];
#endif
	return rc;
}
