#pragma once

#include <string>
#include "Common.h"


#ifdef DLL_EXPORT
#ifdef BUILD_DLL
#define HCODEC_API __declspec(dllexport)
#else
#define HCODEC_API __declspec(dllimport)
#endif
#else
#define HCODEC_API
#endif


using std::string;

class HCODEC_API HConvertor {
public:
	HConvertor();
	~HConvertor();

	int Convert(const string& infile, const string& outfile);

protected:
	
};