#include "fsearch.hpp"

bool
fsearch_t::isMp3(std::string& f)
{
	std::string::size_type idx = f.find_last_of(".");

	if ( std::string::npos == idx)
		return false;

	if ( !::_strcmpi(f.substr(idx).c_str(), ".mp3") )
		return true;

	return false;
}

std::string 
fsearch_t::basename(std::string& f)
{
	std::string::size_type idx = f.find_last_of('\\');

	if ( std::string::npos == idx ) {
		idx = f.find_last_of('/');

		if ( std::string::npos == idx )
			throw fileException_t("Parameter passed does not contain any directory separators.");
	}

	if ( f.length() <= idx )
		throw fileException_t("Parameter passed does not contain a file name following the last directory separator.");

	return f.substr(idx+sizeof('\\'));
}

std::string
fsearch_t::getLastErrorString(DWORD le)
{
	LPVOID		b(nullptr);
	std::string	r("");

	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					 FORMAT_MESSAGE_FROM_SYSTEM |
					 FORMAT_MESSAGE_IGNORE_INSERTS,
					 nullptr,
					 ::GetLastError(),
					 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					 (LPSTR)&b,
					 0, nullptr);

	r = reinterpret_cast< LPSTR >( b );
	return r;
}

bool
fsearch_t::find_all_files(std::string& p, std::vector< std::string >& v)
{
	const std::string	sp("*");
	HANDLE				hnd(INVALID_HANDLE_VALUE);
	WIN32_FIND_DATAA	fd = { 0 };

	hnd = ::FindFirstFileA(std::string(p + "\\" + sp).c_str(), &fd);

	if ( INVALID_HANDLE_VALUE == hnd ) {
		DWORD le = ::GetLastError();

		if ( ERROR_FILE_NOT_FOUND != le )
			throw fileException_t(fsearch_t::getLastErrorString(le));
		else
			return true;
	}

	do {
		if ( !::strcmp(fd.cFileName, ".") || !::strcmp(fd.cFileName, "..") )
			continue;

		if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
			if ( false == fsearch_t::find_all_files(std::string(p + "\\" + fd.cFileName), v) )
				return false;
		} else
			v.push_back(p + "\\" + fd.cFileName);

	} while ( ::FindNextFileA(hnd, &fd) );

	::FindClose(hnd);
	return true;
}

bool
fsearch_t::isDirectory(std::string& p)
{
	char	buf[ MAX_PATH + 1 ] = { 0 };
	DWORD	ret = ::GetFullPathNameA(p.c_str(), MAX_PATH, &buf[ 0 ], nullptr);

	if ( 0 == ret || MAX_PATH < ret )
		throw fileNotFound_t();

	ret = ::GetFileAttributesA(p.c_str());

	if ( INVALID_FILE_ATTRIBUTES == ret )
		throw fileNotFound_t();

	if ( ret & FILE_ATTRIBUTE_DIRECTORY )
		return true;

	return false;
}

std::vector< std::string >
fsearch_t::find_files(std::string& d)
{
	char						buf[ MAX_PATH + 1 ] = { 0 };
	DWORD						ret = ::GetFullPathNameA(d.c_str(), MAX_PATH, &buf[ 0 ], nullptr);
	std::vector< std::string >	vec;

	if ( 0 == ret || MAX_PATH < ret )
		throw fileNotFound_t();

	ret = ::GetFileAttributesA(d.c_str());

	if ( INVALID_FILE_ATTRIBUTES == ret )
		throw fileNotFound_t();

	if ( !( ret & FILE_ATTRIBUTE_DIRECTORY ) )
		throw fileNotADirectory_t();

	if ( false == find_all_files(d, vec) )
		throw fileException_t(fsearch_t::getLastErrorString().c_str());

	return vec;
}
