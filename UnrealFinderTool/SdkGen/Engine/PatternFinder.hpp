#pragma once

#include <cstdint>
#include <windows.h>

/// <summary>
/// Searches for the first pattern in the module.
/// </summary>
/// <param name="module">The module to scan.</param>
/// <param name="pattern">The pattern (Example: "\x12\xAB\x34")</param>
/// <param name="mask">The mask (Example: "x?x")</param>
/// <returns>The address of the found pattern or -1 if the pattern was not found.</returns>
uintptr_t FindPattern(HMODULE module, const unsigned char* pattern, const char* mask);

/// <summary>
/// Searches for the first pattern in the memory region.
/// </summary>
/// <param name="start">The start address of the memory region to scan.</param>
/// <param name="length">The length of the memory region.</param>
/// <param name="pattern">The pattern (Example: "\x12\xAB\x34")</param>
/// <param name="mask">The mask (Example: "x?x")</param>
/// <returns>The address of the found pattern or -1 if the pattern was not found.</returns>
uintptr_t FindPattern(uintptr_t start, size_t length, const unsigned char* pattern, const char* mask);
