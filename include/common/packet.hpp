#pragma once

#include <vector>
#include <string_view>
#include <span>
#include <cstdint>

//========================================

/// \brief Network packet
///
/// Convenient buffered network packet. Provides read/write functions for 
/// most basic types. Holds reading/writing offsets to read or insert
/// values at the desired location.
/// 
/// \note All the reading/writing methods assumes little-endian number representation.
class Packet
{
public:
	/// Offset type
	enum class OffsetType
	{
		Reading, //!< Reading offset
		Writing  //!< Writing offset
	};

	/// Default constructor
	Packet() = default;

	/// \brief Inserts raw data at the writing offset.
	///
	/// \param buffer Data buffer pointer
	/// \param size Data buffer size
	/// 
	/// After the data is written to the buffer, writing offset
	/// will be increased by the amount of bytes written.
	/// 
	/// \see writeGeneric, writeUint8, writeUint16, writeString, writeBoolean, read
	void write(const uint8_t* buffer, size_t size);

	/// \brief Inserts arbitrary value at the writing offset
	///
	/// \param value The value
	/// \tparam T Value type
	/// 
	/// Coies given value, as it is represented in memory,
	/// into the buffer at writing offset.
	template<typename T> 
	void writeGeneric(const T& value);

	void writeUint8(uint8_t value);            //!< Writes uint8_t
	void writeUint16(uint16_t value);		   //!< Writes uint16_t
	void writeUint32(uint32_t value);		   //!< Writes uint32_t
	void writeString(std::string_view string); //!< Writes string
	void writeBoolean(bool value);			   //!< Writes boolean

	/// \brief Reads bytes from the buffer at the reading offset.
	///
	/// \param buffer Buffer pointer to read bytes into
	/// \param size Amount of bytes to read
	/// 
	/// \throws std::range_error in case there is not enough
	/// data in the buffer to read.
	/// 
	/// After the data is read from the buffer, reading offset
	/// will be increased by the amount of bytes read.
	/// 
	/// \see readGeneric, readUint8, readUint16, readString, readBoolean, write
	void read(uint8_t* buffer, size_t size);

	/// \brief Reads arbitrary value at the reading offset
	///
	/// \tparam T Value type
	template<typename T>
	T readGeneric();

	uint8_t readUint8();   //!< Reads uint8_t
	uint16_t readUint16(); //!< Reads uint16_t
	uint32_t readUint32(); //!< Reads uint32_t

	/// \brief Reads string
	///
	/// \note The returned std::string_view instance holds a pointer directly
	/// to the packet buffer, which lifetime is determined by the packet. Be
	/// careful not to use it after packet content has been erased.
	std::string_view readString();

	bool readBoolean(); //!< Reads bool

	      uint8_t* data();       //!< Returns data pointer
	const uint8_t* data() const; //!< Returns const data pointer

	operator std::span<const uint8_t>() const; //!< std::span conversion operator

	size_t size() const; //!< Returns packet total size in bytes
	int remain() const;  //!< Returns the number of bytes remaining available to read
	void clear();        //!< Clears buffer contents

	/// \brief Moves reading or writing offset
	///
	/// \param position Absolute or relative offset value
	/// \param absolute Specifies whether offset shold be set absolutely or relatively
	/// \param offset_type Offset type to move
	void seek(
		int position, 
		bool absolute = false, 
		OffsetType offset_type = OffsetType::Reading
	);

	/// \brief Returns reading or writing offset
	///
	/// \param offset_type Offset type to return
	int offset(OffsetType offset_type = OffsetType::Reading) const;

	/// \brief Discard all read bytes
	///
	/// Erases all data before the reading offset and resets offset value
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