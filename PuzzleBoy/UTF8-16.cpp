#include "UTF8-16.h"

u8string s;
u16string s16;

u8string toUTF8(const u16string& src){
	u8string ret;
	size_t m=src.size();
	if(m>0){
		ret.reserve(m*3);
		for(size_t i=0;i<m;i++){
			//decode UTF-16
			int c=(unsigned short)src[i];
			if(c<0xD800){
			}else if(c<0xDC00){
				//lead surrogate
				i++;
				if(i>=m) c='?';
				else{
					int c2=(unsigned short)src[i];
					if(c>=0xDC00 && c<0xE000){
						//trail surrogate
						c=0x10000 + (((c & 0x3FF)<<10) | (c2 & 0x3FF));
					}else{
						//invalid
						c='?';
						i--;
					}
				}
			}else if(c<0xE000){
				//invalid trail surrogate
				c='?';
			}

			//encode UTF-8
			if(c<0x80){
				ret.push_back(c);
			}else if(c<0x800){
				ret.push_back(0xC0 | (c>>6));
				ret.push_back(0x80 | (c & 0x3F));
			}else if(c<0x10000){
				ret.push_back(0xE0 | (c>>12));
				ret.push_back(0x80 | ((c>>6) & 0x3F));
				ret.push_back(0x80 | (c & 0x3F));
			}else{
				ret.push_back(0xF0 | (c>>18));
				ret.push_back(0x80 | ((c>>12) & 0x3F));
				ret.push_back(0x80 | ((c>>6) & 0x3F));
				ret.push_back(0x80 | (c & 0x3F));
			}
		}
	}
	return ret;
}

u16string toUTF16(const u8string& src){
	u16string ret;
	size_t m=src.size();
	if(m>0){
		ret.reserve(m);
		for(size_t i=0;i<m;i++){
			//decode UTF-8
			int c=(unsigned char)src[i];
			if(c<0x80){
			}else if(c<0xC0){
				c='?';
			}else if(c<0xE0){
				if(i+1>=m) c='?';
				else{
					int c2=(unsigned char)src[i+1];
					if((c2&0xC0)!=0x80) c='?';
					else{
						c=((c & 0x1F)<<6) | (c2 & 0x3F);
						i++;
					}
				}
			}else if(c<0xF0){
				if(i+2>=m) c='?';
				else{
					int c2=(unsigned char)src[i+1];
					int c3=(unsigned char)src[i+2];
					if((c2&0xC0)!=0x80 || (c3&0xC0)!=0x80) c='?';
					else{
						c=((c & 0xF)<<12) | ((c2 & 0x3F)<<6) | (c3 & 0x3F);
						i+=2;
					}
				}
			}else if(c<0xF8){
				if(i+3>=m) c='?';
				else{
					int c2=(unsigned char)src[i+1];
					int c3=(unsigned char)src[i+2];
					int c4=(unsigned char)src[i+3];
					if((c2&0xC0)!=0x80 || (c3&0xC0)!=0x80 || (c4&0xC0)!=0x80) c='?';
					else{
						c=((c & 0x7)<<18) | ((c2 & 0x3F)<<12) | ((c3 & 0x3F)<<6) | (c4 & 0x3F);
						if(c>=0x110000) c='?';
						else i+=3;
					}
				}
			}else{
				c='?';
			}

			//encode UTF-16
			if(c<0x10000){
				ret.push_back(c);
			}else{
				c-=0x10000;
				ret.push_back(0xD800 | (c>>10));
				ret.push_back(0xDC00 | (c & 0x3FF));
			}
		}
	}
	return ret;
}
