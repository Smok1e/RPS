#pragma once

#include <vector>
#include <string_view>
#include <span>

//========================================

class Packet
{
public:
	enum class OffsetType
	{
		Reading,
		Writing
	};

	Packet() = default;

	void write(const uint8_t* buffer, size_t size);

	template<typename T> 
	void writeGeneric(const T& value);

	void writeUint8(uint8_t value);
	void writeUint16(uint16_t value);
	void writeUint32(uint32_t value);
	void writeString(std::string_view string);
	void writeBoolean(bool value);

	void read(uint8_t* buffer, size_t size);

	template<typename T>
	T readGeneric();

	uint8_t readUint8();
	uint16_t readUint16();
	uint32_t readUint32();
	std::string_view readString();
	bool readBoolean();

	uint8_t* data();
	const uint8_t* data() const;

	operator std::span<const uint8_t>() const;

	size_t size() const;
	int remain() const;
	void clear();

	void seek(
		int position, 
		bool absolute = false, 
		OffsetType offset_type = OffsetType::Reading
	);

	int offset(OffsetType offset_type = OffsetType::Reading) const;
	void discard();

private:
	std::vector<uint8_t> m_buffer;
	int m_reading_offset = 0;
	int m_writing_offset = 0;

};

//========================================

template<typename T>
void Packet::writeGeneric(const T& value)
{
	write(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template<typename T>
T Packet::readGeneric()
{
	T value;
	read(reinterpret_cast<uint8_t*>(&value), sizeof(T));

	return value;
}

//========================================