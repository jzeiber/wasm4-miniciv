#pragma once
/*
	ver 2023-09-23
*/

#include <stdint.h>

class OutputStringStream
{
public:
    OutputStringStream();
    ~OutputStringStream();

    void Clear();

    char *Buffer();

    int16_t TextLength() const;

    //void SetFill(const char fill);
    void SetWidth(const int32_t width);
    void SetPrecision(const int32_t precision);

    OutputStringStream &operator<<(const char *str);
    OutputStringStream &operator<<(const char c);
    OutputStringStream &operator<<(const uint32_t val);
    OutputStringStream &operator<<(const int32_t val);
    OutputStringStream &operator<<(const uint64_t val);
    OutputStringStream &operator<<(const int64_t val);
    OutputStringStream &operator<<(const float val);

private:
    char *m_buffer;
    int16_t m_buffersize;
    int16_t m_pos;
    //char m_fill;
    int32_t m_width;
    int32_t m_precision;

    void Concat(const char *str);
    void ExpandBuffer();
};