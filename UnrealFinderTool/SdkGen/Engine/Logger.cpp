#include "pch.h"
#include "Logger.h"

std::ostream* Logger::stream = nullptr;

void Logger::SetStream(std::ostream* _stream)
{
	stream = _stream;
}

void Logger::Log(const std::string& message)
{
	if (stream != nullptr)
	{
		(*stream) << message << '\n' << std::flush;
	}
}
