#ifndef UTF8_FORM_JS_H_
#define UTF8_FORM_JS_H_

#include <cstdint>
#include <string>
#include <vector>

class UTF8FormJS
{
public:
	UTF8FormJS(void);
	~UTF8FormJS(void);
	std::string Encode(const std::vector<uint8_t>& str);
	std::string Decode(const std::vector<uint8_t>& byteString);
private:
	std::string ucs2decode(std::string str);
	std::string ucs2encode(std::string arr);
	uint32_t createByte(uint32_t codePoint,uint32_t shift);
	uint32_t encodeCodePoint(uint32_t codePoint);
	std::string utf8encode(std::string str);
	uint32_t readContinuationByte();
	uint32_t decodeSymbol();
	std::string byteArray;
	uint32_t byteCount;
	uint32_t byteIndex;
};


#endif // !UTF8_FORM_JS_H_
