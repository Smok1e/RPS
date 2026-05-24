#include <stdexcept>

#include <packet.hpp>

//========================================

void Packet::write(const uint8_t* buffer, size_t size)
{
	m_buffer.insert(
		m_buffer.begin() + m_writing_offset, 
		buffer, 
		buffer + size
	);

	m_writing_offset += size;
}

void Packet::writeUint8(uint8_t value)
{
	writeGeneric(value);
}

void Packet::writeUint16(uint16_t value)
{
	writeGeneric(value);
}

void Packet::writeUint32(uint32_t value)
{
	writeGeneric(value);
}

void Packet::writeString(std::string_view string)
{
	writeUint32(string.length());
	write(
		reinterpret_cast<const uint8_t*>(string.data()), 
		string.length()
	);
}

void Packet::writeBoolean(bool value)
{
	writeUint8(value);
}

//========================================

void Packet::read(uint8_t* buffer, size_t size)
{
	if (remain() < size)
		throw std::range_error("not enough data to read");

	std::copy(
		m_buffer.begin() + m_reading_offset,
		m_buffer.begin() + m_reading_offset + size,
		buffer
	);

	m_reading_offset += size;
}

uint8_t Packet::readUint8()
{
	return readGeneric<uint8_t>();
}

uint16_t Packet::readUint16()
{
	return readGeneric<uint16_t>();
}

uint32_t Packet::readUint32()
{
	return readGeneric<uint32_t>();
}

std::string_view Packet::readString()
{
	auto length = readUint32();
	std::string_view string(
		reinterpret_cast<const char*>(m_buffer.data()) + m_reading_offset,
		length
	);

	m_reading_offset += length;
	return string;
}

bool Packet::readBoolean()
{
	return readUint8();
}

//========================================

const uint8_t* Packet::data() const
{
	return m_buffer.data();
}

size_t Packet::size() const
{
	return m_buffer.size();
}

int Packet::remain() const
{
	return size() - m_reading_offset;
}

void Packet::clear()
{
	m_buffer.clear();
	m_reading_offset = 0;
	m_writing_offset = 0;
}

void Packet::seek(
	int position,
	bool absolute /*= false*/,
	OffsetType offset_type /*= OffsetType::Reading*/
)
{
	auto& offset = offset_type == OffsetType::Reading
		? m_reading_offset
		: m_writing_offset;

	if (absolute)
		offset = position;

	else
		offset += position;
}

int Packet::offset(OffsetType offset_type /*= OffsetType::Reading*/) const
{
	return offset_type == OffsetType::Reading
		? m_reading_offset
		: m_writing_offset;
}

void Packet::discard()
{
	m_buffer.erase(
		m_buffer.begin(),
		m_buffer.begin() + m_reading_offset
	);

	m_reading_offset = 0;
	m_writing_offset = 0;
}

//========================================