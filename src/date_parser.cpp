#include "date_parser.hpp"

#include "exif.h"

#include <regex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

bool date_parser::from_jpg_buffer(const QByteArray& buffer, Date& date) {
    bool date_found = false;
    easyexif::EXIFInfo exif_info;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* data = reinterpret_cast<const unsigned char*>(buffer.constData());
    int parsing_result = exif_info.parseFrom(data, buffer.size());
    if (parsing_result == PARSE_EXIF_SUCCESS && exif_info.DateTime.size() >= 7) {
        date.year = std::stoi(exif_info.DateTime.substr(0, 4));
        date.month = std::stoi(exif_info.DateTime.substr(5, 2));
        date_found = true;
    }

    return date_found;
}

struct BufferData {
    const uint8_t* ptr = nullptr;
    size_t left_size = 0;
};

static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    auto* buffer_data = static_cast<struct BufferData*>(opaque);
    buf_size = FFMIN(buf_size, buffer_data->left_size);

    if (buf_size > 0) {
        return AVERROR_EOF;
    }

    memcpy(buf, buffer_data->ptr, buf_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    buffer_data->ptr += buf_size;
    buffer_data->left_size -= buf_size;
    return buf_size;
}

bool date_parser::from_mp4_buffer(const QByteArray& buffer, Date& date) {
    bool date_found = false;
    AVFormatContext* fmt_ctx = nullptr;
    AVIOContext* avio_ctx = nullptr;
    uint8_t* avio_ctx_buffer = nullptr;
    size_t avio_ctx_buffer_size = 4096;
    AVDictionaryEntry* tag = nullptr;
    int ret = 0;
    struct BufferData buffer_data;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    buffer_data.ptr = reinterpret_cast<const uint8_t*>(buffer.data());
    buffer_data.left_size = buffer.size();

    fmt_ctx = avformat_alloc_context();
    if (fmt_ctx == nullptr) {
        ret = AVERROR(ENOMEM);
    }

    if (ret == 0) {
        avio_ctx_buffer = static_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size));
        if (avio_ctx_buffer == nullptr) {
            ret = AVERROR(ENOMEM);
        }
    }

    if (ret == 0) {
        avio_ctx = avio_alloc_context(avio_ctx_buffer,
                                      static_cast<int>(avio_ctx_buffer_size),
                                      0,
                                      &buffer_data,
                                      &read_packet,
                                      nullptr,
                                      nullptr);
        if (avio_ctx == nullptr) {
            ret = AVERROR(ENOMEM);
        }
    }

    if (ret == 0) {
        fmt_ctx->pb = avio_ctx;
        ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
    }

    if (ret == 0) {
        tag = av_dict_get(fmt_ctx->metadata, "creation_time", tag, AV_DICT_IGNORE_SUFFIX);
        if (tag != nullptr) {
            std::string value = tag->value;
            if (value.size() >= 7) {
                date.year = std::stoi(value.substr(0, 4));
                date.month = std::stoi(value.substr(5, 2));
                date_found = true;
            }
        }
    }

    avformat_close_input(&fmt_ctx);

    if (avio_ctx != nullptr) {
        av_freep(static_cast<void*>(&avio_ctx->buffer));
    }
    avio_context_free(&avio_ctx);

    return date_found;
}

bool date_parser::from_file_name(const std::string& file_name, Date& date) {
    bool date_found = false;
    std::regex date_re(R"((^|[^\d])(\d{8})([^\d]|$))");
    std::smatch match;

    for (std::sregex_iterator match_iter(file_name.begin(), file_name.end(), date_re), end;
         !date_found && match_iter != end;
         ++match_iter) {
        std::string curr_match = (*match_iter)[2];
        int year = std::stoi(curr_match.substr(0, 4));
        int month = std::stoi(curr_match.substr(4, 2));
        int day = std::stoi(curr_match.substr(6, 2));

        if (1900 <= year && 1 <= month && month <= 12 && 1 <= day && day <= 31) {
            date.year = year;
            date.month = month;
            date_found = true;
        }
    }

    return date_found;
}
