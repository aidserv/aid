#ifndef ABI_IOS_AUTHORIZE_RS_SIG_GEN_H_
#define ABI_IOS_AUTHORIZE_RS_SIG_GEN_H_
namespace ABI{
	namespace internal{
		class RSSigKeyGen
		{
		public:
			explicit RSSigKeyGen(){
				set_data(0);
				set_length(0);
			}
			~RSSigKeyGen(){
				reset();
			}
			void reset(){
				if(data()!=0){
					free(data());
					set_data(0);
				}
				if(length()!=0){
					set_length(0);
				}
			}
			unsigned char* data() const{
				return data_;
			}
			unsigned long length() const{
				return length_;
			}
			static RSSigKeyGen* GetInstance(){
				static RSSigKeyGen* info;
				if(!info){
					RSSigKeyGen* new_info = new RSSigKeyGen();
					if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
						delete new_info;
					}
				}
				return info;
			}
			void set_data(unsigned char* auto_data) {
				data_ = auto_data;
			}
			void set_length(unsigned long auto_length) {
				length_ = auto_length;
			}
		private:
			unsigned char *data_;
			unsigned long length_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif