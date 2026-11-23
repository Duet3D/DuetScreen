/*
 * CRC32.cpp
 *
 * A byte at a time from a table: files sent to a printer are small enough that nothing faster is needed.
 */

#include "CRC32.h"

#include <array>

namespace
{
	/** The remainder of each byte value, for the reflected polynomial */
	constexpr std::array<uint32_t, 256> makeTable() noexcept
	{
		std::array<uint32_t, 256> table{};
		for (uint32_t i = 0; i < table.size(); i++)
		{
			uint32_t value = i;
			for (int bit = 0; bit < 8; bit++)
			{
				value = (value & 1u) != 0 ? (value >> 1) ^ 0xEDB88320u : value >> 1;
			}
			table[i] = value;
		}
		return table;
	}

	constexpr std::array<uint32_t, 256> crc32Table = makeTable(); // makeTable(), worked out when compiled
} // namespace

CRC32::CRC32() noexcept
{
	Reset();
}

void CRC32::Update(char c) noexcept
{
	crc = crc32Table[(crc ^ static_cast<uint8_t>(c)) & 0xFFu] ^ (crc >> 8);
}

void CRC32::Update(const char *s, size_t len) noexcept
{
	for (size_t i = 0; i < len; i++)
	{
		Update(s[i]);
	}
}
