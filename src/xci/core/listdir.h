// listdir.h created on 2020-10-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// Inspiration:
// - https://github.com/romkatv/gitstatus/blob/master/docs/listdir.md
// - https://news.ycombinator.com/item?id=19597460

#ifndef XCI_CORE_LISTDIR_H
#define XCI_CORE_LISTDIR_H

#include <xci/config.h>

#include <vector>
#include <memory>

#ifndef XCI_LISTDIR_GETDENTS
    #include <cerrno>
    #include <dirent.h>
    #include <unistd.h>

    struct sys_dirent_t {
        uint8_t   d_type;
        char      d_name[];
    };
#else
    #ifdef __APPLE__
        extern "C" int __getdirentries64(int fd, char *buf, int nbytes, long *basep);  // NOLINT

        #include <sys/dirent.h>
        using sys_dirent_t = struct dirent;
    #else
        #include <unistd.h>
        #include <sys/syscall.h>
        struct sys_dirent_t {
            ino64_t d_ino;
            off64_t d_off;
            unsigned short d_reclen;
            unsigned char d_type;
            char d_name[];
        };
    #endif
#endif


namespace xci::core {


// recursive arena for sys_dirent_t records
class DirEntryArena {
public:
    static constexpr size_t block_size = 16 << 10;

    char* get_block() {
        if (m_next == m_blocks.size())
            m_blocks.emplace_back(new char[block_size]);
        return m_blocks[m_next++].get();
    }

    size_t next_block() const { return m_next; }
    void set_next_block(size_t next) { m_next = next; }

private:
    size_t m_next = 0;
    std::vector<std::unique_ptr<char[]>> m_blocks;
};


// save and restore DirEntryArena's last block
class DirEntryArenaGuard {
public:
    DirEntryArenaGuard(DirEntryArena& arena) : m_arena(arena) {
        m_next = m_arena.next_block();
    }
    ~DirEntryArenaGuard() {
        m_arena.set_next_block(m_next);
    }
private:
    DirEntryArena& m_arena;
    size_t m_next;
};


/// check if file `name` is "." or ".."
inline bool is_dots_entry(const char* name) {
    return name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0));
}


#ifdef XCI_LISTDIR_GETDENTS
inline bool list_dir_sys(int dir_fd, DirEntryArena& arena, std::vector<sys_dirent_t*>& entries)
{
    long basep = 0;
    (void) basep;
    for (;;) {
        char* buf = arena.get_block();
#ifdef __APPLE__
        int n = __getdirentries64(dir_fd, buf, DirEntryArena::block_size, &basep);
#else
        int n = syscall(SYS_getdents64, dir_fd, buf, DirEntryArena::block_size);
#endif
        if (n == 0)
            break;
        if (n < 0)
            return false;

        for (int i = 0; i < n;) {
            auto* entry = reinterpret_cast<sys_dirent_t*>(buf + i);
            if (!is_dots_entry(entry->d_name)) {
                entries.emplace_back(entry);
            }
            i += entry->d_reclen;
        }
    }
    return true;
}
#endif


#ifndef XCI_LISTDIR_GETDENTS
/// \param[out] dirp        dir_fd ownership is transferred into dirp,
///                         which should be closed by `closedir(dirp);`
/// \return                 on success, return true and sets dirp
///                         on error, returns false and closes dir_fd / dirp
inline bool list_dir_posix(int dir_fd, DIR*& dirp, DirEntryArena& arena,
                           std::vector<sys_dirent_t*>& entries)
{
    dirp = fdopendir(dir_fd);
    if (dirp == nullptr) {
        close(dir_fd);
        return false;
    }

    errno = 0;
    char* buf = arena.get_block();
    size_t buf_pos = 0;
    for (;;) {
        auto* entry = readdir(dirp);
        if (entry == nullptr) {
            // end or error
            if (errno != 0) {
                closedir(dirp);
                return false;
            }
            break;
        }
        if (is_dots_entry(entry->d_name))
            continue;

        const size_t entry_size = entry->d_namlen + 2;  // 1 for d_type, 1 for d_name terminator
        if (buf_pos + entry_size > DirEntryArena::block_size) {
            buf = arena.get_block();
            buf_pos = 0;
        }

        std::memcpy(buf + buf_pos, &entry->d_type, entry_size);
        entries.emplace_back(reinterpret_cast<sys_dirent_t*>(buf + buf_pos));
        buf_pos += entry_size;
    }
    return true;
}
#endif

} // namespace xci::core

#endif // include guard
