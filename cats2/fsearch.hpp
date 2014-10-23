#ifndef HAVE_WIN_FILE_IO_HPP
#define HAVE_WIN_FILE_IO_HPP

#if !defined(_WIN64) || !defined(WIN32) 
#error "Attempting to compile windows specific file functionality on non-windows platform"
#endif

#include <cstdint>
#include <vector>
#include <exception>
#include <map>
#include <Windows.h>

class fileException_t : public std::exception
{
	private:
	protected:
	std::string m_msg;
	public:
		explicit fileException_t(const char* m) : std::exception(m), m_msg(m) { return; }
		explicit fileException_t(const std::string& m) : std::exception(m.c_str()), m_msg(m) { return; }
		virtual ~fileException_t(void) throw( ) { return; }
		virtual const char* what(void) throw( ) { return m_msg.c_str(); }
};

class fileNotFound_t : public fileException_t
{
	private:
	protected:
	public:
		fileNotFound_t(void) : fileException_t("The requested file was not found") { return; }
		explicit fileNotFound_t(const char* m) : fileException_t(m) { return; }
		explicit fileNotFound_t(const std::string& m) : fileException_t(m) { return; }
};

class fileNotADirectory_t : public fileException_t
{
	private:
	protected:
	public:
		fileNotADirectory_t(void) : fileException_t("The requested file is not a directory!") { return; }
		explicit fileNotADirectory_t(const char* m) : fileException_t(m) { return; }
		explicit fileNotADirectory_t(const std::string& m) : fileException_t(m) { return; }
};

class fsearch_t
{
	private:
	protected:
		static std::string getLastErrorString(DWORD le = ::GetLastError());
		static bool find_all_files(std::string&, std::vector< std::string >&);


	public:
		fsearch_t(void) { return; }
		~fsearch_t(void) { return; }
		static std::vector< std::string > find_files(std::string&);
		static bool isDirectory(std::string&);
		static bool isMp3(std::string&);
		static std::string basename(std::string&);
};

#endif

