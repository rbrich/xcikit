// ZipArchive.cpp created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ZipArchive.h"
#include <xci/core/log.h>

#include <zip.h>

namespace xci::vfs {

using namespace core::log;


bool ZipArchiveLoader::can_load_stream(std::istream& stream)
{
    char magic[2];
    stream.seekg(0);
    stream.read(magic, sizeof(magic));
    if (!stream) {
        log::debug("Vfs: ZipArchiveLoader: Couldn't read magic: first 2 bytes");
        return {};
    }
    return (magic[0] == 'P' && magic[1] == 'K');
}

std::shared_ptr<VfsDirectory>
ZipArchiveLoader::load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream)
{
    auto archive = std::make_shared<ZipArchive>(std::move(path), std::move(stream));
    if (!archive->is_open())
        return {};
    return archive;
}


ZipArchive::ZipArchive(std::string&& path, std::unique_ptr<std::istream>&& stream)
    : m_path(std::move(path)), m_stream(std::move(stream))
{
    TRACE("ZipArchive: Opening archive: {}", m_path);

    // obtain file size
    m_stream->seekg(0, std::ios_base::end);
    m_size = size_t(m_stream->tellg());
    m_stream->seekg(0);
    // reset errno - it will be read in the source callback
    errno = 0;
    zip_error_t err {};
    zip_source_t* zip_source = zip_source_function_create(
            [](void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd) -> zip_int64_t {
                auto* self = static_cast<ZipArchive*>(userdata);
                auto& sp = self->m_stream;
                struct Finally {
                    ~Finally() { self->m_last_sys_err = errno; }
                    ZipArchive* self;
                };
                Finally finally;
                finally.self = self;
                switch (cmd) {
                    case ZIP_SOURCE_OPEN:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_OPEN;
                            return -1;
                        }
                        return 0;
                    case ZIP_SOURCE_READ:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        sp->read((char*)data, std::streamsize(len));
                        if (sp->eof())
                            return 0;
                        if (sp->fail()) {
                            self->m_last_zip_err = ZIP_ER_READ;
                            return -1;
                        }
                        return sp->gcount();
                    case ZIP_SOURCE_CLOSE:
                        if (sp)
                            sp.reset();
                        return 0;
                    case ZIP_SOURCE_STAT: {
                        auto* st = static_cast<zip_stat_t*>(data);
                        zip_stat_init(st);
                        st->name = self->m_path.c_str();
                        st->size = self->m_size;
                        st->valid = ZIP_STAT_SIZE | ZIP_STAT_NAME;
                        return sizeof(zip_stat_t);
                    }
                    case ZIP_SOURCE_ERROR: {
                        auto* err = static_cast<zip_error_t*>(data);
                        err->zip_err = self->m_last_zip_err;
                        err->sys_err = self->m_last_sys_err;
                        self->m_last_zip_err = ZIP_ER_OK;
                        return 0;
                    }
                    case ZIP_SOURCE_SEEK: {
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        if (len < sizeof(zip_source_args_seek_t)) {
                            self->m_last_zip_err = ZIP_ER_INVAL;
                            return -1;
                        }
                        auto* args = static_cast<zip_source_args_seek_t*>(data);
                        std::ios_base::seekdir dir;
                        switch (args->whence) {
                            case SEEK_SET:  dir = std::ios_base::beg; break;
                            case SEEK_CUR:  dir = std::ios_base::cur; break;
                            case SEEK_END:  dir = std::ios_base::end; break;
                            default:
                                self->m_last_zip_err = ZIP_ER_INVAL;
                                return -1;
                        }
                        sp->seekg(args->offset, dir);
                        if (sp->fail()) {
                            self->m_last_zip_err = ZIP_ER_SEEK;
                            return -1;
                        }
                        return 0;
                    }
                    case ZIP_SOURCE_TELL:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        return sp->tellg();
                    case ZIP_SOURCE_FREE:
                        return 0;
                    case ZIP_SOURCE_SUPPORTS:
                        return ZIP_SOURCE_SUPPORTS_SEEKABLE;
                    default:
                        self->m_last_zip_err = ZIP_ER_OPNOTSUPP;
                        return -1;
                }
                return 0;
            }, this, &err);
    if (zip_source == nullptr) {
        log::error("ZipArchive: zip_source_function_create: {}: {}", m_path, err.zip_err);
        return;
    }

    // open archive file
    zip_t* f = zip_open_from_source(zip_source, ZIP_RDONLY, &err);
    if (f == nullptr) {
        zip_source_free(zip_source);
        log::error("ZipArchive: Failed to open archive: {}: {}", m_path, zip_error_strerror(&err));
        return;
    }

    m_zip = f;
}


ZipArchive::~ZipArchive()
{
    if (m_zip != nullptr) {
        TRACE("ZipArchive: Closing archive: {}", m_path);
        zip_close((zip_t*) m_zip);
    }
}


VfsFile ZipArchive::read_file(const std::string& path) const
{
    if (!is_open()) {
        log::error("ZipArchive: Cannot read - archive is not open");
        return {};
    }

    struct zip_stat st = {};
    zip_stat_init(&st);
    if (zip_stat((zip_t*) m_zip, path.c_str(), ZIP_FL_ENC_RAW, &st) == -1) {
        auto* err = zip_get_error((zip_t*) m_zip);
        if (err->zip_err == ZIP_ER_NOENT) {
            log::debug("ZipArchive: Not found in archive: {}", path);
        } else {
            log::error("ZipArchive: Cannot read: {}: {}", path, err->str);
        }
        return {};
    }
    if ((st.valid & ZIP_STAT_SIZE) != ZIP_STAT_SIZE) {
        log::error("ZipArchive: Cannot read: {} (missing size?!)", path);
        return {};
    }

    auto* data = new std::byte[st.size];
    BufferPtr buffer_ptr(new Buffer{data, size_t(st.size)},
            [](Buffer* b){ delete[] b->data(); delete b; });

    zip_file* f = zip_fopen_index((zip_t*) m_zip, st.index, 0);
    if (f == nullptr) {
        auto* err = zip_get_error((zip_t*) m_zip);
        log::error("ZipArchive: zip_fopen_index({}): {}", path, err->str);
        return {};
    }
    zip_int64_t nbytes = zip_fread(f, data, st.size);
    zip_fclose(f);
    if (nbytes < 0 || (zip_uint64_t)nbytes != st.size) {
        log::error("ZipArchive: Cannot read: {}: Read {} bytes of {}",
                path, nbytes, st.size);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


unsigned ZipArchive::num_entries() const
{
    return zip_get_num_entries((zip_t*) m_zip, 0);
}


std::string ZipArchive::get_entry_name(unsigned index) const
{
    return zip_get_name((zip_t*) m_zip, index, 0);
}


VfsFile ZipArchive::read_entry(unsigned index) const
{
    return read_file(get_entry_name(index));
}


}  // namespace xci::vfs
