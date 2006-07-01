AUTOMAKE_OPTIONS = foreign subdir-objects

EXTRA_DIST= version

noinst_HEADERS = libarchive/archive_entry.h \
                 libarchive/archive.h \
                 libarchive/archive_entry.h \
                 libarchive/archive_platform.h \
                 libarchive/archive_private.h \
                 libarchive/archive_string.h

noinst_LIBRARIES = libarchive.a

libarchive_a_CPPFLAGS=-I$(top_builddir)/libarchive

libarchive_a_SOURCES = libarchive/archive_check_magic.c \
                       libarchive/archive_entry.c \
                       libarchive/archive_read.c \
                       libarchive/archive_read_data_into_buffer.c \
                       libarchive/archive_read_data_into_fd.c \
                       libarchive/archive_read_extract.c \
                       libarchive/archive_read_open_fd.c \
                       libarchive/archive_read_open_file.c \
                       libarchive/archive_read_support_compression_all.c \
                       libarchive/archive_read_support_compression_bzip2.c \
                       libarchive/archive_read_support_compression_compress.c \
                       libarchive/archive_read_support_compression_gzip.c \
                       libarchive/archive_read_support_compression_none.c \
                       libarchive/archive_read_support_format_all.c \
                       libarchive/archive_read_support_format_cpio.c \
                       libarchive/archive_read_support_format_iso9660.c \
                       libarchive/archive_read_support_format_tar.c \
                       libarchive/archive_read_support_format_zip.c \
                       libarchive/archive_string.c \
                       libarchive/archive_string_sprintf.c \
                       libarchive/archive_util.c \
                       libarchive/archive_write.c \
                       libarchive/archive_write_open_fd.c \
                       libarchive/archive_write_open_file.c \
                       libarchive/archive_write_set_compression_bzip2.c \
                       libarchive/archive_write_set_compression_gzip.c \
                       libarchive/archive_write_set_compression_none.c \
                       libarchive/archive_write_set_format.c \
                       libarchive/archive_write_set_format_by_name.c \
                       libarchive/archive_write_set_format_cpio.c \
                       libarchive/archive_write_set_format_pax.c \
                       libarchive/archive_write_set_format_shar.c \
                       libarchive/archive_write_set_format_ustar.c
