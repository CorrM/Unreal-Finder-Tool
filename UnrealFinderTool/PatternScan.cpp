#include "pch.h"
#include "Memory.h"
#include "PatternScan.h"
#include "ParallelWorker.h"

#include <future>

Pattern PatternScan::Parse(const std::string& name, const int offset, std::string hexStr, const uchar_t wildcard, const std::string& delimiter)
{
	if (!delimiter.empty())
		hexStr = Utils::ReplaceString(hexStr, delimiter, "");

	char c_wildcard[3] = { '\0' }; // 3 for Null terminator
	sprintf_s(c_wildcard, sizeof(c_wildcard), "%x", wildcard);

	std::vector<unsigned char> bytes;
	for (unsigned int i = 0; i < hexStr.length(); i += 2)
	{
		auto byte_string = hexStr.substr(i, 2);

		if (byte_string == "??")
			byte_string = c_wildcard;

		bytes.push_back(static_cast<char>(strtol(byte_string.c_str(), nullptr, 16)));
	}

	// Fill Struct
	Pattern ret;
	ret.Name = name;
	ret.Sig = bytes;
	ret.Offset = offset;
	ret.Len = bytes.size();
	ret.Wildcard = wildcard;

	return ret;
}

Pattern PatternScan::Parse(const std::string& name, const int offset, const std::string& patternStr, const uchar_t wildcard)
{
	return Parse(name, offset, patternStr, wildcard, " ");
}

/**
 * \brief Scan pattern of memory in process memory
 * \param mem Handle to process
 * \param dwStart Starting memory address
 * \param dwEnd How many bytes to read
 * \param patterns Attempt to match this pattern
 * \param firstOnly Get first address only
 * \param useThreads if u want to be fast use threads
 * \return uintptr_t
 */
std::map<std::string, std::vector<uintptr_t>> PatternScan::FindPattern(Memory* mem, uintptr_t dwStart, uintptr_t dwEnd, std::vector<Pattern> patterns,
	const bool firstOnly, const bool useThreads)
{
	std::map<std::string, std::vector<uintptr_t>> ret;
	std::vector<uintptr_t> result;

	// Init map
	for (auto& pattern : patterns)
		ret[pattern.Name] = std::vector<uintptr_t>();

	SYSTEM_INFO si = { 0 };
	GetSystemInfo(&si);

	if (dwStart < reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress) || dwStart == 0)
		dwStart = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

	if (dwEnd > reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress) || dwEnd == 0)
		dwEnd = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

	MEMORY_BASIC_INFORMATION info;
	std::vector<uintptr_t> mem_regions;

	// Cycle through memory based on RegionSize
	for (uintptr_t i = dwStart; (VirtualQueryEx(mem->ProcessHandle, LPVOID(i), &info, sizeof info) == sizeof info && i < dwEnd); i += info.RegionSize)
	{
		// Bad Memory
		if (info.State != MEM_COMMIT) continue;
		if (info.Type != MEM_PRIVATE) continue;
		if (info.Protect != PAGE_READWRITE) continue;

		mem_regions.push_back(i);
	}

	if (useThreads)
	{
		ParallelWorker<uintptr_t> worker(mem_regions, 0, Utils::Settings.SdkGen.Threads, [&](uintptr_t& address, ParallelOptions& options)
		{
			SIZE_T allocCount = (dwEnd - dwStart) > info.RegionSize ? info.RegionSize : dwEnd - dwStart;
			const auto pBuf = static_cast<PBYTE>(malloc(allocCount));

			// Read one page or skip if failed
			const SIZE_T dwOut = mem->ReadBytes(address, pBuf, allocCount);
			if (dwOut == 0)
			{
				free(pBuf);
			}
			else
			{
				// Scan for all pattern in the same memory region
				for (auto& pattern : patterns)
				{
					size_t k = 0;
					const uchar_t* uPattern = pattern.Sig.data();
					const auto nLen = pattern.Len;

					for (size_t j = 0; j <= dwOut; j++)
					{
						// If the byte matches our pattern or wildcard
						if (pBuf[j] == uPattern[k] || uPattern[k] == pattern.Wildcard)
						{
							// Did we find it?
							if (++k == nLen)
							{
								// Our match function places us at the begin of the pattern
								// To locate the pointer we need to subtract nOffset bytes
								std::lock_guard lock(options.Locker);
								ret.find(pattern.Name)->second.push_back(address + j - (nLen - 1) + pattern.Offset);
								if (firstOnly)
								{
									options.ForceStop = true;
									break;
								}
							}
						}
						else
						{
							k = 0;
						}
					}
				}

				free(pBuf);
			}
		});
		worker.Start();
		worker.WaitAll();
	}
	else
	{
		for (uintptr_t i : mem_regions)
		{
			SIZE_T allocCount = (dwEnd - dwStart) > info.RegionSize ? info.RegionSize : dwEnd - dwStart;
			const auto pBuf = static_cast<PBYTE>(malloc(allocCount));

			// Read one page or skip if failed
			const SIZE_T dwOut = mem->ReadBytes(i, pBuf, allocCount);
			if (dwOut == 0)
			{
				free(pBuf);
				continue;
			}

			// Scan for all pattern in the same memory region
			for (auto& pattern : patterns)
			{
				size_t k = 0;
				const uchar_t* uPattern = pattern.Sig.data();
				const auto nLen = pattern.Len;

				for (size_t j = 0; j <= dwOut; j++)
				{
					// If the byte matches our pattern or wildcard
					if (pBuf[j] == uPattern[k] || uPattern[k] == pattern.Wildcard)
					{
						// Did we find it?
						if (++k == nLen)
						{
							// Our match function places us at the begin of the pattern
							// To locate the pointer we need to subtract nOffset bytes
							ret.find(pattern.Name)->second.push_back(i + j - (nLen - 1) + pattern.Offset);

							if (firstOnly)
								break;
						}
					}
					else
					{
						k = 0;
					}
				}
			}

			free(pBuf);
		}
	}
	return ret;
}