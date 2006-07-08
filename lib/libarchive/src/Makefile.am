## Copyright 2005 The vserver-utils Developers
## See AUTHORS for details
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by  *
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the
## Free Software Foundation, Inc.,
## 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

if ENABLE_SERVER

noinst_HEADERS = archive_entry.h \
                 archive.h \
                 archive_entry.h \
                 archive_platform.h \
                 archive_private.h \
                 archive_string.h

noinst_LIBRARIES = libarchive.a

libarchive_a_SOURCES = archive_check_magic.c \
                       archive_entry.c \
                       archive_read.c \
                       archive_read_data_into_buffer.c \
                       archive_read_data_into_fd.c \
                       archive_read_extract.c \
                       archive_read_open_fd.c \
                       archive_read_open_file.c \
                       archive_read_support_compression_all.c \
                       archive_read_support_compression_bzip2.c \
                       archive_read_support_compression_compress.c \
                       archive_read_support_compression_gzip.c \
                       archive_read_support_compression_none.c \
                       archive_read_support_format_all.c \
                       archive_read_support_format_cpio.c \
                       archive_read_support_format_iso9660.c \
                       archive_read_support_format_tar.c \
                       archive_read_support_format_zip.c \
                       archive_string.c \
                       archive_string_sprintf.c \
                       archive_util.c \
                       archive_write.c \
                       archive_write_open_fd.c \
                       archive_write_open_file.c \
                       archive_write_set_compression_bzip2.c \
                       archive_write_set_compression_gzip.c \
                       archive_write_set_compression_none.c \
                       archive_write_set_format.c \
                       archive_write_set_format_by_name.c \
                       archive_write_set_format_cpio.c \
                       archive_write_set_format_pax.c \
                       archive_write_set_format_shar.c \
                       archive_write_set_format_ustar.c

endif
