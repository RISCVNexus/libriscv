#include "event.hpp"
#include <chrono>
#include <fmt/core.h>
using namespace riscv;
template <unsigned SAMPLES = 2000>
static void benchmark(std::string_view name, Script& script, std::function<void()> fn);

// ScriptCallable is a function that can be requested from the script
using ScriptCallable = std::function<void(Script&)>;
// A map of host functions that can be called from the script
static std::array<ScriptCallable, 64> g_script_functions {};
static void register_script_function(uint32_t number, ScriptCallable&& fn) {
	g_script_functions.at(number) = std::move(fn);
}

void Script::setup_syscall_interface()
{
	// Add a custom system call that executes a function based on a hash
	Script::machine_t::install_syscall_handler(510,
	[] (auto& machine) {
		auto& script = *machine.template get_userdata<Script>();
		g_script_functions.at(machine.cpu.reg(riscv::REG_T0))(script);
	});
}

struct Location {
	int x = 0, y = 0, z = 0;

	bool operator==(const Location& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
};
namespace std {
	template<> struct hash<Location> {
		std::size_t operator()(const Location& loc) const {
			return std::hash<int>()(loc.x) ^ std::hash<int>()(loc.y) ^ std::hash<int>()(loc.z);
		}
	};
}
struct LocationData
{
	std::vector<uint8_t> data;
};
static std::unordered_map<Location, LocationData> locations;

static Script::gaddr_t         remote_addr;
static std::array<uint8_t, 32> remote_capture;
static std::function<void(int)> on_event;

int main(int argc, char** argv)
{
	if (argc < 2) {
		fmt::print("Usage: {} [program file] [arguments ...]\n", argv[0]);
		return -1;
	}

	// Register a custom function that can be called from the script
	// This is the handler for dyncall1
	register_script_function(1, [](Script& script) {
		auto [arg] = script.machine().sysargs<int>();

		fmt::print("dyncall1 called with argument: 0x{:x}\n", arg);

		script.machine().set_result(42);
	});
	// This is the handler for dyncall2
	register_script_function(2, [](Script& script) {
		// string_view consumes 2 arguments, the first is the pointer, the second is the length
		// unlike std::string, which consumes only 1 argument (zero-terminated string pointer)
		auto [view, str] = script.machine().sysargs<std::string_view, std::string>();

		fmt::print("dyncall2 called with arguments: '{}' and '{}'\n", view, str);
	});
	// This is the handler for dyncall_empty
	register_script_function(3, [](Script&) {
	});
	// This is the handler for dyncall_data
	register_script_function(4, [](Script& script) {
		struct MyData {
			char buffer[32];
		};
		auto [data_span, data] = script.machine().sysargs<std::span<MyData>, const MyData*>();

		fmt::print("dyncall_data called with args: '{}' and '{}'\n", data_span[0].buffer, data->buffer);
	});

	// This is the callback for sys_location_get
	register_script_function(10, [](Script& script) {
		auto [x, y, z] = script.machine().sysargs<int, int, int>();
		auto it = locations.find(Location(x, y, z));
		if (it != locations.end()) {
			auto alloc = script.guest_alloc(it->second.data.size());
			script.machine().copy_to_guest(alloc, it->second.data.data(), it->second.data.size());
			script.machine().set_result(alloc, it->second.data.size());
		} else {
			script.machine().set_result(0, 0);
		}
	});
	// This is the callback for sys_location_commit
	register_script_function(11, [](Script& script) {
		auto [x, y, z, data] = script.machine().sysargs<int, int, int, std::span<uint8_t>>();
		// This will create a new location or update an existing one
		auto& loc = locations[Location(x, y, z)];
		loc.data = std::vector<uint8_t>(data.begin(), data.end());
	});

	register_script_function(12, [](Script& script) {
		auto [addr, capture] = script.machine().sysargs<Script::gaddr_t, std::array<uint8_t, 32>>();

		remote_addr = addr;
		remote_capture = capture;
	});

	register_script_function(13, [](Script& script) {
		auto [name, func, capture] = script.machine().sysargs<std::string, Script::gaddr_t, std::array<uint8_t, 32>*>();

		on_event =
		[func, &script, capture = *capture] (auto id) {
			// Call the function with the entity ID as an argument
			script.call(func, id, capture);
		};
	});

	// Create a new script instance, loading and initializing the given program file
	// The programs main() function will be called
	Script script("myscript", argv[1]);

	// Create an event for the 'test1' function with 4 arguments and returns an int
	Event<int(int, int, int, int)> test1(script, "test1");
	if (auto ret = test1(1, 2, 3, 4))
		fmt::print("test1 returned: {}\n", *ret);
	else
		throw std::runtime_error("Failed to call test1!?");

	// Benchmark the test2 function, which allocates and frees 1024 bytes
	Event<void()> test2(script, "test2");
	if (auto ret = test2(); !ret)
		throw std::runtime_error("Failed to call test2!?");

	benchmark("std::make_unique[1024] alloc+free", script, [&] {
		test2();
	});

	// Create an event for the 'test3' function with a single string argument
	// This function will throw an exception, which is immediately caught
	Event<void(std::string)> test3(script, "test3");
	if (auto ret = test3("Oh, no! An exception!"); !ret)
		throw std::runtime_error("Failed to call test3!?");

	// Pass data structure to the script
	struct Data {
		int a, b, c, d;
		float e, f, g, h;
		double i, j, k, l;
		char buffer[32];
	};
	const Data data = { 1, 2, 3, 4, 5.0f, 6.0f, 7.0f, 8.0f, 9.0, 10.0, 11.0, 12.0, "Hello, World!" };
	Event<void(Data)> test4(script, "test4");
	if (auto ret = test4(data); !ret)
		throw std::runtime_error("Failed to call test4!?");

	// Benchmark the overhead of dynamic calls
	Event<void()> bench_dyncall_overhead(script, "bench_dyncall_overhead");
	benchmark("Overhead of dynamic calls", script, [&] {
		bench_dyncall_overhead();
	});

	// Call test5 function
	Event<void()> test5(script, "test5");
	if (auto ret = test5(); !ret)
		throw std::runtime_error("Failed to call test5!?");

	auto script2 = script.clone("myscript2");

	// Call the remote function, with the capture pushed to stack
	script2.call(remote_addr, remote_capture);
}

// A simple benchmarking function that subtracts the call overhead
template <unsigned SAMPLES>
void benchmark(std::string_view name, Script &script, std::function<void()> fn)
{
	static unsigned overhead = 0;
	if (overhead == 0)
	{
		Event<void()> measure_overhead(script, "measure_overhead");
		auto start = std::chrono::high_resolution_clock::now();
		for (unsigned i = 0; i < SAMPLES; i++)
			measure_overhead();
		auto end = std::chrono::high_resolution_clock::now();
		overhead = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / SAMPLES;
		fmt::print("Call overhead: {}ns\n", overhead);
	}

	auto start = std::chrono::high_resolution_clock::now();
	for (unsigned i = 0; i < SAMPLES; i++)
		fn();
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / SAMPLES;
	fmt::print("Benchmark: {}  Elapsed time: {}ns\n",
			   name, elapsed - overhead);
}
