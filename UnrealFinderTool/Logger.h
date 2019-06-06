#pragma once

#include <ostream>
#include <string>
#include <chrono>

#include "tinyformat.h"

class Logger
{
public:

	/// <summary>
	/// Sets the stream where the output goes to.
	/// </summary>
	/// <param name="stream">[in] The stream.</param>
	static void SetStream(std::ostream* stream);

	/// <summary>
	/// Logs the given message.
	/// </summary>
	/// <param name="message">The message.</param>
	static void Log(const std::string& message);

	/// <summary>
	/// Formats and logs the given message.
	/// </summary>
	/// <typeparam name="Args">Type of the arguments.</typeparam>
	/// <param name="fmt">Describes the format to use.</param>
	/// <param name="args">Variable arguments providing the arguments.</param>
	template<typename... Args>
	static void Log(const char* fmt, const Args&... args)
	{
		Log(tfm::format(fmt, args...));
	}

private:
	static std::ostream *stream;
};
