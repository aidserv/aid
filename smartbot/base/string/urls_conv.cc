#include "ABI/base/string/urls_conv.h"

namespace ABI{
	namespace base{
		inline unsigned char to_hex( unsigned char x )  
		{
			return x + (x > 9 ? ('A'-10) : '0');
		}

		const std::string UrlEncode( const std::string& s )  
		{
			std::ostringstream os;

			for ( std::string::const_iterator ci = s.begin(); ci != s.end(); ++ci )
			{
				if ( (*ci >= 'a' && *ci <= 'z') ||
					(*ci >= 'A' && *ci <= 'Z') ||
					(*ci >= '0' && *ci <= '9') )
				{ // allowed
					os << *ci;
				}
				else if ( *ci == ' ')
				{
					os << '+';
				}
				else
				{
					os << '%' << to_hex(*ci >> 4) << to_hex(*ci % 16);
				}
			}

			return os.str();
		}

		inline unsigned char from_hex (
			unsigned char ch
			) 
		{
			if (ch <= '9' && ch >= '0')
				ch -= '0';
			else if (ch <= 'f' && ch >= 'a')
				ch -= 'a' - 10;
			else if (ch <= 'F' && ch >= 'A')
				ch -= 'A' - 10;
			else 
				ch = 0;
			return ch;
		}

		const std::string UrlDecode (
			const std::string& str
			) 
		{
			using namespace std;
			string result;
			string::size_type i;
			for (i = 0; i < str.size(); ++i)
			{
				if (str[i] == '+')
				{
					result += ' ';
				}
				else if (str[i] == '%' && str.size() > i+2)
				{
					const unsigned char ch1 = from_hex(str[i+1]);
					const unsigned char ch2 = from_hex(str[i+2]);
					const unsigned char ch = (ch1 << 4) | ch2;
					result += ch;
					i += 2;
				}
				else
				{
					result += str[i];
				}
			}
			return result;
		}

	}
}