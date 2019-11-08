#include "7z/LzmaLib.h"

namespace zip// namespace begin
{
	uchar* compress(const uchar *inBuf, uint source_len, uint *pDest_len)
	{
		printf("%s\n",__FUNCTION__);
	  size_t propsSize = LZMA_PROPS_SIZE;
	  size_t destLen = source_len + source_len / 3 + 128;
	   uchar* outBuf= (uchar*)malloc(destLen+4+propsSize); 
	     
	  int res = LzmaCompress(
		&outBuf[4+propsSize], &destLen,
		&inBuf[0], source_len,
		&outBuf[4], &propsSize,
		-1, 0, -1, -1, -1, -1, -1);
  
	  if(propsSize != LZMA_PROPS_SIZE)error_stop("zip::compress LZMA_PROPS_SIZE failed (%d)",res);;;
	  if(res != SZ_OK)error_stop("zip::compress failed (%d)",res);;
  
		outBuf[0]=(source_len);
		outBuf[1]=(source_len)>>8;
		outBuf[2]=(source_len)>>16;
		outBuf[3]=(source_len)>>24;

	  *pDest_len=destLen+LZMA_PROPS_SIZE+4;
	  return outBuf;
	}

	uchar* uncompress(const uchar *inBuf, uint source_len, uint *pDest_len)
	{
		printf("%s\n",__FUNCTION__);
	  size_t dstLen=
			   (uint)	inBuf[0]+
			 (((uint)	inBuf[1])<<8)+
			 (((uint)	inBuf[2])<<16)+
			 (((uint)	inBuf[3])<<24);

	  uchar* outBuf= (uchar*)malloc(dstLen); 
	  size_t srcLen = source_len - LZMA_PROPS_SIZE - 4;
	  SRes res = LzmaUncompress(
		&outBuf[0], &dstLen,
		&inBuf[4+LZMA_PROPS_SIZE], &srcLen,
		&inBuf[4], LZMA_PROPS_SIZE);
	  if(res != SZ_OK)error_stop("zip::uncompress failed (%d)",res);;
	  *pDest_len=dstLen;
	  return outBuf;
	}
}
