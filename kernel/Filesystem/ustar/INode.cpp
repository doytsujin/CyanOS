#include "INode.h"
#include "Lib/stdlib.h"
#include "Tasking/ScopedLock.h"
#include "utils/ErrorCodes.h"
#include "utils/stl.h"

INode::INode(const StringView& name, FSNode::NodeType type, size_t size, char* data) :
    FSNode(name, 0, 0, type, size),
    m_children{},
    m_data{data},
    m_lock{}
{
	m_lock.init();
}

INode::~INode()
{
}

Result<void> INode::open(OpenMode mode, OpenFlags flags)
{
	UNUSED(mode);
	UNUSED(flags);

	ScopedLock local_lock(m_lock);
	return ResultError(ERROR_SUCCESS);
}

Result<void> INode::close()
{
	ScopedLock local_lock(m_lock);
	return ResultError(ERROR_SUCCESS);
}

Result<void> INode::read(void* buff, size_t offset, size_t size)
{
	ScopedLock local_lock(m_lock);
	ASSERT((offset + size) <= m_size);
	memcpy(buff, m_data + offset, size);
	return ResultError(ERROR_SUCCESS);
}

Result<void> INode::write(const void* buff, size_t offset, size_t size)
{
	UNUSED(buff);
	UNUSED(offset);
	UNUSED(size);

	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<bool> INode::can_read()
{
	return true;
}

Result<bool> INode::can_write()
{
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<void> INode::remove()
{
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<FSNode&> INode::create(const StringView& name, OpenMode mode, OpenFlags flags)
{
	UNUSED(name);
	UNUSED(mode);
	UNUSED(flags);
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<void> INode::mkdir(const StringView& name, int flags, int access)
{
	UNUSED(name);
	UNUSED(flags);
	UNUSED(access);
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<void> INode::link(FSNode& node)
{
	UNUSED(node);
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<void> INode::unlink(FSNode& node)
{
	UNUSED(node);
	return ResultError(ERROR_INVALID_PARAMETERS);
}

Result<FSNode&> INode::dir_lookup(const StringView& file_name)
{
	ScopedLock local_lock(m_lock);
	for (auto& i : m_children) {
		if (i.m_name == file_name) {
			return i;
		}
	}
	return ResultError(ERROR_FILE_DOES_NOT_EXIST);
}
