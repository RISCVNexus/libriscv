#pragma once
#include <cstddef>

#define DEFINE_DYNCALL(number, name, type) \
	asm(".pushsection .text\n" \
		".global sys_" #name "\n" \
		"sys_" #name ":\n" \
		"	li t0, " #number "\n" \
		"   li a7, 510\n" \
		"   ecall\n" \
		"   ret\n"   \
		".popsection .text\n" \
		".pushsection .data\n" \
		"sys_" #name "_end:\n"     \
		".string \"" #name "\"\n" \
		".byte 0\n"               \
		".popsection .data\n"); \
	using name##_t = type; \
	extern "C" __attribute__((used, retain)) void sys_##name(); \
	template <typename... Args> \
	static inline auto name(Args... args) { \
		auto fn = (name##_t*) sys_##name; \
		return fn(args...); \
	}

#define EXTERN_DYNCALL(name, type) \
	using name##_t = type; \
	extern "C" __attribute__((used, retain)) void sys_##name(); \
	template <typename... Args> \
	static inline auto name(Args... args) { \
		auto fn = (name##_t*) sys_##name; \
		return fn(args...); \
	}