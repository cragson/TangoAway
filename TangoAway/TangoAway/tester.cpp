#include "tester.hpp"

#include "utils.hpp"

#include "logging.hpp"
#include "tc_defines.hpp"
#include "tc_macro_with_params.hpp"
#include "tc_math.hpp"
#include "tc_simple-macro.hpp"
#include "tc_simple_import.hpp"
#include "tc_single_line_comments.hpp"
#include "tc_use_float_alias.hpp"
#include "tc_add_vdr_imm.hpp"
#include "tc_add_vdr_vdr.hpp"
#include "tc_and_vdr_imm.hpp"
#include "tc_and_vdr_vdr.hpp"
#include "tc_cmp.hpp"
#include "tc_cmp_with_jmps.hpp"
#include "tc_cmp_with_jmps2.hpp"
#include "tc_cmp_with_jmps3.hpp"
#include "tc_cmp_with_jmps4.hpp"
#include "tc_cmp_with_jmps5.hpp"
#include "tc_cmp_with_jmps6.hpp"
#include "tc_cmp_with_jmps7.hpp"
#include "tc_div_vdr_imm.hpp"
#include "tc_div_vdr_vdr.hpp"
#include "tc_find_signature.hpp"
#include "tc_forward_backward_jmp.hpp"
#include "tc_get_image_base.hpp"
#include "tc_get_image_size.hpp"
#include "tc_if.hpp"
#include "tc_if_else.hpp"
#include "tc_inc_dec_vdr.hpp"
#include "tc_invalid_inc_vdr.hpp"
#include "tc_jmp.hpp"
#include "tc_mov_vdr_imm.hpp"
#include "tc_mov_vdr_vdr.hpp"
#include "tc_mul_vdr_imm.hpp"
#include "tc_mul_vdr_vdr.hpp"
#include "tc_nested_ifs.hpp"
#include "tc_nested_ifs2.hpp"
#include "tc_nested_ifs3.hpp"
#include "tc_nested_ifs4.hpp"
#include "tc_nested_ifs5.hpp"
#include "tc_or_vdr_imm.hpp"
#include "tc_or_vdr_vdr.hpp"
#include "tc_patch_bytes.hpp"
#include "tc_println.hpp"
#include "tc_read_mem.hpp"
#include "tc_show_images_info.hpp"
#include "tc_show_threads_info.hpp"
#include "tc_sleep_ms.hpp"
#include "tc_sleep_ms_vdr.hpp"
#include "tc_sub_vdr_imm.hpp"
#include "tc_sub_vdr_vdr.hpp"
#include "tc_write_mem.hpp"
#include "tc_xor_vdr_imm.hpp"
#include "tc_xor_vdr_vdr.hpp"


bool tester::run_tests()
{
	std::vector< std::unique_ptr< test_case > > tests = {};

	tests.push_back( std::make_unique< tc_defines >( "defines.ta" ) );
	tests.push_back( std::make_unique< tc_macro_with_params >( "macro-with-params.ta" ) );
	tests.push_back( std::make_unique< tc_math >( "math.ta" ) );
	tests.push_back( std::make_unique< tc_simple_import >( "simple-import.ta" ) );
	tests.push_back( std::make_unique< tc_simple_macro >( "simple-macro.ta" ) );
	tests.push_back( std::make_unique< tc_single_line_comments >( "single-line-comments.ta" ) );
	tests.push_back( std::make_unique< tc_use_float_alias >( "use-float-alias.ta" ) );
	tests.push_back( std::make_unique< tc_add_vdr_imm >( "add-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_add_vdr_vdr >( "add-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_and_vdr_imm >( "and-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_and_vdr_vdr >( "and-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_cmp >( "cmp.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps >( "cmp-with-jmps.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps2 >( "cmp-with-jmps2.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps3 >( "cmp-with-jmps3.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps4 >( "cmp-with-jmps4.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps5 >( "cmp-with-jmps5.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps6 >( "cmp-with-jmps6.ta" ) );
	tests.push_back( std::make_unique< tc_cmp_with_jmps7 >( "cmp-with-jmps7.ta" ) );
	tests.push_back( std::make_unique< tc_div_vdr_imm >( "div-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_div_vdr_vdr >( "div-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_find_signature >( "find-signature.ta" ) );
	tests.push_back( std::make_unique< tc_forward_backward_jmp >( "forward-backward-jmp.ta" ) );
	tests.push_back( std::make_unique< tc_get_image_base >( "get-image-base.ta" ) );
	tests.push_back( std::make_unique< tc_get_image_size >( "get-image-size.ta" ) );
	tests.push_back( std::make_unique< tc_if >( "if.ta" ) );
	tests.push_back( std::make_unique< tc_if_else >( "if-else.ta" ) );
	tests.push_back( std::make_unique< tc_inc_dec_vdr >( "inc-dec-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_invalid_inc_vdr >( "invalid-inc-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_jmp >( "jmp.ta" ) );
	tests.push_back( std::make_unique< tc_mov_vdr_imm >( "mov-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_mov_vdr_vdr >( "mov-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_mul_vdr_imm >( "mul-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_mul_vdr_vdr >( "mul-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_nested_ifs >( "nested-ifs.ta" ) );
	tests.push_back( std::make_unique< tc_nested_ifs2 >( "nested-ifs2.ta" ) );
	tests.push_back( std::make_unique< tc_nested_ifs3 >( "nested-ifs3.ta" ) );
	tests.push_back( std::make_unique< tc_nested_ifs4 >( "nested-ifs4.ta" ) );
	tests.push_back( std::make_unique< tc_nested_ifs5 >( "nested-ifs5.ta" ) );
	tests.push_back( std::make_unique< tc_or_vdr_imm >( "or-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_or_vdr_vdr >( "or-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_patch_bytes >( "patch-bytes.ta" ) );
	tests.push_back( std::make_unique< tc_println >( "println.ta" ) );
	tests.push_back( std::make_unique< tc_read_mem >( "read_mem.ta" ) );
	tests.push_back( std::make_unique< tc_show_images_info >( "show_images_info.ta" ) );
	tests.push_back( std::make_unique< tc_show_threads_info >( "show_threads_info.ta" ) );
	tests.push_back( std::make_unique< tc_sleep_ms >( "sleep_ms.ta" ) );
	tests.push_back( std::make_unique< tc_sleep_ms_vdr >( "sleep_ms-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_sub_vdr_imm >( "sub-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_sub_vdr_vdr >( "sub-vdr-vdr.ta" ) );
	tests.push_back( std::make_unique< tc_write_mem >( "write-mem.ta" ) );
	tests.push_back( std::make_unique< tc_xor_vdr_imm >( "xor-vdr-imm.ta" ) );
	tests.push_back( std::make_unique< tc_xor_vdr_vdr >( "xor-vdr-vdr.ta" ) );

	uint32_t tc_success = 0;
	uint32_t tc_failed  = 0;

	for( const auto& test : tests )
		if( test->test(); test->get_status() == TEST_SUCCESS )
			tc_success += 1;
		else
			tc_failed += 1;

	printf( "\n" );

	Log::test( "%ld/%ld tests were successful.\n", tc_success, tc_success + tc_failed );
	Log::test( "%ld/%ld tests failed.\n", tc_failed, tc_failed + tc_success );

	return true;
}
