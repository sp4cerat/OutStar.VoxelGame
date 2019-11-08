///////////////////////////////////////////
#include "core.h"
///////////////////////////////////////////
float cubicInterpolate (float p[4], float x) 
{
	return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}
///////////////////////////////////////////
float bicubicInterpolate (float p[4][4], float x, float y)
{
	float arr[4];
	arr[0] = cubicInterpolate(p[0], y);
	arr[1] = cubicInterpolate(p[1], y);
	arr[2] = cubicInterpolate(p[2], y);
	arr[3] = cubicInterpolate(p[3], y);
	return cubicInterpolate(arr, x);
}
///////////////////////////////////////////
std::string int_to_str(const int x)
{
	static char int_to_str_out[100];
	sprintf(int_to_str_out,"%d",x);
	return std::string(int_to_str_out);
};
///////////////////////////////////////////
std::string get_pure_filename ( std::string filename )
{
	uint pos1 = filename.find_last_of( "/" );
	uint pos2 = filename.find_last_of( "\\" );
	uint pos3 = filename.find_last_of( "." );

	if ( pos1 ==  std::string::npos ) pos1 = pos2;
	if ( pos1 ==  std::string::npos ) pos1 = 0;
	if ( pos1 < filename.size())
	if ( pos1 != 0 )pos1++;

	if ( pos3 == std::string::npos ) 
	{
		pos3 = filename.size();
	}		
	//printf( "input %s substr = %s\n" ,filename.c_str(), filename.substr(pos1,pos3-pos1).c_str());
	return (filename.substr(pos1,pos3-pos1));
}
///////////////////////////////////////////
std::string get_path ( std::string filename )
{
	uint pos1 = filename.find_last_of( "/" );
	uint pos2 = filename.find_last_of( "\\" );

	if ( pos1 ==  std::string::npos ) pos1 = pos2;
	if ( pos1 ==  std::string::npos ) return "./";

	if (pos1 < filename.size())
	if (pos1 != 0)
		pos1++;
	//printf( "substr = %s\n" , filename.substr(0,pos1).c_str());
	return (filename.substr(0,pos1));
}
///////////////////////////////////////////
char* str( const char* format, ... ) 
{
	static char txt[50000];
    va_list args;
    va_start( args, format );
    vsprintf( txt, format, args );
    va_end( args );
	return &txt[0];
}
///////////////////////////////////////////
bool file_exists(char* f)
{
	return fopen(f,"rb") ? 1 : 0;
}
///////////////////////////////////////////

