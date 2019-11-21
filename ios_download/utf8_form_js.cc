#include "utf8_form_js.h"

//reference:https://mothereff.in/utf-8

UTF8FormJS::UTF8FormJS(void)
	:byteArray(""),
	byteCount(0),
	byteIndex(0){
}
UTF8FormJS::~UTF8FormJS(void){
}
std::string UTF8FormJS::Encode(const std::vector<uint8_t>& str){
	std::string src;
	src.append((char*)&str[0],str.size());
	std::string codePoints = ucs2decode(src);
	uint32_t length = codePoints.length();
	uint32_t index = -1;
	uint32_t codePoint = 0;
	std::string byteString = "";
	while (++index < length) {
		codePoint = codePoints[index];
		byteString += encodeCodePoint(codePoint);
	}
	return byteString;
}
std::string UTF8FormJS::Decode(const std::vector<uint8_t>& byteString){
	std::string src;
	src.append((char*)&byteString[0],byteString.size());
	byteArray = ucs2decode(src);
	byteCount = byteArray.length();
	byteIndex = 0;
	std::string codePoints = "";
	uint32_t tmp = 0;
	while ((tmp = decodeSymbol())!=0) {
		codePoints.append(1,tmp);
	}
	return ucs2encode(codePoints);
}
std::string UTF8FormJS::ucs2decode(std::string str) {
	std::string output = "";
	uint32_t counter = 0;
	uint32_t length = str.length();
	uint32_t value;
	uint32_t extra;
	while (counter < length) {
		value = str[counter++];
		if (value >= 0xD800 && value <= 0xDBFF && counter < length) {
			// high surrogate, and there is a next character
			extra = str[counter++];
			if ((extra & 0xFC00) == 0xDC00) { // low surrogate
				output.append(sizeof(char),((value & 0x3FF) << 10) + (extra & 0x3FF) + 0x10000);
			} else {
				// unmatched surrogate; only append this code unit, in case the next
				// code unit is the high surrogate of a surrogate pair
				output.append(sizeof(char),value);
				counter--;
			}
		} else {
			output.append(sizeof(char),value);
		}
	}
	return output;
}

// Taken from https://mths.be/punycode
std::string UTF8FormJS::ucs2encode(std::string arr) {
	uint32_t length = arr.length();
	uint32_t index = -1;
	uint32_t value;
	std::string output = "";
	while (++index < length) {
		value = arr[index];
		if (value > 0xFFFF) {
			value -= 0x10000;
			output.append(sizeof(char),value >> 10 & 0x3FF | 0xD800);
			value = 0xDC00 | value & 0x3FF;
		}
		output.append(sizeof(char),value);
	}
	return output;
}

/*--------------------------------------------------------------------------*/

uint32_t UTF8FormJS::createByte(uint32_t codePoint,uint32_t shift) {
	return (((codePoint >> shift) & 0x3F) | 0x80);
}

uint32_t UTF8FormJS::encodeCodePoint(uint32_t codePoint) {
	if ((codePoint & 0xFFFFFF80) == 0) { // 1-byte sequence
		return codePoint;
	}
	uint32_t symbol = 0;
	if ((codePoint & 0xFFFFF800) == 0) { // 2-byte sequence
		symbol = ((codePoint >> 6) & 0x1F) | 0xC0;
	}
	else if ((codePoint & 0xFFFF0000) == 0) { // 3-byte sequence
		symbol = ((codePoint >> 12) & 0x0F) | 0xE0;
		symbol += createByte(codePoint, 6);
	}
	else if ((codePoint & 0xFFE00000) == 0) { // 4-byte sequence
		symbol = ((codePoint >> 18) & 0x07) | 0xF0;
		symbol += createByte(codePoint, 12);
		symbol += createByte(codePoint, 6);
	}
	symbol += ((codePoint & 0x3F) | 0x80);
	return symbol;
}

std::string UTF8FormJS::utf8encode(std::string str) {
	std::string codePoints = ucs2decode(str);
	uint32_t length = codePoints.length();
	uint32_t index = -1;
	uint32_t codePoint;
	std::string byteString = "";
	while (++index < length) {
		codePoint = codePoints[index];
		byteString += encodeCodePoint(codePoint);
	}
	return byteString;
}

/*--------------------------------------------------------------------------*/

uint32_t UTF8FormJS::readContinuationByte() {
	if (byteIndex >= byteCount) {
		throw std::exception("Invalid byte index");
	}

	uint32_t continuationByte = byteArray[byteIndex] & 0xFF;
	byteIndex++;

	if ((continuationByte & 0xC0) == 0x80) {
		return continuationByte & 0x3F;
	}

	// If we end up here, it¡¯s not a continuation byte
	throw std::exception("Invalid continuation byte");
}

uint32_t UTF8FormJS::decodeSymbol() {
	uint32_t byte1;
	uint32_t byte2;
	uint32_t byte3;
	uint32_t byte4;
	uint32_t codePoint;

	if (byteIndex > byteCount) {
		throw std::exception("Invalid byte index");
	}

	if (byteIndex == byteCount) {
		return false;
	}

	// Read first byte
	byte1 = byteArray[byteIndex] & 0xFF;
	byteIndex++;

	// 1-byte sequence (no continuation bytes)
	if ((byte1 & 0x80) == 0) {
		return byte1;
	}

	// 2-byte sequence
	if ((byte1 & 0xE0) == 0xC0) {
		uint32_t byte2 = readContinuationByte();
		codePoint = ((byte1 & 0x1F) << 6) | byte2;
		if (codePoint >= 0x80) {
			return codePoint;
		} else {
			throw std::exception("Invalid continuation byte");
		}
	}

	// 3-byte sequence (may include unpaired surrogates)
	if ((byte1 & 0xF0) == 0xE0) {
		byte2 = readContinuationByte();
		byte3 = readContinuationByte();
		codePoint = ((byte1 & 0x0F) << 12) | (byte2 << 6) | byte3;
		if (codePoint >= 0x0800) {
			return codePoint;
		} else {
			throw std::exception("Invalid continuation byte");
		}
	}

	// 4-byte sequence
	if ((byte1 & 0xF8) == 0xF0) {
		byte2 = readContinuationByte();
		byte3 = readContinuationByte();
		byte4 = readContinuationByte();
		codePoint = ((byte1 & 0x0F) << 0x12) | (byte2 << 0x0C) |
			(byte3 << 0x06) | byte4;
		if (codePoint >= 0x010000 && codePoint <= 0x10FFFF) {
			return codePoint;
		}
	}

	throw std::exception("Invalid UTF-8 detected");
}