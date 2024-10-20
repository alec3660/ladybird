/*
 * Copyright (c) 2018-2020, Andreas Kling <andreas@ladybird.org>
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/DirIterator.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace Core {

struct DirIterator::Impl {
    DIR* dir { nullptr };
};

DirIterator::DirIterator(ByteString path, Flags flags)
    : m_impl(make<Impl>())
    , m_path(move(path))
    , m_flags(flags)
{
    m_impl->dir = opendir(m_path.characters());
    if (!m_impl->dir) {
        m_error = Error::from_errno(errno);
    }
}

DirIterator::~DirIterator()
{
    if (m_impl && m_impl->dir) {
        closedir(m_impl->dir);
        m_impl->dir = nullptr;
    }
}

DirIterator::DirIterator(DirIterator&& other)
    : m_impl(move(other.m_impl))
    , m_error(move(other.m_error))
    , m_next(move(other.m_next))
    , m_path(move(other.m_path))
    , m_flags(other.m_flags)
{
}

static constexpr bool dirent_has_d_type =
#if defined(AK_OS_SOLARIS) || defined(AK_OS_HAIKU)
    false;
#else
    true;
#endif

bool DirIterator::advance_next()
{
    if (!m_impl || !m_impl->dir)
        return false;

    while (true) {
        errno = 0;
        auto* de = readdir(m_impl->dir);
        if (!de) {
            if (errno != 0) {
                m_error = Error::from_errno(errno);
                dbgln("DirIteration error: {}", m_error.value());
            }
            m_next.clear();
            return false;
        }

        if constexpr (dirent_has_d_type)
            m_next = DirectoryEntry::from_dirent(*de);
        else
            m_next = DirectoryEntry::from_stat(m_impl->dir, *de);

        if (m_next->name.is_empty())
            return false;

        if (m_flags & Flags::SkipDots && m_next->name.starts_with('.'))
            continue;

        if (m_flags & Flags::SkipParentAndBaseDir && (m_next->name == "." || m_next->name == ".."))
            continue;

        if constexpr (dirent_has_d_type) {
            // dirent structures from readdir aren't guaranteed to contain valid file types,
            // as it is possible that the underlying filesystem doesn't keep track of those.
            // However, if we were requested to not do stat on the entries of this directory,
            // the calling code will be given the raw unknown type.
            if ((m_flags & Flags::NoStat) == 0 && m_next->type == DirectoryEntry::Type::Unknown) {
                struct stat statbuf;
                if (fstatat(dirfd(m_impl->dir), de->d_name, &statbuf, AT_SYMLINK_NOFOLLOW) < 0) {
                    m_error = Error::from_errno(errno);
                    dbgln("DirIteration error: {}", m_error.value());
                    return false;
                }
                m_next->type = DirectoryEntry::directory_entry_type_from_stat(statbuf.st_mode);
            }
        }

        return !m_next->name.is_empty();
    }
}

bool DirIterator::has_next()
{
    if (m_next.has_value())
        return true;

    return advance_next();
}

Optional<DirectoryEntry> DirIterator::next()
{
    if (!m_next.has_value())
        advance_next();

    auto result = m_next;
    m_next.clear();
    return result;
}

ByteString DirIterator::next_path()
{
    auto entry = next();
    if (entry.has_value())
        return entry->name;
    return "";
}

ByteString DirIterator::next_full_path()
{
    StringBuilder builder;
    builder.append(m_path);
    if (!m_path.ends_with('/'))
        builder.append('/');
    builder.append(next_path());
    return builder.to_byte_string();
}

int DirIterator::fd() const
{
    if (!m_impl || !m_impl->dir)
        return -1;
    return dirfd(m_impl->dir);
}

}
