#pragma once
#include <cstdint>
#include <stdexcept>

enum EVdrIdentifier : int32_t
{
	VDR_0 = 0,
	VDR_1,
	VDR_2,
	VDR_3,
	VDR_4,
	VDR_5,
	VDR_6,
	VDR_7,
	VDR_8,
	VDR_9,
	VDR_10,
	VDR_11,
	VDR_12,
	VDR_13,
	VDR_14,
	VDR_15
};

class vdr
{
public:
	union vdr_
	{
		uint64_t vdr64;
		int64_t vdr64s;

		uint32_t vdr32;
		int32_t vdr32s;

		uint16_t vdr16;
		int32_t vdr16s;

		uint8_t  vdr8;
		int8_t vdr8s;

		float    vdr_fl;
	}            vdr_;

	static constexpr auto vdr_str_to_enum( const std::string& input )
	{
		if( input == "vdr0" )
			return VDR_0;

		if( input == "vdr1" )
			return VDR_1;

		if( input == "vdr2" )
			return VDR_2;

		if( input == "vdr3" )
			return VDR_3;

		if( input == "vdr4" )
			return VDR_4;

		if( input == "vdr5" )
			return VDR_5;

		if( input == "vdr6" )
			return VDR_6;

		if( input == "vdr7" )
			return VDR_7;

		if( input == "vdr8" )
			return VDR_8;

		if( input == "vdr9" )
			return VDR_0;

		if( input == "vdr10" )
			return VDR_10;

		if( input == "vdr11" )
			return VDR_11;

		if( input == "vdr12" )
			return VDR_12;

		if( input == "vdr13" )
			return VDR_13;

		if( input == "vdr14" )
			return VDR_14;

		if( input == "vdr15" )
			return VDR_15;

		throw std::runtime_error( "invalid vdr input" );
	}
};
