#pragma once
#include <array>
#include <Windows.h>

#include "vdr.hpp"
#include "vfr_flags.hpp"

class environment
{
public:
	environment()
	{
		this->m_name = {};

		this->m_VDR = {};

		this->m_iPC = {};

		this->m_target_process = INVALID_HANDLE_VALUE;

		this->m_target_pid = 0;

		this->m_target_hwnd = INVALID_HANDLE_VALUE;

		this->m_initialized = false;

		this->m_printing = true;

		this->m_VFR = {};
	}

	explicit environment( const std::string& nm )
		: environment()
	{
		this->m_name = nm;
	}

	~environment()
	{
		free_winapi_ressources();
	}

	inline void free_winapi_ressources()
	{
		if( this->m_target_process != INVALID_HANDLE_VALUE )
			CloseHandle( this->m_target_process );

		if( this->m_target_hwnd != INVALID_HANDLE_VALUE )
			CloseHandle( this->m_target_hwnd );

		this->m_target_pid = 0;
	}

	template< typename T >
	void write_to_vdr( EVdrIdentifier vdr_identifier, T value )
	{
		// well this is the point where I tell the compiler to have a beer with me and sit down.
		*reinterpret_cast< T* >( &this->m_VDR.at( vdr_identifier ) ) = value;
	}

	[[nodiscard]] auto read_from_vdr( EVdrIdentifier vdr_identifier ) noexcept
	{
		return reinterpret_cast< vdr* >( &this->m_VDR.at( vdr_identifier ) );
	}

	void print_vdr()
	{
		printf( "\nVDR | byte | word | dword | qword | float | hex\n" );

		for( size_t idx = 0; idx < this->m_VDR.size(); idx++ )
			printf( "%2lld | %08hhu | %016hu | %016d | %032lld | 0x%016llX | %.5f\n", idx,
			        static_cast< uint8_t >( this->m_VDR.at( idx ) ), static_cast< uint16_t >( this->m_VDR.at( idx ) ),
			        static_cast< uint32_t >( this->m_VDR.at( idx ) ), this->m_VDR.at( idx ), this->m_VDR.at( idx ),
			        *reinterpret_cast< float* >( &this->m_VDR.at( idx ) )
			);

		printf( "\n" );
	}

	void print_vfr() const
	{
		printf( "VFR: 0x%d | EQUAL: %s | LESS: %s | GREATER: %s\n", this->m_VFR, is_equal_flag_set() ? "yes" : "no",
		        is_less_flag_set() ? "yes" : "no", is_greater_flag_set() ? "yes" : "no"
		);
	}

	[[nodiscard]] inline auto get_pc() const noexcept
	{
		return this->m_iPC;
	}

	inline void increase_pc() noexcept
	{
		this->m_iPC++;
	}

	inline void decrease_pc() noexcept
	{
		this->m_iPC--;
	}

	inline void reset_pc() noexcept
	{
		this->m_iPC = 0;
	}

	inline void set_pc( const std::uintptr_t pc )
	{
		this->m_iPC = pc;
	}

	[[nodiscard]] inline auto get_name() const noexcept
	{
		return this->m_name;
	}

	inline void set_name( const std::string& nm )
	{
		this->m_name = nm;
	}

	inline void set_target_process_handle( const HANDLE proc )
	{
		this->m_target_process = proc;
	}

	[[nodiscard]] auto get_handle_to_target_process() const noexcept
	{
		return this->m_target_process;
	}

	inline void set_target_process_pid( const DWORD pid )
	{
		this->m_target_pid = pid;
	}

	[[nodiscard]] auto get_pid_of_target_process() const noexcept
	{
		return this->m_target_pid;
	}

	inline void set_target_window_handle( const HANDLE hwnd )
	{
		this->m_target_hwnd = hwnd;
	}

	[[nodiscard]] auto get_handle_to_target_window() const noexcept
	{
		return this->m_target_hwnd;
	}

	[[nodiscard]] inline auto is_initialized() const noexcept
	{
		return this->m_initialized;
	}

	inline void mark_as_initialized() noexcept
	{
		this->m_initialized = true;
	}

	inline void unmark_as_initialized() noexcept
	{
		this->m_initialized = false;

		this->free_winapi_ressources();
	}

	[[nodiscard]] inline auto get_vfr() const noexcept
	{
		return this->m_VFR;
	}

	[[nodiscard]] inline bool is_equal_flag_set() const noexcept
	{
		return this->m_VFR & vfr_flags::VFR_EQUAL_FLAG;
	}

	[[nodiscard]] inline bool is_less_flag_set() const noexcept
	{
		return this->m_VFR & vfr_flags::VFR_LESS_FLAG;
	}

	[[nodiscard]] inline bool is_greater_flag_set() const noexcept
	{
		return this->m_VFR & vfr_flags::VFR_GREATER_FLAG;
	}

	inline void set_flag( int32_t flag )
	{
		this->m_VFR |= flag;
	}

	inline void unset_flag( int32_t flag )
	{
		this->m_VFR &= ~flag;
	}

	inline void clear_flags() noexcept
	{
		this->m_VFR = {};
	}

	inline void set_equal_flag() noexcept
	{
		this->m_VFR |= vfr_flags::VFR_EQUAL_FLAG;
	}

	inline void set_less_flag() noexcept
	{
		this->m_VFR |= vfr_flags::VFR_LESS_FLAG;
	}

	inline void set_greater_flag() noexcept
	{
		this->m_VFR |= vfr_flags::VFR_GREATER_FLAG;
	}

	[[nodiscard]] inline auto is_printing() const noexcept
	{
		return this->m_printing;
	}

	inline void enable_printing() noexcept
	{
		this->m_printing = true;
	}

	inline void disable_printing() noexcept
	{
		this->m_printing = false;
	}

	inline void toggle_printing() noexcept
	{
		this->m_printing = !this->m_printing;
	}

private:
	std::string                      m_name;
	std::array< std::uintptr_t, 16 > m_VDR;
	std::uintptr_t                   m_iPC;
	int32_t                          m_VFR;

	HANDLE m_target_process;
	DWORD  m_target_pid;
	HANDLE m_target_hwnd;

	bool m_initialized;
	bool m_printing;
};
