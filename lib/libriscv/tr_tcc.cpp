#include "common.hpp"
#ifdef LIBTCC_PACKAGE
#include <libtcc.h>
#else
#include <tcc/libtcc.h>
#endif

namespace riscv
{
	void* libtcc_compile(const std::string& code,
		int, const std::unordered_map<std::string, std::string>& cflags, const std::string& libtcc1)
	{
		TCCState* state = tcc_new();
		tcc_set_output_type(state, TCC_OUTPUT_MEMORY);

		for (const auto& pair : cflags) {
			tcc_define_symbol(state, pair.first.c_str(), pair.second.c_str());
		}

		tcc_define_symbol(state, "ARCH", "HOST_UNKNOWN");
		tcc_set_options(state, "-std=c99 -O2");

		if (!libtcc1.empty())
			tcc_add_library_path(state, libtcc1.c_str());
#ifndef LIBTCC_PACKAGE
		else
			tcc_add_library_path(state, ".");
#endif

		if (tcc_compile_string(state, code.c_str()) < 0) {
			tcc_delete(state);
			return nullptr;
		}

		if (tcc_relocate(state, TCC_RELOCATE_AUTO) < 0) {
			tcc_delete(state);
			return nullptr;
		}

		return state;
	}

	void* dylib_lookup(void* state, const char* symbol)
	{
		return tcc_get_symbol((TCCState *)state, symbol);
	}

	void dylib_close(void* state)
	{
		tcc_delete((TCCState *)state);
	}
}
