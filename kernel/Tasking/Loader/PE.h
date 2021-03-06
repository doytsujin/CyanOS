
#pragma once

#include "Utils/Result.h"
#include "Utils/Types.h"
#include "Winnt.h"

class PELoader
{
  private:
	static uintptr_t align_to(uintptr_t size, size_t alignment);
	static void* load_pe_sections(const char* file, const IMAGE_NT_HEADERS32* nt_header);

  public:
	static Result<uintptr_t> load(const char* file, size_t size);
	PELoader() = delete;
	~PELoader() = delete;
};
