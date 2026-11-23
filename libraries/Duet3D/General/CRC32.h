/*
 * CRC32.h
 *
 * CRC-32 as RepRapFirmware calculates it to check a file it was sent (M559 C): the IEEE 802.3 polynomial
 * (0xEDB88320, reflected), starting from 0xFFFFFFFF and inverted at the end, the same as zlib's crc32().
 */

#ifndef SRC_GENERAL_CRC32_H_
#define SRC_GENERAL_CRC32_H_

#include <cstddef>
#include <cstdint>

class CRC32
{
private:
	uint32_t crc; // The CRC so far, before the final inversion

public:
	/** A CRC of nothing yet */
	CRC32() noexcept;

	/** Add `c` */
	void Update(char c) noexcept;

	/** Add the `len` characters at `s` */
	void Update(const char *s, size_t len) noexcept;

	/** Start again from `initialValue` */
	void Reset(uint32_t initialValue = 0xFFFFFFFFu) noexcept;

	/** The CRC of everything added */
	uint32_t Get() const noexcept;
};

inline uint32_t CRC32::Get() const noexcept
{
	return ~crc;
}

inline void CRC32::Reset(uint32_t initialValue) noexcept
{
	crc = initialValue;
}

#endif /* SRC_GENERAL_CRC32_H_ */
