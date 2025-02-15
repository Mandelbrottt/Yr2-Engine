#pragma once

#pragma region Environment Macros
#	if defined(OYL_DISTRIBUTION) && defined(OYL_EDITOR)
#		error Oyl Editor doesn't support Distribution configuration!
#	endif

#	ifdef _WIN32
#		define OYL_WINDOWS
#		ifdef _WIN64
#			define OYL_WIN64
#		else
#			define OYL_WIN32
#		endif
#		define WIN32_LEAN_AND_MEAN
#		define NOMINMAX
#	else
#		warning "Oyl3D only supports windows!"
#	endif 
#pragma endregion

#pragma region Common Macros
#	define OYL_DEPRECATED(_message_) [[deprecated(_message_)]]
#	define OYL_UNUSED(_var_) static_cast<void>(_var_)
#	define OYL_FORCE_SEMICOLON static_assert(true)
#	define OYL_FORCE_FORMAT_INDENT static_assert(true);
#pragma endregion

#pragma region Internal Macros
#	define _OYL_EXPAND(_x_) _x_
#	define _OYL_STRINGIFY(_x_) #_x_
#	define _OYL_STRINGIFY_MACRO(_x_) _OYL_STRINGIFY(_x_)

#	define _OYL_REQUIRE_SEMICOLON static_assert(true)

#pragma endregion

#pragma region Debug Macros
#	if !defined(OYL_DISTRIBUTION)
#		define OYL_ENABLE_ASSERTS
#	if defined(_MSC_VER)
            #define OYL_BREAKPOINT ::__debugbreak()
#		else
#			warning "Breakpoints only implemented for MSVC"
#			define OYL_BREAKPOINT
#		endif
#	else
#		define OYL_BREAKPOINT
#	endif

#	if defined(OYL_ENABLE_ASSERTS)
#		define OYL_ASSERT(...) OYL_MACRO_OVERLOAD(_OYL_ASSERT, __VA_ARGS__)

#		define _OYL_ASSERT_1(_expr_)             { if(!(_expr_)) { OYL_BREAKPOINT; } }
#		define _OYL_ASSERT_2(_expr_, _str_)      { if(!(_expr_)) { OYL_LOG_ERROR("Assert Failed [" __FILE__ ":" _OYL_STRINGIFY_MACRO(__LINE__) "] (" #_expr_ "): " _str_); OYL_BREAKPOINT; } }
#		define _OYL_ASSERT_3(_expr_, _str_, ...) { if(!(_expr_)) { OYL_LOG_ERROR("Assert Failed [" __FILE__ ":" _OYL_STRINGIFY_MACRO(__LINE__) "] (" #_expr_ "): " _str_, ##__VA_ARGS__); OYL_BREAKPOINT; } }
#		define _OYL_ASSERT_4(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#		define _OYL_ASSERT_5(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#		define _OYL_ASSERT_6(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#		define _OYL_ASSERT_7(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#		define _OYL_ASSERT_8(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#		define _OYL_ASSERT_9(_expr_, _str_, ...) _OYL_EXPAND(_OYL_ASSERT_3(_expr_, _str_, __VA_ARGS__))
#	else
#		define OYL_ASSERT(_expr_)
#	endif
#pragma endregion

#pragma region Macro Argument Overloading
	/**
     * Overload a macro, and allow for one macro definition to map to multiple different macros, depending on
     * argument count
     *
     * \param _name_ The name of the defined overloaded macros minus the underscore and number at the end
     *
     * <code>
     * #define OVERLOADED_MACRO(...) OYL_MACRO_OVERLOAD(_OVERLOADED_MACRO, __VA_ARGS__)
     *
     * #define _OVERLOADED_MACRO_1() // 1 arguments
     * #define _OVERLOADED_MACRO_2() // 2 arguments
     * #define _OVERLOADED_MACRO_3() // 3 arguments
     *
     * and so on...
     *
     * // You can omit any number of macro overload definitions if you only want to support specific amounts
     * </code>
     */
#	define OYL_MACRO_OVERLOAD(_name_, ...) _OYL_EXPAND(_OYL_MACRO_APPEND_ARG_COUNT(_name_, __VA_ARGS__))

#	define _OYL_CAT(_a_, _b_) _a_##_b_
#	define _OYL_CAT_EXPAND(_a_, _b_) _OYL_CAT(_a_, _b_)
#	define _OYL_CAT_WITH_UNDERSCORE(_name_, _num_) _OYL_CAT(_name_##_, _num_)

#	define _OYL_GET_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _count_, ...) _count_

#	define _OYL_EXPAND_ARGS_COUNT(...)              _OYL_EXPAND(_OYL_GET_ARG_COUNT(__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#	define _OYL_MACRO_APPEND_ARG_COUNT(_name_, ...) _OYL_EXPAND(_OYL_CAT_WITH_UNDERSCORE(_name_, _OYL_EXPAND_ARGS_COUNT(unused, __VA_ARGS__))(__VA_ARGS__))
#pragma endregion

#pragma region Shared Library
#	if defined(OYL_EDITOR)
#		define OYL_BUILD_SHARED_LIB
#	else
#		define OYL_BUILD_STATIC_LIB
#	endif

#	if defined(OYL_BUILD_STATIC_LIB)
#		define OYL_CORE_API
#	elif defined(OYL_BUILD_SHARED_LIB)
#		if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#			define _OYL_CORE_EXPORT __declspec(dllexport)
#			define _OYL_CORE_IMPORT __declspec(dllimport)
#		else
#			define _OYL_CORE_EXPORT __attribute__((visibility("default")))
#			define _OYL_CORE_IMPORT __attribute__((visibility("default")))
#		endif

#		ifdef _INSIDE_OYL_CORE
#			define OYL_CORE_API _OYL_EXPAND(_OYL_CORE_EXPORT)
#		else
#			define OYL_CORE_API _OYL_EXPAND(_OYL_CORE_IMPORT)
#		endif
#	endif
#pragma endregion