#include "base/string/string_split.h"
#include <openssl/md5.h>

namespace ABI{
	namespace base{
		std::vector<std::wstring> SplitMakePair(const std::wstring & str,const std::wstring& s1,const std::wstring& s2){
			std::vector<std::wstring> v;
			std::wstring pair_src_str = str;
			size_t s1_pos = pair_src_str.find(s1);
			while(s1_pos!=std::wstring::npos){
				if(pair_src_str.length()<s1_pos+s1.length()){
					break;
				}
				std::size_t s2_pos = std::wstring(&pair_src_str[s1_pos+s1.length()]).find(s2);
				if(s2_pos==std::wstring::npos){
					break;
				}
				v.push_back(pair_src_str.substr(s1_pos+s1.length(), s2_pos));
				s1_pos = s1_pos + s2_pos + s1.length() + s2.length();
				if(pair_src_str.length()<s1_pos){
					break;
				}
				pair_src_str = std::wstring(&pair_src_str[s1_pos]);
				s1_pos = pair_src_str.find(s1);
			}
			return v;
		}
		std::wstring SplitDetail(const std::wstring& str,const std::wstring& detail){
			std::wstring token;
			std::wstring detail_str = str;
			for(size_t pos = 0;(pos = detail_str.find(detail)) != std::wstring::npos;) {
				token.append(detail_str.substr(0, pos));
				detail_str.erase(0, pos + detail.length());
			}
			return std::wstring(token.append(detail_str));
		}
		std::vector<std::wstring> SplitArray(const std::wstring & str,const std::wstring & delimiters){
			std::vector<std::wstring> v;
			std::wstring::size_type start =0;
			size_t pos = str.find_first_of(delimiters, start);
			while(pos !=std::wstring::npos){
				if(pos != start){
					v.push_back(str.substr(start, pos - start));
				}
				start = pos +1;
				pos = str.find_first_of(delimiters, start);
			}
			if(start < str.length()){
				v.push_back(str.substr(start));
			}
			return v;
		}
		std::string HexStrFormByteArray(unsigned char *data, int len){
			std::stringstream ss;
			ss<<std::hex<<std::setw(2)<<std::setfill('0');
			for(int i(0);i<len;++i){
				ss<<(int)data[i]<<std::setw(2)<<std::setfill('0');
			}
			return std::string(ss.str());
		}
		std::string Md5(const void* str,size_t length,size_t block_length){
			MD5_CTX md5_ctx = {0};
			unsigned char sign[16] = {0};
			if(length){
				MD5_Init(&md5_ctx);
				MD5_Update(&md5_ctx, str, length);
				MD5_Final(sign,&md5_ctx);
			}
			if(block_length){
				return std::string(HexStrFormByteArray(sign,16),0,block_length);
			}
			else{
				return std::string(HexStrFormByteArray(sign,16),0);
			}
		}
		std::string StringReplace(std::string& str, const std::string& from, const std::string& to){
			size_t start_pos = 0;
			while((start_pos = str.find(from, start_pos)) != std::string::npos) {
				str.replace(start_pos, from.length(), to);
				start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
			}
			return str;
		}
		size_t EVPLength(const std::string& str){
			size_t ignore_count = 0;
			size_t count = str.length()-1;
			for(int i=0;i<2;i++){
				if(str[count]=='='){
					--count;
					++ignore_count;
				}
			}
			return ignore_count;
		}
	}
}